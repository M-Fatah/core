#include "core/platform/platform.h"

#include "core/assert.h"
#include "core/logger.h"

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
#include <X11/Xlib-xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

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

inline static PLATFORM_KEY
_platform_key_from_x_key_sym(KeySym key)
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

Platform_Api
platform_api_init(const char *filepath)
{
	Platform_Api self = {};

	char path[128] = {};
	_string_concat(filepath, ".so", path);

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	[[maybe_unused]] bool copy_result = platform_file_copy(path, path_tmp);
	ASSERT(copy_result, "[PLATFORM]: Failed to copy library.");

	self.handle = ::dlopen(path_tmp, RTLD_LAZY);
	ASSERT(self.handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self.handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");
	self.api = proc(nullptr, false);
	ASSERT(self.api, "[PLATFORM]: Failed to get api.");

	struct stat file_stat = {};
	[[maybe_unused]] i32 stat_result = ::stat(path, &file_stat);
	ASSERT(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");
	self.last_write_time = file_stat.st_mtime;

	self.filepath = filepath;

	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
		ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");
		self->api = proc(self->api, false);
	}

	::dlclose(self->handle);
}

void *
platform_api_load(Platform_Api *self)
{
	char path[128] = {};
	_string_concat(self->filepath, ".so", path);

	struct stat file_stat = {};
	i32 stat_result = ::stat(path, &file_stat);
	ASSERT(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");

	u64 last_write_time = file_stat.st_mtime;
	if ((last_write_time == self->last_write_time) || (stat_result != 0))
		return self->api;

	::dlclose(self->handle);

	char path_tmp[128] = {};
	_string_concat(path, ".tmp", path_tmp);

	bool copy_result = platform_file_copy(path, path_tmp);

	self->handle = ::dlopen(path_tmp, RTLD_LAZY);
	ASSERT(self->handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");
	self->api = proc(self->api, true);
	ASSERT(self->api, "[PLATFORM]: Failed to get api.");

	// If copying failed we don't update last write time so that we can try copying it again in the next frame.
	if (copy_result == true)
		self->last_write_time = last_write_time;

	return self->api;
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

struct Platform_Window_Context
{
	Display *display;
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_atom_t wm_delete_window_atom;
	xcb_atom_t wm_protocols_atom;
};

Platform_Window
platform_window_init(u32 width, u32 height, const char *title)
{
	Display *display             = ::XOpenDisplay(nullptr);
	xcb_connection_t *connection = ::XGetXCBConnection(display);
	if (::xcb_connection_has_error(connection))
	{
		ASSERT(false, "[PLATFORM]: Failed to connect to X server via XCB.");
		return {};
	}

	const struct xcb_setup_t *setup = ::xcb_get_setup(connection);
	xcb_screen_iterator_t iterator  = ::xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen             = iterator.data;

	xcb_window_t window = ::xcb_generate_id(connection);

	u32 event_mask   = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS   |
					   XCB_EVENT_MASK_BUTTON_RELEASE |
					   XCB_EVENT_MASK_KEY_PRESS      |
					   XCB_EVENT_MASK_KEY_RELEASE    |
					   XCB_EVENT_MASK_EXPOSURE       |
					   XCB_EVENT_MASK_POINTER_MOTION |
					   XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	u32 value_list[] = {screen->black_pixel, event_values};

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

	// TODO: Affects the whole system, also, properly name variables.
	u32 kb_mask = XCB_KB_AUTO_REPEAT_MODE;
	u32 kb_values[] = {XCB_AUTO_REPEAT_MODE_OFF, None};
	xcb_change_keyboard_control(connection, kb_mask, kb_values);

	// Map the window to the screen.
	::xcb_map_window(connection, window);

	// Flush the stream.
	i32 stream_result = ::xcb_flush(connection);
	if (stream_result <= 0)
	{
		ASSERT(false, "[PLATFORM]: An error occurred when flusing the stream.");
		return {};
	}

	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();
	ctx->display               = display;
	ctx->connection            = connection;
	ctx->window                = window;
	ctx->wm_delete_window_atom = wm_delete_reply->atom;
	ctx->wm_protocols_atom     = wm_protocols_reply->atom;

	return Platform_Window {
		.handle = ctx,
		.width  = width,
		.height = height,
		.input  = {}
	};
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	::xcb_destroy_window(ctx->connection, ctx->window);
	::xcb_disconnect(ctx->connection);

	memory::deallocate(ctx);
}

// TODO: Cleanup and use proper naming.
bool
platform_window_poll(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	for (i32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
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
					return false;
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *xcb_key_press_event = (xcb_key_press_event_t *)xcb_event;
				KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_press_event->detail, 0, 0);

				PLATFORM_KEY key = _platform_key_from_x_key_sym(key_sym);
				self->input.keys[key].pressed  = true;
				self->input.keys[key].down     = true;
				self->input.keys[key].press_count++;
				break;
			}
			case XCB_KEY_RELEASE:
			{
				xcb_key_release_event_t *xcb_key_release_event = (xcb_key_release_event_t *)xcb_event;
				KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_release_event->detail, 0, 0);

				PLATFORM_KEY key = _platform_key_from_x_key_sym(key_sym);
				self->input.keys[key].released = true;
				self->input.keys[key].down     = false;
				self->input.keys[key].release_count++;
				break;
			}
			case XCB_BUTTON_PRESS:
			{
				// TODO: Collapse?
				xcb_button_press_event_t *xcb_mouse_press_event = (xcb_button_press_event_t *)xcb_event;
				switch (xcb_mouse_press_event->detail)
				{
					case XCB_BUTTON_INDEX_1:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].pressed = true;
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].down    = true;
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].press_count++;
						break;
					}
					case XCB_BUTTON_INDEX_2:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].pressed = true;
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].down    = true;
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].press_count++;
						break;
					}
					case XCB_BUTTON_INDEX_3:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].pressed = true;
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].down    = true;
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].press_count++;
						break;
					}
					case XCB_BUTTON_INDEX_4:
					{
						self->input.mouse_wheel += 1.0f;
						break;
					}
					case XCB_BUTTON_INDEX_5:
					{
						self->input.mouse_wheel += -1.0f;
						break;
					}
				}
				break;
			}
			case XCB_BUTTON_RELEASE:
			{
				// TODO: Collapse?
				xcb_button_release_event_t *xcb_mouse_release_event = (xcb_button_release_event_t *)xcb_event;
				switch (xcb_mouse_release_event->detail)
				{
					case XCB_BUTTON_INDEX_1:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].released = true;
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].down     = false;
						self->input.keys[PLATFORM_KEY_MOUSE_LEFT].release_count++;
						break;
					}
					case XCB_BUTTON_INDEX_2:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].released = true;
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].down     = false;
						self->input.keys[PLATFORM_KEY_MOUSE_MIDDLE].release_count++;
						break;
					}
					case XCB_BUTTON_INDEX_3:
					{
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].released = true;
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].down     = false;
						self->input.keys[PLATFORM_KEY_MOUSE_RIGHT].release_count++;
						break;
					}
				}
				break;
			}
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t *xcb_configure_notify_event = (xcb_configure_notify_event_t *)xcb_event;
				if (self->width != (u32)xcb_configure_notify_event->width || self->height != (u32)xcb_configure_notify_event->height)
				{
					self->width  = xcb_configure_notify_event->width;
					self->height = xcb_configure_notify_event->height;
				}
				break;
			}
			default:
				break;
		}

		::free(xcb_event);
	}

	{
		// NOTE: Mouse movement.
		xcb_query_pointer_cookie_t xcb_query_pointer_cookie = ::xcb_query_pointer_unchecked(ctx->connection, ctx->window);
		xcb_query_pointer_reply_t *xcb_query_pointer_reply  = ::xcb_query_pointer_reply(ctx->connection, xcb_query_pointer_cookie, nullptr);

		i32 window_mouse_x = xcb_query_pointer_reply->win_x;
		i32 window_mouse_y = xcb_query_pointer_reply->win_y;
		if (window_mouse_x >= 0 && (u32)window_mouse_x < self->width && window_mouse_y >= 0 && (u32)window_mouse_y < self->height)
		{
			if (window_mouse_x != self->input.mouse_x || window_mouse_y != self->input.mouse_y)
			{
				u32 mouse_point_y_inverted = (self->height - 1) - window_mouse_y;
				self->input.mouse_dx = window_mouse_x - self->input.mouse_x;
				self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
				self->input.mouse_x  = window_mouse_x;
				self->input.mouse_y  = mouse_point_y_inverted; // NOTE(M-Fatah): We want mouse coords to start bottom-left.
			}
		}
		::free(xcb_query_pointer_reply);
	}

	return true;
}

void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_display)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	if (native_handle)
		*native_handle = &ctx->window;
	if (native_display)
		*native_display = ctx->display;
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

	XEvent event;
	::memset(&event, 0, sizeof (event));
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
platform_file_copy(const char *from, const char *to)
{
	i32 src_file = ::open(from, O_RDONLY);
	if (src_file < 0)
		return false;

	i32 dst_file = ::open(to, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
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
		i64 bytes_read = ::read(src_file, buffer, sizeof(buffer));
		if (bytes_read == 0)
			break;

		if (bytes_read == -1)
			return false;

		i64 bytes_written = ::write(dst_file, buffer, bytes_read);
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
	char *result= ::fgets(path, path_length, file_handle);
	if (result == nullptr)
		return false;
	return ::pclose(file_handle) == 0;
}

bool
platform_file_dialog_save(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);

	char command[2048];
	::sprintf(command, "/usr/bin/zenity --file-selection --modal --save --file-filter=%s --title=\"Save file.\"", filters);
	FILE *file_handle = ::popen(command, "r");
	char *result = ::fgets(path, path_length, file_handle);
	if (result == nullptr)
		return false;
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
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] u32 frame_count)
{
#if DEBUG
	::memset(callstack, 0, frame_count * sizeof(callstack));
	return ::backtrace(callstack, frame_count);
#else
	return 0;
#endif
}

void
platform_callstack_log([[maybe_unused]] void **callstack, [[maybe_unused]] u32 frame_count)
{
#if DEBUG
	char** symbols = ::backtrace_symbols(callstack, frame_count);
	if (symbols)
	{
		LOG_WARNING("callstack:");
		for (u32 i = 0; i < frame_count; ++i)
			LOG_WARNING("\t[{}]: {}", frame_count - i - 1, symbols[i]);

		::free(symbols);
	}
#endif
}

Platform_Font
platform_font_init(const char *, const char *, u32, bool)
{
	return {};
}

void
platform_font_deinit(Platform_Font *)
{

}