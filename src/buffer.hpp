#pragma once

#include "stream.pb.h"

#include <obs.h>

#include <queue>
#include <string>

class buffer {
   public:
    buffer(int min_starting_ms = 0);

    void set_starting_buffer(int ms);

    void queue_video(VideoFrame const& frame);
    void queue_audio(AudioFrame const& frame);

    void clear();

    bool has_data(uint64_t time);

    VideoFrame* get_video(uint64_t time);
    void pop_video();
    AudioFrame* get_audio(uint64_t time);
    void pop_audio();

   private:
    std::deque<VideoFrame> video;
    std::deque<AudioFrame> audio;

    bool calculated_offset = false;
    int64_t time_offset = 0;
    uint64_t min_starting_buffer = 0;
    bool waiting_for_buffer = true;

    void calculate_offset(uint64_t time);
    bool has_data_with_offset(uint64_t time);
    bool has_min_buffer();
};
