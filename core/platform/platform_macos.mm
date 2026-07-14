#include "core/platform/platform.h"

#include "core/validate.h"
#include "core/defer.h"
#include "core/formatter.h"
#include "core/math/u64.h"
#include "core/memory/allocator.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <mach-o/dyld.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <dirent.h>

static char current_executable_directory[PATH_MAX] = {};

@class Content_View;
@class Window_Delegate;

struct Platform_Window_Context
{
	NSWindow *window;
	Content_View *content_view;
	Window_Delegate *window_delegate;
	Platform_Input *input;
	Platform_Text_Input_Desc text_input;
	String text_input_text;
	IOPMAssertionID power_assertion;
	bool fullscreen;
	bool edge_to_edge;
	bool power_assertion_active;
	bool should_quit;
};

inline static Platform_Window_Orientation
_platform_macos_window_orientation(U32 width, U32 height)
{
	if (width == 0 || height == 0)
		return PLATFORM_WINDOW_ORIENTATION_UNKNOWN;
	return width >= height ? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE : PLATFORM_WINDOW_ORIENTATION_PORTRAIT;
}

inline static Platform_Window_Metrics
_platform_macos_window_metrics(Platform_Window_Context *ctx, U32 width, U32 height)
{
	F32 density_scale = 1.0f;
	Platform_Window_Insets safe_area = {};
	@autoreleasepool
	{
		if (ctx && ctx->content_view)
		{
			NSEdgeInsets insets = [ctx->content_view safeAreaInsets];
			safe_area.left = insets.left > 0.0 ? (U32)insets.left : 0;
			safe_area.top = insets.top > 0.0 ? (U32)insets.top : 0;
			safe_area.right = insets.right > 0.0 ? (U32)insets.right : 0;
			safe_area.bottom = insets.bottom > 0.0 ? (U32)insets.bottom : 0;
		}

		NSScreen *screen = ctx && ctx->window ? [ctx->window screen] : nil;
		if (screen == nil)
			screen = [NSScreen mainScreen];
		if (screen)
			density_scale = (F32)[screen backingScaleFactor];
	}

	U32 content_width = width > safe_area.left + safe_area.right ? width - safe_area.left - safe_area.right : 0;
	U32 content_height = height > safe_area.top + safe_area.bottom ? height - safe_area.top - safe_area.bottom : 0;
	return Platform_Window_Metrics {
		.content_rect = Platform_Window_Rect { .x = (I32)safe_area.left, .y = (I32)safe_area.bottom, .width = content_width, .height = content_height },
		.safe_area = safe_area,
		.density_scale = density_scale,
		.dpi_x = 72.0f * density_scale,
		.dpi_y = 72.0f * density_scale,
		.orientation = _platform_macos_window_orientation(width, height)
	};
}

inline static void
_platform_macos_window_keep_screen_on_set(Platform_Window_Context *ctx, bool enabled)
{
	if (ctx->power_assertion_active == enabled)
		return;

	if (enabled)
	{
		CFStringRef reason = CFSTR("Core window requested keep screen on");
		IOReturn result = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reason, &ctx->power_assertion);
		if (result == kIOReturnSuccess)
			ctx->power_assertion_active = true;
	}
	else
	{
		IOPMAssertionRelease(ctx->power_assertion);
		ctx->power_assertion = 0;
		ctx->power_assertion_active = false;
	}
}

inline static void
_platform_macos_window_fullscreen_set(Platform_Window_Context *ctx, bool enabled)
{
	if (ctx->fullscreen == enabled)
		return;

	@autoreleasepool
	{
		[ctx->window toggleFullScreen:nil];
	}
	ctx->fullscreen = enabled;
}

inline static void
_platform_macos_window_edge_to_edge_set(Platform_Window_Context *ctx, bool enabled)
{
	if (ctx->edge_to_edge == enabled)
		return;

	@autoreleasepool
	{
		NSWindowStyleMask style = [ctx->window styleMask];
		if (enabled)
		{
			[ctx->window setStyleMask:style | NSWindowStyleMaskFullSizeContentView];
			[ctx->window setTitleVisibility:NSWindowTitleHidden];
			[ctx->window setTitlebarAppearsTransparent:YES];
		}
		else
		{
			[ctx->window setStyleMask:style & ~NSWindowStyleMaskFullSizeContentView];
			[ctx->window setTitleVisibility:NSWindowTitleVisible];
			[ctx->window setTitlebarAppearsTransparent:NO];
		}
	}
	ctx->edge_to_edge = enabled;
}

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
_platform_key_from_button_number(I32 button_number)
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
_platform_key_from_key_code(I32 key_code)
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

inline static void
_platform_macos_text_input_events_reset(Platform_Input &input)
{
	for (U64 i = 0; i < input.text_input_events.count; ++i)
	{
		string_deinit(input.text_input_events[i].text);
		input.text_input_events[i] = {};
	}
	input.text_input_events.count = 0;
}

inline static NSString *
_platform_macos_text_input_string(Platform_Window_Context *ctx)
{
	if (ctx->text_input_text.count == 0)
		return @"";
	NSString *text = [NSString stringWithUTF8String:ctx->text_input_text.data];
	return text ? text : @"";
}

inline static NSUInteger
_platform_macos_utf16_index_from_utf8_offset(const String &text, U32 offset)
{
	U64 byte_count = offset > text.count ? text.count : offset;
	if (byte_count == 0 || text.data == nullptr)
		return 0;
	String prefix = string_from(text.data, text.data + byte_count, memory::temp_allocator());
	NSString *prefix_string = [NSString stringWithUTF8String:prefix.data];
	return prefix_string ? [prefix_string length] : 0;
}

inline static NSRange
_platform_macos_text_input_range_from_utf8_offsets(Platform_Window_Context *ctx, U32 start, U32 end)
{
	NSUInteger location = _platform_macos_utf16_index_from_utf8_offset(ctx->text_input_text, start);
	NSUInteger limit = _platform_macos_utf16_index_from_utf8_offset(ctx->text_input_text, end);
	if (limit < location)
	{
		NSUInteger swap = location;
		location = limit;
		limit = swap;
	}
	return NSMakeRange(location, limit - location);
}

@interface Content_View : NSView<NSTextInputClient>
{
	Platform_Window_Context *ctx;
}

- (instancetype)
init:(Platform_Window_Context *)ctx;
@end

@implementation Content_View
- (instancetype)
init:(Platform_Window_Context *)ictx
{
	self = [super init];
	if (self != nil)
	{
		ctx = ictx;
	}
	return self;
}

- (BOOL)
canBecomeKeyView
{
	return YES;
}

- (BOOL)
acceptsFirstResponder
{
	return YES;
}

- (BOOL)
wantsUpdateLayer
{
	return YES;
}

- (BOOL)
acceptsFirstMouse:(NSEvent *)event
{
	return YES;
}

- (void)
insertText:(id)string replacementRange:(NSRange)replacementRange
{
	unused(replacementRange);
	if (ctx->input == nullptr || !ctx->text_input.enabled)
		return;

	NSString *text = [string isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString *)string string] : (NSString *)string;
	const char *utf8 = [text UTF8String];
	if (utf8)
	{
		array_push(ctx->input->text_input_events, Platform_Text_Input_Event {
			.type = PLATFORM_TEXT_INPUT_EVENT_COMMIT,
			.text = string_from(utf8, utf8 + ::strlen(utf8))
		});
	}
}

- (void)
setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
	unused(selectedRange, replacementRange);
	if (ctx->input == nullptr || !ctx->text_input.enabled)
		return;

	NSString *text = [string isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString *)string string] : (NSString *)string;
	const char *utf8 = [text UTF8String];
	if (utf8)
	{
		array_push(ctx->input->text_input_events, Platform_Text_Input_Event {
			.type = PLATFORM_TEXT_INPUT_EVENT_COMPOSE,
			.text = string_from(utf8, utf8 + ::strlen(utf8))
		});
	}
}

- (void)
unmarkText
{
	if (ctx->input && ctx->text_input.enabled)
	{
		array_push(ctx->input->text_input_events, Platform_Text_Input_Event {
			.type = PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END
		});
	}
}

- (NSRange)
selectedRange
{
	if (!ctx->text_input.enabled)
		return {NSNotFound, 0};
	return _platform_macos_text_input_range_from_utf8_offsets(ctx, ctx->text_input.selection_start, ctx->text_input.selection_end);
}

- (NSRange)
markedRange
{
	if (!ctx->text_input.enabled || ctx->text_input.composing_start == ctx->text_input.composing_end)
		return {NSNotFound, 0};
	return _platform_macos_text_input_range_from_utf8_offsets(ctx, ctx->text_input.composing_start, ctx->text_input.composing_end);
}

- (BOOL)
hasMarkedText
{
	return ctx->text_input.enabled && ctx->text_input.composing_start != ctx->text_input.composing_end;
}

- (nullable NSAttributedString *)
attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	NSString *text = _platform_macos_text_input_string(ctx);
	if (range.location == NSNotFound || range.location >= [text length])
		return nil;

	NSUInteger end = range.location + range.length;
	if (end > [text length])
		end = [text length];

	NSRange clamped_range = NSMakeRange(range.location, end - range.location);
	if (actualRange)
		*actualRange = clamped_range;
	return [[NSAttributedString alloc] initWithString:[text substringWithRange:clamped_range]];
}

- (NSArray<NSAttributedStringKey> *)
validAttributesForMarkedText
{
	return [NSArray array];
}

- (NSRect)
firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	unused(range);
	if (actualRange)
		*actualRange = NSMakeRange(0, 0);
	if (!ctx->text_input.enabled)
		return NSMakeRect(0, 0, 0, 0);

	NSRect rect = NSMakeRect(ctx->text_input.x, ctx->text_input.y, ctx->text_input.width, ctx->text_input.height);
	return [[self window] convertRectToScreen:[self convertRect:rect toView:nil]];
}

- (NSUInteger)
characterIndexForPoint:(NSPoint)point
{
	unused(point);
	if (!ctx->text_input.enabled)
		return 0;
	return _platform_macos_utf16_index_from_utf8_offset(ctx->text_input_text, ctx->text_input.selection_end);
}
@end

@interface Window_Delegate : NSObject<NSWindowDelegate>
{
	Platform_Window_Context *ctx;
}

- (instancetype)init:(Platform_Window_Context *)ictx;
@end

@implementation Window_Delegate
- (instancetype)
init:(Platform_Window_Context *)ictx
{
	self = [super init];
	if (self != nil)
	{
		ctx = ictx;
	}
	return self;
}

- (BOOL)
windowShouldClose:(id)sender
{
	ctx->should_quit = true;
	return YES;
}

- (void)
windowDidResize:(NSNotification *)notification
{

}

- (NSSize)
windowWillResize:(NSWindow *)sender toSize:(NSSize)size
{
	return size;
}

- (void)
windowDidMove:(NSNotification *)notification
{

}

- (void)
windowDidMiniaturize:(NSNotification *)notification
{

}

- (void)
windowDidDeminiaturize:(NSNotification *)notification
{

}

- (void)
windowDidBecomeKey:(NSNotification *)notification
{

}
@end

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
platform_path_get_app_data_directory(memory::Allocator *allocator)
{
	String result = string_init(allocator);
	@autoreleasepool
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
		if ([paths count] > 0)
		{
			string_deinit(result);
			result = string_from([[paths objectAtIndex:0] UTF8String], allocator);
		}
	}

	if (result.count == 0)
	{
		string_deinit(result);
		return platform_path_get_temp_directory(allocator);
	}

	if (result[result.count - 1] != '/')
		string_append(result, '/');
	return result;
}

String
platform_path_get_cache_directory(memory::Allocator *allocator)
{
	String result = string_init(allocator);
	@autoreleasepool
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		if ([paths count] > 0)
		{
			string_deinit(result);
			result = string_from([[paths objectAtIndex:0] UTF8String], allocator);
		}
	}

	if (result.count == 0)
	{
		string_deinit(result);
		return platform_path_get_temp_directory(allocator);
	}

	if (result[result.count - 1] != '/')
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
	U32 module_path_relative_size = sizeof(module_path_relative);
	::memset(module_path_relative, 0, module_path_relative_size);

	char module_path_absolute[PATH_MAX + 1];
	::memset(module_path_absolute, 0, sizeof(module_path_absolute));

	I64 module_path_relative_length = ::_NSGetExecutablePath(module_path_relative, &module_path_relative_size);
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

inline static bool
_platform_macos_extension_matches(const String &file_name, const String &extension_filter)
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
_platform_macos_path_join(const String &directory, const String &name, memory::Allocator *allocator)
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
_platform_macos_path_parent(const String &path, memory::Allocator *allocator)
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

	String directory_temp = string_copy(directory, memory::temp_allocator());
	string_replace(directory_temp, "\\", "/");

	if (!platform_path_is_directory(directory_temp))
		return files;

	DIR *dir = ::opendir(directory_temp.data);
	if (!dir)
		return files;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][MACOS]: Failed to close directory."););

	struct dirent *entry = nullptr;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		if (entry->d_type == DT_DIR)
			continue;

		String file_name = string_from(entry->d_name, memory::temp_allocator());
		if (!_platform_macos_extension_matches(file_name, extension_filter))
			continue;

		array_push(files, string_copy(file_name, allocator));
	}

	return files;
}

inline static void
_platform_macos_path_list_files_recursive(Array<String> &files, const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	DIR *dir = ::opendir(directory.data);
	if (!dir)
		return;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][MACOS]: Failed to close recursive directory."););

	struct dirent *entry = nullptr;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		String file_name = string_from(entry->d_name, memory::temp_allocator());
		if (file_name == "." || file_name == "..")
			continue;

		String child_path = _platform_macos_path_join(directory, file_name, memory::temp_allocator());
		if (platform_path_is_directory(child_path))
		{
			_platform_macos_path_list_files_recursive(files, child_path, extension_filter, allocator);
			continue;
		}

		if (_platform_macos_extension_matches(file_name, extension_filter))
			array_push(files, string_copy(child_path, allocator));
	}
}

Array<String>
platform_path_list_files_recursive(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);
	_platform_macos_path_list_files_recursive(files, directory, extension_filter, allocator);
	return files;
}

String
platform_path_create_file(const String &directory, const String &name, memory::Allocator *allocator)
{
	String result = _platform_macos_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	I32 file_handle = ::open(result.data, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
	if (file_handle == -1)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	validate(::close(file_handle) == 0, "[PLATFORM][MACOS]: Failed to close created file.");
	return result;
}

String
platform_path_create_directory(const String &directory, const String &name, memory::Allocator *allocator)
{
	String result = _platform_macos_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (::mkdir(result.data, S_IRWXU) != 0)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

String
platform_path_rename(const String &path, const String &name, memory::Allocator *allocator)
{
	String directory = _platform_macos_path_parent(path, memory::temp_allocator());
	String result = _platform_macos_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (::rename(path.data, result.data) != 0)
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
	String result = _platform_macos_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (::rename(path.data, result.data) == 0)
		return result;

	if (errno == EXDEV && platform_path_is_file(path) && platform_file_copy(path.data, result.data) && platform_file_delete(path.data))
		return result;

	if (platform_path_is_file(result))
		platform_file_delete(result.data);
	string_deinit(result);
	return string_init(allocator);
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
	_string_concat(filepath, ".dylib", src_relative_path);

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

	// platform_sleep(100);

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
	return Memory_Block{data, aligned_size};
}

bool
platform_virtual_memory_commit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][MACOS]: Cannot commit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][MACOS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][MACOS]: Virtual memory block size is not page-aligned.");

	return ::mprotect(block.data, block.size, PROT_READ | PROT_WRITE) == 0;
}

bool
platform_virtual_memory_decommit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][MACOS]: Cannot decommit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][MACOS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][MACOS]: Virtual memory block size is not page-aligned.");

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
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][MACOS]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][MACOS]: Virtual memory block size is not page-aligned.");

	[[maybe_unused]] I32 result = ::munmap(block.data, block.size);
	validate(result == 0, "[PLATFORM][MACOS]: Failed to release virtual memory.");
}

U32
platform_get_logical_processor_count()
{
	long count = ::sysconf(_SC_NPROCESSORS_ONLN);
	return count > 0 ? (U32)count : U32(1);
}

struct Platform_Thread
{
	pthread_t handle;
	Platform_Thread_Function function;
	void *data;
	String name;
	bool joined;
};

static void *
_platform_thread_main_routine(void *thread)
{
	Platform_Thread *self = (Platform_Thread *)thread;
	if (self->name.count != 0)
		platform_thread_set_current_name(self->name.data);
	self->function(self->data);
	return nullptr;
}

Platform_Thread *
platform_thread_init(Platform_Thread_Desc desc)
{
	validate(desc.function != nullptr, "[PLATFORM][MACOS]: Thread function is not valid.");

	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->function = desc.function;
	self->data = desc.data;
	if (desc.name != nullptr)
		self->name = string_from(desc.name);
	validate(::pthread_create(&self->handle, nullptr, _platform_thread_main_routine, self) == 0, "[PLATFORM][MACOS]: Failed to create thread.");
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

	validate(::pthread_join(self->handle, nullptr) == 0, "[PLATFORM][MACOS]: Failed to join thread.");
	self->joined = true;
}

void
platform_thread_sleep(U32 milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	::nanosleep(&ts, 0);
}

void
platform_thread_set_current_name(const char *name)
{
	if (name == nullptr || name[0] == '\0')
		return;

	validate(::pthread_setname_np(name) == 0, "[PLATFORM][MACOS]: Failed to set thread name.");
}

struct Platform_Mutex
{
	pthread_mutex_t handle;
};

Platform_Mutex *
platform_mutex_init()
{
	Platform_Mutex *self = memory::allocate_zeroed<Platform_Mutex>();
	validate(::pthread_mutex_init(&self->handle, nullptr) == 0, "[PLATFORM][MACOS]: Failed to initialize mutex.");
	return self;
}

void
platform_mutex_deinit(Platform_Mutex *self)
{
	validate(::pthread_mutex_destroy(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to destroy mutex.");
	memory::deallocate(self);
}

void
platform_mutex_lock(Platform_Mutex *self)
{
	validate(::pthread_mutex_lock(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to lock mutex.");
}

void
platform_mutex_unlock(Platform_Mutex *self)
{
	validate(::pthread_mutex_unlock(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to unlock mutex.");
}

struct Platform_Condition_Variable
{
	pthread_cond_t handle;
};

Platform_Condition_Variable *
platform_condition_variable_init()
{
	Platform_Condition_Variable *self = memory::allocate_zeroed<Platform_Condition_Variable>();
	validate(::pthread_cond_init(&self->handle, nullptr) == 0, "[PLATFORM][MACOS]: Failed to initialize condition variable.");
	return self;
}

void
platform_condition_variable_deinit(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_destroy(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to destroy condition variable.");
	memory::deallocate(self);
}

void
platform_condition_variable_wait(Platform_Condition_Variable *self, Platform_Mutex *mutex)
{
	validate(::pthread_cond_wait(&self->handle, &mutex->handle) == 0, "[PLATFORM][MACOS]: Failed to wait for condition variable.");
}

void
platform_condition_variable_signal(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_signal(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to signal condition variable.");
}

void
platform_condition_variable_broadcast(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_broadcast(&self->handle) == 0, "[PLATFORM][MACOS]: Failed to broadcast condition variable.");
}

struct Platform_Semaphore
{
	pthread_mutex_t mutex;
	pthread_cond_t condition_variable;
	U32 count;
};

Platform_Semaphore *
platform_semaphore_init(U32 initial_count)
{
	Platform_Semaphore *self = memory::allocate_zeroed<Platform_Semaphore>();
	validate(::pthread_mutex_init(&self->mutex, nullptr) == 0, "[PLATFORM][MACOS]: Failed to initialize semaphore mutex.");
	validate(::pthread_cond_init(&self->condition_variable, nullptr) == 0, "[PLATFORM][MACOS]: Failed to initialize semaphore condition variable.");
	self->count = initial_count;
	return self;
}

void
platform_semaphore_deinit(Platform_Semaphore *self)
{
	validate(::pthread_cond_destroy(&self->condition_variable) == 0, "[PLATFORM][MACOS]: Failed to destroy semaphore condition variable.");
	validate(::pthread_mutex_destroy(&self->mutex) == 0, "[PLATFORM][MACOS]: Failed to destroy semaphore mutex.");
	memory::deallocate(self);
}

void
platform_semaphore_wait(Platform_Semaphore *self)
{
	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][MACOS]: Failed to lock semaphore mutex.");
	while (self->count == 0)
		validate(::pthread_cond_wait(&self->condition_variable, &self->mutex) == 0, "[PLATFORM][MACOS]: Failed to wait for semaphore condition variable.");
	--self->count;
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][MACOS]: Failed to unlock semaphore mutex.");
}

void
platform_semaphore_signal(Platform_Semaphore *self, U32 count)
{
	if (count == 0)
		return;

	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][MACOS]: Failed to lock semaphore mutex.");
	self->count += count;
	if (count == 1)
		validate(::pthread_cond_signal(&self->condition_variable) == 0, "[PLATFORM][MACOS]: Failed to signal semaphore condition variable.");
	else
		validate(::pthread_cond_broadcast(&self->condition_variable) == 0, "[PLATFORM][MACOS]: Failed to broadcast semaphore condition variable.");
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][MACOS]: Failed to unlock semaphore mutex.");
}

// TODO: Return early with error message if failed to create objects.
Platform_Window
platform_window_init(U32 width, U32 height, const char *title)
{
	Platform_Window_Context *ctx = memory::allocate_zeroed<Platform_Window_Context>();

	@autoreleasepool
	{
		NSRect rect = NSMakeRect(0, 0, width, height);
		NSWindow *window = [[NSWindow alloc] initWithContentRect:rect
							styleMask:NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
							backing:NSBackingStoreBuffered
							defer:NO];
		[window makeKeyAndOrderFront:window];
		validate(window, "[PLATFORM][MACOS]: Failed to create window.");
		ctx->window = window;

		Content_View *content_view = [[Content_View alloc] init:ctx];
		validate(content_view, "[PLATFORM][MACOS]: Failed to create content view.");
		[content_view setWantsLayer:YES];

		Window_Delegate *window_delegate = [[Window_Delegate alloc] init:ctx];
		validate(window_delegate, "[PLATFORM][MACOS]: Failed to create window delegate.");

		[window setLevel:NSNormalWindowLevel];
		[window setContentView:content_view];
		[window makeFirstResponder:content_view];
		[window setTitle:@(title)];
		[window setDelegate:window_delegate];
		[window setAcceptsMouseMovedEvents:YES];
		[window setRestorable:NO];

		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];

		ctx->content_view    = content_view;
		ctx->window_delegate = window_delegate;
	}

	Platform_Window self {
		.handle = ctx,
		.width  = width,
		.height = height,
		.metrics = _platform_macos_window_metrics(ctx, width, height),
		.input  = {},
		.focused = true,
		.started = true,
		.surface_valid = true,
		.surface_changed = true
	};
	self.input.text_input_events = array_init<Platform_Text_Input_Event>(memory::heap_allocator());
	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	@autoreleasepool
	{
		[ctx->window orderOut:nil];
		[ctx->window setDelegate:nil];
		_platform_macos_window_keep_screen_on_set(ctx, false);
		[ctx->content_view release];
		[ctx->window close];
		[ctx->window_delegate release];
		[NSApp setDelegate:nil];
	}

	if (ctx->text_input_text.capacity > 0)
		string_deinit(ctx->text_input_text);
	memory::deallocate(ctx);
	self->handle = nullptr;
	self->metrics = {};
	self->presentation = {};
	self->close_requested = true;
	self->started = false;
	self->surface_valid = false;
	_platform_macos_text_input_events_reset(self->input);
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
	_platform_macos_text_input_events_reset(self->input);

	@autoreleasepool
	{
		ctx->input = &self->input;
		DEFER(ctx->input = nullptr;);
		while (NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])
		{
			switch (event.type)
			{
				case NSLeftMouseDown:
				case NSRightMouseDown:
				case NSOtherMouseDown:
				{
					PLATFORM_KEY key = _platform_key_from_button_number(event.buttonNumber);
					if (key != PLATFORM_KEY_COUNT)
					{
						self->input.keys[key].pressed = true;
						self->input.keys[key].down    = true;
						self->input.keys[key].press_count++;
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

		// NOTE: Window size.
		NSSize window_size = ctx->content_view.frame.size;
		U32 width = (U32)window_size.width;
		U32 height = (U32)window_size.height;
		if (self->width != width || self->height != height)
		{
			self->width = width;
			self->height = height;
			surface_changed = true;
		}
		self->metrics = _platform_macos_window_metrics(ctx, self->width, self->height);

		// NOTE: Mouse movement.
		NSPoint mouse_position = [ctx->content_view convertPoint:[ctx->window convertScreenToBase:[NSEvent mouseLocation]] fromView:nil];
		self->input.mouse_dx = mouse_position.x - self->input.mouse_x;
		self->input.mouse_dy = mouse_position.y - self->input.mouse_y;
		self->input.mouse_x  = mouse_position.x;
		self->input.mouse_y  = mouse_position.y;

		self->focused = [ctx->window isKeyWindow];
	}

	self->close_requested = ctx->should_quit;
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
		.context = ctx->content_view
	};
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	[ctx->window setTitle:@(title)];
}

void
platform_window_close(Platform_Window *self)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)self->handle;

	self->close_requested = true;
	self->surface_valid = false;
	[ctx->window performClose:ctx->window];
}

void
platform_window_presentation_set(Platform_Window &window, const Platform_Window_Presentation_Desc &desc)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	bool fullscreen = (desc.flags & (PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN | PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE)) != 0;
	_platform_macos_window_fullscreen_set(ctx, fullscreen);
	_platform_macos_window_edge_to_edge_set(ctx, (desc.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE) != 0);
	_platform_macos_window_keep_screen_on_set(ctx, (desc.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0);
	window.presentation = desc;
	window.surface_changed = true;
}

void
platform_window_text_input_set(Platform_Window &window, const Platform_Text_Input_Desc &desc)
{
	Platform_Window_Context *ctx = (Platform_Window_Context *)window.handle;
	Platform_Text_Input_Desc text_input_desc = desc;
	window.text_input = text_input_desc;
	window.text_input.text = {};
	if (ctx->text_input_text.capacity > 0)
		string_deinit(ctx->text_input_text);
	ctx->text_input = text_input_desc;
	ctx->text_input.text = {};
	ctx->text_input_text = text_input_desc.enabled && text_input_desc.text.count > 0 ? string_copy(text_input_desc.text) : String {};
	if (text_input_desc.enabled)
		[ctx->window makeFirstResponder:ctx->content_view];
}

void
platform_set_current_directory()
{
	char module_path_relative[PATH_MAX + 1];
	U32 module_path_relative_size = sizeof(module_path_relative);
	::memset(module_path_relative, 0, module_path_relative_size);

	char module_path_absolute[PATH_MAX + 1];
	::memset(module_path_absolute, 0, sizeof(module_path_absolute));

	[[maybe_unused]] I64 module_path_relative_length = ::_NSGetExecutablePath(module_path_relative, &module_path_relative_size);
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

String
platform_file_dialog_open(const char *filters, memory::Allocator *allocator)
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		[application setActivationPolicy:NSApplicationActivationPolicyAccessory];

		// TODO: Fix filters.
		NSOpenPanel *open_panel = [NSOpenPanel openPanel];
		open_panel.allowsMultipleSelection = false;
		open_panel.canChooseDirectories    = false;
		open_panel.canChooseFiles          = true;
		open_panel.allowedContentTypes     = [UTType typesWithTag:@(filters) tagClass:UTTagClassFilenameExtension conformingToType:nil];

		if ([open_panel runModal] == NSModalResponseOK)
		{
			NSURL *url = [[open_panel URLs] objectAtIndex:0];
			return string_from([url.path UTF8String], allocator);
		}
	}

	return string_init(allocator);
}

String
platform_file_dialog_save(const char *filters, memory::Allocator *allocator)
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		[application setActivationPolicy:NSApplicationActivationPolicyAccessory];

		// TODO: Fix filters.
		NSSavePanel *save_panel = [NSSavePanel savePanel];
		save_panel.canCreateDirectories = true;
		save_panel.allowedContentTypes  = [UTType typesWithTag:@(filters) tagClass:UTTagClassFilenameExtension conformingToType:nil];

		if ([save_panel runModal] == NSModalResponseOK)
		{
			NSURL *url = [save_panel URL];
			return string_from([url.path UTF8String], allocator);
		}
	}

	return string_init(allocator);
}

String
platform_directory_dialog_open(memory::Allocator *allocator)
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		[application setActivationPolicy:NSApplicationActivationPolicyAccessory];

		NSOpenPanel *open_panel = [NSOpenPanel openPanel];
		open_panel.allowsMultipleSelection = false;
		open_panel.canChooseDirectories    = true;
		open_panel.canChooseFiles          = false;
		open_panel.canCreateDirectories    = true;

		if ([open_panel runModal] == NSModalResponseOK)
		{
			NSURL *url = [[open_panel URLs] objectAtIndex:0];
			return string_from([url.path UTF8String], allocator);
		}
	}

	return string_init(allocator);
}

inline static NSString *
_platform_macos_clipboard_type_from_media_type(const String &media_type)
{
	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || media_type == "text/plain")
		return NSPasteboardTypeString;
	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG)
		return @"public.png";
	return [NSString stringWithUTF8String:(media_type.data ? media_type.data : "")];
}

inline static String
_platform_macos_clipboard_media_type_from_type(NSString *type, memory::Allocator *allocator)
{
	if ([type isEqualToString:NSPasteboardTypeString])
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator);
	if ([type isEqualToString:@"public.png"])
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG, allocator);
	if ([type isEqualToString:@"public.tiff"])
		return string_from("image/tiff", allocator);
	return string_from([type UTF8String], allocator);
}

Array<String>
platform_window_clipboard_query_media_types(Platform_Window &, memory::Allocator *allocator)
{
	Array<String> media_types = array_init<String>(allocator);
	@autoreleasepool
	{
		NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
		for (NSString *type in [pasteboard types])
			array_push(media_types, _platform_macos_clipboard_media_type_from_type(type, allocator));
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

	@autoreleasepool
	{
		NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
		NSString *type = _platform_macos_clipboard_type_from_media_type(media_type);
		if (type == nil)
			return result;

		if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || media_type == "text/plain")
		{
			NSString *text = [pasteboard stringForType:type];
			if (text == nil)
				return result;

			const char *text_data = [text UTF8String];
			string_deinit(result.media_type);
			result.media_type = string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator);
			array_resize(result.data, ::strlen(text_data));
			::memcpy(result.data.data, text_data, result.data.count);
			return result;
		}

		NSData *data = [pasteboard dataForType:type];
		if (data == nil)
			return result;

		string_deinit(result.media_type);
		result.media_type = string_copy(media_type, allocator);
		array_resize(result.data, [data length]);
		::memcpy(result.data.data, [data bytes], result.data.count);
	}

	return result;
}

bool
platform_window_clipboard_item_write(Platform_Window &, const Platform_Clipboard_Item *items, U32 item_count)
{
	if (items == nullptr || item_count == 0)
		return false;

	@autoreleasepool
	{
		NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
		[pasteboard clearContents];

		for (U32 i = 0; i < item_count; ++i)
		{
			NSString *type = _platform_macos_clipboard_type_from_media_type(items[i].media_type);
			if (type == nil)
				return false;

			if (items[i].media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || items[i].media_type == "text/plain")
			{
				NSString *text = [[NSString alloc] initWithBytes:items[i].data.data length:items[i].data.count encoding:NSUTF8StringEncoding];
				if (text == nil)
					return false;
				DEFER([text release];);

				if (![pasteboard setString:text forType:NSPasteboardTypeString])
					return false;
			}
			else
			{
				NSData *data = [NSData dataWithBytes:items[i].data.data length:items[i].data.count];
				if (![pasteboard setData:data forType:type])
					return false;
			}
		}
	}

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
	::nanosleep(&ts, 0);
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