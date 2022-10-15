#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/format.h"
#include "core/containers/string.h"

#define LOG_FATAL(...) logger_write_to_console(LOG_TAG_FATAL, ##__VA_ARGS__)
#define LOG_ERROR(...) logger_write_to_console(LOG_TAG_ERROR, ##__VA_ARGS__)
#define LOG_WARNING(...) logger_write_to_console(LOG_TAG_WARNING, ##__VA_ARGS__)

// TODO: This flag doesn't work most of the time!
#ifdef DEBUG
	#define LOG_INFO(...) logger_write_to_console(LOG_TAG_INFO, ##__VA_ARGS__)
	#define LOG_DEBUG(...) logger_write_to_console(LOG_TAG_DEBUG, ##__VA_ARGS__)
#else
	#define LOG_INFO(...)
	#define LOG_DEBUG(...)
#endif

enum LOG_TAG
{
	LOG_TAG_FATAL,
	LOG_TAG_ERROR,
	LOG_TAG_WARNING,
	LOG_TAG_INFO,
	LOG_TAG_DEBUG
};

CORE_API void
logger_write_to_console(LOG_TAG tag, const String &message);

template <typename ...TArgs>
inline static void
logger_write_to_console(LOG_TAG tag, const char *fmt, const TArgs &...args)
{
	Formatter formatter = {};
	formatter_format(formatter, fmt, args...);
	logger_write_to_console(tag, string_literal(formatter.buffer));
}