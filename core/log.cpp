#include "core/log.h"

#include <stdio.h>

enum LOG_TAG_COLOR
{
	LOG_TAG_COLOR_FG_GRAY,
	LOG_TAG_COLOR_FG_RED,
	LOG_TAG_COLOR_FG_GREEN,
	LOG_TAG_COLOR_FG_YELLOW,
	LOG_TAG_COLOR_FG_BLUE,
	LOG_TAG_COLOR_FG_WHITE_DIMMED,
	LOG_TAG_COLOR_FG_WHITE,
	LOG_TAG_COLOR_BG_RED,
};

inline static const char *
_log_tag_color_to_string(LOG_TAG_COLOR color)
{
	//
	// NOTE:
	// The first element in the string determines whether the color should be rendered dimmed or bright.
	//     '0' for dimmed colors e.x. "0;31" for dimmed red.
	//     '1' for bright colors e.x. "1;31" for bright red.
	// If passed color value is not supported we return dimmed white color (0;37) as default.
	//
	switch(color)
	{
		case LOG_TAG_COLOR_FG_GRAY:         return "1;30";
		case LOG_TAG_COLOR_FG_RED:          return "1;31";
		case LOG_TAG_COLOR_FG_GREEN:        return "1;32";
		case LOG_TAG_COLOR_FG_YELLOW:       return "1;33";
		case LOG_TAG_COLOR_FG_BLUE:         return "1;34";
		case LOG_TAG_COLOR_FG_WHITE_DIMMED: return "0;37";
		case LOG_TAG_COLOR_FG_WHITE:        return "1;37";
		case LOG_TAG_COLOR_BG_RED:          return "1;41";
		default:                            return "0;37";
	}
}

inline static LOG_TAG_COLOR
_log_tag_name_to_color(LOG_TAG tag)
{
	switch(tag)
	{
		case LOG_TAG_FATAL:   return LOG_TAG_COLOR_BG_RED;
		case LOG_TAG_ERROR:   return LOG_TAG_COLOR_FG_RED;
		case LOG_TAG_WARNING: return LOG_TAG_COLOR_FG_YELLOW;
		case LOG_TAG_INFO:    return LOG_TAG_COLOR_FG_WHITE_DIMMED;
		case LOG_TAG_DEBUG:   return LOG_TAG_COLOR_FG_BLUE;
		default:              return LOG_TAG_COLOR_FG_WHITE_DIMMED;
	}
}

inline static const char *
_log_tag_name_to_string(LOG_TAG tag)
{
	switch(tag)
	{
		case LOG_TAG_FATAL:   return "[FATAL]: ";
		case LOG_TAG_ERROR:   return "[ERROR]: ";
		case LOG_TAG_WARNING: return "[WARNING]: ";
		case LOG_TAG_INFO:    return "[INFO]: ";
		case LOG_TAG_DEBUG:   return "[DEBUG]: ";
		default:              return "";
	}
}

void
log_to_console(LOG_TAG tag, const String &message)
{
	const char *tag_string       = _log_tag_name_to_string(tag);
	const char *tag_color_string = _log_tag_color_to_string(_log_tag_name_to_color(tag));

	// Print the formatted message to the console, with the color specified for its tag.
	bool is_error = tag < LOG_TAG_WARNING;
	if (is_error)
		fprintf(stderr, "\033[%sm%s%s\n\033[0m", tag_color_string, tag_string, message.data);
	else
		fprintf(stdout, "\033[%sm%s%s\n\033[0m", tag_color_string, tag_string, message.data);
}