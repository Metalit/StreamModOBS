#pragma once

#include <obs.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <string>

class decoder {
   public:
    decoder();
    ~decoder();

    bool init(AVCodecID codec_id, bool try_hw = true);
    bool valid();
    void reset();

    enum error {
        ok,
        again,
        skip,
    };

    error queue(std::string const& data, uint64_t timestamp);
    bool get_frame(obs_source_frame& out_frame);

   private:
    AVBufferRef* hw_device_ctx = NULL;
    AVCodecContext* context = NULL;
    AVCodec const* codec = NULL;

    AVFrame* hw_frame = NULL;
    AVFrame* frame = NULL;
    bool got_sps = false;
    bool got_keyframe = false;
};
