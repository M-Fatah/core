#include "core/platform/platform.h"

#include "core/defer.h"
#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"

#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <DbgHelp.h>
#include <math.h>
#include <atomic>

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

// API.

// C++.
bool
platform_path_is_valid(const String &path)
{
	DWORD attributes = ::GetFileAttributes(path.data);
	return attributes != INVALID_FILE_ATTRIBUTES;
}

bool
platform_path_is_file(const String &path)
{
	DWORD attributes = ::GetFileAttributes(path.data);
	return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool
platform_path_is_directory(const String &path)
{
	DWORD attributes = ::GetFileAttributes(path.data);
	return (attributes != INVALID_FILE_ATTRIBUTES && attributes & FILE_ATTRIBUTE_DIRECTORY);
}

String
platform_path_get_absolute(const String &path, memory::Allocator *allocator)
{
	DWORD full_path_length = ::GetFullPathName(path.data, 0, nullptr, nullptr);

	String full_path = string_init(allocator);
	string_resize(full_path, full_path_length);

	validate(::GetFullPathName(path.data, full_path_length, full_path.data, nullptr));

	string_replace(full_path, '\\', '/');

	return full_path;
}

String
platform_path_get_directory(const String &path, memory::Allocator *allocator)
{
	if (!platform_path_is_valid(path))
		return string_literal("");

	String path_directory = string_copy(path, allocator);
	string_replace(path_directory, '\\', '/');

	if (platform_path_is_directory(path))
		return path_directory;

	U64 path_directory_length = string_find_last_of(path_directory, '/');
	if (path_directory_length != U64(-1))
		string_resize(path_directory, path_directory_length);
	return path_directory;
}

String
platform_path_get_current_working_directory(memory::Allocator *allocator)
{
	DWORD path_directory_length = ::GetCurrentDirectory(0, nullptr);

	String path_directory = string_init(allocator);
	string_resize(path_directory, path_directory_length);
	validate(::GetCurrentDirectory(path_directory_length, path_directory.data));
	string_replace(path_directory, '\\', '/');
	return path_directory;
}

String
platform_path_get_temp_directory(memory::Allocator *allocator)
{
	DWORD required_length = ::GetTempPathA(0, nullptr);
	validate(required_length > 0, "[PLATFORM][WINDOWS]: Failed to get temp path length.");

	String path = string_with_capacity(required_length + 1, allocator);
	DWORD path_length = ::GetTempPathA((DWORD)path.capacity, path.data);
	validate(path_length > 0 && path_length < path.capacity, "[PLATFORM][WINDOWS]: Failed to get temp path.");

	string_resize(path, path_length);
	string_replace(path, '\\', '/');
	return path;
}

void
platform_path_set_current_working_directory(const String &path)
{
	String path_temp = string_copy(path, memory::temp_allocator());
	string_replace(path_temp, '\\', '/');
	validate(::SetCurrentDirectory(path_temp.data));
}

String
platform_path_get_executable_path(memory::Allocator *allocator)
{
	String path_executable_temp = string_with_capacity(4096, memory::temp_allocator());
	U64 path_executable_length = ::GetModuleFileName(0, path_executable_temp.data, (DWORD)path_executable_temp.count);
	string_resize(path_executable_temp, path_executable_length);
	string_replace(path_executable_temp, '\\', '/');
	return string_copy(path_executable_temp, allocator);
}

String
platform_path_get_file_name(const String &path, memory::Allocator *allocator)
{
	String path_temp = string_copy(path, memory::temp_allocator());
	string_replace(path_temp, "\\", "/");
	Array<String> splits = string_split(path_temp, "/", true, memory::temp_allocator());
	return string_copy(array_back(splits), allocator);
}

String
platform_path_read_file(const String &path, memory::Allocator *allocator)
{
	String content = string_init(allocator);

	HANDLE file_handle = ::CreateFileA(path.data, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return content;

	U64 file_size = platform_file_size(path.data);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);

	DWORD bytes_read = 0;
	::ReadFile(file_handle, content.data, (U32)content.count, &bytes_read, 0);
	validate(::CloseHandle(file_handle), "[PLATFORM][WINDOWS]: Failed to close file handle.");
	validate(content.count == bytes_read);

	return content;
}

U64
platform_path_write_file(const String &path, Memory_Block block)
{
	HANDLE file_handle = ::CreateFileA(path.data, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return 0;

	DWORD bytes_written = 0;
	::WriteFile(file_handle, block.data, (DWORD)block.size, &bytes_written, 0);
	validate(::CloseHandle(file_handle));
	validate(bytes_written == block.size);

	return (U64)bytes_written;
}

Platform_File_Handle
platform_file_open(const String &path, Platform_File_Mode mode)
{
	DWORD access   = 0;
	DWORD creation = 0;

	switch (mode)
	{
		case PLATFORM_FILE_MODE_READ:
			access   = GENERIC_READ;
			creation = OPEN_EXISTING;
			break;
		case PLATFORM_FILE_MODE_WRITE:
			access   = GENERIC_WRITE;
			creation = CREATE_ALWAYS;
			break;
		case PLATFORM_FILE_MODE_READ_WRITE:
			access   = GENERIC_READ | GENERIC_WRITE;
			creation = OPEN_ALWAYS;
			break;
		case PLATFORM_FILE_MODE_APPEND:
			access   = FILE_APPEND_DATA;
			creation = OPEN_ALWAYS;
			break;
	}

	HANDLE handle = ::CreateFileA(path.data, access, FILE_SHARE_READ, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
	return handle == INVALID_HANDLE_VALUE ? PLATFORM_FILE_HANDLE_INVALID : (Platform_File_Handle)handle;
}

void
platform_file_close(Platform_File_Handle handle)
{
	if (handle)
		::CloseHandle((HANDLE)handle);
}

U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size)
{
	DWORD bytes_read = 0;
	::ReadFile((HANDLE)handle, data, (DWORD)size, &bytes_read, nullptr);
	return (U64)bytes_read;
}

U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size)
{
	DWORD bytes_written = 0;
	::WriteFile((HANDLE)handle, data, (DWORD)size, &bytes_written, nullptr);
	return (U64)bytes_written;
}

bool
platform_file_seek(Platform_File_Handle handle, I64 offset, Platform_File_Seek_Origin origin)
{
	DWORD method = FILE_BEGIN;
	switch (origin)
	{
		case PLATFORM_FILE_SEEK_ORIGIN_BEGIN:   method = FILE_BEGIN;   break;
		case PLATFORM_FILE_SEEK_ORIGIN_CURRENT: method = FILE_CURRENT; break;
		case PLATFORM_FILE_SEEK_ORIGIN_END:     method = FILE_END;     break;
	}
	LARGE_INTEGER li;
	li.QuadPart = offset;
	return ::SetFilePointerEx((HANDLE)handle, li, nullptr, method) != 0;
}

U64
platform_file_tell(Platform_File_Handle handle)
{
	LARGE_INTEGER li     = {};
	LARGE_INTEGER result = {};
	::SetFilePointerEx((HANDLE)handle, li, &result, FILE_CURRENT);
	return (U64)result.QuadPart;
}

U64
platform_file_size(Platform_File_Handle handle)
{
	LARGE_INTEGER size = {};
	::GetFileSizeEx((HANDLE)handle, &size);
	return (U64)size.QuadPart;
}

Array<String>
platform_path_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);

	String search_path = string_copy(directory, memory::temp_allocator());
	if (search_path.count > 0 && search_path.data[search_path.count - 1] != '/')
		array_push(search_path, '/');
	string_append(search_path, "*.*");

	WIN32_FIND_DATA find_data = {};
	HANDLE find_handle = ::FindFirstFile(search_path.data, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return files;
	DEFER(validate(::FindClose(find_handle), "[PLATFORM][WIN32]: Failed to close find handle."););

	do
	{
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		String file_name = string_from(find_data.cFileName, memory::temp_allocator());
		if (extension_filter.count > 0)
		{
			U64 extension_position = string_find_last_of(file_name, '.');
			if (extension_position == U64(-1))
				continue;

			String file_extension = string_with_capacity(file_name.count - extension_position - 1, memory::temp_allocator());
			for (U64 i = extension_position + 1; i < file_name.count; ++i)
				string_append(file_extension, file_name.data[i]);

			if (file_extension != extension_filter)
				continue;
		}

		array_push(files, file_name);
	} while (::FindNextFile(find_handle, &find_data));

	return files;
}

// C.
Platform_Api
platform_api_init(const char *filepath)
{
	Platform_Api self = {};

	char path[128] = {};
	_string_concat(filepath, ".dll", path);

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool result = ::CopyFileA(path, path_tmp, false);
	validate(result, "[PLATFORM]: Failed to copy library.");

	self.handle = ::LoadLibraryA(path_tmp);
	validate(self.handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::GetProcAddress((HMODULE)self.handle, "platform_api");
	validate(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self.api = proc(nullptr, PLATFORM_API_STATE_INIT);
	validate(self.api, "[PLATFORM]: Failed to get api.");

	WIN32_FILE_ATTRIBUTE_DATA data = {};
	result = ::GetFileAttributesExA(path, GetFileExInfoStandard, &data);
	validate(result, "[PLATFORM]: Failed to get file attributes.");

	self.last_write_time = *(I64 *)&data.ftLastWriteTime;
	::strcpy_s(self.filepath, filepath);

	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)GetProcAddress((HMODULE)self->handle, "platform_api");
		validate(proc, "failed to get proc platform_api");
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

	I64 last_write_time = *(I64 *)&data.ftLastWriteTime;
	if ((last_write_time == self->last_write_time) || (result == false))
		return self->api;

	result = ::FreeLibrary((HMODULE)self->handle);
	validate(result, "[PLATFORM]: Failed to free library.");

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool copy_result = ::CopyFileA(path, path_tmp, false);

	self->handle = ::LoadLibraryA(path_tmp);
	validate(self->handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::GetProcAddress((HMODULE)self->handle, "platform_api");
	validate(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self->api = proc(self->api, PLATFORM_API_STATE_LOAD);
	validate(self->api, "[PLATFORM]: Failed to get api.");

	// If copying failed we don't update last write time so that we can try copying it again in the next frame.
	if (copy_result)
		self->last_write_time = last_write_time;

	return self->api;
}


U64
platform_virtual_memory_get_page_size()
{
	SYSTEM_INFO system_info = {};
	::GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

U64
platform_virtual_memory_page_align(U64 size)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	return u64_align_up(size, page_size);
}

Memory_Block
platform_virtual_memory_reserve(U64 size)
{
	U64 aligned_size = platform_virtual_memory_page_align(size);
	if (aligned_size == 0)
		return {};

	void *data = ::VirtualAlloc(nullptr, aligned_size, MEM_RESERVE, PAGE_NOACCESS);
	return Memory_Block{data, data ? aligned_size : 0};
}

bool
platform_virtual_memory_commit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][WINDOWS]: Cannot commit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][WINDOWS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][WINDOWS]: Virtual memory block size is not page-aligned.");

	return ::VirtualAlloc(block.data, block.size, MEM_COMMIT, PAGE_READWRITE) == block.data;
}

bool
platform_virtual_memory_decommit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][WINDOWS]: Cannot decommit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][WINDOWS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][WINDOWS]: Virtual memory block size is not page-aligned.");

	return ::VirtualFree(block.data, block.size, MEM_DECOMMIT) != 0;
}

void
platform_virtual_memory_release(Memory_Block block)
{
	if (block.data == nullptr)
		return;

	U64 page_size = platform_virtual_memory_get_page_size();
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][WINDOWS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][WINDOWS]: Virtual memory block size is not page-aligned.");

	[[maybe_unused]] bool result = ::VirtualFree(block.data, 0, MEM_RELEASE);
	validate(result, "[PLATFORM][WINDOWS]: Failed to release virtual memory.");
}

struct Platform_Task
{
	void (*function)(void *);
	void *user_data;
};

struct Platform_Thread
{
	HANDLE handle;
	std::atomic<bool> is_running;
	Platform_Task task;
};

static DWORD
_platform_thread_main_routine(void *user_data)
{
	Platform_Thread *self = (Platform_Thread *)user_data;
	while (self->is_running)
	{
		if (self->task.function)
		{
			self->task.function(self->task.user_data);
			self->task = {};
		}
	}
	return 0;
}

Platform_Thread *
platform_thread_init()
{
	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->is_running = true;
	self->handle = ::CreateThread(nullptr, 0, _platform_thread_main_routine, self, 0, nullptr);
	return self;
}

void
platform_thread_deinit(Platform_Thread *self)
{
	self->is_running = false;
	::CloseHandle(self->handle);
	memory::deallocate(self);
}

void
platform_thread_run(Platform_Thread *self, void (*function)(void *), void *user_data)
{
	Platform_Task task {
		.function  = function,
		.user_data = user_data
	};
	self->task = task;
}

Platform_Window
platform_window_init(U32 width, U32 height, const char *title)
{
	validate(width > 0 && height > 0, "[PLATFORM]: Windows cannot have zero width or height.");

	Platform_Window self = {};

	WNDCLASSEXA wc = {};
	wc.cbSize		 = sizeof(wc);
	wc.style		 = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = _window_proc;
	wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "Mist_Window_Class";
	[[maybe_unused]] ATOM class_atom = RegisterClassExA(&wc);
	validate(class_atom != 0, "[PLATFORM]: Failed to register window class.");

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
	validate(self.handle, "[PLATFORM]: Failed to create window.");

	self.width  = width;
	self.height = height;

	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	bool res = false;
	res = DestroyWindow((HWND)self->handle);
	validate(res, "[PLATFORM]: Failed to destroy window.");
}

bool
platform_window_poll(Platform_Window *self)
{
	for (I32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
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
						self->input.mouse_wheel += (F32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (F32)WHEEL_DELTA;
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
		U32 mouse_point_y_inverted = (self->height - 1) - mouse_point.y;
		self->input.mouse_dx = mouse_point.x - self->input.mouse_x;
		self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
		self->input.mouse_x  = mouse_point.x;
		self->input.mouse_y  = mouse_point_y_inverted;
	}

	return true;
}

void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_connection)
{
	if (native_handle)
		*native_handle = self->handle;
	if (native_connection)
		*native_connection = nullptr;
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
	validate(result, "[PLATFORM]: Failed to set current directory.");
}

bool
platform_file_exists(const char *filepath)
{
	DWORD attributes = ::GetFileAttributes(filepath);
	return attributes != INVALID_FILE_ATTRIBUTES;
}

U64
platform_file_size(const char *filepath)
{
	WIN32_FILE_ATTRIBUTE_DATA data = {};
	if (::GetFileAttributesExA(filepath, GetFileExInfoStandard, &data) == false)
		return 0;

	LARGE_INTEGER size;
	size.HighPart = data.nFileSizeHigh;
	size.LowPart  = data.nFileSizeLow;
	return size.QuadPart;
}

U64
platform_file_read(const char *filepath, Memory_Block block)
{
	HANDLE file_handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return 0;

	// TODO(M-Fatah): Handle reading files that are bigger than 4GB size.
	DWORD bytes_read = 0;
	ReadFile(file_handle, block.data, (U32)block.size, &bytes_read, 0);
	CloseHandle(file_handle);

	return (U64)bytes_read;
}

String
platform_file_read(const String &file_path, memory::Allocator *allocator)
{
	String content = string_init(allocator);

	HANDLE file_handle = ::CreateFileA(file_path.data, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return content;

	U64 file_size = platform_file_size(file_path.data);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);

	DWORD bytes_read = 0;
	::ReadFile(file_handle, content.data, (U32)content.count, &bytes_read, 0);
	validate(::CloseHandle(file_handle), "[PLATFORM][WINDOWS]: Failed to close file handle.");

	validate(content.count == bytes_read);

	return content;
}

U64
platform_file_write(const char *filepath, Memory_Block block)
{
	HANDLE file_handle = CreateFileA(filepath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return 0;

	// TODO(M-Fatah): Properly handle large files (files with size over 4GB as a single file).
	DWORD bytes_written = 0;
	WriteFile(file_handle, block.data, (DWORD)block.size, &bytes_written, 0);
	CloseHandle(file_handle);

	return (U64)bytes_written;
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
platform_file_dialog_open(char *path, U32 path_length, const char *filters)
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
platform_file_dialog_save(char *path, U32 path_length, const char *filters)
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


U64
platform_query_microseconds()
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER ticks;
	if (QueryPerformanceFrequency(&frequency) == false)
	{
		validate(false, "[PLATFORM]: Failed to query performance frequency.");
	}
	if (QueryPerformanceCounter(&ticks) == false)
	{
		validate(false, "[PLATFORM]: Failed to query performance counter.");
	}
	return ticks.QuadPart * 1000000 / frequency.QuadPart;
}

void
platform_sleep_set_period(U32 period)
{
	if (timeBeginPeriod(period) != TIMERR_NOERROR)
	{
		validate(false, "[PLATFORM]: Failed to set time begin period.");
	}
}

void
platform_sleep(U32 milliseconds)
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

U32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	::memset(callstack, 0, frame_count * sizeof(callstack));
	return CaptureStackBackTrace(1, frame_count, callstack, NULL);
#else
	return 0;
#endif
}

void
platform_callstack_log([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
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
	constexpr U64 MAX_NAME_LENGTH = 256;
	char symbol_buffer[MAX_NAME_LENGTH + sizeof(SYMBOL_INFO)];

	SYMBOL_INFO *symbol_info = (SYMBOL_INFO *)symbol_buffer;
	::memset(symbol_info, 0, sizeof(SYMBOL_INFO));
	symbol_info->MaxNameLen   = MAX_NAME_LENGTH;
	symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);

	log_warning("callstack:");
	for (U64 i = 0; i < frame_count; ++i)
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

		log_warning(
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
platform_font_init(const char *filepath, const char *face_name, U32 font_height, bool origin_top_left)
{
	// Supported glyph range.
	constexpr I32 GLYPH_RANGE[2]             = {'!', '~'};
	constexpr U32 GLYPH_COUNT                = (GLYPH_RANGE[1] + 1) - GLYPH_RANGE[0];

	// Font bitmap config. This is used to rasterize each glyph by winapi.
	constexpr U32 BITMAP_MAX_WIDTH           = 1024;
	constexpr U32 BITMAP_MAX_HEIGHT          = 1024;
	constexpr U32 BYTES_PER_PIXEL            = 1;
	constexpr U32 APRON                      = 1;
	U8 *temp_glyph_bitmaps[GLYPH_COUNT] = {};

	// Atlas texture config.
	constexpr U32 XPADDING                   = 3;
	constexpr U32 YPADDING                   = 3;
	U32 xoffset                              = XPADDING;
	U32 total_glyph_width                    = 0;
	U32 max_glyph_height                     = 0;

	// Kerning config.
	constexpr U32 KERNING_ADJUSTMENT         = 3;
	Memory_Block kerning_table_block         = memory::allocate_zeroed(GLYPH_COUNT * GLYPH_COUNT * sizeof(I32), alignof(I32));
	I32 *kerning_table                       = (I32 *)kerning_table_block.data;

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
	U32 kerning_pair_count           = GetKerningPairsW(device_context, 0, 0);
	Memory_Block kerning_pairs_block = memory::allocate(memory::temp_allocator(), kerning_pair_count * sizeof(KERNINGPAIR), alignof(KERNINGPAIR));
	KERNINGPAIR *kerning_pairs = (KERNINGPAIR *)kerning_pairs_block.data;
	GetKerningPairsW(device_context, kerning_pair_count, kerning_pairs);
	if (kerning_pair_count > 0)
	{
		for (U32 i = 0; i <= kerning_pair_count; ++i)
		{
			KERNINGPAIR *pair = kerning_pairs + i;
			if ((pair->wFirst  >= GLYPH_RANGE[0]) &&
				(pair->wFirst  <= GLYPH_RANGE[1]) &&
				(pair->wSecond >= GLYPH_RANGE[0]) &&
				(pair->wSecond <= GLYPH_RANGE[1]))
			{
				I32 kern_index_1 = (pair->wFirst  - GLYPH_RANGE[0]);
				I32 kern_index_2 = (pair->wSecond - GLYPH_RANGE[0]);
				kerning_table[kern_index_1 + kern_index_2 * GLYPH_COUNT] = pair->iKernAmount;
			}
		}
	}

	// Get font metrics.
	TEXTMETRIC text_metrics;
	GetTextMetrics(device_context, &text_metrics);

	// NOTE: We don't deinit this array here, since we will pass its pointer to the font structure, the user is left to free that pointer when done with font.
	Array<Glyph> glyphs = array_init<Glyph>();
	for (I32 c = GLYPH_RANGE[0]; c <= GLYPH_RANGE[1]; ++c)
	{
		wchar_t point = (wchar_t)c;
		SIZE size;
		GetTextExtentPoint32W(device_context, &point, 1, &size);

		I32 w = size.cx;
		if (w > BITMAP_MAX_WIDTH)
			w = BITMAP_MAX_WIDTH;

		I32 h = size.cy;
		if (h > BITMAP_MAX_HEIGHT)
			h = BITMAP_MAX_HEIGHT;

		TextOutW(device_context, 0, 0, &point, 1);

		// Loop over the glyph's pixels and delete all empty pixels surrounding the font glyph.
		I32 min_x =  10000;
		I32 min_y =  10000;
		I32 max_x = -10000;
		I32 max_y = -10000;
		U32 *row = (U32 *)bitmap_data + (BITMAP_MAX_HEIGHT - 1) * BITMAP_MAX_WIDTH;
		for (I32 y = 0; y < h; ++y)
		{
			U32 *pixel = row;
			for (I32 x = 0; x < w; ++x)
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
			for (I32 c1 = GLYPH_RANGE[0]; c1 <= GLYPH_RANGE[1]; ++c1)
			{
				I32 kern_index_1 = (glyph.codepoint - GLYPH_RANGE[0]);
				I32 kern_index_2 = (c1 - GLYPH_RANGE[0]);
				kerning_table[kern_index_1 + kern_index_2 * GLYPH_COUNT] += min_x - this_abc.abcA + KERNING_ADJUSTMENT;
			}

			// Allocate a temporary memory buffer to store the current glyph's bitmap.
			I32 index = c - GLYPH_RANGE[0];
			Memory_Block temp_glyph_bitmap_block = memory::allocate_zeroed(memory::temp_allocator(), glyph.width * glyph.height * BYTES_PER_PIXEL, alignof(U8));
			temp_glyph_bitmaps[index] = (U8 *)temp_glyph_bitmap_block.data;

			// Fill the glyph's bitmap.
			U8  *dst_row = temp_glyph_bitmaps[index] + APRON * glyph.width * BYTES_PER_PIXEL;
			U32 *src_row = (U32 *)bitmap_data + (BITMAP_MAX_HEIGHT - APRON - min_y) * BITMAP_MAX_WIDTH;
			for (I32 y = min_y; y <= max_y; ++y)
			{
				U32 *src = (U32 *)src_row + min_x;
				U8 *dst  = dst_row + APRON;
				for (I32 x = min_x; x <= max_x; ++x)
				{
					U32 pixel = *src;
					*dst++ = (U8)(pixel & 0xFF);
					++src;
				}
				dst_row += glyph.width * BYTES_PER_PIXEL;
				src_row -= BITMAP_MAX_WIDTH;
			}
		}
	}

	// NOTE: Account for the extra XPADDING at the left and the YPADDING at the bottom and top.
	U32 atlas_width  = total_glyph_width + XPADDING;
	U32 atlas_height = max_glyph_height  + YPADDING * 2;

	// Fill the atlas texture.
	Memory_Block atlas_block = memory::allocate_zeroed(atlas_width * atlas_height * BYTES_PER_PIXEL, alignof(U8));
	U8 *atlas = (U8 *)atlas_block.data;
	for (U32 i = 0; i < glyphs.count; ++i)
	{
		Glyph &glyph = glyphs[i];
		U8 *src      = temp_glyph_bitmaps[i];
		for (U32 y = 0; y < glyph.height; ++y)
		{
			for (U32 x = 0; x < glyph.width; ++x)
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
			glyph.uv_min_x = (F32)xoffset  / (F32)atlas_width;
			glyph.uv_min_y = (F32)YPADDING / (F32)atlas_height;
			// Max UV coordinates (x2, y2).
			glyph.uv_max_x = (F32)(xoffset  + glyph.width)  / (F32)atlas_width;
			glyph.uv_max_y = (F32)(YPADDING + glyph.height) / (F32)atlas_height;
		}
		else
		{
			// Min UV coordinates (x1, y1).
			glyph.uv_min_x = (F32)xoffset / (F32)atlas_width;
			glyph.uv_min_y = (F32)(atlas_height - YPADDING + 1) / (F32)atlas_height;
			// Max UV coordinates (x2, y2).
			glyph.uv_max_x = (F32)(xoffset + glyph.width) / (F32)atlas_width;
			glyph.uv_max_y = (F32)(atlas_height - YPADDING - glyph.height + 1) / (F32)atlas_height;
		}

		xoffset += glyph.width + XPADDING;
	}

	// Get whitespace size in pixels, and store it in the font structure for later use.
	SIZE whitespace_size;
	wchar_t whitespace_point = ' ';
	GetTextExtentPoint32W(device_context, &whitespace_point, 1, &whitespace_size);

	// Fill font data.
	Platform_Font font = {};
	font.ascent              = text_metrics.tmAscent;
	font.descent             = text_metrics.tmDescent;
	font.line_spacing        = text_metrics.tmHeight + text_metrics.tmExternalLeading;
	font.whitespace_width    = whitespace_size.cx;
	font.max_glyph_height    = max_glyph_height;
	font.kerning_table       = kerning_table;
	font.kerning_table_block = kerning_table_block;
	font.glyphs              = glyphs.data;
	font.glyph_count         = (U32)glyphs.count;
	font.glyphs_block        = Memory_Block{glyphs.data, sizeof(Glyph) * glyphs.capacity};
	font.atlas               = atlas;
	font.atlas_width         = atlas_width;
	font.atlas_height        = atlas_height;
	font.atlas_block         = atlas_block;

	return font;
}

void
platform_font_deinit(Platform_Font *font)
{
	memory::deallocate(font->kerning_table_block);
	memory::deallocate(font->glyphs_block);
	memory::deallocate(font->atlas_block);
}