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

typedef void * (*platform_api_proc)(void *api, bool reload);

// TODO: Remove from here.
inline static void
_string_concat(const char *a, const char *b, char *result)
{
	while (*a != '\0')
	{
		*result++ = *a++;
	}

	while(*b != '\0')
	{
		*result++ = *b++;
	}
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

	bool res = CopyFileA(path, path_tmp, false);
	ASSERT(res, "failed to copy library");

	self.handle = LoadLibraryA(path_tmp);
	ASSERT(self.handle, "failed to load library");

	platform_api_proc proc = (platform_api_proc)GetProcAddress((HMODULE)self.handle, "platform_api");
	ASSERT(proc, "failed to get proc platform_api");
	self.api = proc(nullptr, false);
	ASSERT(self.api, "failed to get api");

	WIN32_FILE_ATTRIBUTE_DATA data = {};
	res = GetFileAttributesExA(path, GetFileExInfoStandard, &data);
	ASSERT(res, "failed to get file attributes");
	self.last_write_time = *(u64 *)&data.ftLastWriteTime;

	self.filepath = filepath;

	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)GetProcAddress((HMODULE)self->handle, "platform_api");
		ASSERT(proc, "failed to get proc platform_api");
		self->api = proc(self->api, false);
	}

	FreeLibrary((HMODULE)self->handle);
}

void *
platform_api_load(Platform_Api *self)
{
	char path[128] = {};
	_string_concat(self->filepath, ".dll", path);

	WIN32_FILE_ATTRIBUTE_DATA data = {};
	bool res = GetFileAttributesExA(path, GetFileExInfoStandard, &data);

	u64 last_write_time = *(u64 *)&data.ftLastWriteTime;
	if ((last_write_time == self->last_write_time) || (res == false))
		return self->api;

	res = FreeLibrary((HMODULE)self->handle);
	ASSERT(res, "failed to free library");

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool copy_res = CopyFileA(path, path_tmp, false);

	self->handle = LoadLibraryA(path_tmp);
	ASSERT(self->handle, "failed to load library");

	platform_api_proc proc = (platform_api_proc)GetProcAddress((HMODULE)self->handle, "platform_api");
	ASSERT(proc, "failed to get proc platform_api");
	self->api = proc(self->api, true);
	ASSERT(self->api, "failed to get api");

	// If copying failed we don't update last write time so that we can try copying it again in the next frame.
	if (copy_res)
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
	bool res = false;
	res = VirtualFree(self->ptr, 0, MEM_RELEASE);
	ASSERT(res, "[PLATFORM]: Failed to free virtual memory.");
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

// TODO: Add rest of the keyboard and mouse events.
bool
platform_window_poll(Platform_Window *self)
{
	for (i32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		self->input.keys[i].pressed = false;
		self->input.keys[i].released = false;
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
				return false;

			case WM_LBUTTONDOWN:
				SetCapture((HWND)self->handle);
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].pressed = true;
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].down    = true;
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].press_count++;
				break;
			case WM_LBUTTONUP:
				SetCapture(nullptr);
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].released = true;
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].down     = false;
				self->input.keys[PLATFORM_KEY_MOUSE_LEFT].release_count++;
				break;

			case WM_RBUTTONDOWN:
				SetCapture((HWND)self->handle);
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].pressed = true;
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].down    = true;
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].press_count++;
				break;
			case WM_RBUTTONUP:
				SetCapture(nullptr);
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].released = true;
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].down     = false;
				self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].release_count++;
				break;

			case WM_MBUTTONDOWN:
				SetCapture((HWND)self->handle);
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].pressed = true;
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].down    = true;
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].press_count++;
				break;
			case WM_MBUTTONUP:
				SetCapture(nullptr);
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].released = true;
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].down     = false;
				self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].release_count++;
				break;

			case WM_MOUSEWHEEL:
				self->input.mouse_wheel += (f32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (f32)WHEEL_DELTA;
				break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				i32 was_down =  (msg.lParam & (1 << 30));
				i32 is_down  = !(msg.lParam & (1 << 31));

				switch (msg.wParam)
				{
					case 'W':
						self->input.keys[PLATFORM_KEY_W].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_W].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_W].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_W].down = false;
						break;
					case 'S':
						self->input.keys[PLATFORM_KEY_S].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_S].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_S].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_S].down = false;
						break;
					case 'A':
						self->input.keys[PLATFORM_KEY_A].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_A].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_A].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_A].down = false;
						break;
					case 'D':
						self->input.keys[PLATFORM_KEY_D].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_D].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_D].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_D].down = false;
						break;
					case 'E':
						self->input.keys[PLATFORM_KEY_E].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_E].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_E].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_E].down = false;
						break;
					case 'Q':
						self->input.keys[PLATFORM_KEY_Q].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_Q].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_Q].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_Q].down = false;
						break;
					case VK_UP:
						self->input.keys[PLATFORM_KEY_ARROW_UP].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_ARROW_UP].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_ARROW_UP].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_ARROW_UP].down = false;
						break;
					case VK_DOWN:
						self->input.keys[PLATFORM_KEY_ARROW_DOWN].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_ARROW_DOWN].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_ARROW_DOWN].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_ARROW_DOWN].down = false;
						break;
					case VK_LEFT:
						self->input.keys[PLATFORM_KEY_ARROW_LEFT].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_ARROW_LEFT].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_ARROW_LEFT].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_ARROW_LEFT].down = false;
						break;
					case VK_RIGHT:
						self->input.keys[PLATFORM_KEY_ARROW_RIGHT].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_ARROW_RIGHT].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_ARROW_RIGHT].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_ARROW_RIGHT].down = false;
						break;
					case VK_ESCAPE:
						self->input.keys[PLATFORM_KEY_ESC].pressed  = is_down;
						self->input.keys[PLATFORM_KEY_ESC].released = was_down;
						if (is_down)
							self->input.keys[PLATFORM_KEY_ESC].down = true;
						else if (was_down)
							self->input.keys[PLATFORM_KEY_ESC].down = false;
						break;
				}
			}
		}
	}

	RECT rect;
	GetClientRect((HWND)self->handle, &rect);
	self->width = rect.right - rect.left;
	self->height = rect.bottom - rect.top;

	POINT mouse_point;
	GetCursorPos(&mouse_point);
	ScreenToClient((HWND)self->handle, &mouse_point);

	u32 mouse_point_y_inverted = (self->height - 1) - mouse_point.y;
	self->input.mouse_dx = mouse_point.x - self->input.mouse_x;
	self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
	self->input.mouse_x  = mouse_point.x;
	self->input.mouse_y  = mouse_point_y_inverted; // NOTE(M-Fatah): We want mouse coords to start bottom-left.

	return true;
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

	bool result = SetCurrentDirectoryA(module_path);
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
	::memset(callstack, 0, sizeof(callstack) * frame_count);
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


Font
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
	Font font = {};
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
platform_font_deinit(Font *font)
{
	memory::deallocate(font->kerning_table);
	memory::deallocate(font->glyphs);
	memory::deallocate(font->atlas);
}