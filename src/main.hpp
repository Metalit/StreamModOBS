#pragma once

template <class T, class R, class I, class... Ts>
auto bind_mem(R (I::*fn)(Ts...), T* obj) {
    return [obj, fn](Ts... args) {
        return (obj->*fn)(args...);
    };
}

#include <util/base.h>

// #define DEBUG_LOGS

#define log_level(level, ...) \
    blog(level, "[" PLUGIN_NAME "] " __VA_ARGS__)

#define log_info(...) \
    log_level(LOG_INFO, __VA_ARGS__)
#define log_warning(...) \
    log_level(LOG_WARNING, __VA_ARGS__)
#define log_error(...) \
    log_level(LOG_ERROR, __VA_ARGS__)

#ifdef DEBUG_LOGS
#define log_debug(...) \
    log_info(__VA_ARGS__)
#define log_entry() \
    log_info("enter " __FUNCTION__ "()")
#else
#define log_debug(...)
#define log_entry()
#endif
