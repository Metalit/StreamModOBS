#include "source.hpp"
#include "main.hpp"
#include "settings.hpp"

#include <util/platform.h>

bs_source::bs_source(obs_source_t* source_in, obs_data_t* settings) {
    source = source_in;
    update_settings(settings);
    client.set_message_callback(bind_mem(&bs_source::on_socket_message, this));
    client.set_close_callback(bind_mem(&bs_source::on_socket_close, this));
}

bs_source::~bs_source() {
    log_entry();
    // just in case of weird threading stuff
    source = NULL;
    data = NULL;
}

void bs_source::update_settings(obs_data_t* settings) {
    data = settings;

    std::string address = obs_data_get_string(data, PropAddress);
    if (address != current_address) {
        current_address = address;
        if (!address.empty() && obs_source_showing(source) && obs_source_active(source))
            client.connect(address);
        else
            client.disconnect();
        log_info("set connection to \"%s\"", current_address.c_str());
    } else
        send_settings();

    data_buffer.set_starting_buffer((int) obs_data_get_int(data, PropBufferMs));

    cached_width = (int) obs_data_get_int(data, PropWidth);
    cached_height = (int) obs_data_get_int(data, PropHeight);
    log_info("updated width/height to %dx%d", cached_width, cached_height);
}

void bs_source::tick() {
    if (!source || cached_channels < 0)
        return;

    uint64_t time = os_gettime_ns();

    obs_source_frame obs_video;
    while (video_decoder.has_frame(time) && video_decoder.get_frame(obs_video))
        obs_source_output_video(source, &obs_video);  // copies data

    std::lock_guard lock(data_buffer_mutex);

    if (!data_buffer.has_data(time))
        return;

    if (!video_decoder.valid())
        video_decoder.init(AV_CODEC_ID_H264, true);

    // try to keep the decoder buffer full enough that we have synced video to output for the next timestamp
    uint64_t frames_time_ahead = 10 * 1000000;  // 10ms
    if (camera_settings)
        frames_time_ahead += video_decoder.delay() * (1000000000 / camera_settings->fps());

    while (auto frame = data_buffer.get_video(time + frames_time_ahead)) {
        if (video_decoder.queue(frame->data(), data_buffer.offset_time(frame->time())) == decoder::again)
            break;  // decoder is full
        data_buffer.pop_video();
    }

    while (auto frame = data_buffer.get_audio(time)) {
        obs_source_audio obs_audio;
        obs_audio.format = AUDIO_FORMAT_FLOAT;
        obs_audio.timestamp = frame->time();
        obs_audio.samples_per_sec = cached_samplerate;
        obs_audio.speakers = (speaker_layout) cached_channels;
        obs_audio.data[0] = (uint8_t*) frame->data().data();  // leave as interleaved
        obs_audio.frames = frame->data().size() / cached_channels;
        obs_source_output_audio(source, &obs_audio);  // copies data
        data_buffer.pop_audio();
    }
}

void bs_source::show() {
    if (!current_address.empty() && !client.connecting() && !client.connected())
        client.connect(current_address);
}

void bs_source::hide() {
    reset_playback();
    client.disconnect();
}

int bs_source::get_width() {
    return cached_width;
}

int bs_source::get_height() {
    return cached_height;
}

obs_properties_t* bs_source::get_properties() {
    obs_properties_t* props = obs_properties_create();

    obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);
    obs_properties_add_text(props, PropAddress, get_str("props.addr"), OBS_TEXT_DEFAULT);
    obs_properties_add_int_slider(props, PropBufferMs, get_str("props.buffer"), 0, 2000, 10);

    obs_property_t* resolution = obs_properties_add_list(props, PropResolution, get_str("props.res"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    for (size_t i = 0; i < Resolutions.size(); i++)
        obs_property_list_add_int(resolution, get_str(Resolutions[i]), i);
    obs_property_list_add_int(resolution, get_str("res.custom"), Resolutions.size());
    obs_property_set_modified_callback2(
        resolution,
        [](void* data, obs_properties_t* props, obs_property_t*, obs_data_t*) {
            return reinterpret_cast<bs_source*>(data)->update_custom_resolution(props);
        },
        this
    );

    obs_property_t* width = obs_properties_add_int(props, PropWidth, get_str("props.custom_width"), 1, 4096, 1);
    obs_property_t* height = obs_properties_add_int(props, PropHeight, get_str("props.custom_height"), 1, 4096, 1);
    obs_property_set_visible(width, custom_resolution_shown);
    obs_property_set_visible(height, custom_resolution_shown);

    obs_properties_add_int_slider(props, PropBitrate, get_str("props.bitrate"), 1000, 20000, 1000);

    obs_properties_add_float_slider(props, PropGameVolume, get_str("props.game_vol"), 0, 2, 0.1);
    obs_properties_add_bool(props, PropMicrophone, get_str("props.mic"));
    obs_properties_add_float_slider(props, PropMicVolume, get_str("props.mic_vol"), 0, 2, 0.1);
    obs_properties_add_float_slider(props, PropMicThreshold, get_str("props.mic_threshold"), 0, 2, 0.1);

    obs_property_t* micMix = obs_properties_add_list(props, PropMicMix, get_str("props.mic_mix"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    for (size_t i = 0; i < MicMixes.size(); i++)
        obs_property_list_add_int(micMix, get_str(MicMixes[i]), i);

    return props;
}

bool bs_source::update_custom_resolution(obs_properties_t* props) {
    log_entry();
    bool was_shown = custom_resolution_shown;

    int value = (int) obs_data_get_int(data, PropResolution);
    custom_resolution_shown = value == Resolutions.size();

    if (!custom_resolution_shown) {
        auto [width, height] = ResolutionValues[value];
        obs_data_set_int(data, PropWidth, width);
        obs_data_set_int(data, PropHeight, height);
        cached_width = width;
        cached_height = height;
    }

    obs_property_t* width = obs_properties_get(props, PropWidth);
    obs_property_t* height = obs_properties_get(props, PropHeight);
    obs_property_set_visible(width, custom_resolution_shown);
    obs_property_set_visible(height, custom_resolution_shown);

    return was_shown != custom_resolution_shown;
}

void bs_source::reset_playback() {
    std::lock_guard lock(data_buffer_mutex);
    data_buffer.clear();
    video_decoder.reset();
    log_info("reset video playback");
}

void bs_source::on_socket_message(std::string const& message) {
    PacketWrapper packet;
    packet.ParseFromString(message);
    switch (packet.Packet_case()) {
        case PacketWrapper::kVideoSettings:
            receive_video_settings(packet.videosettings());
            break;
        case PacketWrapper::kAudioSettings:
            receive_audio_settings(packet.audiosettings());
            break;
        case PacketWrapper::kCameraSettings:
            camera_settings = packet.camerasettings();
            break;
        case PacketWrapper::kVideoFrame:
            receive_video(packet.videoframe());
            break;
        case PacketWrapper::kAudioConfig:
            cached_samplerate = packet.audioconfig().samplerate();
            cached_channels = packet.audioconfig().channels();
            break;
        case PacketWrapper::kAudioFrame:
            receive_audio(packet.audioframe());
            break;
        default:
            break;
    }
}

void bs_source::on_socket_close() {
    if (!source)
        return;
    reset_playback();
    obs_source_output_video(source, NULL);
}

void bs_source::send_settings() {
    if (!data)
        return;

    {
        PacketWrapper packet;
        auto& settings = *packet.mutable_videosettings();
        settings.set_horizontal((int) obs_data_get_int(data, PropWidth));
        settings.set_vertical((int) obs_data_get_int(data, PropHeight));
        settings.set_bitrate((int) obs_data_get_int(data, PropBitrate));
        client.send(packet.SerializeAsString());
        reset_playback();
    }
    {
        PacketWrapper packet;
        auto& settings = *packet.mutable_audiosettings();
        settings.set_gamevolume((float) obs_data_get_double(data, PropGameVolume));
        settings.set_mic(obs_data_get_bool(data, PropMicrophone));
        settings.set_micvolume((float) obs_data_get_double(data, PropMicVolume));
        settings.set_micthreshold((float) obs_data_get_double(data, PropMicThreshold));
        settings.set_micmix((AudioSettings::MicMix) obs_data_get_int(data, PropMicMix));
        client.send(packet.SerializeAsString());
    }
    {
        PacketWrapper packet;
        auto& settings = *packet.mutable_camerasettings();
        settings = *camera_settings;
        auto& position = *settings.mutable_position();
        if (position.cameramode() == CameraSettings::Position::FPFC)
            position.set_cameramode(CameraSettings::Position::Smooth);
        client.send(packet.SerializeAsString());
    }
}

void bs_source::receive_video_settings(VideoSettings const& settings) {
    log_info("received new video settings");

    if (!data)
        return;

    reset_playback();
    cached_width = settings.horizontal();
    cached_height = settings.vertical();

    obs_data_set_int(data, PropWidth, cached_width);
    obs_data_set_int(data, PropHeight, cached_height);
    obs_data_set_int(data, PropBitrate, settings.bitrate());

    size_t resolution = Resolutions.size();
    for (size_t i = 0; i < Resolutions.size(); i++) {
        if (cached_width == ResolutionValues[i].first && cached_height == ResolutionValues[i].second)
            resolution = i;
    }
    obs_data_set_int(data, PropResolution, resolution);
}

void bs_source::receive_audio_settings(AudioSettings const& settings) {
    log_info("received new audio settings");

    if (!data)
        return;

    obs_data_set_double(data, PropGameVolume, settings.gamevolume());
    obs_data_set_bool(data, PropMicrophone, settings.mic());
    obs_data_set_double(data, PropMicVolume, settings.micvolume());
    obs_data_set_double(data, PropMicThreshold, settings.micthreshold());
    obs_data_set_int(data, PropMicMix, settings.micmix());
}

void bs_source::receive_video(VideoFrame const& video) {
    if (!source)
        return;
    std::lock_guard lock(data_buffer_mutex);
    data_buffer.queue_video(video);
}

void bs_source::receive_audio(AudioFrame const& audio) {
    if (!source)
        return;
    std::lock_guard lock(data_buffer_mutex);
    data_buffer.queue_audio(audio);
}
