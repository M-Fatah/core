#include "core/platform/platform.h"

#include "core/defer.h"
#include "core/assert.h"
#include "core/logger.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"

#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <math.h>

// TODO: Remove from here.
inline static void
_string_concat(const char *a, const char *b, char *result)
{
	while (*a != '\0')
		*result++ = *a++;

	while (*b != '\0')
		*result++ = *b++;
}

inline static PLATFORM_KEY
_platform_key_from_msg(MSG msg)
{
	switch (msg.message)
	{
		case WM_LBUTTONDOWN: return PLATFORM_KEY_MOUSE_LEFT;
		case WM_LBUTTONUP:   return PLATFORM_KEY_MOUSE_LEFT;
		case WM_MBUTTONDOWN: return PLATFORM_KEY_MOUSE_MIDDLE;
		case WM_MBUTTONUP:   return PLATFORM_KEY_MOUSE_MIDDLE;
		case WM_RBUTTONDOWN: return PLATFORM_KEY_MOUSE_RIGHT;
		case WM_RBUTTONUP:   return PLATFORM_KEY_MOUSE_RIGHT;
		case WM_MOUSEWHEEL:  return (GET_WHEEL_DELTA_WPARAM(msg.wParam) > 0)? PLATFORM_KEY_MOUSE_WHEEL_UP : PLATFORM_KEY_MOUSE_WHEEL_DOWN;
	}
	return PLATFORM_KEY_COUNT;
}

inline static PLATFORM_KEY
_platform_key_from_wparam(WPARAM wparam)
{
	switch (wparam)
	{
		case 'A':           return PLATFORM_KEY_A;
		case 'B':           return PLATFORM_KEY_B;
		case 'C':           return PLATFORM_KEY_C;
		case 'D':           return PLATFORM_KEY_D;
		case 'E':           return PLATFORM_KEY_E;
		case 'F':           return PLATFORM_KEY_F;
		case 'G':           return PLATFORM_KEY_G;
		case 'H':           return PLATFORM_KEY_H;
		case 'I':           return PLATFORM_KEY_I;
		case 'J':           return PLATFORM_KEY_J;
		case 'K':           return PLATFORM_KEY_K;
		case 'L':           return PLATFORM_KEY_L;
		case 'M':           return PLATFORM_KEY_M;
		case 'N':           return PLATFORM_KEY_N;
		case 'O':           return PLATFORM_KEY_O;
		case 'P':           return PLATFORM_KEY_P;
		case 'Q':           return PLATFORM_KEY_Q;
		case 'R':           return PLATFORM_KEY_R;
		case 'S':           return PLATFORM_KEY_S;
		case 'T':           return PLATFORM_KEY_T;
		case 'U':           return PLATFORM_KEY_U;
		case 'V':           return PLATFORM_KEY_V;
		case 'W':           return PLATFORM_KEY_W;
		case 'X':           return PLATFORM_KEY_X;
		case 'Y':           return PLATFORM_KEY_Y;
		case 'Z':           return PLATFORM_KEY_Z;
		case '0':           return PLATFORM_KEY_NUM_0;
		case '1':           return PLATFORM_KEY_NUM_1;
		case '2':           return PLATFORM_KEY_NUM_2;
		case '3':           return PLATFORM_KEY_NUM_3;
		case '4':           return PLATFORM_KEY_NUM_4;
		case '5':           return PLATFORM_KEY_NUM_5;
		case '6':           return PLATFORM_KEY_NUM_6;
		case '7':           return PLATFORM_KEY_NUM_7;
		case '8':           return PLATFORM_KEY_NUM_8;
		case '9':           return PLATFORM_KEY_NUM_9;
		case VK_NUMPAD0:    return PLATFORM_KEY_NUMPAD_0;
		case VK_NUMPAD1:    return PLATFORM_KEY_NUMPAD_1;
		case VK_NUMPAD2:    return PLATFORM_KEY_NUMPAD_2;
		case VK_NUMPAD3:    return PLATFORM_KEY_NUMPAD_3;
		case VK_NUMPAD4:    return PLATFORM_KEY_NUMPAD_4;
		case VK_NUMPAD5:    return PLATFORM_KEY_NUMPAD_5;
		case VK_NUMPAD6:    return PLATFORM_KEY_NUMPAD_6;
		case VK_NUMPAD7:    return PLATFORM_KEY_NUMPAD_7;
		case VK_NUMPAD8:    return PLATFORM_KEY_NUMPAD_8;
		case VK_NUMPAD9:    return PLATFORM_KEY_NUMPAD_9;
		case VK_F1:         return PLATFORM_KEY_F1;
		case VK_F2:         return PLATFORM_KEY_F2;
		case VK_F3:         return PLATFORM_KEY_F3;
		case VK_F4:         return PLATFORM_KEY_F4;
		case VK_F5:         return PLATFORM_KEY_F5;
		case VK_F6:         return PLATFORM_KEY_F6;
		case VK_F7:         return PLATFORM_KEY_F7;
		case VK_F8:         return PLATFORM_KEY_F8;
		case VK_F9:         return PLATFORM_KEY_F9;
		case VK_F10:        return PLATFORM_KEY_F10;
		case VK_F11:        return PLATFORM_KEY_F11;
		case VK_F12:        return PLATFORM_KEY_F12;
		case VK_UP:         return PLATFORM_KEY_ARROW_UP;
		case VK_DOWN:       return PLATFORM_KEY_ARROW_DOWN;
		case VK_LEFT:       return PLATFORM_KEY_ARROW_LEFT;
		case VK_RIGHT:      return PLATFORM_KEY_ARROW_RIGHT;
		case VK_SHIFT:      return PLATFORM_KEY_SHIFT_LEFT;
		case VK_LSHIFT:     return PLATFORM_KEY_SHIFT_LEFT;
		case VK_RSHIFT:     return PLATFORM_KEY_SHIFT_RIGHT;
		case VK_CONTROL:    return PLATFORM_KEY_CONTROL_LEFT;
		case VK_LCONTROL:   return PLATFORM_KEY_CONTROL_LEFT;
		case VK_RCONTROL:   return PLATFORM_KEY_CONTROL_RIGHT;
		case VK_MENU:       return PLATFORM_KEY_ALT_LEFT;
		case VK_LMENU:      return PLATFORM_KEY_ALT_LEFT;
		case VK_RMENU:      return PLATFORM_KEY_ALT_RIGHT;
		case VK_BACK:       return PLATFORM_KEY_BACKSPACE;
		case VK_TAB:        return PLATFORM_KEY_TAB;
		case VK_RETURN:     return PLATFORM_KEY_ENTER;
		case VK_ESCAPE:     return PLATFORM_KEY_ESCAPE;
		case VK_DELETE:     return PLATFORM_KEY_DELETE;
		case VK_INSERT:     return PLATFORM_KEY_INSERT;
		case VK_HOME:       return PLATFORM_KEY_HOME;
		case VK_END:        return PLATFORM_KEY_END;
		case VK_PRIOR:      return PLATFORM_KEY_PAGE_UP;
		case VK_NEXT:       return PLATFORM_KEY_PAGE_DOWN;
		case VK_OEM_2:      return PLATFORM_KEY_SLASH;
		case VK_OEM_5:      return PLATFORM_KEY_BACKSLASH;
		case VK_OEM_4:      return PLATFORM_KEY_BRACKET_LEFT;
		case VK_OEM_6:      return PLATFORM_KEY_BRACKET_RIGHT;
		case VK_OEM_3:      return PLATFORM_KEY_BACKQUOTE;
		case VK_OEM_PERIOD: return PLATFORM_KEY_PERIOD;
		case VK_OEM_MINUS:  return PLATFORM_KEY_MINUS;
		case VK_OEM_PLUS:   return PLATFORM_KEY_EQUAL;
		case VK_OEM_COMMA:  return PLATFORM_KEY_COMMA;
		case VK_OEM_1:      return PLATFORM_KEY_SEMICOLON;
		case VK_SPACE:      return PLATFORM_KEY_SPACE;
	}
	return PLATFORM_KEY_COUNT;
}

LRESULT CALLBACK
_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// API
Platform_Api
platform_api_init(const char *filepath)
{
	Platform_Api self = {};

	char path[128] = {};
	_string_concat(filepath, ".dll", path);

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool result = ::CopyFileA(path, path_tmp, false);
	ASSERT(result, "[PLATFORM]: Failed to copy library.");

	self.handle = ::LoadLibraryA(path_tmp);
	ASSERT(self.handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::GetProcAddress((HMODULE)self.handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self.api = proc(nullptr, PLATFORM_API_STATE_INIT);
	ASSERT(self.api, "[PLATFORM]: Failed to get api.");

	WIN32_FILE_ATTRIBUTE_DATA data = {};
	result = ::GetFileAttributesExA(path, GetFileExInfoStandard, &data);
	ASSERT(result, "[PLATFORM]: Failed to get file attributes.");

	self.last_write_time = *(i64 *)&data.ftLastWriteTime;
	::strcpy_s(self.filepath, filepath);

	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)GetProcAddress((HMODULE)self->handle, "platform_api");
		ASSERT(proc, "failed to get proc platform_api");
		self->api = proc(self->api, PLATFORM_API_STATE_DEINIT);
	}

	FreeLibrary((HMODULE)self->handle);
}

void *
platform_api_load(Platform_Api *self)
{
	char path[128] = {};
	_string_concat(self->filepath, ".dll", path);

	WIN32_FILE_ATTRIBUTE_DATA data = {};
	bool result = ::GetFileAttributesExA(path, GetFileExInfoStandard, &data);

	i64 last_write_time = *(i64 *)&data.ftLastWriteTime;
	if ((last_write_time == self->last_write_time) || (result == false))
		return self->api;

	result = ::FreeLibrary((HMODULE)self->handle);
	ASSERT(result, "[PLATFORM]: Failed to free library.");

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool copy_result = ::CopyFileA(path, path_tmp, false);

	self->handle = ::LoadLibraryA(path_tmp);
	ASSERT(self->handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::GetProcAddress((HMODULE)self->handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self->api = proc(self->api, PLATFORM_API_STATE_LOAD);
	ASSERT(self->api, "[PLATFORM]: Failed to get api.");

	// If copying failed we don't update last write time so that we can try copying it again in the next frame.
	if (copy_result)
		self->last_write_time = last_write_time;

	return self->api;
}


Platform_Allocator
platform_allocator_init(u64 size_in_bytes)
{
	Platform_Allocator self = {};
	self.ptr = (u8 *)VirtualAlloc(0, size_in_bytes, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (self.ptr)
		self.size = size_in_bytes;
	return self;
}

void
platform_allocator_deinit(Platform_Allocator *self)
{
	[[maybe_unused]] bool result = VirtualFree(self->ptr, 0, MEM_RELEASE);
	ASSERT(result, "[PLATFORM]: Failed to free virtual memory.");
}

Platform_Memory
platform_allocator_alloc(Platform_Allocator *self, u64 size_in_bytes)
{
	// TODO(M-Fatah): We need a way to free allocated memory from the arena we created.
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
platform_window_init(u32 width, u32 height, const char *title)
{
	ASSERT(width > 0 && height > 0, "[PLATFORM]: Windows cannot have zero width or height.");

	Platform_Window self = {};

	WNDCLASSEXA wc = {};
	wc.cbSize		 = sizeof(wc);
	wc.style		 = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = _window_proc;
	wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "Mist_Window_Class";
	[[maybe_unused]] ATOM class_atom = RegisterClassExA(&wc);
	ASSERT(class_atom != 0, "[PLATFORM]: Failed to register window class.");

	self.handle = CreateWindowExA(
		0,
		wc.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		nullptr,
		nullptr,
		nullptr,
		nullptr);
	ASSERT(self.handle, "[PLATFORM]: Failed to create window.");

	self.width  = width;
	self.height = height;

	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	bool res = false;
	res = DestroyWindow((HWND)self->handle);
	ASSERT(res, "[PLATFORM]: Failed to destroy window.");
}

bool
platform_window_poll(Platform_Window *self)
{
	for (i32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		self->input.keys[i].pressed       = false;
		self->input.keys[i].released      = false;
		self->input.keys[i].press_count   = 0;
		self->input.keys[i].release_count = 0;
	}
	self->input.mouse_wheel = 0.0f;

	MSG msg = {};
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		switch (msg.message)
		{
			case WM_QUIT:
			{
				return false;
			}
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MOUSEWHEEL:
			{
				SetCapture((HWND)self->handle);

				PLATFORM_KEY key = _platform_key_from_msg(msg);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].pressed = true;
					self->input.keys[key].down    = true;
					self->input.keys[key].press_count++;
					if (key == PLATFORM_KEY_MOUSE_WHEEL_UP || key == PLATFORM_KEY_MOUSE_WHEEL_DOWN)
						self->input.mouse_wheel += (f32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (f32)WHEEL_DELTA;
				}
				break;
			}
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			{
				ReleaseCapture();

				PLATFORM_KEY key = _platform_key_from_msg(msg);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].released = true;
					self->input.keys[key].down     = false;
					self->input.keys[key].release_count++;
				}
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				PLATFORM_KEY key = _platform_key_from_wparam(msg.wParam);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].pressed  = true;
					self->input.keys[key].down     = true;
					self->input.keys[key].press_count++;
				}
				break;
			}
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				PLATFORM_KEY key = _platform_key_from_wparam(msg.wParam);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].released = true;
					self->input.keys[key].down     = false;
					self->input.keys[key].release_count++;
				}
				break;
			}
		}
	}

	{
		// NOTE: Resizing.
		RECT rect;
		GetClientRect((HWND)self->handle, &rect);
		self->width = rect.right - rect.left;
		self->height = rect.bottom - rect.top;
	}

	{
		// NOTE: Mouse movement.
		POINT mouse_point;
		GetCursorPos(&mouse_point);
		ScreenToClient((HWND)self->handle, &mouse_point);

		// NOTE: We want mouse coords to start bottom-left.
		u32 mouse_point_y_inverted = (self->height - 1) - mouse_point.y;
		self->input.mouse_dx = mouse_point.x - self->input.mouse_x;
		self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
		self->input.mouse_x  = mouse_point.x;
		self->input.mouse_y  = mouse_point_y_inverted;
	}

	return true;
}

void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_display)
{
	if (native_handle)
		*native_handle = self->handle;
	if (native_display)
		*native_display = nullptr;
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	SetWindowText((HWND)self->handle, title);
}

void
platform_window_close(Platform_Window *self)
{
	PostMessageW((HWND)self->handle, WM_QUIT, 0, 0);
}

void
platform_set_current_directory()
{
	char module_path[1024];
	GetModuleFileNameA(0, module_path, sizeof(module_path));

	char *last_slash = module_path;
	char *iterator = module_path;
	while (*iterator++)
	{
		if (*iterator == '\\')
			last_slash = ++iterator;
	}
	*last_slash = '\0';

	[[maybe_unused]] bool result = SetCurrentDirectoryA(module_path);
	ASSERT(result, "[PLATFORM]: Failed to set current directory.");
}


u64
platform_file_size(const char *filepath)
{
	WIN32_FILE_ATTRIBUTE_DATA data = {};
	if (GetFileAttributesExA(filepath, GetFileExInfoStandard, &data) == false)
		return 0;

	LARGE_INTEGER size;
	size.HighPart = data.nFileSizeHigh;
	size.LowPart  = data.nFileSizeLow;
	return size.QuadPart;
}

u64
platform_file_read(const char *filepath, Platform_Memory mem)
{
	HANDLE file_handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return 0;

	// TODO(M-Fatah): Handle reading files that are bigger than 4GB size.
	DWORD bytes_read = 0;
	ReadFile(file_handle, mem.ptr, (u32)mem.size, &bytes_read, 0);
	CloseHandle(file_handle);

	return (u64)bytes_read;
}

u64
platform_file_write(const char *filepath, Platform_Memory mem)
{
	HANDLE file_handle = CreateFileA(filepath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return 0;

	// TODO(M-Fatah): Properly handle large files (files with size over 4GB as a single file).
	DWORD bytes_written = 0;
	WriteFile(file_handle, mem.ptr, (DWORD)mem.size, &bytes_written, 0);
	CloseHandle(file_handle);

	return (u64)bytes_written;
}

bool
platform_file_copy(const char *from, const char *to)
{
	return CopyFileA(from, to, false);
}

bool
platform_file_delete(const char *filepath)
{
	return DeleteFileA(filepath);
}

bool
platform_file_dialog_open(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);

	OPENFILENAME ofn = {};
	ofn.lStructSize     = sizeof(ofn);
	ofn.lpstrFile       = path;
	ofn.nMaxFile        = path_length;
	ofn.lpstrFilter     = filters;
	ofn.nFilterIndex    = 1;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn) == TRUE)
		return true;

	return false;
}

bool
platform_file_dialog_save(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);

	OPENFILENAME ofn = {};
	ofn.lStructSize     = sizeof(ofn);
	ofn.lpstrFile       = path;
	ofn.nMaxFile        = path_length;
	ofn.lpstrFilter     = filters;
	ofn.nFilterIndex    = 1;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	// TODO: Ensure that we write extension, if the user forgets to do so.
	if (GetSaveFileName(&ofn) == TRUE)
		return true;

	return false;
}


u64
platform_query_microseconds()
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER ticks;
	if (QueryPerformanceFrequency(&frequency) == false)
	{
		ASSERT(false, "[PLATFORM]: Failed to query performance frequency.");
	}
	if (QueryPerformanceCounter(&ticks) == false)
	{
		ASSERT(false, "[PLATFORM]: Failed to query performance counter.");
	}
	return ticks.QuadPart * 1000000 / frequency.QuadPart;
}

void
platform_sleep_set_period(u32 period)
{
	if (timeBeginPeriod(period) != TIMERR_NOERROR)
	{
		ASSERT(false, "[PLATFORM]: Failed to set time begin period.");
	}
}

void
platform_sleep(u32 milliseconds)
{
	Sleep(milliseconds);
}


struct Callstack
{
	Callstack()
	{
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
		SymInitialize(GetCurrentProcess(), NULL, TRUE);
	}

	~Callstack()
	{
		SymCleanup(GetCurrentProcess());
	}
};

u32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] u32 frame_count)
{
#if DEBUG
	::memset(callstack, 0, frame_count * sizeof(callstack));
	return CaptureStackBackTrace(1, frame_count, callstack, NULL);
#else
	return 0;
#endif
}

void
platform_callstack_log([[maybe_unused]] void **callstack, [[maybe_unused]] u32 frame_count)
{
#if DEBUG
	static Callstack _callstack;

	// Get all loaded modules.
	Array<void *> modules = array_init<void *>();
	DEFER(array_deinit(modules));
	array_push(modules, GetCurrentProcess());

	// First we enumerate to get the count of modules.
	DWORD bytes_needed = 0;
	if (EnumProcessModules(modules[0], NULL, 0, &bytes_needed))
	{
		// Expand the array to account for the added modules.
		array_resize(modules, modules.count + bytes_needed/sizeof(HMODULE));

		// Then enumerate again to get the actual modules data.
		// If this fails for some reason, we resize the array back to hold only the current process' module.
		if (EnumProcessModules(modules[0], (HMODULE*)(modules.data + 1), bytes_needed, &bytes_needed) == FALSE)
			array_resize(modules, 1);
	}

	// Allocate a buffer for the symbol info.
	// Windows lays symbol info in memory in the form [struct][name buffer].
	constexpr u64 MAX_NAME_LENGTH = 256;
	char symbol_buffer[MAX_NAME_LENGTH + sizeof(SYMBOL_INFO)];

	SYMBOL_INFO *symbol_info = (SYMBOL_INFO *)symbol_buffer;
	::memset(symbol_info, 0, sizeof(SYMBOL_INFO));
	symbol_info->MaxNameLen   = MAX_NAME_LENGTH;
	symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);

	LOG_WARNING("callstack:");
	for(u64 i = 0; i < frame_count; ++i)
	{
		bool symbol_found = false;
		bool line_found   = false;

		IMAGEHLP_LINE64 line = {};
		line.SizeOfStruct = sizeof(line);

		DWORD displacement = 0;
		for (const auto &module : modules)
		{
			if (SymFromAddr(module, (DWORD64)(callstack[i]), NULL, symbol_info))
			{
				symbol_found = true;
				line_found   = SymGetLineFromAddr64(module, (DWORD64)(callstack[i]), &displacement, &line);
				break;
			}
		}

		LOG_WARNING(
			"\t[{}]: {}, {}:{}",
			frame_count - i - 1,
			symbol_found ? symbol_info->Name : "<SYMBOL NOT FOUND>",
			line_found   ? line.FileName     : "<FILE NOT FOUND>",
			line_found   ? line.LineNumber   : 0
		);
	}
#endif
}

/* TODO:
*   - [ ] Figure out the weird crash we get when we set APRON value to 0.
*   - [ ] Add SDF fonts.
*   - [ ] Supersample fonts.
*/


Platform_Font
platform_font_init(const char *filepath, const char *face_name, u32 font_height, bool origin_top_left)
{
	// Supported glyph range.
	constexpr i32 GLYPH_RANGE[2]             = {'!', '~'};
	constexpr u32 GLYPH_COUNT                = (GLYPH_RANGE[1] + 1) - GLYPH_RANGE[0];

	// Font bitmap config. This is used to rasterize each glyph by winapi.
	constexpr u32 BITMAP_MAX_WIDTH           = 1024;
	constexpr u32 BITMAP_MAX_HEIGHT          = 1024;
	constexpr u32 BYTES_PER_PIXEL            = 1;
	constexpr u32 APRON                      = 1;
	u8 *temp_glyph_bitmaps[GLYPH_COUNT] = {};

	// Atlas texture config.
	constexpr u32 XPADDING                   = 3;
	constexpr u32 YPADDING                   = 3;
	u32 xoffset                              = XPADDING;
	u32 total_glyph_width                    = 0;
	u32 max_glyph_height                     = 0;

	// Kerning config.
	constexpr u32 KERNING_ADJUSTMENT         = 3;
	i32 *kerning_table                       = (i32 *)memory::allocate_zeroed(GLYPH_COUNT * GLYPH_COUNT * sizeof(i32));

	// Extract the font from Windows.
	AddFontResourceEx(filepath, FR_PRIVATE, 0);
	HFONT font_handle = CreateFont(font_height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH|FF_DONTCARE, face_name);
	HDC device_context = CreateCompatibleDC(GetDC(0));

	// Create a bitmap to rasterize glyphs into.
	BITMAPINFO bitmap_info              = {};
	bitmap_info.bmiHeader.biSize        = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth       = BITMAP_MAX_WIDTH;
	bitmap_info.bmiHeader.biHeight      = BITMAP_MAX_HEIGHT;
	bitmap_info.bmiHeader.biPlanes      = 1;
	bitmap_info.bmiHeader.biBitCount    = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	void *bitmap_data;
	HBITMAP bitmap_handle = CreateDIBSection(device_context, &bitmap_info, DIB_RGB_COLORS, &bitmap_data, 0, 0);
	SelectObject(device_context, bitmap_handle);
	SelectObject(device_context, font_handle);
	SetBkColor(device_context, RGB(0, 0, 0));
	SetTextColor(device_context, RGB(255, 255, 255));

	// Free resources at the end of the function scope.
	DEFER({
		RemoveFontResource(filepath);
		DeleteObject(font_handle);
		DeleteObject(bitmap_handle);
		DeleteDC(device_context);
	});

	// Get kerning pairs.
	u32 kerning_pair_count     = GetKerningPairsW(device_context, 0, 0);
	KERNINGPAIR *kerning_pairs = (KERNINGPAIR *)memory::allocate(memory::temp_allocator(), kerning_pair_count * sizeof(KERNINGPAIR));
	GetKerningPairsW(device_context, kerning_pair_count, kerning_pairs);
	if (kerning_pair_count > 0)
	{
		for (u32 i = 0; i <= kerning_pair_count; ++i)
		{
			KERNINGPAIR *pair = kerning_pairs + i;
			if ((pair->wFirst  >= GLYPH_RANGE[0]) &&
				(pair->wFirst  <= GLYPH_RANGE[1]) &&
				(pair->wSecond >= GLYPH_RANGE[0]) &&
				(pair->wSecond <= GLYPH_RANGE[1]))
			{
				i32 kern_index_1 = (pair->wFirst  - GLYPH_RANGE[0]);
				i32 kern_index_2 = (pair->wSecond - GLYPH_RANGE[0]);
				kerning_table[kern_index_1 + kern_index_2 * GLYPH_COUNT] = pair->iKernAmount;
			}
		}
	}

	// Get font metrics.
	TEXTMETRIC text_metrics;
	GetTextMetrics(device_context, &text_metrics);

	// NOTE: We don't deinit this array here, since we will pass its pointer to the font structure, the user is left to free that pointer when done with font.
	Array<Glyph> glyphs = array_init<Glyph>();
	for (i32 c = GLYPH_RANGE[0]; c <= GLYPH_RANGE[1]; ++c)
	{
		wchar_t point = (wchar_t)c;
		SIZE size;
		GetTextExtentPoint32W(device_context, &point, 1, &size);

		i32 w = size.cx;
		if (w > BITMAP_MAX_WIDTH)
			w = BITMAP_MAX_WIDTH;

		i32 h = size.cy;
		if (h > BITMAP_MAX_HEIGHT)
			h = BITMAP_MAX_HEIGHT;

		TextOutW(device_context, 0, 0, &point, 1);

		// Loop over the glyph's pixels and delete all empty pixels surrounding the font glyph.
		i32 min_x =  10000;
		i32 min_y =  10000;
		i32 max_x = -10000;
		i32 max_y = -10000;
		u32 *row = (u32 *)bitmap_data + (BITMAP_MAX_HEIGHT - 1) * BITMAP_MAX_WIDTH;
		for (i32 y = 0; y < h; ++y)
		{
			u32 *pixel = row;
			for (i32 x = 0; x < w; ++x)
			{
				if (*pixel != 0)
				{
					if (min_x > x)
						min_x = x;

					if (min_y > y)
						min_y = y;

					if (max_x < x)
						max_x = x;

					if (max_y < y)
						max_y = y;
				}
				++pixel;
			}
			row -= BITMAP_MAX_WIDTH;
		}

		if (min_x <= max_x)
		{
			// Add the current glyph data to the glyph's array.
			Glyph glyph = {};
			{
				glyph.codepoint = point;
				glyph.width     = (max_x - min_x) + APRON * 2;
				glyph.height    = (max_y - min_y) + APRON * 2;
				if (origin_top_left)
					glyph.yadvance = min_y - text_metrics.tmDescent;
				else
					glyph.yadvance = font_height - max_y - text_metrics.tmDescent;
			}
			array_push(glyphs, glyph);

			// Calculate the total width of all glyphs + XPADDING and the maximum height a glyph can be.
			// NOTE: This doesn't take into account the first XPADDING, right before the first glyph in the atlas texture.
			total_glyph_width += glyph.width + XPADDING;
			if (glyph.height > max_glyph_height)
				max_glyph_height = glyph.height;

			// Adjust the kerning amount for the current glyph relative to the rest of the supported glyphs.
			ABC this_abc;
			GetCharABCWidthsW(device_context, glyph.codepoint, glyph.codepoint, &this_abc);
			for (i32 c1 = GLYPH_RANGE[0]; c1 <= GLYPH_RANGE[1]; ++c1)
			{
				i32 kern_index_1 = (glyph.codepoint - GLYPH_RANGE[0]);
				i32 kern_index_2 = (c1 - GLYPH_RANGE[0]);
				kerning_table[kern_index_1 + kern_index_2 * GLYPH_COUNT] += min_x - this_abc.abcA + KERNING_ADJUSTMENT;
			}

			// Allocate a temporary memory buffer to store the current glyph's bitmap.
			i32 index = c - GLYPH_RANGE[0];
			temp_glyph_bitmaps[index] = (u8 *)memory::allocate_zeroed(memory::temp_allocator(), glyph.width * glyph.height * BYTES_PER_PIXEL);

			// Fill the glyph's bitmap.
			u8  *dst_row = temp_glyph_bitmaps[index] + APRON * glyph.width * BYTES_PER_PIXEL;
			u32 *src_row = (u32 *)bitmap_data + (BITMAP_MAX_HEIGHT - APRON - min_y) * BITMAP_MAX_WIDTH;
			for (i32 y = min_y; y <= max_y; ++y)
			{
				u32 *src = (u32 *)src_row + min_x;
				u8 *dst  = dst_row + APRON;
				for (i32 x = min_x; x <= max_x; ++x)
				{
					u32 pixel = *src;
					*dst++ = (u8)(pixel & 0xFF);
					++src;
				}
				dst_row += glyph.width * BYTES_PER_PIXEL;
				src_row -= BITMAP_MAX_WIDTH;
			}
		}
	}

	// NOTE: Account for the extra XPADDING at the left and the YPADDING at the bottom and top.
	u32 atlas_width  = total_glyph_width + XPADDING;
	u32 atlas_height = max_glyph_height  + YPADDING * 2;

	// Fill the atlas texture.
	u8 *atlas = (u8 *)memory::allocate_zeroed(atlas_width * atlas_height * BYTES_PER_PIXEL);
	for (u32 i = 0; i < glyphs.count; ++i)
	{
		Glyph &glyph = glyphs[i];
		u8 *src      = temp_glyph_bitmaps[i];
		for (u32 y = 0; y < glyph.height; ++y)
		{
			for (u32 x = 0; x < glyph.width; ++x)
			{
				if (origin_top_left)
					atlas[x + xoffset + (y + YPADDING) * atlas_width] = src[x + y * glyph.width];
				else
					atlas[x + xoffset + (atlas_height - 1 - (y + YPADDING)) * atlas_width] = src[x + (glyph.height - 1 - y) * glyph.width];
			}
		}

		if (origin_top_left)
		{
			// Min UV coordinates (x1, y1).
			glyph.uv_min_x = (f32)xoffset  / (f32)atlas_width;
			glyph.uv_min_y = (f32)YPADDING / (f32)atlas_height;
			// Max UV coordinates (x2, y2).
			glyph.uv_max_x = (f32)(xoffset  + glyph.width)  / (f32)atlas_width;
			glyph.uv_max_y = (f32)(YPADDING + glyph.height) / (f32)atlas_height;
		}
		else
		{
			// Min UV coordinates (x1, y1).
			glyph.uv_min_x = (f32)xoffset / (f32)atlas_width;
			glyph.uv_min_y = (f32)(atlas_height - YPADDING + 1) / (f32)atlas_height;
			// Max UV coordinates (x2, y2).
			glyph.uv_max_x = (f32)(xoffset + glyph.width) / (f32)atlas_width;
			glyph.uv_max_y = (f32)(atlas_height - YPADDING - glyph.height + 1) / (f32)atlas_height;
		}

		xoffset += glyph.width + XPADDING;
	}

	// Get whitespace size in pixels, and store it in the font structure for later use.
	SIZE whitespace_size;
	wchar_t whitespace_point = ' ';
	GetTextExtentPoint32W(device_context, &whitespace_point, 1, &whitespace_size);

	// Fill font data.
	Platform_Font font = {};
	font.ascent           = text_metrics.tmAscent;
	font.descent          = text_metrics.tmDescent;
	font.line_spacing     = text_metrics.tmHeight + text_metrics.tmExternalLeading;
	font.whitespace_width = whitespace_size.cx;
	font.max_glyph_height = max_glyph_height;
	font.kerning_table    = kerning_table;
	font.glyphs           = glyphs.data;
	font.glyph_count      = (u32)glyphs.count;
	font.atlas            = atlas;
	font.atlas_width      = atlas_width;
	font.atlas_height     = atlas_height;

	return font;
}

void
platform_font_deinit(Platform_Font *font)
{
	memory::deallocate(font->kerning_table);
	memory::deallocate(font->glyphs);
	memory::deallocate(font->atlas);
}