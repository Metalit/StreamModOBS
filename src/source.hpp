#pragma once

#include "buffer.hpp"
#include "decoder.hpp"
#include "socket.hpp"

#include "stream.pb.h"

#include <obs.h>

class bs_source {
   public:
    bs_source(obs_source_t* source, obs_data_t* settings);
    ~bs_source();

    void update_settings(obs_data_t* settings);

    void tick();

    void show();
    void hide();

    int get_width();
    int get_height();

    obs_properties_t* get_properties();

   private:
    socket_manager client;
    buffer data_buffer;
    std::mutex data_buffer_mutex;
    decoder video_decoder;

    obs_source_t* source;
    obs_data_t* data;

    std::string current_address;
    int cached_width;
    int cached_height;

    bool custom_resolution_shown = false;

    bool update_custom_resolution(obs_properties_t* props);

    void reset_playback();

    void on_socket_message(std::string const& message);
    void on_socket_close();

    void send_settings();
    void receive_settings(Settings const& settings);

    void receive_video(VideoFrame const& video);
    void receive_audio(AudioFrame const& audio);
};
