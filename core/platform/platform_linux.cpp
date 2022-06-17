#include "core/platform/platform.h"

#include "core/assert.h"
#include "core/logger.h"

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
platform_allocator_init(u64 size_in_bytes)
{
	Platform_Allocator self = {};
	self.ptr = (u8 *)::mmap(0, size_in_bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(self.ptr)
		self.size = size_in_bytes;
	return self;
}

void
platform_allocator_deinit(Platform_Allocator *self)
{
	[[maybe_unused]] i32 result = ::munmap(self->ptr, self->size);
	ASSERT(result == 0, "[PLATFORM]: Failed to free virtual memory.");
}

Platform_Memory
platform_allocator_alloc(Platform_Allocator *self, u64 size_in_bytes)
{
	Platform_Memory res = {};
	if (self->used + size_in_bytes >= self->size)
		return res;
	self->used += size_in_bytes;
	res.ptr = self->ptr + self->used;
	res.size = size_in_bytes;
	return res;
}

void
platform_allocator_clear(Platform_Allocator *self)
{
	self->used = 0;
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
platform_file_size(const char *filepath)
{
	struct stat file_stat;
	if(::stat(filepath, &file_stat) == 0)
		return file_stat.st_size;
	return 0;
}

u64
platform_file_read(const char *filepath, Platform_Memory mem)
{
	i32 file_handle = ::open(filepath, O_RDONLY, S_IRWXU);
	if(file_handle == -1)
		return 0;

	i64 bytes_read = ::read(file_handle, mem.ptr, mem.size);
	[[maybe_unused]] i32 close_result = ::close(file_handle);
	ASSERT(close_result == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_read == -1)
		return 0;
	return bytes_read;
}

u64
platform_file_write(const char *filepath, Platform_Memory mem)
{
	i32 file_handle = ::open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if(file_handle == -1)
		return 0;

	i64 bytes_written = ::write(file_handle, mem.ptr, mem.size);
	[[maybe_unused]] i32 close_result = ::close(file_handle);
	ASSERT(close_result == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_written == -1)
		return 0;
	return bytes_written;
}

bool
platform_file_delete(const char *filepath)
{
	return ::unlink(filepath) == 0;
}

/*
	TODO:
	[ ] Make sure zenity is installed on the user's system.
	[ ] Filters on Linux does not match how its used on Windows atm.
	[ ] Also file filter works only for a single filter for now.
*/
bool
platform_file_dialog_open(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);

	char command[2048];
	::sprintf(command, "/usr/bin/zenity --file-selection --modal --file-filter=%s --title=\"Select a file.\"", filters);
	FILE *file_handle = ::popen(command, "r");
	::fgets(path, path_length, file_handle);
	return ::pclose(file_handle) == 0;
}

bool
platform_file_dialog_save(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);

	char command[2048];
	::sprintf(command, "/usr/bin/zenity --file-selection --modal --save --file-filter=%s --title=\"Save file.\"", filters);
	FILE *file_handle = ::popen(command, "r");
	::fgets(path, path_length, file_handle);
	return ::pclose(file_handle) == 0;
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