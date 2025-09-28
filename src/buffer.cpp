#include "buffer.hpp"
#include "main.hpp"

buffer::buffer(int min_starting_ms) {
    set_starting_buffer(min_starting_ms);
}

void buffer::set_starting_buffer(int ms) {
    min_starting_buffer = ms * 1000000L;
    waiting_for_buffer = true;
}

void buffer::queue_video(VideoFrame const& frame) {
    video.emplace_back(std::move(frame));
}

void buffer::queue_audio(AudioFrame const& frame) {
    audio.emplace_back(std::move(frame));
}

void buffer::clear() {
    video.clear();
    audio.clear();
    calculated_offset = false;
    time_offset = 0;
    waiting_for_buffer = true;
}

bool buffer::has_data(uint64_t time) {
    if (waiting_for_buffer && !has_min_buffer())
        return false;
    if (waiting_for_buffer)
        log_debug("reached minimum buffer size");
    waiting_for_buffer = false;
    if (!calculated_offset)
        calculate_offset(time);
    if (!has_data_with_offset(time)) {
        log_debug("ran out of data in buffer");
        if (min_starting_buffer != 0)
            clear();
        else
            calculated_offset = false;
    }
    return calculated_offset;
}

std::vector<VideoFrame> buffer::pop_video(uint64_t time) {
    std::vector<VideoFrame> frames;
    while (!video.empty() && video.front().time() + time_offset <= time) {
        frames.emplace_back(std::move(video.front()));
        video.pop_front();
    }
    if (frames.size() > 5)
        log_debug("popped %lu video frames at once", frames.size());
    return frames;
}

std::vector<AudioFrame> buffer::pop_audio(uint64_t time) {
    std::vector<AudioFrame> frames;
    while (!audio.empty() && audio.front().time() + time_offset <= time) {
        frames.emplace_back(std::move(audio.front()));
        audio.pop_front();
    }
    if (frames.size() > 5)
        log_debug("popped %lu audio frames at once", frames.size());
    return frames;
}

void buffer::calculate_offset(uint64_t time) {
    if (video.empty() || audio.empty())
        return;
    uint64_t video_time = video.front().time();
    uint64_t audio_time = audio.front().time();
    time_offset = time - std::min(video_time, audio_time);
    calculated_offset = true;
    log_debug("set buffer offset to %ld", time_offset);
}

bool buffer::has_data_with_offset(uint64_t time) {
    if (video.empty() || audio.empty())
        return false;
    if (video.back().time() + time_offset < time)
        return false;
    if (audio.back().time() + time_offset < time)
        return false;
    return true;
}

bool buffer::has_min_buffer() {
    if (video.empty() || audio.empty())
        return false;
    if (video.back().time() - video.front().time() < min_starting_buffer)
        return false;
    if (audio.back().time() - audio.front().time() < min_starting_buffer)
        return false;
    return true;
}
