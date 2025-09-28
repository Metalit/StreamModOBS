#include "decoder.hpp"
#include "main.hpp"

#include <obs-avc.h>
#include <obs.h>
#include <util/platform.h>

#include <array>

// some reference from https://github.com/summershrimp/obs-ssp/blob/master/src/ffmpeg-decode.c

#ifdef DEBUG_LOGS
#define CASE_STR(value) \
    case AV_LOG_##value: return #value

static char const* level_str(int level) {
    switch (level) {
        CASE_STR(QUIET);
        CASE_STR(PANIC);
        CASE_STR(FATAL);
        CASE_STR(ERROR);
        CASE_STR(WARNING);
        CASE_STR(INFO);
        CASE_STR(VERBOSE);
        CASE_STR(DEBUG);
        CASE_STR(TRACE);
        default:
            return "unknown";
    }
}

static void log_ffmpeg(void* ptr, int level, char const* fmt, va_list args) {
    if (level > AV_LOG_VERBOSE)
        return;
    static char out[1024];
    int length = vsnprintf(out, sizeof(out), fmt, args);
    if (length <= sizeof(out))
        out[length - 1] = '\0';
    log_info("[ffmpeg: %s] %s", level_str(level), out);
}
#endif

static std::array hw_priority = {
    AV_HWDEVICE_TYPE_QSV, AV_HWDEVICE_TYPE_CUDA, AV_HWDEVICE_TYPE_DXVA2, AV_HWDEVICE_TYPE_D3D11VA, AV_HWDEVICE_TYPE_VIDEOTOOLBOX
};

static bool has_hw_type(AVCodec const* c, AVHWDeviceType type) {
    for (int i = 0;; i++) {
        AVCodecHWConfig const* config = avcodec_get_hw_config(c, i);
        if (!config)
            break;
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type)
            return true;
    }
    return false;
}

static video_format convert_pixel_format(int format) {
    switch (format) {
        case AV_PIX_FMT_GRAY8:
            return VIDEO_FORMAT_Y800;
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            return VIDEO_FORMAT_I420;
        case AV_PIX_FMT_NV12:
            return VIDEO_FORMAT_NV12;
        case AV_PIX_FMT_YUYV422:
            return VIDEO_FORMAT_YUY2;
        case AV_PIX_FMT_YVYU422:
            return VIDEO_FORMAT_YVYU;
        case AV_PIX_FMT_UYVY422:
            return VIDEO_FORMAT_UYVY;
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P:
            return VIDEO_FORMAT_I422;
        case AV_PIX_FMT_RGBA:
            return VIDEO_FORMAT_RGBA;
        case AV_PIX_FMT_BGRA:
            return VIDEO_FORMAT_BGRA;
        case AV_PIX_FMT_YUV420P10LE:
            return VIDEO_FORMAT_I010;
        case AV_PIX_FMT_BGR0:
            return VIDEO_FORMAT_BGRX;
        case AV_PIX_FMT_P010LE:
            return VIDEO_FORMAT_P010;
        default:
            return VIDEO_FORMAT_NONE;
    }
}

static bool is_sps(std::string const& data) {
    uint8_t const* start = (uint8_t*) data.data();
    uint8_t const* end = start + data.size();

    start = obs_avc_find_startcode(start, end);
    while (start < end && *(start++) == 0)  // startcode is 0, 0, 1, so go 1 past the first nonzero
        continue;
    if ((*start & 0x1F) == OBS_NAL_SPS)
        return true;
    return false;
}

static bool is_keyframe(std::string const& data) {
    return obs_avc_keyframe((uint8_t*) data.data(), (int) data.size());
}

decoder::decoder() {
    log_entry();
    if (LIBAVCODEC_VERSION_INT != avcodec_version())
        log_warning("avcodec version mismatch! headers %d lib %d", LIBAVCODEC_VERSION_INT, avcodec_version());
#ifdef DEBUG_LOGS
    av_log_set_callback(log_ffmpeg);
#endif
}

decoder::~decoder() {
    log_entry();
    reset();
}

bool decoder::init(AVCodecID id, bool try_hw) {
    codec = avcodec_find_decoder(id);
    if (!codec)
        return false;

    context = avcodec_alloc_context3(codec);
    context->thread_count = 0;
    context->delay = 0;

    if (try_hw) {
        for (AVHWDeviceType priority : hw_priority) {
            if (has_hw_type(codec, priority) && av_hwdevice_ctx_create(&hw_device_ctx, priority, NULL, NULL, 0) == 0)
                break;
        }
        if (hw_device_ctx)
            context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    if (avcodec_open2(context, codec, NULL) != 0)
        return false;

    return true;
}

bool decoder::valid() {
    return context;
}

void decoder::reset() {
    if (context)
        avcodec_free_context(&context);
    if (hw_device_ctx)
        av_buffer_unref(&hw_device_ctx);
    if (frame)
        av_frame_free(&frame);
    if (hw_frame)
        av_frame_free(&hw_frame);
    got_sps = false;
    got_keyframe = false;
}

bool decoder::queue(std::string const& data, uint64_t timestamp) {
    if (!valid())
        return false;
    if (!got_sps) {
        if (!(got_sps = is_sps(data)))
            return false;
    } else if (!got_keyframe) {
        if (!(got_keyframe = is_keyframe(data)))
            return false;
    }
    // I think it might want both the sps and first keyframe in the same packet to avoid the "no frame" ffmpeg logs,
    // but well it's not like they break anything...
    AVPacket* packet = av_packet_alloc();
    packet->data = (uint8_t*) data.data();  // not sure about ownership here
    packet->size = (int) data.size();
    packet->pts = timestamp;
    int err = avcodec_send_packet(context, packet);
    av_packet_free(&packet);
    return err == 0;
}

bool decoder::get_frame(obs_source_frame& out_frame) {
    if (!valid())
        return false;
    if (!frame)
        frame = av_frame_alloc();
    if (hw_device_ctx && !hw_frame)
        hw_frame = av_frame_alloc();
    if (!frame || (hw_device_ctx && !hw_frame))
        return false;

    if (avcodec_receive_frame(context, hw_device_ctx ? hw_frame : frame) != 0)
        return false;
    if (hw_device_ctx) {
        if (av_hwframe_transfer_data(frame, hw_frame, 0) != 0)
            return false;
        frame->pts = hw_frame->pts;
    }

    for (int i = 0; i < MAX_AV_PLANES; i++) {
        out_frame.data[i] = frame->data[i];
        out_frame.linesize[i] = frame->linesize[i];
    }
    out_frame.width = frame->width;
    out_frame.height = frame->height;
    out_frame.timestamp = frame->pts;
    out_frame.format = convert_pixel_format(frame->format);

    out_frame.full_range = false;
    out_frame.flip = false;

    return video_format_get_parameters_for_format(
        VIDEO_CS_DEFAULT, VIDEO_RANGE_PARTIAL, out_frame.format, out_frame.color_matrix, out_frame.color_range_min, out_frame.color_range_max
    );
    // not sure about max_luminance or trc
}
