#include "info.hpp"
#include "main.hpp"
#include "settings.hpp"
#include "source.hpp"

static void* bs_create(obs_data_t* settings, obs_source_t* source) {
    log_entry();
    return new bs_source(source, settings);
}

static void bs_update(void* data, obs_data_t* settings) {
    log_entry();
    reinterpret_cast<bs_source*>(data)->update_settings(settings);
}

static void bs_destroy(void* data) {
    log_entry();
    delete reinterpret_cast<bs_source*>(data);
}

static void bs_tick(void* data, float) {
    reinterpret_cast<bs_source*>(data)->tick();
}

static void show_activate(void* data) {
    reinterpret_cast<bs_source*>(data)->show();
}

static void hide(void* data) {
    reinterpret_cast<bs_source*>(data)->hide();
}

static uint32_t get_width(void* data) {
    return reinterpret_cast<bs_source*>(data)->get_width();
}

static uint32_t get_height(void* data) {
    return reinterpret_cast<bs_source*>(data)->get_height();
}

static char const* get_name(void*) {
    log_entry();
    return get_str("source.name");
}

static obs_properties_t* get_properties(void* data) {
    log_entry();
    if (!data)
        return NULL;
    return reinterpret_cast<bs_source*>(data)->get_properties();
}

static void get_defaults(obs_data_t* data) {
    obs_data_set_string(data, PropAddress, "192.168.0.1:3308");
    obs_data_set_int(data, PropBufferMs, 500);

    obs_data_set_int(data, PropResolution, 0);
    obs_data_set_int(data, PropWidth, ResolutionValues[0].first);
    obs_data_set_int(data, PropHeight, ResolutionValues[0].second);
    obs_data_set_int(data, PropBitrate, 10000);

    obs_data_set_double(data, PropGameVolume, 1);
    obs_data_set_bool(data, PropMicrophone, false);
    obs_data_set_double(data, PropMicVolume, 1);
    obs_data_set_double(data, PropMicThreshold, 1);
    obs_data_set_int(data, PropMicMix, 0);
}

obs_source_info get_bs_source_info() {
    log_entry();
    obs_source_info source = {0};
    source.id = PLUGIN_NAME;
    source.type = OBS_SOURCE_TYPE_INPUT;
    source.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;
    source.create = bs_create;
    source.update = bs_update;
    source.destroy = bs_destroy;
    source.video_tick = bs_tick;
    source.activate = show_activate;
    source.show = show_activate;
    source.hide = hide;
    source.get_width = get_width;
    source.get_height = get_height;
    source.get_name = get_name;
    source.get_properties = get_properties;
    // source.get_defaults = get_defaults;  // seems to reset every launch?
    return source;
}
