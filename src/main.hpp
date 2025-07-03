#pragma once

#include <util/base.h>

#define obs_log(level, ...) \
    blog(level, "[" PLUGIN_NAME "] " __VA_ARGS__)

template <class T, class R, class I, class... Ts>
auto bind_mem(R (I::*fn)(Ts...), T* obj) {
    return [obj, fn](Ts... args) {
        return (obj->*fn)(args...);
    };
}

// #define DEBUG_LOGS

#ifdef DEBUG_LOGS
#define log_entry() \
    obs_log(LOG_INFO, "enter " __FUNCTION__ "()")
#else
#define log_entry()
#endif
