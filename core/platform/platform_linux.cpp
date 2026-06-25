#include "core/platform/platform.h"

#include "core/validate.h"
#include "core/defer.h"
#include "core/formatter.h"
#include "core/math/u64.h"
#include "core/memory/memory.h"

#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <pthread.h>
#include <atomic>
#include <dirent.h>
#include <dbus/dbus.h>

static char current_executable_directory[PATH_MAX] = {};

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
_platform_key_from_xcb_button(xcb_button_t button)
{
	switch (button)
	{
		case XCB_BUTTON_INDEX_1: return PLATFORM_KEY_MOUSE_LEFT;
		case XCB_BUTTON_INDEX_2: return PLATFORM_KEY_MOUSE_MIDDLE;
		case XCB_BUTTON_INDEX_3: return PLATFORM_KEY_MOUSE_RIGHT;
		case XCB_BUTTON_INDEX_4: return PLATFORM_KEY_MOUSE_WHEEL_UP;
		case XCB_BUTTON_INDEX_5: return PLATFORM_KEY_MOUSE_WHEEL_DOWN;
	}
	return PLATFORM_KEY_COUNT;
}

inline static xcb_atom_t
_platform_linux_intern_atom(xcb_connection_t *connection, const char *name)
{
	xcb_intern_atom_cookie_t cookie = ::xcb_intern_atom(connection, 0, ::strlen(name), name);
	xcb_intern_atom_reply_t *reply = ::xcb_intern_atom_reply(connection, cookie, nullptr);
	if (reply == nullptr)
		return XCB_ATOM_NONE;
	DEFER(::free(reply););
	return reply->atom;
}

inline static PLATFORM_KEY
_platform_key_from_key_sym(KeySym key)
{
	switch (key)
	{
		case XK_a:            return PLATFORM_KEY_A;
		case XK_b:            return PLATFORM_KEY_B;
		case XK_c:            return PLATFORM_KEY_C;
		case XK_d:            return PLATFORM_KEY_D;
		case XK_e:            return PLATFORM_KEY_E;
		case XK_f:            return PLATFORM_KEY_F;
		case XK_g:            return PLATFORM_KEY_G;
		case XK_h:            return PLATFORM_KEY_H;
		case XK_i:            return PLATFORM_KEY_I;
		case XK_j:            return PLATFORM_KEY_J;
		case XK_k:            return PLATFORM_KEY_K;
		case XK_l:            return PLATFORM_KEY_L;
		case XK_m:            return PLATFORM_KEY_M;
		case XK_n:            return PLATFORM_KEY_N;
		case XK_o:            return PLATFORM_KEY_O;
		case XK_p:            return PLATFORM_KEY_P;
		case XK_q:            return PLATFORM_KEY_Q;
		case XK_r:            return PLATFORM_KEY_R;
		case XK_s:            return PLATFORM_KEY_S;
		case XK_t:            return PLATFORM_KEY_T;
		case XK_u:            return PLATFORM_KEY_U;
		case XK_v:            return PLATFORM_KEY_V;
		case XK_w:            return PLATFORM_KEY_W;
		case XK_x:            return PLATFORM_KEY_X;
		case XK_y:            return PLATFORM_KEY_Y;
		case XK_z:            return PLATFORM_KEY_Z;
		case XK_0:            return PLATFORM_KEY_NUM_0;
		case XK_1:            return PLATFORM_KEY_NUM_1;
		case XK_2:            return PLATFORM_KEY_NUM_2;
		case XK_3:            return PLATFORM_KEY_NUM_3;
		case XK_4:            return PLATFORM_KEY_NUM_4;
		case XK_5:            return PLATFORM_KEY_NUM_5;
		case XK_6:            return PLATFORM_KEY_NUM_6;
		case XK_7:            return PLATFORM_KEY_NUM_7;
		case XK_8:            return PLATFORM_KEY_NUM_8;
		case XK_9:            return PLATFORM_KEY_NUM_9;
		case XK_KP_0:         return PLATFORM_KEY_NUMPAD_0;
		case XK_KP_1:         return PLATFORM_KEY_NUMPAD_1;
		case XK_KP_2:         return PLATFORM_KEY_NUMPAD_2;
		case XK_KP_3:         return PLATFORM_KEY_NUMPAD_3;
		case XK_KP_4:         return PLATFORM_KEY_NUMPAD_4;
		case XK_KP_5:         return PLATFORM_KEY_NUMPAD_5;
		case XK_KP_6:         return PLATFORM_KEY_NUMPAD_6;
		case XK_KP_7:         return PLATFORM_KEY_NUMPAD_7;
		case XK_KP_8:         return PLATFORM_KEY_NUMPAD_8;
		case XK_KP_9:         return PLATFORM_KEY_NUMPAD_9;
		case XK_F1:           return PLATFORM_KEY_F1;
		case XK_F2:           return PLATFORM_KEY_F2;
		case XK_F3:           return PLATFORM_KEY_F3;
		case XK_F4:           return PLATFORM_KEY_F4;
		case XK_F5:           return PLATFORM_KEY_F5;
		case XK_F6:           return PLATFORM_KEY_F6;
		case XK_F7:           return PLATFORM_KEY_F7;
		case XK_F8:           return PLATFORM_KEY_F8;
		case XK_F9:           return PLATFORM_KEY_F9;
		case XK_F10:          return PLATFORM_KEY_F10;
		case XK_F11:          return PLATFORM_KEY_F11;
		case XK_F12:          return PLATFORM_KEY_F12;
		case XK_Up:           return PLATFORM_KEY_ARROW_UP;
		case XK_Down:         return PLATFORM_KEY_ARROW_DOWN;
		case XK_Left:         return PLATFORM_KEY_ARROW_LEFT;
		case XK_Right:        return PLATFORM_KEY_ARROW_RIGHT;
		case XK_Shift_L:      return PLATFORM_KEY_SHIFT_LEFT;
		case XK_Shift_R:      return PLATFORM_KEY_SHIFT_RIGHT;
		case XK_Control_L:    return PLATFORM_KEY_CONTROL_LEFT;
		case XK_Control_R:    return PLATFORM_KEY_CONTROL_RIGHT;
		case XK_Alt_L:        return PLATFORM_KEY_ALT_LEFT;
		case XK_Alt_R:        return PLATFORM_KEY_ALT_RIGHT;
		case XK_BackSpace:    return PLATFORM_KEY_BACKSPACE;
		case XK_Tab:          return PLATFORM_KEY_TAB;
		case XK_Return:       return PLATFORM_KEY_ENTER;
		case XK_Escape:       return PLATFORM_KEY_ESCAPE;
		case XK_Delete:       return PLATFORM_KEY_DELETE;
		case XK_Insert:       return PLATFORM_KEY_INSERT;
		case XK_Home:         return PLATFORM_KEY_HOME;
		case XK_End:          return PLATFORM_KEY_END;
		case XK_Page_Up:      return PLATFORM_KEY_PAGE_UP;
		case XK_Page_Down:    return PLATFORM_KEY_PAGE_DOWN;
		case XK_slash:        return PLATFORM_KEY_SLASH;
		case XK_backslash:    return PLATFORM_KEY_BACKSLASH;
		case XK_bracketleft:  return PLATFORM_KEY_BRACKET_LEFT;
		case XK_bracketright: return PLATFORM_KEY_BRACKET_RIGHT;
		case XK_grave:        return PLATFORM_KEY_BACKQUOTE;
		case XK_period:       return PLATFORM_KEY_PERIOD;
		case XK_minus:        return PLATFORM_KEY_MINUS;
		case XK_equal:        return PLATFORM_KEY_EQUAL;
		case XK_comma:        return PLATFORM_KEY_COMMA;
		case XK_semicolon:    return PLATFORM_KEY_SEMICOLON;
		case XK_space:        return PLATFORM_KEY_SPACE;
	}
	return PLATFORM_KEY_COUNT;
}

// API.
// C++.
bool
platform_path_is_valid(const String &path)
{
	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0;
}

bool
platform_path_is_file(const String &path)
{
	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISREG(path_stat.st_mode);
}

bool
platform_path_is_directory(const String &path)
{
	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

String
platform_path_get_absolute(const String &path, memory::Allocator *allocator)
{
	char buffer[PATH_MAX] = {};
	if(::realpath(path.data, buffer))
		return string_from(buffer, allocator);

	String full_path = platform_path_get_current_working_directory(allocator);
	string_append(full_path, format("/{}", path));
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
	char buffer[PATH_MAX] = {};
	validate(::getcwd(buffer, PATH_MAX));
	return string_from(buffer, allocator);
}

String
platform_path_get_temp_directory(memory::Allocator *allocator)
{
	String result = platform_environment_variable_get("TMPDIR", allocator);
	if (result.count == 0)
		result = string_from("/tmp/", allocator);
	if (result.count > 0 && result[result.count - 1] != '/')
		string_append(result, '/');
	return result;
}

String
platform_environment_variable_get(const String &name, memory::Allocator *allocator)
{
	const char *value = ::getenv(name.data);
	if (value == nullptr || value[0] == '\0')
		return string_literal("");
	return string_from(value, allocator);
}

void
platform_path_set_current_working_directory(const String &path)
{
	validate(::chdir(path.data) == 0);
}

String
platform_path_get_executable_path(memory::Allocator *allocator)
{
	char module_path_relative[PATH_MAX + 1];
	::memset(module_path_relative, 0, sizeof(module_path_relative));

	char module_path_absolute[PATH_MAX + 1];
	::memset(module_path_absolute, 0, sizeof(module_path_absolute));

	I64 module_path_relative_length = ::readlink("/proc/self/exe", module_path_relative, sizeof(module_path_relative));
	validate(module_path_relative_length != -1 && module_path_relative_length < (I64)sizeof(module_path_relative), "[PLATFORM]: Failed to get relative path of the current executable.");

	char *path_absolute = ::realpath(module_path_relative, module_path_absolute);
	validate(path_absolute == module_path_absolute, "[PLATFORM]: Failed to get absolute path of the current executable.");

	return string_from(path_absolute, allocator);
}

String
platform_path_get_current_module_path(memory::Allocator *allocator)
{
	Dl_info info = {};
	if (::dladdr((void *)&platform_path_get_current_module_path, &info) == 0 || info.dli_fname == nullptr)
		return string_literal("");

	char path_absolute[PATH_MAX + 1];
	::memset(path_absolute, 0, sizeof(path_absolute));
	char *absolute = ::realpath(info.dli_fname, path_absolute);
	if (absolute)
		return string_from(absolute, allocator);
	return string_from(info.dli_fname, allocator);
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

	I32 file_handle = ::open(path.data, O_RDONLY, S_IRWXU);
	if (file_handle == -1)
		return content;

	U64 file_size = platform_file_size(path.data);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);

	I64 bytes_read = ::read(file_handle, content.data, content.count);
	validate(::close(file_handle) == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_read == -1)
		return content;

	validate((I64)content.count == bytes_read);

	return content;
}

U64
platform_path_write_file(const String &path, Memory_Block block)
{
	I32 file_handle = ::open(path.data, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (file_handle == -1)
		return 0;

	I64 bytes_written = ::write(file_handle, block.data, block.size);
	validate(::close(file_handle) == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_written == -1)
		return 0;
	return bytes_written;
}

Array<String>
platform_path_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);

	String directory_temp = string_copy(directory, memory::temp_allocator());
	string_replace(directory_temp, "\\", "/");

	if (!platform_path_is_directory(directory_temp))
		return files;

	DIR *dir = ::opendir(directory_temp.data);
	if (!dir)
		return files;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][LINUX]: Failed to close directory."););

	struct dirent *entry = nullptr;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		if (entry->d_type == DT_DIR)
			continue;

		String file_name = string_from(entry->d_name, memory::temp_allocator());
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

		array_push(files, string_copy(file_name, allocator));
	}

	return files;
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

	char src_relative_path[128] = {};
	_string_concat(filepath, ".so", src_relative_path);

	char dst_relative_path[128] = {};
	_string_concat(src_relative_path, ".tmp", dst_relative_path);

	char src_absolute_path[PATH_MAX] = {};
	_string_concat(current_executable_directory, src_relative_path, src_absolute_path);

	char dst_absolute_path[PATH_MAX] = {};
	_string_concat(current_executable_directory, src_relative_path, dst_absolute_path);

	[[maybe_unused]] bool copy_successful = platform_file_copy(src_relative_path, dst_relative_path);
	validate(copy_successful, "[PLATFORM]: Failed to copy library.");

	self.handle = ::dlopen(dst_absolute_path, RTLD_LAZY);
	validate(self.handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self.handle, "platform_api");
	validate(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self.api = proc(nullptr, PLATFORM_API_STATE_INIT);
	validate(self.api, "[PLATFORM]: Failed to get api.");

	struct stat file_stat = {};
	[[maybe_unused]] I32 stat_result = ::stat(src_relative_path, &file_stat);
	validate(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");

	self.last_write_time = file_stat.st_mtime;
	::strcpy(self.filepath, src_absolute_path);

	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
		validate(proc, "[PLATFORM]: Failed to get proc platform_api.");
		self->api = proc(self->api, PLATFORM_API_STATE_DEINIT);
	}

	::dlclose(self->handle);
}

void *
platform_api_load(Platform_Api *self)
{
	char dst_absolute_path[PATH_MAX] = {};
	_string_concat(self->filepath, ".tmp", dst_absolute_path);

	struct stat file_stat = {};
	I32 stat_result = ::stat(self->filepath, &file_stat);
	validate(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");

	I64 last_write_time = file_stat.st_mtime;
	if ((last_write_time == self->last_write_time) || (stat_result != 0))
		return self->api;

	::dlclose(self->handle);

	platform_file_delete(dst_absolute_path);

	platform_sleep(100);

	bool copy_result = platform_file_copy(self->filepath, dst_absolute_path);

	self->handle = ::dlopen(dst_absolute_path, RTLD_LAZY);
	validate(self->handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
	validate(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self->api = proc(self->api, PLATFORM_API_STATE_LOAD);
	validate(self->api, "[PLATFORM]: Failed to get api.");

	// If copying failed we don't update last write time so that we can try copying it again in the next frame.
	if (copy_result == true)
		self->last_write_time = last_write_time;

	return self->api;
}


U64
platform_virtual_memory_get_page_size()
{
	return (U64)::sysconf(_SC_PAGESIZE);
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
		return Memory_Block{};

	void *data = ::mmap(nullptr, aligned_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (data == MAP_FAILED)
		return Memory_Block{};
	return Memory_Block{.data = data, .size = aligned_size};
}

bool
platform_virtual_memory_commit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][LINUX]: Cannot commit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][LINUX]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][LINUX]: Virtual memory block size is not page-aligned.");

	return ::mprotect(block.data, block.size, PROT_READ|PROT_WRITE) == 0;
}

bool
platform_virtual_memory_decommit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][LINUX]: Cannot decommit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][LINUX]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][LINUX]: Virtual memory block size is not page-aligned.");

	I32 advise_result = ::madvise(block.data, block.size, MADV_DONTNEED);
	I32 protect_result = ::mprotect(block.data, block.size, PROT_NONE);
	return advise_result == 0 && protect_result == 0;
}

void
platform_virtual_memory_release(Memory_Block block)
{
	if (block.data == nullptr)
		return;

	U64 page_size = platform_virtual_memory_get_page_size();
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][LINUX]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][LINUX]: Virtual memory block size is not page-aligned.");

	[[maybe_unused]] I32 result = ::munmap(block.data, block.size);
	validate(result == 0, "[PLATFORM][LINUX]: Failed to release virtual memory.");
}

struct Platform_Task
{
	void (*function)(void *);
	void *user_data;
};

struct Platform_Thread
{
	pthread_t handle;
	std::atomic<bool> is_running;
	Platform_Task task;
};

static void *
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
	return nullptr;
}

Platform_Thread *
platform_thread_init()
{
	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->is_running = true;
	::pthread_create(&self->handle, nullptr, _platform_thread_main_routine, self);
	return self;
}

void
platform_thread_deinit(Platform_Thread *self)
{
	self->is_running = false;
	::pthread_join(self->handle, nullptr);
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

struct Platform_Window_Context
{
	Display *display;
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_atom_t wm_delete_window_atom;
	xcb_atom_t wm_protocols_atom;
	xcb_atom_t clipboard_atom;
	xcb_atom_t targets_atom;
	xcb_atom_t utf8_string_atom;
	xcb_atom_t text_atom;
	String clipboard_text;
	bool clipboard_owned;
};

inline static void
_platform_linux_clipboard_handle_selection_request(Platform_Window_Context *ctx, xcb_selection_request_event_t *request)
{
	xcb_selection_notify_event_t response = {};
	response.response_type = XCB_SELECTION_NOTIFY;
	response.time         = request->time;
	response.requestor    = request->requestor;
	response.selection    = request->selection;
	response.target       = request->target;
	response.property     = XCB_ATOM_NONE;

	if (ctx->clipboard_owned && request->selection == ctx->clipboard_atom)
	{
		xcb_atom_t property = request->property == XCB_ATOM_NONE ? request->target : request->property;
		if (request->target == ctx->targets_atom)
		{
			xcb_atom_t targets[] = {
				ctx->targets_atom,
				ctx->utf8_string_atom,
				ctx->text_atom,
				XCB_ATOM_STRING
			};
			::xcb_change_property(ctx->connection, XCB_PROP_MODE_REPLACE, request->requestor, property, XCB_ATOM_ATOM, 32, COUNT_OF(targets), targets);
			response.property = property;
		}
		else if (request->target == ctx->utf8_string_atom || request->target == ctx->text_atom || request->target == XCB_ATOM_STRING)
		{
			xcb_atom_t type = ctx->utf8_string_atom;
			if (request->target == XCB_ATOM_STRING)
				type = XCB_ATOM_STRING;
			::xcb_change_property(ctx->connection, XCB_PROP_MODE_REPLACE, request->requestor, property, type, 8, (U32)ctx->clipboard_text.count, ctx->clipboard_text.data);
			response.property = property;
		}
	}

	::xcb_send_event(ctx->connection, false, request->requestor, 0, (const char *)&response);
	::xcb_flush(ctx->connection);
}

Platform_Window
platform_window_init(U32 width, U32 height, const char *title)
{
	Display *display             = ::XOpenDisplay(nullptr);
	xcb_connection_t *connection = ::XGetXCBConnection(display);
	if (::xcb_connection_has_error(connection))
	{
		validate(false, "[PLATFORM]: Failed to connect to X server via XCB.");
		return {};
	}

	const struct xcb_setup_t *setup = ::xcb_get_setup(connection);
	xcb_screen_iterator_t iterator  = ::xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen            = iterator.data;

	xcb_window_t window = ::xcb_generate_id(connection);

	U32 event_mask   = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	U32 event_values = XCB_EVENT_MASK_BUTTON_PRESS   |
					   XCB_EVENT_MASK_BUTTON_RELEASE |
					   XCB_EVENT_MASK_KEY_PRESS      |
					   XCB_EVENT_MASK_KEY_RELEASE    |
					   XCB_EVENT_MASK_EXPOSURE       |
					   XCB_EVENT_MASK_POINTER_MOTION |
					   XCB_EVENT_MASK_FOCUS_CHANGE   |
					   XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	U32 value_list[] = {screen->black_pixel, event_values};

	// Create the window.
	::xcb_create_window(
		connection,
		XCB_COPY_FROM_PARENT,
		window,
		screen->root,
		0,
		0,
		width,
		height,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		event_mask,
		value_list
	);

	// Set the title.
	::xcb_change_property(
		connection,
		XCB_PROP_MODE_REPLACE,
		window,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		8,
		::strlen(title),
		title
	);

	// Tell the server to notify when the window manager attempts to destroy the window.
	xcb_intern_atom_cookie_t wm_delete_cookie    = ::xcb_intern_atom(connection, 0, ::strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	xcb_intern_atom_cookie_t wm_protocols_cookie = ::xcb_intern_atom(connection, 0, ::strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *wm_delete_reply     = ::xcb_intern_atom_reply(connection, wm_delete_cookie, nullptr);
	xcb_intern_atom_reply_t *wm_protocols_reply  = ::xcb_intern_atom_reply(connection, wm_protocols_cookie, nullptr);

	::xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

	// Set auto repeat keys to off.
	::XAutoRepeatOff(display);

	// Map the window to the screen.
	::xcb_map_window(connection, window);

	// Flush the stream.
	I32 stream_result = ::xcb_flush(connection);
	if (stream_result <= 0)
	{
		validate(false, "[PLATFORM]: An error occurred when flusing the stream.");
		return {};
	}

	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();
	ctx->display               = display;
	ctx->connection            = connection;
	ctx->window                = window;
	ctx->wm_delete_window_atom = wm_delete_reply->atom;
	ctx->wm_protocols_atom     = wm_protocols_reply->atom;
	ctx->clipboard_atom        = _platform_linux_intern_atom(connection, "CLIPBOARD");
	ctx->targets_atom          = _platform_linux_intern_atom(connection, "TARGETS");
	ctx->utf8_string_atom      = _platform_linux_intern_atom(connection, "UTF8_STRING");
	ctx->text_atom             = _platform_linux_intern_atom(connection, "TEXT");

	return Platform_Window {
		.handle = ctx,
		.width  = width,
		.height = height,
		.input  = {},
		.focused = true,
		.surface_valid = true,
		.surface_changed = true
	};
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	// Set auto repeat keys back to on.
	::XAutoRepeatOn(ctx->display);
	if (ctx->clipboard_owned)
		::xcb_set_selection_owner(ctx->connection, XCB_NONE, ctx->clipboard_atom, XCB_CURRENT_TIME);
	if (ctx->clipboard_text.capacity > 0)
		string_deinit(ctx->clipboard_text);
	::xcb_destroy_window(ctx->connection, ctx->window);

	memory::deallocate(ctx);
	self->handle = nullptr;
	self->close_requested = true;
	self->surface_valid = false;
}

bool
platform_window_poll(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	bool surface_changed = self->surface_changed;
	self->surface_changed = false;

	for (I32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		self->input.keys[i].pressed       = false;
		self->input.keys[i].released      = false;
		self->input.keys[i].press_count   = 0;
		self->input.keys[i].release_count = 0;
	}
	self->input.mouse_wheel = 0.0f;

	while (xcb_generic_event_t *xcb_event = ::xcb_poll_for_event(ctx->connection))
	{
		switch (xcb_event->response_type & ~0x80)
		{
			case XCB_CLIENT_MESSAGE:
			{
				xcb_client_message_event_t *xcb_client_message = (xcb_client_message_event_t *)xcb_event;
				if (xcb_client_message->data.data32[0] == ctx->wm_delete_window_atom)
				{
					self->close_requested = true;
					self->surface_valid = false;
				}
				break;
			}
			case XCB_SELECTION_REQUEST:
			{
				_platform_linux_clipboard_handle_selection_request(ctx, (xcb_selection_request_event_t *)xcb_event);
				break;
			}
			case XCB_SELECTION_CLEAR:
			{
				xcb_selection_clear_event_t *selection_clear = (xcb_selection_clear_event_t *)xcb_event;
				if (selection_clear->selection == ctx->clipboard_atom)
				{
					ctx->clipboard_owned = false;
					if (ctx->clipboard_text.capacity > 0)
						string_deinit(ctx->clipboard_text);
					ctx->clipboard_text = {};
				}
				break;
			}
			case XCB_FOCUS_IN:  self->focused = true;  break;
			case XCB_FOCUS_OUT: self->focused = false; break;
			case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t *xcb_mouse_press_event = (xcb_button_press_event_t *)xcb_event;

				PLATFORM_KEY key = _platform_key_from_xcb_button(xcb_mouse_press_event->detail);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].pressed = true;
					self->input.keys[key].down    = true;
					self->input.keys[key].press_count++;
					if (key == PLATFORM_KEY_MOUSE_WHEEL_UP)
						self->input.mouse_wheel += 1.0f;
					else if (key == PLATFORM_KEY_MOUSE_WHEEL_DOWN)
						self->input.mouse_wheel -= 1.0f;
				}
				break;
			}
			case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t *xcb_mouse_release_event = (xcb_button_release_event_t *)xcb_event;

				PLATFORM_KEY key = _platform_key_from_xcb_button(xcb_mouse_release_event->detail);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].released = true;
					self->input.keys[key].down     = false;
					self->input.keys[key].release_count++;
				}
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *xcb_key_press_event = (xcb_key_press_event_t *)xcb_event;
				KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_press_event->detail, 0, 0);

				PLATFORM_KEY key = _platform_key_from_key_sym(key_sym);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].pressed  = true;
					self->input.keys[key].down     = true;
					self->input.keys[key].press_count++;
				}
				break;
			}
			case XCB_KEY_RELEASE:
			{
				xcb_key_release_event_t *xcb_key_release_event = (xcb_key_release_event_t *)xcb_event;
				KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_release_event->detail, 0, 0);

				PLATFORM_KEY key = _platform_key_from_key_sym(key_sym);
				if (key != PLATFORM_KEY_COUNT)
				{
					self->input.keys[key].released = true;
					self->input.keys[key].down     = false;
					self->input.keys[key].release_count++;
				}
				break;
			}
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t *xcb_configure_notify_event = (xcb_configure_notify_event_t *)xcb_event;
				if (self->width != (U32)xcb_configure_notify_event->width || self->height != (U32)xcb_configure_notify_event->height)
				{
					self->width  = xcb_configure_notify_event->width;
					self->height = xcb_configure_notify_event->height;
					surface_changed = true;
				}
				break;
			}
			default:
				break;
		}

		::free(xcb_event);
	}

	if (self->close_requested)
		return false;

	{
		// NOTE: Mouse movement.
		xcb_query_pointer_cookie_t xcb_query_pointer_cookie = ::xcb_query_pointer_unchecked(ctx->connection, ctx->window);
		xcb_query_pointer_reply_t *xcb_query_pointer_reply  = ::xcb_query_pointer_reply(ctx->connection, xcb_query_pointer_cookie, nullptr);

		I32 window_mouse_x = xcb_query_pointer_reply->win_x;
		I32 window_mouse_y = xcb_query_pointer_reply->win_y;
		if (window_mouse_x >= 0 && (U32)window_mouse_x < self->width && window_mouse_y >= 0 && (U32)window_mouse_y < self->height)
		{
			if (window_mouse_x != self->input.mouse_x || window_mouse_y != self->input.mouse_y)
			{
				// NOTE: We want mouse coords to start bottom-left.
				U32 mouse_point_y_inverted = (self->height - 1) - window_mouse_y;
				self->input.mouse_dx = window_mouse_x - self->input.mouse_x;
				self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
				self->input.mouse_x  = window_mouse_x;
				self->input.mouse_y  = mouse_point_y_inverted;
			}
		}

		::free(xcb_query_pointer_reply);
	}

	self->paused = false;
	self->surface_valid = self->width > 0 && self->height > 0;
	self->surface_changed = surface_changed;
	return !self->close_requested;
}

Platform_Window_Native_Handles
platform_window_get_native_handles(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	return Platform_Window_Native_Handles {
		.window = &ctx->window,
		.context = ctx->connection
	};
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	::xcb_change_property(ctx->connection, XCB_PROP_MODE_REPLACE, ctx->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, ::strlen(title), title);
}

void
platform_window_close(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	self->close_requested = true;
	self->surface_valid = false;

	XEvent event;
	::memset(&event, 0, sizeof(event));
	event.xclient.type         = ClientMessage;
	event.xclient.window       = ctx->window;
	event.xclient.message_type = ::XInternAtom(ctx->display, "WM_PROTOCOLS", true);
	event.xclient.format       = 32;
	event.xclient.data.l[0]    = ::XInternAtom(ctx->display, "WM_DELETE_WINDOW", false);
	event.xclient.data.l[1]    = CurrentTime;
	::XSendEvent(ctx->display, ctx->window, false, NoEventMask, &event);
}

void
platform_set_current_directory()
{
	char module_path_relative[PATH_MAX + 1];
	::memset(module_path_relative, 0, sizeof(module_path_relative));

	char module_path_absolute[PATH_MAX + 1];
	::memset(module_path_absolute, 0, sizeof(module_path_absolute));

	[[maybe_unused]] I64 module_path_relative_length = ::readlink("/proc/self/exe", module_path_relative, sizeof(module_path_relative));
	validate(module_path_relative_length != -1 && module_path_relative_length < (I64)sizeof(module_path_relative), "[PLATFORM]: Failed to get relative path of the current executable.");

	[[maybe_unused]] char *path_absolute = ::realpath(module_path_relative, module_path_absolute);
	validate(path_absolute == module_path_absolute, "[PLATFORM]: Failed to get absolute path of the current executable.");

	char *last_slash = module_path_absolute;
	char *iterator = module_path_absolute;
	while (*iterator++)
	{
		if (*iterator == '/')
			last_slash = ++iterator;
	}
	*last_slash = '\0';

	[[maybe_unused]] I32 result = ::chdir(module_path_absolute);
	validate(result == 0, "[PLATFORM]: Failed to set current directory.");
	::strcpy(current_executable_directory, module_path_absolute);
}

bool
platform_file_exists(const char *filepath)
{
	struct stat file_stat = {};
	return ::stat(filepath, &file_stat) == 0;
}

U64
platform_file_size(const char *filepath)
{
	struct stat file_stat = {};
	if (::stat(filepath, &file_stat) == 0)
		return file_stat.st_size;
	return 0;
}

String
platform_file_read(const String &file_path, memory::Allocator *allocator)
{
	String content = string_init(allocator);

	I32 file_handle = ::open(file_path.data, O_RDONLY, S_IRWXU);
	if (file_handle == -1)
		return content;

	U64 file_size = platform_file_size(file_path.data);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);

	I64 bytes_read = ::read(file_handle, content.data, content.count);
	validate(::close(file_handle) == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_read == -1)
		return content;

	validate((I64)content.count == bytes_read);

	return content;
}

U64
platform_file_read(const char *filepath, Memory_Block block)
{
	I32 file_handle = ::open(filepath, O_RDONLY, S_IRWXU);
	if (file_handle == -1)
		return 0;

	I64 bytes_read = ::read(file_handle, block.data, block.size);
	[[maybe_unused]] I32 close_result = ::close(file_handle);
	validate(close_result == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_read == -1)
		return 0;
	return bytes_read;
}

U64
platform_file_write(const char *filepath, Memory_Block block)
{
	I32 file_handle = ::open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (file_handle == -1)
		return 0;

	I64 bytes_written = ::write(file_handle, block.data, block.size);
	[[maybe_unused]] I32 close_result = ::close(file_handle);
	validate(close_result == 0, "[PLATFORM]: Failed to close file handle.");
	if (bytes_written == -1)
		return 0;
	return bytes_written;
}

Platform_File_Handle
platform_file_open(const String &path, Platform_File_Mode mode)
{
	int flags = 0;
	switch (mode)
	{
		case PLATFORM_FILE_MODE_READ:       flags = O_RDONLY;                     break;
		case PLATFORM_FILE_MODE_WRITE:      flags = O_WRONLY | O_CREAT | O_TRUNC; break;
		case PLATFORM_FILE_MODE_READ_WRITE: flags = O_RDWR   | O_CREAT;           break;
		case PLATFORM_FILE_MODE_APPEND:     flags = O_WRONLY | O_CREAT | O_APPEND; break;
	}
	int fd = ::open(path.data, flags, S_IRWXU);
	return fd == -1 ? PLATFORM_FILE_HANDLE_INVALID : (Platform_File_Handle)(intptr_t)fd;
}

void
platform_file_close(Platform_File_Handle handle)
{
	if (handle)
		::close((int)(intptr_t)handle);
}

U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size)
{
	ssize_t bytes_read = ::read((int)(intptr_t)handle, data, size);
	return bytes_read < 0 ? 0 : (U64)bytes_read;
}

U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size)
{
	ssize_t bytes_written = ::write((int)(intptr_t)handle, data, size);
	return bytes_written < 0 ? 0 : (U64)bytes_written;
}

bool
platform_file_seek(Platform_File_Handle handle, I64 offset, Platform_File_Seek_Origin origin)
{
	int whence = SEEK_SET;
	switch (origin)
	{
		case PLATFORM_FILE_SEEK_ORIGIN_BEGIN:   whence = SEEK_SET; break;
		case PLATFORM_FILE_SEEK_ORIGIN_CURRENT: whence = SEEK_CUR; break;
		case PLATFORM_FILE_SEEK_ORIGIN_END:     whence = SEEK_END; break;
	}
	return ::lseek((int)(intptr_t)handle, (off_t)offset, whence) != (off_t)-1;
}

U64
platform_file_tell(Platform_File_Handle handle)
{
	return (U64)::lseek((int)(intptr_t)handle, 0, SEEK_CUR);
}

U64
platform_file_size(Platform_File_Handle handle)
{
	struct stat st = {};
	::fstat((int)(intptr_t)handle, &st);
	return (U64)st.st_size;
}

bool
platform_file_copy(const char *from, const char *to)
{
	I32 src_file = ::open(from, O_RDONLY);
	if (src_file < 0)
		return false;

	I32 dst_file = ::open(to, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (dst_file < 0)
	{
		::close(src_file);
		return false;
	}

	DEFER({
		::close(src_file);
		::close(dst_file);
	});

	char buffer[8192];
	while (true)
	{
		I64 bytes_read = ::read(src_file, buffer, sizeof(buffer));
		if (bytes_read == 0)
			break;

		if (bytes_read == -1)
			return false;

		I64 bytes_written = ::write(dst_file, buffer, bytes_read);
		if (bytes_written != bytes_read)
			return false;
	}

	return true;
}

bool
platform_file_delete(const char *filepath)
{
	return ::unlink(filepath) == 0;
}

inline static I32
_platform_linux_hex_value(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

inline static String
_platform_linux_file_uri_to_path(const char *uri, memory::Allocator *allocator)
{
	if (uri == nullptr)
		return string_init(allocator);

	const char *prefix = "file://";
	U64 prefix_count = ::strlen(prefix);
	if (::strncmp(uri, prefix, prefix_count) != 0)
		return string_from(uri, allocator);

	const char *path = uri + prefix_count;
	if (::strncmp(path, "localhost/", 10) == 0)
		path += 9;

	String result = string_init(allocator);
	for (const char *at = path; *at != '\0'; ++at)
	{
		if (at[0] == '%' && at[1] != '\0' && at[2] != '\0')
		{
			I32 high = _platform_linux_hex_value(at[1]);
			I32 low = _platform_linux_hex_value(at[2]);
			if (high >= 0 && low >= 0)
			{
				string_append(result, (char)((high << 4) | low));
				at += 2;
				continue;
			}
		}

		string_append(result, *at);
	}
	return result;
}

inline static void
_platform_linux_portal_append_filter_patterns(DBusMessageIter *patterns_array, const char *patterns)
{
	const char *pattern = patterns;
	while (pattern && *pattern != '\0')
	{
		while (*pattern == ' ')
			++pattern;

		const char *pattern_end = pattern;
		while (*pattern_end != '\0' && *pattern_end != ';' && *pattern_end != ',')
			++pattern_end;

		if (pattern_end != pattern)
		{
			char pattern_buffer[256] = {};
			U64 pattern_count = u64_min((U64)(pattern_end - pattern), sizeof(pattern_buffer) - 1);
			::memcpy(pattern_buffer, pattern, pattern_count);

			DBusMessageIter pattern_struct = {};
			dbus_uint32_t pattern_type = 0;
			const char *pattern_string = pattern_buffer;
			dbus_message_iter_open_container(patterns_array, DBUS_TYPE_STRUCT, nullptr, &pattern_struct);
			dbus_message_iter_append_basic(&pattern_struct, DBUS_TYPE_UINT32, &pattern_type);
			dbus_message_iter_append_basic(&pattern_struct, DBUS_TYPE_STRING, &pattern_string);
			dbus_message_iter_close_container(patterns_array, &pattern_struct);
		}

		pattern = *pattern_end == '\0' ? pattern_end : pattern_end + 1;
	}
}

inline static void
_platform_linux_portal_append_filters(DBusMessageIter *options_array, const char *filters)
{
	if (filters == nullptr || filters[0] == '\0')
		return;

	DBusMessageIter dict_entry = {};
	DBusMessageIter variant = {};
	DBusMessageIter filters_array = {};
	const char *filters_key = "filters";
	dbus_message_iter_open_container(options_array, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_entry);
	dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &filters_key);
	dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "a(sa(us))", &variant);
	dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "(sa(us))", &filters_array);

	const char *label = filters;
	while (label[0] != '\0')
	{
		const char *patterns = label + ::strlen(label) + 1;
		if (patterns[0] == '\0')
			break;

		DBusMessageIter filter_struct = {};
		DBusMessageIter patterns_array = {};
		dbus_message_iter_open_container(&filters_array, DBUS_TYPE_STRUCT, nullptr, &filter_struct);
		dbus_message_iter_append_basic(&filter_struct, DBUS_TYPE_STRING, &label);
		dbus_message_iter_open_container(&filter_struct, DBUS_TYPE_ARRAY, "(us)", &patterns_array);
		_platform_linux_portal_append_filter_patterns(&patterns_array, patterns);
		dbus_message_iter_close_container(&filter_struct, &patterns_array);
		dbus_message_iter_close_container(&filters_array, &filter_struct);

		label = patterns + ::strlen(patterns) + 1;
	}

	dbus_message_iter_close_container(&variant, &filters_array);
	dbus_message_iter_close_container(&dict_entry, &variant);
	dbus_message_iter_close_container(options_array, &dict_entry);
}

inline static void
_platform_linux_portal_append_options(DBusMessageIter *args, const char *filters)
{
	DBusMessageIter options_array = {};
	dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, "{sv}", &options_array);
	_platform_linux_portal_append_filters(&options_array, filters);
	dbus_message_iter_close_container(args, &options_array);
}

inline static String
_platform_linux_portal_response_path(DBusMessage *message, memory::Allocator *allocator)
{
	DBusMessageIter args = {};
	if (!dbus_message_iter_init(message, &args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32)
		return string_init(allocator);

	dbus_uint32_t response = 1;
	dbus_message_iter_get_basic(&args, &response);
	if (response != 0 || !dbus_message_iter_next(&args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
		return string_init(allocator);

	DBusMessageIter results = {};
	dbus_message_iter_recurse(&args, &results);
	while (dbus_message_iter_get_arg_type(&results) == DBUS_TYPE_DICT_ENTRY)
	{
		DBusMessageIter entry = {};
		dbus_message_iter_recurse(&results, &entry);
		if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
		{
			const char *key = nullptr;
			dbus_message_iter_get_basic(&entry, &key);
			if (key && ::strcmp(key, "uris") == 0 && dbus_message_iter_next(&entry) && dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT)
			{
				DBusMessageIter variant = {};
				dbus_message_iter_recurse(&entry, &variant);
				if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY)
				{
					DBusMessageIter uris = {};
					dbus_message_iter_recurse(&variant, &uris);
					if (dbus_message_iter_get_arg_type(&uris) == DBUS_TYPE_STRING)
					{
						const char *uri = nullptr;
						dbus_message_iter_get_basic(&uris, &uri);
						return _platform_linux_file_uri_to_path(uri, allocator);
					}
				}
			}
		}

		dbus_message_iter_next(&results);
	}

	return string_init(allocator);
}

inline static String
_platform_linux_portal_file_dialog_run(const char *method, const char *title, const char *filters, memory::Allocator *allocator)
{
	DBusError error = {};
	dbus_error_init(&error);
	DBusConnection *connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if (dbus_error_is_set(&error) || connection == nullptr)
	{
		dbus_error_free(&error);
		return string_init(allocator);
	}
	DEFER(dbus_connection_unref(connection););

	DBusMessage *message = dbus_message_new_method_call("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.FileChooser", method);
	if (message == nullptr)
		return string_init(allocator);
	DEFER(dbus_message_unref(message););

	const char *parent_window = "";
	DBusMessageIter args = {};
	dbus_message_iter_init_append(message, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parent_window);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &title);
	_platform_linux_portal_append_options(&args, filters);

	DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, message, 30000, &error);
	if (dbus_error_is_set(&error) || reply == nullptr)
	{
		dbus_error_free(&error);
		return string_init(allocator);
	}
	DEFER(dbus_message_unref(reply););

	const char *handle_path = nullptr;
	if (!dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &handle_path, DBUS_TYPE_INVALID) || handle_path == nullptr)
	{
		dbus_error_free(&error);
		return string_init(allocator);
	}

	char match_rule[1024] = {};
	::snprintf(match_rule, sizeof(match_rule), "type='signal',interface='org.freedesktop.portal.Request',member='Response',path='%s'", handle_path);
	dbus_bus_add_match(connection, match_rule, &error);
	if (dbus_error_is_set(&error))
	{
		dbus_error_free(&error);
		return string_init(allocator);
	}
	DEFER({
		dbus_bus_remove_match(connection, match_rule, nullptr);
		dbus_connection_flush(connection);
	});
	dbus_connection_flush(connection);

	while (true)
	{
		dbus_connection_read_write(connection, -1);
		DBusMessage *signal = dbus_connection_pop_message(connection);
		if (signal == nullptr)
			continue;
		DEFER(dbus_message_unref(signal););

		const char *signal_path = dbus_message_get_path(signal);
		if (signal_path && dbus_message_is_signal(signal, "org.freedesktop.portal.Request", "Response") && ::strcmp(signal_path, handle_path) == 0)
			return _platform_linux_portal_response_path(signal, allocator);
	}
}

String
platform_file_dialog_open(const char *filters, memory::Allocator *allocator)
{
	return _platform_linux_portal_file_dialog_run("OpenFile", "Open File", filters, allocator);
}

String
platform_file_dialog_save(const char *filters, memory::Allocator *allocator)
{
	return _platform_linux_portal_file_dialog_run("SaveFile", "Save File", filters, allocator);
}

inline static String
_platform_linux_clipboard_read_target(xcb_connection_t *connection, xcb_window_t window, xcb_atom_t clipboard, xcb_atom_t target, xcb_atom_t property, memory::Allocator *allocator)
{
	::xcb_delete_property(connection, window, property);
	::xcb_convert_selection(connection, window, clipboard, target, property, XCB_CURRENT_TIME);
	::xcb_flush(connection);

	I32 fd = ::xcb_get_file_descriptor(connection);
	if (fd < 0)
		return string_init(allocator);

	while (true)
	{
		while (xcb_generic_event_t *event = ::xcb_poll_for_event(connection))
		{
			DEFER(::free(event););
			if ((event->response_type & ~0x80) != XCB_SELECTION_NOTIFY)
				continue;

			xcb_selection_notify_event_t *notify = (xcb_selection_notify_event_t *)event;
			if (notify->selection != clipboard || notify->target != target)
				continue;

			if (notify->property == XCB_ATOM_NONE)
				return string_init(allocator);

			xcb_get_property_cookie_t cookie = ::xcb_get_property(connection, false, window, property, XCB_GET_PROPERTY_TYPE_ANY, 0, U32_MAX / 4);
			xcb_get_property_reply_t *reply = ::xcb_get_property_reply(connection, cookie, nullptr);
			if (reply == nullptr)
				return string_init(allocator);
			DEFER(::free(reply););

			I32 value_length = ::xcb_get_property_value_length(reply);
			if (reply->format != 8 || value_length <= 0)
				return string_init(allocator);

			String result = string_init(allocator);
			string_resize(result, (U64)value_length);
			::memcpy(result.data, ::xcb_get_property_value(reply), (U64)value_length);
			return result;
		}

		fd_set read_fds = {};
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		timeval timeout = {};
		timeout.tv_sec = 5;
		I32 select_result = ::select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
		if (select_result == 0)
			return string_init(allocator);
		if (select_result < 0 && errno != EINTR)
			return string_init(allocator);
	}
}

String
platform_window_clipboard_read_text(Platform_Window &window, memory::Allocator *allocator)
{
	validate(window.handle != nullptr, "[PLATFORM][LINUX]: Clipboard read requires an initialized platform window.");

	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	if (ctx->clipboard_owned)
		return string_copy(ctx->clipboard_text, allocator);

	I32 screen_index = 0;
	xcb_connection_t *connection = ::xcb_connect(nullptr, &screen_index);
	if (connection == nullptr || ::xcb_connection_has_error(connection))
	{
		if (connection)
			::xcb_disconnect(connection);
		return string_init(allocator);
	}
	DEFER(::xcb_disconnect(connection););

	const xcb_setup_t *setup = ::xcb_get_setup(connection);
	xcb_screen_iterator_t iterator = ::xcb_setup_roots_iterator(setup);
	for (I32 i = 0; i < screen_index; ++i)
		::xcb_screen_next(&iterator);
	xcb_screen_t *screen = iterator.data;

	xcb_window_t read_window = ::xcb_generate_id(connection);
	U32 event_mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
	::xcb_create_window(connection, XCB_COPY_FROM_PARENT, read_window, screen->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, &event_mask);
	DEFER(::xcb_destroy_window(connection, read_window););

	xcb_atom_t clipboard = _platform_linux_intern_atom(connection, "CLIPBOARD");
	xcb_atom_t utf8_string = _platform_linux_intern_atom(connection, "UTF8_STRING");
	xcb_atom_t property = _platform_linux_intern_atom(connection, "CORE_CLIPBOARD_READ");
	if (clipboard == XCB_ATOM_NONE || utf8_string == XCB_ATOM_NONE || property == XCB_ATOM_NONE)
		return string_init(allocator);

	String result = _platform_linux_clipboard_read_target(connection, read_window, clipboard, utf8_string, property, allocator);
	if (result.count == 0)
		result = _platform_linux_clipboard_read_target(connection, read_window, clipboard, XCB_ATOM_STRING, property, allocator);
	return result;
}

bool
platform_window_clipboard_write_text(Platform_Window &window, const String &text)
{
	validate(window.handle != nullptr, "[PLATFORM][LINUX]: Clipboard write requires an initialized platform window.");

	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	if (ctx->clipboard_text.capacity > 0)
		string_deinit(ctx->clipboard_text);
	ctx->clipboard_text = text.data ? string_copy(text, memory::heap_allocator()) : string_init(memory::heap_allocator());

	::xcb_set_selection_owner(ctx->connection, ctx->window, ctx->clipboard_atom, XCB_CURRENT_TIME);
	xcb_get_selection_owner_cookie_t cookie = ::xcb_get_selection_owner(ctx->connection, ctx->clipboard_atom);
	xcb_get_selection_owner_reply_t *reply = ::xcb_get_selection_owner_reply(ctx->connection, cookie, nullptr);
	DEFER(if (reply) ::free(reply););

	ctx->clipboard_owned = reply && reply->owner == ctx->window;
	if (!ctx->clipboard_owned)
	{
		string_deinit(ctx->clipboard_text);
		ctx->clipboard_text = {};
		return false;
	}

	::xcb_flush(ctx->connection);
	return true;
}

U64
platform_query_microseconds()
{
	struct timespec time;
	[[maybe_unused]] I32 result = clock_gettime(CLOCK_MONOTONIC, &time);
	validate(result == 0, "[PLATFORM]: Failed to query clock.");
	return time.tv_sec * 1000000 + time.tv_nsec * 0.001;
}

void
platform_sleep_set_period(U32)
{

}

void
platform_sleep(U32 milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	nanosleep(&ts, 0);
}

U32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	::memset(callstack, 0, frame_count * sizeof(*callstack));
	return ::backtrace(callstack, frame_count);
#else
	return 0;
#endif
}

#if DEBUG
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
#endif

void
platform_callstack_resolve([[maybe_unused]] void **callstack, [[maybe_unused]] Platform_Callstack_Frame *frames, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	char **symbols = ::backtrace_symbols(callstack, frame_count);
	for (U32 i = 0; i < frame_count; ++i)
	{
		Platform_Callstack_Frame *frame = frames + i;
		frame->address      = callstack[i];
		frame->line         = 0;
		frame->symbol_found = symbols != nullptr;
		frame->line_found   = false;
		frame->symbol[0]    = '\0';
		frame->file[0]      = '\0';
		if (symbols)
			_platform_callstack_copy_string(frame->symbol, PLATFORM_CALLSTACK_SYMBOL_LENGTH, symbols[i]);
	}
	if (symbols)
		::free(symbols);
#endif
}