#include "core/platform/platform.h"

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
	return 0;
}

void
platform_sleep_set_period(u32)
{

}

void
platform_sleep(u32)
{

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