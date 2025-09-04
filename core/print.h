
#include "core/formatter.h"
#include "core/containers/string.h"

#include <stdio.h>
#include <functional>

enum PRINT_COLOR
{
	PRINT_COLOR_DEFAULT,
	PRINT_COLOR_FG_BLUE,
	PRINT_COLOR_FG_WHITE_DIMMED,
	PRINT_COLOR_FG_YELLOW,
	PRINT_COLOR_FG_RED,
	PRINT_COLOR_BG_RED,
};

using Print_Callback = std::function<void(PRINT_COLOR, const char *)>;

template <typename ...TArgs>
inline static void
print_to(Print_Callback callback, const char *fmt, TArgs &&...args)
{
	String message = format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator());
	callback(PRINT_COLOR_DEFAULT, message.data);
}

template <typename ...TArgs>
inline static void
print_to(Print_Callback callback, PRINT_COLOR color, const char *fmt, TArgs &&...args)
{
	String message = format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator());
	callback(color, message.data);
}

template <typename ...TArgs>
inline static void
print_to(FILE *file, const char *fmt, TArgs &&...args)
{
	String message = format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator());
	::fprintf(file, "%s", message.data);
}

template <typename ...TArgs>
inline static void
print_to(FILE *file, PRINT_COLOR color, const char *fmt, TArgs &&...args)
{
	constexpr auto print_color_to_ansi_code = [](PRINT_COLOR color) -> const char * {
		switch(color)
		{
			case PRINT_COLOR_DEFAULT:         return "0";
			case PRINT_COLOR_FG_BLUE:         return "1;34";
			case PRINT_COLOR_FG_WHITE_DIMMED: return "0;37";
			case PRINT_COLOR_FG_YELLOW:       return "1;33";
			case PRINT_COLOR_FG_RED:          return "1;31";
			case PRINT_COLOR_BG_RED:          return "1;41";
			default:                          return "0";
		}
	};
	String message = format(fmt, std::forward<TArgs>(args)..., memory::temp_allocator());
	::fprintf(file, "\033[%sm%s\033[0m", print_color_to_ansi_code(color), message.data);
}

template <typename ...TArgs>
inline static void
print_to_stdout(const char *fmt, TArgs &&...args)
{
	print_to(stdout, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
print_to_stdout(PRINT_COLOR color, const char *fmt, TArgs &&...args)
{
	print_to(stdout, color, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
print_to_stderr(const char *fmt, TArgs &&...args)
{
	print_to(stderr, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static void
print_to_stderr(PRINT_COLOR color, const char *fmt, TArgs &&...args)
{
	print_to(stderr, color, fmt, std::forward<TArgs>(args)...);
}