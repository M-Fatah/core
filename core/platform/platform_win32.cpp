#include "core/platform/platform.h"

#include "core/defer.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/memory/allocator.h"
#include "core/containers/array.h"

#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <ShlObj.h>
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

inline static Platform_Window_Orientation
_platform_win32_window_orientation(U32 width, U32 height)
{
	if (width == 0 || height == 0)
		return PLATFORM_WINDOW_ORIENTATION_UNKNOWN;
	return width >= height ? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE : PLATFORM_WINDOW_ORIENTATION_PORTRAIT;
}

inline static Platform_Window_Metrics
_platform_win32_window_metrics(HWND window, U32 width, U32 height)
{
	F32 dpi_x = 96.0f;
	F32 dpi_y = 96.0f;
	HDC dc = ::GetDC(window);
	if (dc)
	{
		dpi_x = (F32)::GetDeviceCaps(dc, LOGPIXELSX);
		dpi_y = (F32)::GetDeviceCaps(dc, LOGPIXELSY);
		::ReleaseDC(window, dc);
	}

	return Platform_Window_Metrics {
		.content_rect = Platform_Window_Rect { .x = 0, .y = 0, .width = width, .height = height },
		.safe_area = {},
		.density_scale = dpi_x / 96.0f,
		.dpi_x = dpi_x,
		.dpi_y = dpi_y,
		.orientation = _platform_win32_window_orientation(width, height)
	};
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

inline static void
_platform_win32_text_input_events_reset(Platform_Input &input)
{
	for (U64 i = 0; i < input.text_input_events.count; ++i)
	{
		string_deinit(input.text_input_events[i].text);
		input.text_input_events[i] = {};
	}
	input.text_input_events.count = 0;
}

inline static void
_platform_win32_text_input_push_utf16(Platform_Window &window, wchar_t character)
{
	if (!window.text_input.enabled)
		return;

	wchar_t text_wide[2] = {};
	I32 text_wide_count = 1;
	if (character == L'\r')
	{
		text_wide[0] = L'\n';
		window.text_input_pending_surrogate = 0;
	}
	else if (character >= 0xd800 && character <= 0xdbff)
	{
		window.text_input_pending_surrogate = (U16)character;
		return;
	}
	else if (character >= 0xdc00 && character <= 0xdfff)
	{
		if (window.text_input_pending_surrogate == 0)
			return;

		text_wide[0] = (wchar_t)window.text_input_pending_surrogate;
		text_wide[1] = character;
		text_wide_count = 2;
		window.text_input_pending_surrogate = 0;
	}
	else
	{
		text_wide[0] = character;
		window.text_input_pending_surrogate = 0;
	}

	I32 utf8_count = ::WideCharToMultiByte(CP_UTF8, 0, text_wide, text_wide_count, nullptr, 0, nullptr, nullptr);
	if (utf8_count <= 0)
		return;

	char text[8] = {};
	::WideCharToMultiByte(CP_UTF8, 0, text_wide, text_wide_count, text, utf8_count, nullptr, nullptr);
	array_push(window.input.text_input_events, Platform_Text_Input_Event {
		.type = PLATFORM_TEXT_INPUT_EVENT_COMMIT,
		.text = string_from(text, text + utf8_count)
	});
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
	DWORD required_length = ::GetFullPathName(path.data, 0, nullptr, nullptr);
	validate(required_length > 0, "[PLATFORM][WIN32]: Failed to get absolute path length.");

	String full_path = string_with_capacity(required_length + 1, allocator);
	DWORD full_path_length = ::GetFullPathName(path.data, (DWORD)full_path.capacity, full_path.data, nullptr);
	validate(full_path_length > 0 && full_path_length < full_path.capacity, "[PLATFORM][WIN32]: Failed to get absolute path.");
	string_resize(full_path, full_path_length);

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
	DWORD required_length = ::GetCurrentDirectory(0, nullptr);
	validate(required_length > 0, "[PLATFORM][WIN32]: Failed to get current working directory length.");

	String path_directory = string_with_capacity(required_length + 1, allocator);
	DWORD path_directory_length = ::GetCurrentDirectory((DWORD)path_directory.capacity, path_directory.data);
	validate(path_directory_length > 0 && path_directory_length < path_directory.capacity, "[PLATFORM][WIN32]: Failed to get current working directory.");
	string_resize(path_directory, path_directory_length);
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

String
platform_path_get_app_data_directory(memory::Allocator *allocator)
{
	char path[MAX_PATH] = {};
	HRESULT result = ::SHGetFolderPathA(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, path);
	if (SUCCEEDED(result))
	{
		String app_data = string_from(path, allocator);
		string_replace(app_data, '\\', '/');
		if (app_data.count > 0 && app_data[app_data.count - 1] != '/')
			string_append(app_data, '/');
		return app_data;
	}

	return platform_path_get_temp_directory(allocator);
}

String
platform_path_get_cache_directory(memory::Allocator *allocator)
{
	char path[MAX_PATH] = {};
	HRESULT result = ::SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, path);
	if (SUCCEEDED(result))
	{
		String cache = string_from(path, allocator);
		string_replace(cache, '\\', '/');
		if (cache.count > 0 && cache[cache.count - 1] != '/')
			string_append(cache, '/');
		return cache;
	}

	return platform_path_get_temp_directory(allocator);
}

String
platform_environment_variable_get(const String &name, memory::Allocator *allocator)
{
	DWORD required_length = ::GetEnvironmentVariableA(name.data, nullptr, 0);
	if (required_length == 0)
		return string_literal("");

	String result = string_with_capacity(required_length, allocator);
	DWORD length = ::GetEnvironmentVariableA(name.data, result.data, (DWORD)result.capacity);
	if (length == 0 || length >= result.capacity)
	{
		string_deinit(result);
		return string_literal("");
	}

	string_resize(result, length);
	return result;
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
	U64 path_executable_length = ::GetModuleFileName(0, path_executable_temp.data, (DWORD)path_executable_temp.capacity);
	string_resize(path_executable_temp, path_executable_length);
	string_replace(path_executable_temp, '\\', '/');
	return string_copy(path_executable_temp, allocator);
}

String
platform_path_get_current_module_path(memory::Allocator *allocator)
{
	HMODULE current_module = nullptr;
	if (!::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&platform_path_get_current_module_path, &current_module))
		return string_literal("");

	String path_module_temp = string_with_capacity(4096, memory::temp_allocator());
	U64 path_module_length = ::GetModuleFileNameA(current_module, path_module_temp.data, (DWORD)path_module_temp.capacity);
	string_resize(path_module_temp, path_module_length);
	string_replace(path_module_temp, '\\', '/');
	return string_copy(path_module_temp, allocator);
}

String
platform_path_get_file_name(const String &path, memory::Allocator *allocator)
{
	if (path.count == 0)
		return string_init(allocator);

	U64 slash = string_find_last_of(path, '/');
	U64 backslash = string_find_last_of(path, '\\');
	U64 separator = backslash != U64(-1) && (slash == U64(-1) || backslash > slash) ? backslash : slash;
	return string_from(path.data + (separator == U64(-1) ? 0 : separator + 1), path.data + path.count, allocator);
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

inline static bool
_platform_win32_extension_matches(const String &file_name, const String &extension_filter)
{
	if (extension_filter.count == 0)
		return true;

	U64 extension_position = string_find_last_of(file_name, '.');
	if (extension_position == U64(-1))
		return false;

	U64 extension_count = file_name.count - extension_position - 1;
	if (extension_count != extension_filter.count)
		return false;

	for (U64 i = 0; i < extension_count; ++i)
	{
		if (file_name.data[extension_position + 1 + i] != extension_filter.data[i])
			return false;
	}

	return true;
}

inline static String
_platform_win32_path_join(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (directory.count == 0 || name.count == 0)
		return string_init(allocator);

	String result = string_copy(directory, allocator);
	string_replace(result, '\\', '/');
	if (result[result.count - 1] != '/')
		string_append(result, '/');
	string_append(result, name);
	string_replace(result, '\\', '/');
	return result;
}

inline static String
_platform_win32_path_parent(const String &path, memory::Allocator *allocator)
{
	String result = string_copy(path, allocator);
	string_replace(result, '\\', '/');
	U64 slash = string_find_last_of(result, '/');
	if (slash == U64(-1))
	{
		string_clear(result);
		string_append(result, '.');
		return result;
	}
	string_resize(result, slash);
	return result;
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
		if (!_platform_win32_extension_matches(file_name, extension_filter))
			continue;

		array_push(files, string_copy(file_name, allocator));
	} while (::FindNextFile(find_handle, &find_data));

	return files;
}

inline static void
_platform_win32_path_list_files_recursive(Array<String> &files, const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	String search_path = string_copy(directory, memory::temp_allocator());
	if (search_path.count > 0 && search_path.data[search_path.count - 1] != '/')
		array_push(search_path, '/');
	string_append(search_path, "*.*");

	WIN32_FIND_DATA find_data = {};
	HANDLE find_handle = ::FindFirstFile(search_path.data, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return;
	DEFER(validate(::FindClose(find_handle), "[PLATFORM][WIN32]: Failed to close recursive find handle."););

	do
	{
		String file_name = string_from(find_data.cFileName, memory::temp_allocator());
		if (file_name == "." || file_name == "..")
			continue;

		String child_path = _platform_win32_path_join(directory, file_name, memory::temp_allocator());
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_platform_win32_path_list_files_recursive(files, child_path, extension_filter, allocator);
			continue;
		}

		if (_platform_win32_extension_matches(file_name, extension_filter))
			array_push(files, string_copy(child_path, allocator));
	} while (::FindNextFile(find_handle, &find_data));
}

Array<String>
platform_path_list_files_recursive(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);
	_platform_win32_path_list_files_recursive(files, directory, extension_filter, allocator);
	return files;
}

String
platform_path_create_file(const String &directory, const String &name, memory::Allocator *allocator)
{
	String result = _platform_win32_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	HANDLE handle = ::CreateFileA(result.data, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	validate(::CloseHandle(handle), "[PLATFORM][WIN32]: Failed to close created file.");
	return result;
}

String
platform_path_create_directory(const String &directory, const String &name, memory::Allocator *allocator)
{
	String result = _platform_win32_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (!::CreateDirectoryA(result.data, nullptr))
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

String
platform_path_rename(const String &path, const String &name, memory::Allocator *allocator)
{
	String directory = _platform_win32_path_parent(path, memory::temp_allocator());
	String result = _platform_win32_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (!::MoveFileExA(path.data, result.data, MOVEFILE_COPY_ALLOWED))
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

String
platform_path_move(const String &path, const String &directory, memory::Allocator *allocator)
{
	String name = platform_path_get_file_name(path, memory::temp_allocator());
	String result = _platform_win32_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (!::MoveFileExA(path.data, result.data, MOVEFILE_COPY_ALLOWED))
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

String
platform_resource_read(const String &path, memory::Allocator *allocator)
{
	return platform_path_read_file(path, allocator);
}

Array<String>
platform_resource_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	return platform_path_list_files(directory, extension_filter, allocator);
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

U32
platform_get_logical_processor_count()
{
	SYSTEM_INFO system_info = {};
	::GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors ? (U32)system_info.dwNumberOfProcessors : U32(1);
}

struct Platform_Thread
{
	HANDLE handle;
	Platform_Thread_Function function;
	void *data;
	String name;
	bool joined;
};

struct Platform_Window_Context
{
	HWND window;
	WINDOWPLACEMENT windowed_placement;
	DWORD windowed_style;
	DWORD windowed_ex_style;
	bool fullscreen;
	bool keep_screen_on;
};

static DWORD
_platform_thread_main_routine(void *thread)
{
	Platform_Thread *self = (Platform_Thread *)thread;
	if (self->name.count != 0)
		platform_thread_set_current_name(self->name.data);
	self->function(self->data);
	return 0;
}

Platform_Thread *
platform_thread_init(Platform_Thread_Desc desc)
{
	validate(desc.function != nullptr, "[PLATFORM][WINDOWS]: Thread function is not valid.");

	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->function = desc.function;
	self->data = desc.data;
	if (desc.name != nullptr)
		self->name = string_from(desc.name);
	self->handle = ::CreateThread(nullptr, 0, _platform_thread_main_routine, self, 0, nullptr);
	validate(self->handle != nullptr, "[PLATFORM][WINDOWS]: Failed to create thread.");
	return self;
}

void
platform_thread_deinit(Platform_Thread *self)
{
	platform_thread_join(self);
	string_deinit(self->name);
	memory::deallocate(self);
}

void
platform_thread_join(Platform_Thread *self)
{
	if (self->joined)
		return;

	DWORD wait_result = ::WaitForSingleObject(self->handle, INFINITE);
	validate(wait_result == WAIT_OBJECT_0, "[PLATFORM][WINDOWS]: Failed to join thread.");
	bool closed = ::CloseHandle(self->handle);
	validate(closed, "[PLATFORM][WINDOWS]: Failed to close thread handle.");
	self->handle = nullptr;
	self->joined = true;
}

void
platform_thread_sleep(U32 milliseconds)
{
	Sleep(milliseconds);
}

void
platform_thread_set_current_name(const char *name)
{
	if (name == nullptr || name[0] == '\0')
		return;

	int name_wide_count = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, nullptr, 0);
	validate(name_wide_count > 0, "[PLATFORM][WINDOWS]: Failed to convert thread name.");

	Memory_Block name_wide_block = memory::allocate((U64)name_wide_count * sizeof(wchar_t), alignof(wchar_t));
	DEFER(memory::deallocate(name_wide_block));
	wchar_t *name_wide = (wchar_t *)name_wide_block.data;
	int converted_count = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, name_wide, name_wide_count);
	validate(converted_count == name_wide_count, "[PLATFORM][WINDOWS]: Failed to convert thread name.");

	validate(SUCCEEDED(::SetThreadDescription(::GetCurrentThread(), name_wide)), "[PLATFORM][WINDOWS]: Failed to set thread name.");
}

struct Platform_Mutex
{
	SRWLOCK handle;
};

Platform_Mutex *
platform_mutex_init()
{
	Platform_Mutex *self = memory::allocate_zeroed<Platform_Mutex>();
	::InitializeSRWLock(&self->handle);
	return self;
}

void
platform_mutex_deinit(Platform_Mutex *self)
{
	memory::deallocate(self);
}

void
platform_mutex_lock(Platform_Mutex *self)
{
	::AcquireSRWLockExclusive(&self->handle);
}

void
platform_mutex_unlock(Platform_Mutex *self)
{
	::ReleaseSRWLockExclusive(&self->handle);
}

struct Platform_Condition_Variable
{
	CONDITION_VARIABLE handle;
};

Platform_Condition_Variable *
platform_condition_variable_init()
{
	Platform_Condition_Variable *self = memory::allocate_zeroed<Platform_Condition_Variable>();
	::InitializeConditionVariable(&self->handle);
	return self;
}

void
platform_condition_variable_deinit(Platform_Condition_Variable *self)
{
	memory::deallocate(self);
}

void
platform_condition_variable_wait(Platform_Condition_Variable *self, Platform_Mutex *mutex)
{
	BOOL result = ::SleepConditionVariableSRW(&self->handle, &mutex->handle, INFINITE, 0);
	validate(result, "[PLATFORM][WINDOWS]: Failed to wait for condition variable.");
}

void
platform_condition_variable_signal(Platform_Condition_Variable *self)
{
	::WakeConditionVariable(&self->handle);
}

void
platform_condition_variable_broadcast(Platform_Condition_Variable *self)
{
	::WakeAllConditionVariable(&self->handle);
}

struct Platform_Semaphore
{
	HANDLE handle;
};

Platform_Semaphore *
platform_semaphore_init(U32 initial_count)
{
	constexpr LONG max_count = 0x7fffffff;
	Platform_Semaphore *self = memory::allocate_zeroed<Platform_Semaphore>();
	self->handle = ::CreateSemaphoreW(nullptr, (LONG)initial_count, max_count, nullptr);
	validate(self->handle != nullptr, "[PLATFORM][WINDOWS]: Failed to initialize semaphore.");
	return self;
}

void
platform_semaphore_deinit(Platform_Semaphore *self)
{
	validate(::CloseHandle(self->handle), "[PLATFORM][WINDOWS]: Failed to close semaphore handle.");
	memory::deallocate(self);
}

void
platform_semaphore_wait(Platform_Semaphore *self)
{
	DWORD wait_result = ::WaitForSingleObject(self->handle, INFINITE);
	validate(wait_result == WAIT_OBJECT_0, "[PLATFORM][WINDOWS]: Failed to wait for semaphore.");
}

void
platform_semaphore_signal(Platform_Semaphore *self, U32 count)
{
	if (count == 0)
		return;
	validate(::ReleaseSemaphore(self->handle, (LONG)count, nullptr), "[PLATFORM][WINDOWS]: Failed to signal semaphore.");
}

inline static void
_platform_win32_window_keep_screen_on_set(Platform_Window_Context *ctx, bool enabled)
{
	if (ctx->keep_screen_on == enabled)
		return;

	if (enabled)
		::SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
	else
		::SetThreadExecutionState(ES_CONTINUOUS);
	ctx->keep_screen_on = enabled;
}

inline static void
_platform_win32_window_fullscreen_set(Platform_Window_Context *ctx, bool enabled)
{
	if (ctx->fullscreen == enabled)
		return;

	HWND window = ctx->window;
	if (enabled)
	{
		ctx->windowed_style = (DWORD)::GetWindowLongPtr(window, GWL_STYLE);
		ctx->windowed_ex_style = (DWORD)::GetWindowLongPtr(window, GWL_EXSTYLE);
		ctx->windowed_placement = {};
		ctx->windowed_placement.length = sizeof(ctx->windowed_placement);
		validate(::GetWindowPlacement(window, &ctx->windowed_placement), "[PLATFORM][WINDOWS]: Failed to get window placement.");

		HMONITOR monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitor_info = {};
		monitor_info.cbSize = sizeof(monitor_info);
		validate(::GetMonitorInfoA(monitor, &monitor_info), "[PLATFORM][WINDOWS]: Failed to get monitor info.");

		::SetWindowLongPtr(window, GWL_STYLE, ctx->windowed_style & ~WS_OVERLAPPEDWINDOW);
		::SetWindowLongPtr(window, GWL_EXSTYLE, ctx->windowed_ex_style & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		::SetWindowPos(
			window,
			HWND_TOP,
			monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);
	}
	else
	{
		::SetWindowLongPtr(window, GWL_STYLE, ctx->windowed_style);
		::SetWindowLongPtr(window, GWL_EXSTYLE, ctx->windowed_ex_style);
		validate(::SetWindowPlacement(window, &ctx->windowed_placement), "[PLATFORM][WINDOWS]: Failed to restore window placement.");
		::SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}

	ctx->fullscreen = enabled;
}

Platform_Window
platform_window_init(U32 width, U32 height, const char *title)
{
	validate(width > 0 && height > 0, "[PLATFORM]: Windows cannot have zero width or height.");

	Platform_Window self = {};
	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();

	WNDCLASSEXA wc = {};
	wc.cbSize		 = sizeof(wc);
	wc.style		 = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = _window_proc;
	wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "Mist_Window_Class";
	[[maybe_unused]] ATOM class_atom = RegisterClassExA(&wc);
	validate(class_atom != 0, "[PLATFORM]: Failed to register window class.");

	ctx->window = CreateWindowExA(
		0,
		wc.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		nullptr,
		nullptr,
		nullptr,
		nullptr);
	validate(ctx->window, "[PLATFORM]: Failed to create window.");

	self.handle = ctx;
	self.width  = width;
	self.height = height;
	self.metrics = _platform_win32_window_metrics(ctx->window, width, height);
	self.focused = true;
	self.started = true;
	self.surface_valid = true;
	self.surface_changed = true;
	self.input.text_input_events = array_init<Platform_Text_Input_Event>();

	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	_platform_win32_window_keep_screen_on_set(ctx, false);
	_platform_win32_window_fullscreen_set(ctx, false);

	bool res = false;
	res = DestroyWindow(ctx->window);
	validate(res, "[PLATFORM]: Failed to destroy window.");
	memory::deallocate(ctx);
	self->handle = nullptr;
	self->metrics = {};
	self->presentation = {};
	self->close_requested = true;
	self->started = false;
	self->surface_valid = false;
	_platform_win32_text_input_events_reset(self->input);
	array_deinit(self->input.text_input_events);
}

bool
platform_window_poll(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	bool surface_changed = self->surface_changed;
	self->surface_changed = false;
	self->low_memory = false;
	self->save_state_requested = false;

	for (I32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		self->input.keys[i].pressed       = false;
		self->input.keys[i].released      = false;
		self->input.keys[i].press_count   = 0;
		self->input.keys[i].release_count = 0;
	}
	self->input.mouse_wheel = 0.0f;
	_platform_win32_text_input_events_reset(self->input);

	MSG msg = {};
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		switch (msg.message)
		{
			case WM_QUIT:
			{
				self->close_requested = true;
				self->surface_valid = false;
				return false;
			}
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MOUSEWHEEL:
			{
				SetCapture(ctx->window);

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
			case WM_CHAR:
			{
				if (msg.wParam >= 0x20 || msg.wParam == L'\r' || msg.wParam == L'\t')
					_platform_win32_text_input_push_utf16(*self, (wchar_t)msg.wParam);
				break;
			}
		}
	}

	{
		// NOTE: Resizing.
		RECT rect;
		GetClientRect(ctx->window, &rect);
		U32 width = rect.right - rect.left;
		U32 height = rect.bottom - rect.top;
		if (self->width != width || self->height != height)
		{
			self->width = width;
			self->height = height;
			surface_changed = true;
		}
		self->metrics = _platform_win32_window_metrics(ctx->window, self->width, self->height);
	}

	{
		// NOTE: Mouse movement.
		POINT mouse_point;
		GetCursorPos(&mouse_point);
		ScreenToClient(ctx->window, &mouse_point);

		// NOTE: We want mouse coords to start bottom-left.
		U32 mouse_point_y_inverted = (self->height - 1) - mouse_point.y;
		self->input.mouse_dx = mouse_point.x - self->input.mouse_x;
		self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
		self->input.mouse_x  = mouse_point.x;
		self->input.mouse_y  = mouse_point_y_inverted;
	}

	self->focused = GetForegroundWindow() == ctx->window;
	self->paused = false;
	self->surface_valid = !self->close_requested && self->width > 0 && self->height > 0;
	self->surface_changed = surface_changed;
	return !self->close_requested;
}

Platform_Window_Native_Handles
platform_window_get_native_handles(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	return Platform_Window_Native_Handles {
		.window = ctx->window,
		.context = nullptr
	};
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	SetWindowText(ctx->window, title);
}

void
platform_window_close(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	self->close_requested = true;
	self->surface_valid = false;
	PostMessageW(ctx->window, WM_QUIT, 0, 0);
}

void
platform_window_presentation_set(Platform_Window &window, const Platform_Window_Presentation_Desc &desc)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	bool fullscreen = (desc.flags & (PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN | PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE)) != 0;
	_platform_win32_window_fullscreen_set(ctx, fullscreen);
	_platform_win32_window_keep_screen_on_set(ctx, (desc.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0);
	window.presentation = desc;
	window.surface_changed = true;
}

void
platform_window_text_input_set(Platform_Window &window, const Platform_Text_Input_Desc &desc)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	window.text_input = desc;
	window.text_input.text = {};
	if (desc.enabled)
		::SetFocus(ctx->window);
	else
		window.text_input_pending_surrogate = 0;
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

String
platform_file_dialog_open(const char *filters, memory::Allocator *allocator)
{
	char path[4096] = {};

	OPENFILENAME ofn = {};
	ofn.lStructSize     = sizeof(ofn);
	ofn.lpstrFile       = path;
	ofn.nMaxFile        = sizeof(path);
	ofn.lpstrFilter     = filters;
	ofn.nFilterIndex    = 1;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn) == TRUE)
		return string_from(path, allocator);

	return string_init(allocator);
}

String
platform_file_dialog_save(const char *filters, memory::Allocator *allocator)
{
	char path[4096] = {};

	OPENFILENAME ofn = {};
	ofn.lStructSize     = sizeof(ofn);
	ofn.lpstrFile       = path;
	ofn.nMaxFile        = sizeof(path);
	ofn.lpstrFilter     = filters;
	ofn.nFilterIndex    = 1;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	// TODO: Ensure that we write extension, if the user forgets to do so.
	if (GetSaveFileName(&ofn) == TRUE)
		return string_from(path, allocator);

	return string_init(allocator);
}

String
platform_directory_dialog_open(memory::Allocator *allocator)
{
	char path[4096] = {};
	HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool com_initialized = SUCCEEDED(com_result);
	DEFER({
		if (com_initialized)
			::CoUninitialize();
	});

	BROWSEINFOA browse_info = {};
	browse_info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST item_list = SHBrowseForFolderA(&browse_info);
	if (item_list == nullptr)
		return string_init(allocator);
	DEFER(::CoTaskMemFree(item_list););

	if (::SHGetPathFromIDListA(item_list, path))
		return string_from(path, allocator);

	return string_init(allocator);
}

String
_platform_win32_clipboard_media_type_from_format(UINT format, memory::Allocator *allocator)
{
	if (format == CF_UNICODETEXT)
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator);

	UINT png_format = ::RegisterClipboardFormatA("PNG");
	if (format == png_format)
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG, allocator);

	char format_name[256] = {};
	if (::GetClipboardFormatNameA(format, format_name, sizeof(format_name)) <= 0)
		return string_init(allocator);

	for (const char *at = format_name; *at != '\0'; ++at)
		if (*at == '/')
			return string_from(format_name, allocator);

	return string_init(allocator);
}

inline static UINT
_platform_win32_clipboard_format_from_media_type(const String &media_type)
{
	if (media_type.count == 0)
		return 0;

	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || media_type == "text/plain")
		return CF_UNICODETEXT;

	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG)
		return ::RegisterClipboardFormatA("PNG");

	return ::RegisterClipboardFormatA(media_type.data);
}

inline static bool
_platform_clipboard_media_type_exists(const Array<String> &media_types, const String &media_type)
{
	for (U64 i = 0; i < media_types.count; ++i)
		if (media_types[i] == media_type)
			return true;
	return false;
}

Array<String>
platform_window_clipboard_query_media_types(Platform_Window &, memory::Allocator *allocator)
{
	Array<String> media_types = array_init<String>(allocator);
	if (!::OpenClipboard(nullptr))
		return media_types;
	DEFER(::CloseClipboard(););

	UINT format = 0;
	while ((format = ::EnumClipboardFormats(format)) != 0)
	{
		String media_type = _platform_win32_clipboard_media_type_from_format(format, memory::temp_allocator());
		if (media_type.count > 0 && !_platform_clipboard_media_type_exists(media_types, media_type))
			array_push(media_types, string_copy(media_type, allocator));
	}

	return media_types;
}

Platform_Clipboard_Item
platform_window_clipboard_item_read(Platform_Window &, const String &media_type, memory::Allocator *allocator)
{
	Platform_Clipboard_Item result {
		.media_type = string_init(allocator),
		.data = array_init<U8>(allocator)
	};

	if (!::OpenClipboard(nullptr))
		return result;
	DEFER(::CloseClipboard(););

	UINT format = _platform_win32_clipboard_format_from_media_type(media_type);
	if (format == 0 || !::IsClipboardFormatAvailable(format))
		return result;

	HANDLE clipboard_data = ::GetClipboardData(format);
	if (clipboard_data == nullptr)
		return result;

	if (format == CF_UNICODETEXT)
	{
		wchar_t *text_wide = (wchar_t *)::GlobalLock(clipboard_data);
		if (text_wide == nullptr)
			return result;
		DEFER(::GlobalUnlock(clipboard_data););

		I32 utf8_count_with_null = ::WideCharToMultiByte(CP_UTF8, 0, text_wide, -1, nullptr, 0, nullptr, nullptr);
		if (utf8_count_with_null <= 0)
			return result;

		string_deinit(result.media_type);
		result.media_type = string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator);
		array_resize(result.data, (U64)utf8_count_with_null - 1);
		::WideCharToMultiByte(CP_UTF8, 0, text_wide, -1, (char *)result.data.data, utf8_count_with_null, nullptr, nullptr);
		return result;
	}

	void *data = ::GlobalLock(clipboard_data);
	if (data == nullptr)
		return result;
	DEFER(::GlobalUnlock(clipboard_data););

	U64 data_size = (U64)::GlobalSize(clipboard_data);
	string_deinit(result.media_type);
	result.media_type = _platform_win32_clipboard_media_type_from_format(format, allocator);
	if (result.media_type.count == 0)
		result.media_type = string_copy(media_type, allocator);
	array_resize(result.data, data_size);
	::memcpy(result.data.data, data, data_size);
	return result;
}

struct Platform_Win32_Clipboard_Data
{
	UINT format;
	HGLOBAL data;
};

inline static HGLOBAL
_platform_win32_clipboard_data_from_text(const Array<U8> &text)
{
	const char *text_data = text.data ? (const char *)text.data : "";
	I32 text_count = text.data ? (I32)text.count : 0;
	I32 wide_count = ::MultiByteToWideChar(CP_UTF8, 0, text_data, text_count, nullptr, 0);
	if (wide_count < 0)
		return nullptr;
	HGLOBAL clipboard_data = ::GlobalAlloc(GMEM_MOVEABLE, ((U64)wide_count + 1) * sizeof(wchar_t));
	if (clipboard_data == nullptr)
		return nullptr;

	wchar_t *text_wide = (wchar_t *)::GlobalLock(clipboard_data);
	if (text_wide == nullptr)
	{
		::GlobalFree(clipboard_data);
		return nullptr;
	}

	::MultiByteToWideChar(CP_UTF8, 0, text_data, text_count, text_wide, wide_count);
	text_wide[wide_count] = L'\0';
	::GlobalUnlock(clipboard_data);
	return clipboard_data;
}

inline static HGLOBAL
_platform_win32_clipboard_data_from_bytes(const Array<U8> &data)
{
	HGLOBAL clipboard_data = ::GlobalAlloc(GMEM_MOVEABLE, data.count);
	if (clipboard_data == nullptr)
		return nullptr;

	void *clipboard_bytes = ::GlobalLock(clipboard_data);
	if (clipboard_bytes == nullptr)
	{
		::GlobalFree(clipboard_data);
		return nullptr;
	}

	if (data.count > 0)
		::memcpy(clipboard_bytes, data.data, data.count);
	::GlobalUnlock(clipboard_data);
	return clipboard_data;
}

bool
platform_window_clipboard_item_write(Platform_Window &, const Platform_Clipboard_Item *items, U32 item_count)
{
	if (items == nullptr || item_count == 0)
		return false;

	Array<Platform_Win32_Clipboard_Data> prepared = array_init_with_capacity<Platform_Win32_Clipboard_Data>(item_count, memory::temp_allocator());
	DEFER({
		for (U64 i = 0; i < prepared.count; ++i)
			if (prepared[i].data)
				::GlobalFree(prepared[i].data);
		array_deinit(prepared);
	});

	for (U32 i = 0; i < item_count; ++i)
	{
		UINT format = _platform_win32_clipboard_format_from_media_type(items[i].media_type);
		if (format == 0)
			return false;

		HGLOBAL data = format == CF_UNICODETEXT ? _platform_win32_clipboard_data_from_text(items[i].data) : _platform_win32_clipboard_data_from_bytes(items[i].data);
		if (data == nullptr)
			return false;

		array_push(prepared, Platform_Win32_Clipboard_Data{format, data});
	}

	if (!::OpenClipboard(nullptr))
		return false;
	DEFER(::CloseClipboard(););

	if (!::EmptyClipboard())
		return false;

	for (U64 i = 0; i < prepared.count; ++i)
	{
		if (::SetClipboardData(prepared[i].format, prepared[i].data) == nullptr)
			return false;
		prepared[i].data = nullptr;
	}

	return true;
}


U64
platform_query_microseconds()
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER ticks;
	validate(QueryPerformanceFrequency(&frequency) != false, "[PLATFORM]: Failed to query performance frequency.");
	validate(QueryPerformanceCounter(&ticks) != false, "[PLATFORM]: Failed to query performance counter.");
	return ticks.QuadPart * 1000000 / frequency.QuadPart;
}

void
platform_sleep_set_period(U32 period)
{
	validate(timeBeginPeriod(period) == TIMERR_NOERROR, "[PLATFORM]: Failed to set time begin period.");
}

void
platform_sleep(U32 milliseconds)
{
	Sleep(milliseconds);
}


inline static void
_platform_callstack_copy_string(char *dst, U64 dst_size, const char *src)
{
	if (dst_size == 0)
		return;

	U64 i = 0;
	if (src)
	{
		for (; i + 1 < dst_size && src[i] != '\0'; ++i)
			dst[i] = src[i];
	}
	dst[i] = '\0';
}

inline static void
_platform_callstack_append_symbol_path_entry(String &symbol_path, const String &entry)
{
	if (entry.count == 0)
		return;

	if (symbol_path.count > 0)
		string_append(symbol_path, ';');
	string_append(symbol_path, entry);
}

inline static void
_platform_callstack_get_symbol_path(char *symbol_path, U64 symbol_path_size)
{
	if (symbol_path_size == 0)
		return;

	symbol_path[0] = '\0';

	memory::Allocator *allocator = memory::temp_allocator();
	String result = string_with_capacity(symbol_path_size, allocator);

	String executable_path = platform_path_get_executable_path(allocator);
	String executable_directory = platform_path_get_directory(executable_path, allocator);
	_platform_callstack_append_symbol_path_entry(result, executable_directory);

	String module_path = platform_path_get_current_module_path(allocator);
	String module_directory = platform_path_get_directory(module_path, allocator);
	_platform_callstack_append_symbol_path_entry(result, module_directory);

	String current_directory = platform_path_get_current_working_directory(allocator);
	_platform_callstack_append_symbol_path_entry(result, current_directory);

	String environment_symbol_path = platform_environment_variable_get("_NT_SYMBOL_PATH", allocator);
	_platform_callstack_append_symbol_path_entry(result, environment_symbol_path);

	_platform_callstack_copy_string(symbol_path, symbol_path_size, result.data);
}

struct Callstack
{
	bool initialized;

	Callstack()
	{
		HANDLE process = ::GetCurrentProcess();

		char symbol_path[4096] = {};
		_platform_callstack_get_symbol_path(symbol_path, sizeof(symbol_path));

		::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
		initialized = ::SymInitialize(process, symbol_path[0] ? symbol_path : nullptr, TRUE) != FALSE;
		if (!initialized && ::GetLastError() == ERROR_INVALID_PARAMETER)
			initialized = true;

		if (initialized)
		{
			if (symbol_path[0])
				::SymSetSearchPath(process, symbol_path);
			::SymRefreshModuleList(process);
		}
	}
};

inline static bool
_platform_callstack_init()
{
	static Callstack callstack;
	return callstack.initialized;
}

U32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	_platform_callstack_init();
	::memset(callstack, 0, frame_count * sizeof(*callstack));
	return CaptureStackBackTrace(1, frame_count, callstack, NULL);
#else
	return 0;
#endif
}

void
platform_callstack_resolve([[maybe_unused]] void **callstack, [[maybe_unused]] Platform_Callstack_Frame *frames, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	bool can_resolve = _platform_callstack_init();
	HANDLE process = ::GetCurrentProcess();
	if (can_resolve)
		::SymRefreshModuleList(process);

	// Windows lays symbol info in memory in the form [struct][name buffer].
	constexpr U64 MAX_NAME_LENGTH = PLATFORM_CALLSTACK_SYMBOL_LENGTH - 1;
	alignas(SYMBOL_INFO) U8 symbol_buffer[MAX_NAME_LENGTH + sizeof(SYMBOL_INFO)] = {};

	SYMBOL_INFO *symbol_info = (SYMBOL_INFO *)symbol_buffer;
	symbol_info->MaxNameLen   = (DWORD)MAX_NAME_LENGTH;
	symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (U64 i = 0; i < frame_count; ++i)
	{
		Platform_Callstack_Frame *frame = frames + i;
		frame->address      = callstack[i];
		frame->line         = 0;
		frame->symbol_found = false;
		frame->line_found   = false;
		frame->symbol[0]    = '\0';
		frame->file[0]      = '\0';

		IMAGEHLP_LINE64 line = {};
		line.SizeOfStruct = sizeof(line);

		DWORD64 address = (DWORD64)callstack[i];
		DWORD64 lookup_address = address > 0 ? address - 1 : address;
		DWORD displacement = 0;
		DWORD64 symbol_displacement = 0;
		if (can_resolve && ::SymFromAddr(process, lookup_address, &symbol_displacement, symbol_info))
		{
			frame->symbol_found = true;
			_platform_callstack_copy_string(frame->symbol, PLATFORM_CALLSTACK_SYMBOL_LENGTH, symbol_info->Name);
		}

		if (can_resolve && ::SymGetLineFromAddr64(process, lookup_address, &displacement, &line))
		{
			frame->line_found = true;
			frame->line       = line.LineNumber;
			_platform_callstack_copy_string(frame->file, PLATFORM_CALLSTACK_FILE_LENGTH, line.FileName);
		}
	}
#endif
}