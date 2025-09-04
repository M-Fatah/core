#pragma once

#include "core/print.h"

template <typename ...TArgs>
inline static void
log_debug(const char *fmt, TArgs &&...args)
{
	unused(fmt, args...);
	#if DEBUG
		print_to(stdout, PRINT_COLOR_FG_BLUE, "[DEBUG]: {}\n", format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator()));
	#endif
}

template <typename ...TArgs>
inline static void
log_info(const char *fmt, TArgs &&...args)
{
	print_to(stdout, PRINT_COLOR_FG_WHITE_DIMMED, "[INFO]: {}\n", format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator()));
}

template <typename ...TArgs>
inline static void
log_warning(const char *fmt, TArgs &&...args)
{
	print_to(stderr, PRINT_COLOR_FG_YELLOW, "[WARNING]: {}\n", format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator()));
}

template <typename ...TArgs>
inline static void
log_error(const char *fmt, TArgs &&...args)
{
	print_to(stderr, PRINT_COLOR_FG_RED, "[ERROR]: {}\n", format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator()));
}

template <typename ...TArgs>
inline static void
log_fatal(const char *fmt, TArgs &&...args)
{
	print_to(stderr, PRINT_COLOR_BG_RED, "[FATAL]: {}\n", format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator()));
	::abort();
}