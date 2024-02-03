#include "core/platform/platform.h"

#include "core/assert.h"
#include "core/defer.h"
#include "core/logger.h"
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
#include <Carbon/Carbon.h>

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
_platform_key_from_button_number(i32 button_number)
{
	switch (button_number)
	{
		case 0: return PLATFORM_KEY_MOUSE_LEFT;
		case 1: return PLATFORM_KEY_MOUSE_RIGHT;
		case 2: return PLATFORM_KEY_MOUSE_MIDDLE;
	}
	return PLATFORM_KEY_COUNT;
}

inline static PLATFORM_KEY
_platform_key_from_key_code(i32 key_code)
{
	switch (key_code)
	{
		// TODO:
		// case kVK_ANSI_Quote:
		// case kVK_ANSI_KeypadDecimal:
		// case kVK_ANSI_KeypadMultiply:
		// case kVK_ANSI_KeypadPlus:
		// case kVK_ANSI_KeypadClear:
		// case kVK_ANSI_KeypadDivide:
		// case kVK_ANSI_KeypadEnter:
		// case kVK_ANSI_KeypadMinus:
		// case kVK_ANSI_KeypadEquals:
		// case kVK_Command:
		// case kVK_RightCommand:
		// case kVK_CapsLock:
		case kVK_ANSI_A:            return PLATFORM_KEY_A;
		case kVK_ANSI_B:            return PLATFORM_KEY_B;
		case kVK_ANSI_C:            return PLATFORM_KEY_C;
		case kVK_ANSI_D:            return PLATFORM_KEY_D;
		case kVK_ANSI_E:            return PLATFORM_KEY_E;
		case kVK_ANSI_F:            return PLATFORM_KEY_F;
		case kVK_ANSI_G:            return PLATFORM_KEY_G;
		case kVK_ANSI_H:            return PLATFORM_KEY_H;
		case kVK_ANSI_I:            return PLATFORM_KEY_I;
		case kVK_ANSI_J:            return PLATFORM_KEY_J;
		case kVK_ANSI_K:            return PLATFORM_KEY_K;
		case kVK_ANSI_L:            return PLATFORM_KEY_L;
		case kVK_ANSI_M:            return PLATFORM_KEY_M;
		case kVK_ANSI_N:            return PLATFORM_KEY_N;
		case kVK_ANSI_O:            return PLATFORM_KEY_O;
		case kVK_ANSI_P:            return PLATFORM_KEY_P;
		case kVK_ANSI_Q:            return PLATFORM_KEY_Q;
		case kVK_ANSI_R:            return PLATFORM_KEY_R;
		case kVK_ANSI_S:            return PLATFORM_KEY_S;
		case kVK_ANSI_T:            return PLATFORM_KEY_T;
		case kVK_ANSI_U:            return PLATFORM_KEY_U;
		case kVK_ANSI_V:            return PLATFORM_KEY_V;
		case kVK_ANSI_W:            return PLATFORM_KEY_W;
		case kVK_ANSI_X:            return PLATFORM_KEY_X;
		case kVK_ANSI_Y:            return PLATFORM_KEY_Y;
		case kVK_ANSI_Z:            return PLATFORM_KEY_Z;
		case kVK_ANSI_0:            return PLATFORM_KEY_NUM_0;
		case kVK_ANSI_1:            return PLATFORM_KEY_NUM_1;
		case kVK_ANSI_2:            return PLATFORM_KEY_NUM_2;
		case kVK_ANSI_3:            return PLATFORM_KEY_NUM_3;
		case kVK_ANSI_4:            return PLATFORM_KEY_NUM_4;
		case kVK_ANSI_5:            return PLATFORM_KEY_NUM_5;
		case kVK_ANSI_6:            return PLATFORM_KEY_NUM_6;
		case kVK_ANSI_7:            return PLATFORM_KEY_NUM_7;
		case kVK_ANSI_8:            return PLATFORM_KEY_NUM_8;
		case kVK_ANSI_9:            return PLATFORM_KEY_NUM_9;
		case kVK_ANSI_Keypad0:      return PLATFORM_KEY_NUMPAD_0;
		case kVK_ANSI_Keypad1:      return PLATFORM_KEY_NUMPAD_1;
		case kVK_ANSI_Keypad2:      return PLATFORM_KEY_NUMPAD_2;
		case kVK_ANSI_Keypad3:      return PLATFORM_KEY_NUMPAD_3;
		case kVK_ANSI_Keypad4:      return PLATFORM_KEY_NUMPAD_4;
		case kVK_ANSI_Keypad5:      return PLATFORM_KEY_NUMPAD_5;
		case kVK_ANSI_Keypad6:      return PLATFORM_KEY_NUMPAD_6;
		case kVK_ANSI_Keypad7:      return PLATFORM_KEY_NUMPAD_7;
		case kVK_ANSI_Keypad8:      return PLATFORM_KEY_NUMPAD_8;
		case kVK_ANSI_Keypad9:      return PLATFORM_KEY_NUMPAD_9;
		case kVK_F1:                return PLATFORM_KEY_F1;
		case kVK_F2:                return PLATFORM_KEY_F2;
		case kVK_F3:                return PLATFORM_KEY_F3;
		case kVK_F4:                return PLATFORM_KEY_F4;
		case kVK_F5:                return PLATFORM_KEY_F5;
		case kVK_F6:                return PLATFORM_KEY_F6;
		case kVK_F7:                return PLATFORM_KEY_F7;
		case kVK_F8:                return PLATFORM_KEY_F8;
		case kVK_F9:                return PLATFORM_KEY_F9;
		case kVK_F10:               return PLATFORM_KEY_F10;
		case kVK_F11:               return PLATFORM_KEY_F11;
		case kVK_F12:               return PLATFORM_KEY_F12;
		case kVK_UpArrow:           return PLATFORM_KEY_ARROW_UP;
		case kVK_DownArrow:         return PLATFORM_KEY_ARROW_DOWN;
		case kVK_LeftArrow:         return PLATFORM_KEY_ARROW_LEFT;
		case kVK_RightArrow:        return PLATFORM_KEY_ARROW_RIGHT;
		case kVK_Shift:             return PLATFORM_KEY_SHIFT_LEFT;
		case kVK_RightShift:        return PLATFORM_KEY_SHIFT_RIGHT;
		case kVK_Control:           return PLATFORM_KEY_CONTROL_LEFT;
		case kVK_RightControl:      return PLATFORM_KEY_CONTROL_RIGHT;
		case kVK_Option:            return PLATFORM_KEY_ALT_LEFT;
		case kVK_RightOption:       return PLATFORM_KEY_ALT_RIGHT;
		case kVK_Delete:            return PLATFORM_KEY_BACKSPACE;
		case kVK_Tab:               return PLATFORM_KEY_TAB;
		case kVK_Return:            return PLATFORM_KEY_ENTER;
		case kVK_Escape:            return PLATFORM_KEY_ESCAPE;
		case kVK_ForwardDelete:     return PLATFORM_KEY_DELETE;
		case kVK_Help:              return PLATFORM_KEY_INSERT;
		case kVK_Home:              return PLATFORM_KEY_HOME;
		case kVK_End:               return PLATFORM_KEY_END;
		case kVK_PageUp:            return PLATFORM_KEY_PAGE_UP;
		case kVK_PageDown:          return PLATFORM_KEY_PAGE_DOWN;
		case kVK_ANSI_Slash:        return PLATFORM_KEY_SLASH;
		case kVK_ANSI_Backslash:    return PLATFORM_KEY_BACKSLASH;
		case kVK_ANSI_LeftBracket:  return PLATFORM_KEY_BRACKET_LEFT;
		case kVK_ANSI_RightBracket: return PLATFORM_KEY_BRACKET_RIGHT;
		case kVK_ANSI_Grave:        return PLATFORM_KEY_BACKQUOTE;
		case kVK_ANSI_Period:       return PLATFORM_KEY_PERIOD;
		case kVK_ANSI_Minus:        return PLATFORM_KEY_MINUS;
		case kVK_ANSI_Equal:        return PLATFORM_KEY_EQUAL;
		case kVK_ANSI_Comma:        return PLATFORM_KEY_COMMA;
		case kVK_ANSI_Semicolon:    return PLATFORM_KEY_SEMICOLON;
		case kVK_Space:             return PLATFORM_KEY_SPACE;
	}
	return PLATFORM_KEY_COUNT;
}

@class Content_View;
@class Window_Delegate;

struct Platform_Window_Context
{
	NSWindow *window;
	Content_View *content_view;
	Window_Delegate *window_delegate;
	bool should_quit;
};

@interface Content_View : NSView<NSTextInputClient>
{
	NSWindow *window;
	// NSTrackingArea* trackingArea;
	// NSMutableAttributedString* markedText;
}

- (instancetype)init:(NSWindow *)window;

@end

@implementation Content_View

- (instancetype)
init:(NSWindow *)iwindow
{
	self = [super init];
	if (self != nil)
	{
		window = iwindow;
	}
	return self;
}

- (BOOL)
canBecomeKeyView
{
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

// Handle modifier keys since they are only registered via modifier flags being set/unset.
- (void) flagsChanged:(NSEvent *) event {
	// handle_modifier_keys([event keyCode], [event modifierFlags]);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {}

- (void)unmarkText {}

- (NSRange)
selectedRange
{
	return {NSNotFound, 0};
}

- (NSRange)
markedRange
{
	return {NSNotFound, 0};
}

- (BOOL)
hasMarkedText
{
	return false;
}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return nil;}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {return [NSArray array];}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return NSMakeRect(0, 0, 0, 0);}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {return 0;}

@end

@interface Window_Delegate : NSObject<NSWindowDelegate>
{
	Platform_Window_Context *ctx;
}

- (instancetype)init:(Platform_Window_Context *)ictx;

@end

@implementation Window_Delegate

- (instancetype)init:(Platform_Window_Context *)ictx
{
	self = [super init];
	if (self != nil)
	{
		ctx = ictx;
	}
	return self;
}

- (BOOL)windowShouldClose:(id)sender
{
	ctx->should_quit = true;
	return YES;
}

- (void)windowDidResize:(NSNotification *)notification
{

}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)size
{
	return size;
}

- (void)windowDidMove:(NSNotification *)notification
{

}

- (void)windowDidMiniaturize:(NSNotification *)notification
{

}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{

}

- (void)windowDidBecomeKey:(NSNotification *)notification
{

}

@end

// API.
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

Platform_Window
platform_window_init(u32 width, u32 height, const char *title)
{
	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();

	NSRect rect = NSMakeRect(0, 0, width, height);
	NSWindow *window = [[NSWindow alloc] initWithContentRect:rect
						styleMask:NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
						backing:NSBackingStoreBuffered
						defer:NO];
	[window makeKeyAndOrderFront:window];

	Content_View *content_view = [[Content_View alloc] init:window];
	[content_view setWantsLayer:YES];

	Window_Delegate *window_delegate = [[Window_Delegate alloc] init:ctx];

	[window setLevel:NSNormalWindowLevel];
	[window setContentView:content_view];
	[window makeFirstResponder:content_view];
	[window setTitle:@(title)];
	[window setDelegate:window_delegate];
	[window setAcceptsMouseMovedEvents:YES];
	[window setRestorable:NO];

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp activateIgnoringOtherApps:YES];

	ctx->window          = window;
	ctx->content_view    = content_view;
	ctx->window_delegate = window_delegate;

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

	@autoreleasepool {
		[ctx->window orderOut:nil];
		[ctx->window setDelegate:nil];
		[ctx->content_view release];
		[ctx->window close];
		[ctx->window_delegate release];
		// [ctx->app_delegate release];
		[NSApp setDelegate:nil];
	}

	memory::deallocate(ctx);
}

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

	@autoreleasepool
	{
		while (NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])
		{
			switch (event.type)
			{
				case NSLeftMouseDown:
				case NSRightMouseDown:
				case NSOtherMouseDown:
				{
					PLATFORM_KEY key = _platform_key_from_button_number(event.buttonNumber);
					LOG_INFO("{}", (i32)event.buttonNumber);
					if (key != PLATFORM_KEY_COUNT)
					{
						self->input.keys[key].pressed = true;
						self->input.keys[key].down    = true;
						self->input.keys[key].press_count++;
						LOG_INFO("Mouse {} down.", (i32)key);
					}
					break;
				}
				case NSLeftMouseUp:
				case NSRightMouseUp:
				case NSOtherMouseUp:
				{
					PLATFORM_KEY key = _platform_key_from_button_number(event.buttonNumber);
					if (key != PLATFORM_KEY_COUNT)
					{
						self->input.keys[key].released = true;
						self->input.keys[key].down     = false;
						self->input.keys[key].release_count++;
					}
					break;
				}
				case NSScrollWheel:
				{
					self->input.mouse_wheel += (event.scrollingDeltaY >= 0.0f) ? 1.0f : -1.0f;
					break;
				}
				case NSMouseMoved:
				{
					break;
				}
				case NSKeyDown:
				{
					PLATFORM_KEY key = _platform_key_from_key_code(event.keyCode);
					if (key != PLATFORM_KEY_COUNT)
					{
						self->input.keys[key].pressed  = true;
						self->input.keys[key].down     = true;
						self->input.keys[key].press_count++;
					}
					break;
				}
				case NSKeyUp:
				{
					PLATFORM_KEY key = _platform_key_from_key_code(event.keyCode);
					if (key != PLATFORM_KEY_COUNT)
					{
						self->input.keys[key].released = true;
						self->input.keys[key].down     = false;
						self->input.keys[key].release_count++;
					}
					break;
				}
				default:
				{
					break;
				}
			}

			[NSApp sendEvent:event];
		}

		// NOTE: Mouse movement.
		NSPoint mouse_position = [ctx->content_view convertPoint:[ctx->window convertScreenToBase:[NSEvent mouseLocation]] fromView:nil];
		self->input.mouse_dx = mouse_position.x - self->input.mouse_x;
		self->input.mouse_dy = mouse_position.y - self->input.mouse_y;
		self->input.mouse_x  = mouse_position.x;
		self->input.mouse_y  = mouse_position.y;
	}

	return !ctx->should_quit;
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