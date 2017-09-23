#ifndef UTILS_H_
#define UTILS_H_

#include <glib.h>

#ifdef DEBUG
#define DEBUG_INFO(fmt, ...)   g_print(fmt, ##__VA_ARGS__)
#define DEBUG_WARN(fmt, ...)   g_warning(fmt, ##__VA_ARGS__)
#define DEBUG_ERROR(fmt, ...)  g_error(fmt, ##__VA_ARGS__)
#else
#define DEBUG_INFO(fmt, ...)
#define DEBUG_WARN(fmt, ...)
#define DEBUG_ERROR(fmt, ...)
#endif

#endif // UTILS_H_
