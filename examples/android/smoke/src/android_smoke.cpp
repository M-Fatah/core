#include "core/defer.h"
#include "core/platform/platform.h"
#include "core/validate.h"

#include <android/native_window.h>

inline static U8
_android_smoke_wave(U32 value)
{
	value &= 511;
	if (value > 255)
		value = 511 - value;
	return (U8)value;
}

inline static U32
_android_smoke_color(U8 r, U8 g, U8 b)
{
	return (255u << 24) | ((U32)b << 16) | ((U32)g << 8) | r;
}

inline static void
_android_smoke_render(ANativeWindow *native_window, U64 time_us)
{
	ANativeWindow_Buffer buffer = {};
	if (::ANativeWindow_lock(native_window, &buffer, nullptr) != 0)
		return;
	DEFER(::ANativeWindow_unlockAndPost(native_window););

	U32 width = (U32)buffer.width;
	U32 height = (U32)buffer.height;
	U32 stride = (U32)buffer.stride;
	U32 time = (U32)(time_us / 12000);
	U32 *pixels = (U32 *)buffer.bits;

	for (U32 y = 0; y < height; ++y)
	{
		U32 *row = pixels + y * stride;
		U32 fy = height > 0 ? (y * 255) / height : 0;
		for (U32 x = 0; x < width; ++x)
		{
			U32 fx = width > 0 ? (x * 255) / width : 0;
			U32 checker = ((x >> 5) ^ (y >> 5) ^ (time >> 4)) & 1;
			U32 glow = _android_smoke_wave(fx + fy + time * 2);
			U8 r = _android_smoke_wave(fx * 2 + time * 3);
			U8 g = _android_smoke_wave(fy * 2 + time * 2);
			U8 b = _android_smoke_wave(fx + fy + time * 4);

			if (checker)
			{
				r = (U8)((r + glow) >> 1);
				g = (U8)((g + 255) >> 1);
			}
			else
			{
				b = (U8)((b + glow) >> 1);
			}

			if (x < 8 || y < 8 || x + 8 >= width || y + 8 >= height)
				row[x] = _android_smoke_color(255, 255, 255);
			else
				row[x] = _android_smoke_color(r, g, b);
		}
	}
}

extern "C" void
core_android_smoke_main()
{
	Platform_Window window = platform_window_init(1280, 720, "Core Android Smoke");
	DEFER(platform_window_deinit(&window););

	String asset = platform_resource_read("hello.txt", memory::heap_allocator());
	DEFER(string_deinit(asset););
	validate(asset.count > 0, "[ANDROID_SMOKE]: Failed to read hello.txt from APK assets.");

	ANativeWindow *last_native_window = nullptr;
	while (platform_window_poll(&window))
	{
		void *native_handle = nullptr;
		platform_window_get_native_handles(&window, &native_handle, nullptr);
		ANativeWindow *native_window = (ANativeWindow *)native_handle;
		if (native_window)
		{
			if (native_window != last_native_window)
			{
				[[maybe_unused]] I32 result = ::ANativeWindow_setBuffersGeometry(native_window, 0, 0, WINDOW_FORMAT_RGBA_8888);
				validate(result == 0, "[ANDROID_SMOKE]: Failed to set window buffer geometry.");
				last_native_window = native_window;
			}
			_android_smoke_render(native_window, platform_query_microseconds());
		}
		platform_sleep(16);
	}
}