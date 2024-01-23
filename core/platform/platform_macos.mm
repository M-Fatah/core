#include "core/platform/platform.h"

#include "core/assert.h"
#include "core/defer.h"
#include "core/memory/memory.h"

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
#include <pthread.h>
#include <atomic>
#include <inttypes.h>

#include <Foundation/Foundation.h>
#include <Cocoa/Cocoa.h>

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

// inline static PLATFORM_KEY
// _platform_key_from_xcb_button(/* TODO: */)
// {
// 	// switch (button)
// 	// {
// 	// 	case XCB_BUTTON_INDEX_1: return PLATFORM_KEY_MOUSE_LEFT;
// 	// 	case XCB_BUTTON_INDEX_2: return PLATFORM_KEY_MOUSE_MIDDLE;
// 	// 	case XCB_BUTTON_INDEX_3: return PLATFORM_KEY_MOUSE_RIGHT;
// 	// 	case XCB_BUTTON_INDEX_4: return PLATFORM_KEY_MOUSE_WHEEL_UP;
// 	// 	case XCB_BUTTON_INDEX_5: return PLATFORM_KEY_MOUSE_WHEEL_DOWN;
// 	// }
// 	return PLATFORM_KEY_COUNT;
// }

// inline static PLATFORM_KEY
// _platform_key_from_key_sym(/* TODO: */)
// {
	// switch (key)
	// {
		// case XK_a:            return PLATFORM_KEY_A;
		// case XK_b:            return PLATFORM_KEY_B;
		// case XK_c:            return PLATFORM_KEY_C;
		// case XK_d:            return PLATFORM_KEY_D;
		// case XK_e:            return PLATFORM_KEY_E;
		// case XK_f:            return PLATFORM_KEY_F;
		// case XK_g:            return PLATFORM_KEY_G;
		// case XK_h:            return PLATFORM_KEY_H;
		// case XK_i:            return PLATFORM_KEY_I;
		// case XK_j:            return PLATFORM_KEY_J;
		// case XK_k:            return PLATFORM_KEY_K;
		// case XK_l:            return PLATFORM_KEY_L;
		// case XK_m:            return PLATFORM_KEY_M;
		// case XK_n:            return PLATFORM_KEY_N;
		// case XK_o:            return PLATFORM_KEY_O;
		// case XK_p:            return PLATFORM_KEY_P;
		// case XK_q:            return PLATFORM_KEY_Q;
		// case XK_r:            return PLATFORM_KEY_R;
		// case XK_s:            return PLATFORM_KEY_S;
		// case XK_t:            return PLATFORM_KEY_T;
		// case XK_u:            return PLATFORM_KEY_U;
		// case XK_v:            return PLATFORM_KEY_V;
		// case XK_w:            return PLATFORM_KEY_W;
		// case XK_x:            return PLATFORM_KEY_X;
		// case XK_y:            return PLATFORM_KEY_Y;
		// case XK_z:            return PLATFORM_KEY_Z;
		// case XK_0:            return PLATFORM_KEY_NUM_0;
		// case XK_1:            return PLATFORM_KEY_NUM_1;
		// case XK_2:            return PLATFORM_KEY_NUM_2;
		// case XK_3:            return PLATFORM_KEY_NUM_3;
		// case XK_4:            return PLATFORM_KEY_NUM_4;
		// case XK_5:            return PLATFORM_KEY_NUM_5;
		// case XK_6:            return PLATFORM_KEY_NUM_6;
		// case XK_7:            return PLATFORM_KEY_NUM_7;
		// case XK_8:            return PLATFORM_KEY_NUM_8;
		// case XK_9:            return PLATFORM_KEY_NUM_9;
		// case XK_KP_0:         return PLATFORM_KEY_NUMPAD_0;
		// case XK_KP_1:         return PLATFORM_KEY_NUMPAD_1;
		// case XK_KP_2:         return PLATFORM_KEY_NUMPAD_2;
		// case XK_KP_3:         return PLATFORM_KEY_NUMPAD_3;
		// case XK_KP_4:         return PLATFORM_KEY_NUMPAD_4;
		// case XK_KP_5:         return PLATFORM_KEY_NUMPAD_5;
		// case XK_KP_6:         return PLATFORM_KEY_NUMPAD_6;
		// case XK_KP_7:         return PLATFORM_KEY_NUMPAD_7;
		// case XK_KP_8:         return PLATFORM_KEY_NUMPAD_8;
		// case XK_KP_9:         return PLATFORM_KEY_NUMPAD_9;
		// case XK_F1:           return PLATFORM_KEY_F1;
		// case XK_F2:           return PLATFORM_KEY_F2;
		// case XK_F3:           return PLATFORM_KEY_F3;
		// case XK_F4:           return PLATFORM_KEY_F4;
		// case XK_F5:           return PLATFORM_KEY_F5;
		// case XK_F6:           return PLATFORM_KEY_F6;
		// case XK_F7:           return PLATFORM_KEY_F7;
		// case XK_F8:           return PLATFORM_KEY_F8;
		// case XK_F9:           return PLATFORM_KEY_F9;
		// case XK_F10:          return PLATFORM_KEY_F10;
		// case XK_F11:          return PLATFORM_KEY_F11;
		// case XK_F12:          return PLATFORM_KEY_F12;
		// case XK_Up:           return PLATFORM_KEY_ARROW_UP;
		// case XK_Down:         return PLATFORM_KEY_ARROW_DOWN;
		// case XK_Left:         return PLATFORM_KEY_ARROW_LEFT;
		// case XK_Right:        return PLATFORM_KEY_ARROW_RIGHT;
		// case XK_Shift_L:      return PLATFORM_KEY_SHIFT_LEFT;
		// case XK_Shift_R:      return PLATFORM_KEY_SHIFT_RIGHT;
		// case XK_Control_L:    return PLATFORM_KEY_CONTROL_LEFT;
		// case XK_Control_R:    return PLATFORM_KEY_CONTROL_RIGHT;
		// case XK_Alt_L:        return PLATFORM_KEY_ALT_LEFT;
		// case XK_Alt_R:        return PLATFORM_KEY_ALT_RIGHT;
		// case XK_BackSpace:    return PLATFORM_KEY_BACKSPACE;
		// case XK_Tab:          return PLATFORM_KEY_TAB;
		// case XK_Return:       return PLATFORM_KEY_ENTER;
		// case XK_Escape:       return PLATFORM_KEY_ESCAPE;
		// case XK_Delete:       return PLATFORM_KEY_DELETE;
		// case XK_Insert:       return PLATFORM_KEY_INSERT;
		// case XK_Home:         return PLATFORM_KEY_HOME;
		// case XK_End:          return PLATFORM_KEY_END;
		// case XK_Page_Up:      return PLATFORM_KEY_PAGE_UP;
		// case XK_Page_Down:    return PLATFORM_KEY_PAGE_DOWN;
		// case XK_slash:        return PLATFORM_KEY_SLASH;
		// case XK_backslash:    return PLATFORM_KEY_BACKSLASH;
		// case XK_bracketleft:  return PLATFORM_KEY_BRACKET_LEFT;
		// case XK_bracketright: return PLATFORM_KEY_BRACKET_RIGHT;
		// case XK_grave:        return PLATFORM_KEY_BACKQUOTE;
		// case XK_period:       return PLATFORM_KEY_PERIOD;
		// case XK_minus:        return PLATFORM_KEY_MINUS;
		// case XK_equal:        return PLATFORM_KEY_EQUAL;
		// case XK_comma:        return PLATFORM_KEY_COMMA;
		// case XK_semicolon:    return PLATFORM_KEY_SEMICOLON;
		// case XK_space:        return PLATFORM_KEY_SPACE;
	// }
// 	return PLATFORM_KEY_COUNT;
// }

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
	ASSERT(copy_successful, "[PLATFORM]: Failed to copy library.");

	self.handle = ::dlopen(dst_absolute_path, RTLD_LAZY);
	ASSERT(self.handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self.handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self.api = proc(nullptr, PLATFORM_API_STATE_INIT);
	ASSERT(self.api, "[PLATFORM]: Failed to get api.");

	struct stat file_stat = {};
	[[maybe_unused]] i32 stat_result = ::stat(src_relative_path, &file_stat);
	ASSERT(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");

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
		ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");
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
	i32 stat_result = ::stat(self->filepath, &file_stat);
	ASSERT(stat_result == 0, "[PLATFORM]: Failed to get file attributes.");

	i64 last_write_time = file_stat.st_mtime;
	if ((last_write_time == self->last_write_time) || (stat_result != 0))
		return self->api;

	::dlclose(self->handle);

	platform_file_delete(dst_absolute_path);

	platform_sleep(100);

	bool copy_result = platform_file_copy(self->filepath, dst_absolute_path);

	self->handle = ::dlopen(dst_absolute_path, RTLD_LAZY);
	ASSERT(self->handle, "[PLATFORM]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
	ASSERT(proc, "[PLATFORM]: Failed to get proc platform_api.");

	self->api = proc(self->api, PLATFORM_API_STATE_LOAD);
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

};

@class ContentView;

@interface ContentView : NSView <NSTextInputClient> {
    NSWindow* window;
    NSTrackingArea* trackingArea;
    NSMutableAttributedString* markedText;
}

- (instancetype)initWithWindow:(NSWindow*)initWindow;

@end // ContentView

@implementation ContentView

- (instancetype)initWithWindow:(NSWindow*)initWindow {
    self = [super init];
    if (self != nil) {
        window = initWindow;
    }

    return self;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    // input_process_button(BUTTON_LEFT, true);
}

- (void)mouseDragged:(NSEvent *)event {
    // Equivalent to moving the mouse for now
    [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent *)event {
    // input_process_button(BUTTON_LEFT, false);
}

- (void)mouseMoved:(NSEvent *)event {
    // const NSPoint pos = [event locationInWindow];

    // // Need to invert Y on macOS, since origin is bottom-left.
    // // Also need to scale the mouse position by the device pixel ratio so screen lookups are correct.
    // NSSize window_size = state_ptr->handle.layer.drawableSize;
    // i16 x = pos.x * state_ptr->handle.layer.contentsScale;
    // i16 y = window_size.height - (pos.y * state_ptr->handle.layer.contentsScale);
    // input_process_mouse_move(x, y);
}

- (void)rightMouseDown:(NSEvent *)event {
    // input_process_button(BUTTON_RIGHT, true);
}

- (void)rightMouseDragged:(NSEvent *)event  {
    // Equivalent to moving the mouse for now
    [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent *)event {
    // input_process_button(BUTTON_RIGHT, false);
}

- (void)otherMouseDown:(NSEvent *)event {
    // Interpreted as middle click
    // input_process_button(BUTTON_MIDDLE, true);
}

- (void)otherMouseDragged:(NSEvent *)event {
    // Equivalent to moving the mouse for now
    [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent *)event {
    // Interpreted as middle click
    // input_process_button(BUTTON_MIDDLE, false);
}

// Handle modifier keys since they are only registered via modifier flags being set/unset.
- (void) flagsChanged:(NSEvent *) event {
    // handle_modifier_keys([event keyCode], [event modifierFlags]);
}

- (void)keyDown:(NSEvent *)event {
    // keys key = translate_keycode((u32)[event keyCode]);

    // input_process_key(key, true);

    // [self interpretKeyEvents:@[event]];
}

- (void)keyUp:(NSEvent *)event {
    // keys key = translate_keycode((u32)[event keyCode]);

    // input_process_key(key, false);
}

- (void)scrollWheel:(NSEvent *)event {
    // input_process_mouse_wheel((i8)[event scrollingDeltaY]);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {}

- (void)unmarkText {}

// Defines a constant for empty ranges in NSTextInputClient
static const NSRange kEmptyRange = { NSNotFound, 0 };

- (NSRange)selectedRange {return kEmptyRange;}

- (NSRange)markedRange {return kEmptyRange;}

- (BOOL)hasMarkedText {return false;}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return nil;}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {return [NSArray array];}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return NSMakeRect(0, 0, 0, 0);}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {return 0;}

@end // ContentView

Platform_Window
platform_window_init(u32 width, u32 height, const char *title)
{

	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();

	NSRect rect = NSMakeRect(0, 0, width, height);
	NSWindow *window = [[NSWindow alloc] initWithContentRect:rect
                    styleMask:NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                    defer:NO];
	[window setBackgroundColor:[NSColor blueColor]];
	[window makeKeyAndOrderFront:window];
	[window display];

	ContentView *content_view = [[ContentView alloc] initWithWindow:window];
    [content_view setWantsLayer:YES];

    [window setLevel:NSNormalWindowLevel];
    [window setContentView:content_view];
    [window makeFirstResponder:content_view];
    [window setTitle:@(title)];
    // [window setDelegate:wnd_delegate];
    [window setAcceptsMouseMovedEvents:YES];
    [window setRestorable:NO];

	 if (![[NSRunningApplication currentApplication] isFinishedLaunching])
        [NSApp run];

    // Making the app a proper UI app since we're unbundled
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];


	unused(title);

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

	memory::deallocate(ctx);
}

bool
platform_window_poll(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	unused(ctx);

	for (i32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		self->input.keys[i].pressed       = false;
		self->input.keys[i].released      = false;
		self->input.keys[i].press_count   = 0;
		self->input.keys[i].release_count = 0;
	}
	self->input.mouse_wheel = 0.0f;

	// while (xcb_generic_event_t *xcb_event = ::xcb_poll_for_event(ctx->connection))
	// {
	// 	switch (xcb_event->response_type & ~0x80)
	// 	{
	// 		case XCB_CLIENT_MESSAGE:
	// 		{
	// 			xcb_client_message_event_t *xcb_client_message = (xcb_client_message_event_t *)xcb_event;
	// 			if (xcb_client_message->data.data32[0] == ctx->wm_delete_window_atom)
	// 				return false;
	// 			break;
	// 		}
	// 		case XCB_BUTTON_PRESS:
	// 		{
	// 			xcb_button_press_event_t *xcb_mouse_press_event = (xcb_button_press_event_t *)xcb_event;

	// 			PLATFORM_KEY key = _platform_key_from_xcb_button(xcb_mouse_press_event->detail);
	// 			if (key != PLATFORM_KEY_COUNT)
	// 			{
	// 				self->input.keys[key].pressed = true;
	// 				self->input.keys[key].down    = true;
	// 				self->input.keys[key].press_count++;
	// 				if (key == PLATFORM_KEY_MOUSE_WHEEL_UP)
	// 					self->input.mouse_wheel += 1.0f;
	// 				else if (key == PLATFORM_KEY_MOUSE_WHEEL_DOWN)
	// 					self->input.mouse_wheel -= 1.0f;
	// 			}
	// 			break;
	// 		}
	// 		case XCB_BUTTON_RELEASE:
	// 		{
	// 			xcb_button_release_event_t *xcb_mouse_release_event = (xcb_button_release_event_t *)xcb_event;

	// 			PLATFORM_KEY key = _platform_key_from_xcb_button(xcb_mouse_release_event->detail);
	// 			if (key != PLATFORM_KEY_COUNT)
	// 			{
	// 				self->input.keys[key].released = true;
	// 				self->input.keys[key].down     = false;
	// 				self->input.keys[key].release_count++;
	// 			}
	// 			break;
	// 		}
	// 		case XCB_KEY_PRESS:
	// 		{
	// 			xcb_key_press_event_t *xcb_key_press_event = (xcb_key_press_event_t *)xcb_event;
	// 			KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_press_event->detail, 0, 0);

	// 			PLATFORM_KEY key = _platform_key_from_key_sym(key_sym);
	// 			if (key != PLATFORM_KEY_COUNT)
	// 			{
	// 				self->input.keys[key].pressed  = true;
	// 				self->input.keys[key].down     = true;
	// 				self->input.keys[key].press_count++;
	// 			}
	// 			break;
	// 		}
	// 		case XCB_KEY_RELEASE:
	// 		{
	// 			xcb_key_release_event_t *xcb_key_release_event = (xcb_key_release_event_t *)xcb_event;
	// 			KeySym key_sym = ::XkbKeycodeToKeysym(ctx->display, (KeyCode)xcb_key_release_event->detail, 0, 0);

	// 			PLATFORM_KEY key = _platform_key_from_key_sym(key_sym);
	// 			if (key != PLATFORM_KEY_COUNT)
	// 			{
	// 				self->input.keys[key].released = true;
	// 				self->input.keys[key].down     = false;
	// 				self->input.keys[key].release_count++;
	// 			}
	// 			break;
	// 		}
	// 		case XCB_CONFIGURE_NOTIFY:
	// 		{
	// 			xcb_configure_notify_event_t *xcb_configure_notify_event = (xcb_configure_notify_event_t *)xcb_event;
	// 			if (self->width != (u32)xcb_configure_notify_event->width || self->height != (u32)xcb_configure_notify_event->height)
	// 			{
	// 				self->width  = xcb_configure_notify_event->width;
	// 				self->height = xcb_configure_notify_event->height;
	// 			}
	// 			break;
	// 		}
	// 		default:
	// 			break;
	// 	}

	// 	::free(xcb_event);
	// }

	// {
	// 	// NOTE: Mouse movement.
	// 	xcb_query_pointer_cookie_t xcb_query_pointer_cookie = ::xcb_query_pointer_unchecked(ctx->connection, ctx->window);
	// 	xcb_query_pointer_reply_t *xcb_query_pointer_reply  = ::xcb_query_pointer_reply(ctx->connection, xcb_query_pointer_cookie, nullptr);

	// 	i32 window_mouse_x = xcb_query_pointer_reply->win_x;
	// 	i32 window_mouse_y = xcb_query_pointer_reply->win_y;
	// 	if (window_mouse_x >= 0 && (u32)window_mouse_x < self->width && window_mouse_y >= 0 && (u32)window_mouse_y < self->height)
	// 	{
	// 		if (window_mouse_x != self->input.mouse_x || window_mouse_y != self->input.mouse_y)
	// 		{
	// 			// NOTE: We want mouse coords to start bottom-left.
	// 			u32 mouse_point_y_inverted = (self->height - 1) - window_mouse_y;
	// 			self->input.mouse_dx = window_mouse_x - self->input.mouse_x;
	// 			self->input.mouse_dy = self->input.mouse_y - mouse_point_y_inverted;
	// 			self->input.mouse_x  = window_mouse_x;
	// 			self->input.mouse_y  = mouse_point_y_inverted;
	// 		}
	// 	}

	// 	::free(xcb_query_pointer_reply);
	// }

	return true;
}

// TODO: Rename native_connection?
void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_connection)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	unused(ctx, native_handle, native_connection);
	// if (native_handle)
	// 	*native_handle = &ctx->window;
	// if (native_connection)
	// 	*native_connection = ctx->connection;
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	unused(ctx, title);
}

// TODO: Do we need this?
void
platform_window_close(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;
	unused(ctx);
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
	::strcpy(current_executable_directory, module_path_absolute);
}

bool
platform_file_exists(const char *filepath)
{
	struct stat file_stat = {};
	return ::stat(filepath, &file_stat) == 0;
}

u64
platform_file_size(const char *filepath)
{
	struct stat file_stat = {};
	if (::stat(filepath, &file_stat) == 0)
		return file_stat.st_size;
	return 0;
}

u64
platform_file_read(const char *filepath, Platform_Memory mem)
{
	i32 file_handle = ::open(filepath, O_RDONLY, S_IRWXU);
	if (file_handle == -1)
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
	if (file_handle == -1)
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
	unused(filters);
	return false;
}

bool
platform_file_dialog_save(char *path, u32 path_length, const char *filters)
{
	::memset(path, 0, path_length);
	unused(filters);
	return false;
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
		// TODO: Use logger.
		::printf("callstack:\n");
		for (u32 i = 0; i < frame_count; ++i)
			::printf("\t[%" PRIu32 "]: %s\n", frame_count - i - 1, symbols[i]);

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