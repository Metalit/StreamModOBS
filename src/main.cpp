#include "main.hpp"
#include "info.hpp"

#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern "C" bool obs_module_load(void) {
    obs_source_info bs_source_info = get_bs_source_info();
    obs_register_source(&bs_source_info);
    obs_log(LOG_INFO, "plugin loaded successfully (version " PLUGIN_VERSION ")");
    return true;
}

extern "C" void obs_module_unload(void) {
    obs_log(LOG_INFO, "plugin unloaded");
}
