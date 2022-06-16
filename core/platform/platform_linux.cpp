#include "core/platform/platform.h"

#include "core/assert.h"
#include "core/logger.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Platform_Api
platform_api_init(const char *)
{
	return {};
}

void
platform_api_deinit(Platform_Api *)
{

}

void *
platform_api_load(Platform_Api *)
{
	return 0;
}


Platform_Allocator
platform_allocator_init(u64)
{
	return {};
}

void
platform_allocator_deinit(Platform_Allocator *)
{

}

Platform_Memory
platform_allocator_alloc(Platform_Allocator *, u64)
{
	return {};
}

void
platform_allocator_clear(Platform_Allocator *)
{

}

Platform_Window
platform_window_init(u32, u32, const char *)
{
	return {};
}

void
platform_window_deinit(Platform_Window *)
{

}

bool
platform_window_poll(Platform_Window *)
{
	return 0;
}

void
platform_window_set_title(Platform_Window *, const char *)
{

}

void
platform_window_close(Platform_Window *)
{

}

void
platform_set_current_directory()
{
	char module_path_relative[PATH_MAX + 1];
	::memset(module_path_relative, 0, sizeof(module_path_relative));

	char module_path_absolute[PATH_MAX + 1];
	::memset(module_path_absolute, 0, sizeof(module_path_absolute));

	[[maybe_unused]] i64 module_path_relative_length = ::readlink("/proc/self/exe", module_path_relative, sizeof(module_path_relative));
	ASSERT(module_path_relative_length != -1 && module_path_relative_length < (i64)sizeof(module_path_relative), "[PLATFORM]: Failed to get relative path of the current executable.");

	[[maybe_unused]] char *path_absolute = ::realpath(module_path_relative, module_path_absolute);
	ASSERT(path_absolute == module_path_absolute, "[PLATFORM]: Failed to get absolute path of the current executable.");

	char *last_slash = module_path_absolute;
	char *iterator = module_path_absolute;
	while (*iterator++)
	{
		if (*iterator == '/')
			last_slash = ++iterator;
	}
	*last_slash = '\0';

	[[maybe_unused]] i32 result = ::chdir(module_path_absolute);
	ASSERT(result == 0, "[PLATFORM]: Failed to set current directory.");
}

u64
platform_file_size(const char *)
{
	return 0;
}

u64
platform_file_read(const char *, Platform_Memory)
{
	return 0;
}

u64
platform_file_write(const char *, Platform_Memory)
{
	return 0;
}

bool
platform_file_delete(const char *)
{
	return 0;
}

bool
platform_file_dialog_open(char *, u32, const char *)
{
	return 0;
}

bool
platform_file_dialog_save(char *, u32, const char *)
{
	return 0;
}

u64
platform_query_microseconds()
{
	struct timespec time;
	[[maybe_unused]] i32 result = clock_gettime(CLOCK_MONOTONIC, &time);
	ASSERT(result == 0, "[PLATFORM]: Failed to query clock.");
	return time.tv_sec * 1000000.0f + time.tv_nsec * 0.001f;
}

void
platform_sleep_set_period(u32)
{

}

void
platform_sleep(u32 milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	nanosleep(&ts, 0);
}

u32
platform_callstack_capture(void **, u32)
{
	return 0;
}

void
platform_callstack_log(void **, u32)
{

}

Font
platform_font_init(const char *, const char *, u32, bool)
{
	return {};
}

void
platform_font_deinit(Font *)
{

}