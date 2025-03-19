#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/format.h"
#include "core/containers/string.h"

enum LOG_TAG
{
	LOG_TAG_FATAL,
	LOG_TAG_ERROR,
	LOG_TAG_WARNING,
	LOG_TAG_INFO,
	LOG_TAG_DEBUG
};

CORE_API void
log_to_console(LOG_TAG tag, const String &message);

template <typename ...TArgs>
inline static void
log_to_console(LOG_TAG tag, const char *fmt, TArgs &&...args)
{
	log_to_console(tag, format2(fmt, std::forward<TArgs>(args)...));
}

template <typename ...TArgs>
inline static void
log_fatal(const char *fmt, TArgs &&...args)
{
	log_to_console(LOG_TAG_FATAL, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
log_error(const char *fmt, TArgs &&...args)
{
	log_to_console(LOG_TAG_ERROR, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
log_warning(const char *fmt, TArgs &&...args)
{
	log_to_console(LOG_TAG_WARNING, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
log_info(const char *fmt, TArgs &&...args)
{
	log_to_console(LOG_TAG_INFO, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
log_debug(const char *fmt, TArgs &&...args)
{
	unused(fmt, args...);
	#if DEBUG
		log_to_console(LOG_TAG_DEBUG, fmt, std::forward<TArgs>(args)...);
	#endif
}