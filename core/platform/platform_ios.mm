#include "core/platform/platform.h"

#include "core/validate.h"
#include "core/defer.h"
#include "core/math/u64.h"
#include "core/memory/allocator.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <dispatch/dispatch.h>
#include <Foundation/Foundation.h>
#include <MobileCoreServices/MobileCoreServices.h>
#include <UIKit/UIKit.h>

#if !defined(PLATFORM_IOS) || PLATFORM_IOS != 1
	#error "[PLATFORM][IOS]: platform_ios.mm requires PLATFORM_IOS=1."
#endif

enum PLATFORM_IOS_SCENE_OBSERVER
{
	PLATFORM_IOS_SCENE_OBSERVER_WILL_ENTER_FOREGROUND,
	PLATFORM_IOS_SCENE_OBSERVER_DID_ENTER_BACKGROUND,
	PLATFORM_IOS_SCENE_OBSERVER_DID_BECOME_ACTIVE,
	PLATFORM_IOS_SCENE_OBSERVER_WILL_RESIGN_ACTIVE,
	PLATFORM_IOS_SCENE_OBSERVER_DID_DISCONNECT,
	PLATFORM_IOS_SCENE_OBSERVER_COUNT
};

enum PLATFORM_IOS_FILE_DIALOG_MODE
{
	PLATFORM_IOS_FILE_DIALOG_MODE_OPEN,
	PLATFORM_IOS_FILE_DIALOG_MODE_DIRECTORY,
	PLATFORM_IOS_FILE_DIALOG_MODE_EXPORT
};

struct Platform_Window_Context;
struct Platform_IOS_File_Dialog;
@class Platform_IOS_Text_Input_View;

@interface Platform_IOS_Text_Position : UITextPosition
{
	NSUInteger _offset;
}

@property(nonatomic, readonly) NSUInteger offset;

+ (instancetype)positionWithOffset:(NSUInteger)offset;
- (instancetype)initWithOffset:(NSUInteger)offset;
@end

@interface Platform_IOS_Text_Range : UITextRange
{
	NSUInteger _start_offset;
	NSUInteger _end_offset;
}

@property(nonatomic, readonly) NSUInteger startOffset;
@property(nonatomic, readonly) NSUInteger endOffset;

+ (instancetype)rangeWithStart:(NSUInteger)start end:(NSUInteger)end;
- (instancetype)initWithStart:(NSUInteger)start end:(NSUInteger)end;
@end

@interface Platform_IOS_View : UIView
{
@public
	Platform_Window_Context *_context;
	Platform_IOS_Text_Input_View *_text_input_view;
}

- (instancetype)initWithFrame:(CGRect)frame context:(Platform_Window_Context *)context;
- (void)platformContextClear;
- (void)platformInputResponderActivate;
- (BOOL)platformKeyUsage:(UIKeyboardHIDUsage)usage down:(BOOL)down API_AVAILABLE(ios(13.4));
- (void)platformMousePosition:(CGPoint)location;
- (void)platformMouseButtons:(UIEventButtonMask)button_mask API_AVAILABLE(ios(13.4));
- (void)platformMouseScroll:(CGFloat)delta;
- (void)platformMouseHover:(UIHoverGestureRecognizer *)recognizer API_AVAILABLE(ios(13.4));
- (void)platformMouseScrollGesture:(UIPanGestureRecognizer *)recognizer API_AVAILABLE(ios(13.4));
- (BOOL)platformMouseTouch:(UITouch *)touch event:(UIEvent *)event cancelled:(BOOL)cancelled API_AVAILABLE(ios(13.4));
- (void)platformTouchBegan:(UITouch *)touch location:(CGPoint)location;
- (void)platformTouchMoved:(UITouch *)touch location:(CGPoint)location;
- (void)platformTouchEnded:(UITouch *)touch location:(CGPoint)location;
@end

@interface Platform_IOS_Text_Input_View : UIView<UITextInput>
{
@public
	Platform_Window_Context *_context;
@private
	id<UITextInputDelegate> _input_delegate;
	UITextInputStringTokenizer *_tokenizer;
	NSDictionary *_marked_text_style;
	UIKeyboardType _keyboard_type;
	UIReturnKeyType _return_key_type;
	UITextAutocapitalizationType _autocapitalization_type;
	UITextAutocorrectionType _autocorrection_type;
	UITextSpellCheckingType _spell_checking_type;
	BOOL _secure_text_entry;
}

- (instancetype)initWithContext:(Platform_Window_Context *)context;
- (void)platformContextClear;
- (void)platformTextInputSet:(const Platform_Text_Input_Desc *)desc;
@end

@interface Platform_IOS_View_Controller : UIViewController
{
	U32 _presentation_flags;
	Platform_Window_Orientation_Policy _orientation_policy;
}

- (void)platformPresentationSet:(U32)flags orientationPolicy:(Platform_Window_Orientation_Policy)orientation_policy;
@end

@interface Platform_IOS_Document_Token : NSObject
+ (String)encodeBookmarkData:(NSData *)bookmark_data allocator:(memory::Allocator *)allocator;
+ (String)encodeURL:(NSURL *)url allocator:(memory::Allocator *)allocator;
+ (NSData *)copyBookmarkDataFromToken:(const String &)token;
+ (NSURL *)copyURLFromToken:(const String &)token;
@end

@interface Platform_IOS_File_Dialog_Controller : UIDocumentPickerViewController<UIDocumentPickerDelegate, UIAdaptivePresentationControllerDelegate>
{
	Platform_IOS_File_Dialog *_dialog;
}

+ (NSArray<NSString *> *)documentTypesFromFilters:(const char *)filters;
- (instancetype)initWithDocumentTypes:(NSArray<NSString *> *)document_types exportURL:(NSURL *)export_url dialog:(Platform_IOS_File_Dialog *)dialog;
- (void)platformCompleteWithURL:(NSURL *)url;
@end

struct Platform_IOS_Touch
{
	UITouch *native_touch;
	Platform_Touch_State state;
};

struct Platform_IOS_Mouse
{
	I32 x, y;
	I32 dx, dy;
	F32 wheel;
	bool position_valid;
};

struct Platform_IOS_File
{
	I32 fd;
	NSURL *security_url;
	bool security_access;
};

struct Platform_IOS_Document_Access
{
	NSURL *url;
};

struct Platform_IOS_File_Dialog
{
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	String token;
	bool return_token;
	bool accepted;
	bool completed;
};

struct Platform_Window_Context
{
	pthread_mutex_t mutex;
	UIWindowScene *scene;
	UISceneSession *session;
	UIWindow *window;
	Platform_IOS_View_Controller *view_controller;
	Platform_IOS_View *view;
	id scene_observers[PLATFORM_IOS_SCENE_OBSERVER_COUNT];
	Platform_Key_State keys[PLATFORM_KEY_COUNT];
	Platform_IOS_Touch touches[PLATFORM_TOUCH_MAX_COUNT];
	Platform_IOS_Mouse mouse;
	Array<Platform_Text_Input_Event> text_input_events;
	Platform_Text_Input_Desc text_input;
	String text_input_text;
	bool connected;
	bool close_requested;
	bool started;
	bool paused;
	bool focused;
	bool surface_changed;
	bool keep_screen_on;
};

static U32 _platform_ios_keep_screen_on_count = 0;
static BOOL _platform_ios_idle_timer_disabled = NO;
inline static constexpr const char *PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX = "core-document://v1/";

@implementation Platform_IOS_Document_Token
+ (String)encodeBookmarkData:(NSData *)bookmark_data allocator:(memory::Allocator *)allocator
{
	if (bookmark_data == nil || [bookmark_data length] == 0)
		return string_init(allocator);

	NSString *encoded = [bookmark_data base64EncodedStringWithOptions:0];
	const char *encoded_data = [encoded UTF8String];
	NSUInteger encoded_count = [encoded lengthOfBytesUsingEncoding:NSASCIIStringEncoding];
	String token = string_with_capacity(string_literal(PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX).count + encoded_count + 1, allocator);
	string_append(token, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	for (NSUInteger i = 0; i < encoded_count; ++i)
	{
		char c = encoded_data[i];
		if (c == '=')
			break;
		string_append(token, c == '+' ? '-' : (c == '/' ? '_' : c));
	}
	return token;
}

+ (String)encodeURL:(NSURL *)url allocator:(memory::Allocator *)allocator
{
	if (url == nil || ![url isFileURL])
		return string_init(allocator);

	NSError *error = nil;
	NSData *bookmark_data = [url bookmarkDataWithOptions:NSURLBookmarkCreationMinimalBookmark includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
	if (bookmark_data == nil || error != nil)
		return string_init(allocator);
	return [self encodeBookmarkData:bookmark_data allocator:allocator];
}

+ (NSData *)copyBookmarkDataFromToken:(const String &)token
{
	String prefix = string_literal(PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	if (!string_starts_with(token, prefix) || token.count <= prefix.count)
		return nil;

	U64 payload_count = token.count - prefix.count;
	U64 remainder = payload_count % 4;
	if (remainder == 1)
		return nil;

	U64 padding_count = remainder == 0 ? 0 : 4 - remainder;
	String encoded = string_with_capacity(payload_count + padding_count + 1, memory::temp_allocator());
	for (U64 i = prefix.count; i < token.count; ++i)
	{
		char c = token[i];
		bool valid =
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '-' ||
			c == '_';
		if (!valid)
			return nil;
		string_append(encoded, c == '-' ? '+' : (c == '_' ? '/' : c));
	}
	string_append(encoded, '=', (I32)padding_count);

	NSString *encoded_string = [[[NSString alloc] initWithBytes:encoded.data length:encoded.count encoding:NSASCIIStringEncoding] autorelease];
	if (encoded_string == nil)
		return nil;

	NSData *bookmark_data = [[NSData alloc] initWithBase64EncodedString:encoded_string options:0];
	if (bookmark_data == nil || [bookmark_data length] == 0)
	{
		[bookmark_data release];
		return nil;
	}

	String canonical_token = [self encodeBookmarkData:bookmark_data allocator:memory::temp_allocator()];
	if (canonical_token != token)
	{
		[bookmark_data release];
		return nil;
	}
	return bookmark_data;
}

+ (NSURL *)copyURLFromToken:(const String &)token
{
	NSData *bookmark_data = [self copyBookmarkDataFromToken:token];
	if (bookmark_data == nil)
		return nil;
	DEFER([bookmark_data release]);

	BOOL stale = NO;
	NSURL *url = [[NSURL alloc] initByResolvingBookmarkData:bookmark_data options:0 relativeToURL:nil bookmarkDataIsStale:&stale error:nil];
	if (url == nil || stale || ![url isFileURL])
	{
		[url release];
		return nil;
	}
	return url;
}
@end

inline static void
_platform_ios_key_state_set(Platform_Key_State *state, bool down)
{
	if (down)
	{
		if (!state->down)
		{
			state->pressed = true;
			state->press_count++;
		}
		state->down = true;
	}
	else if (state->down)
	{
		state->released = true;
		state->release_count++;
		state->down = false;
	}
}

inline static PLATFORM_KEY
_platform_ios_key_from_hid_usage(UIKeyboardHIDUsage usage) API_AVAILABLE(ios(13.4))
{
	switch (usage)
	{
		case UIKeyboardHIDUsageKeyboardA:                    return PLATFORM_KEY_A;
		case UIKeyboardHIDUsageKeyboardB:                    return PLATFORM_KEY_B;
		case UIKeyboardHIDUsageKeyboardC:                    return PLATFORM_KEY_C;
		case UIKeyboardHIDUsageKeyboardD:                    return PLATFORM_KEY_D;
		case UIKeyboardHIDUsageKeyboardE:                    return PLATFORM_KEY_E;
		case UIKeyboardHIDUsageKeyboardF:                    return PLATFORM_KEY_F;
		case UIKeyboardHIDUsageKeyboardG:                    return PLATFORM_KEY_G;
		case UIKeyboardHIDUsageKeyboardH:                    return PLATFORM_KEY_H;
		case UIKeyboardHIDUsageKeyboardI:                    return PLATFORM_KEY_I;
		case UIKeyboardHIDUsageKeyboardJ:                    return PLATFORM_KEY_J;
		case UIKeyboardHIDUsageKeyboardK:                    return PLATFORM_KEY_K;
		case UIKeyboardHIDUsageKeyboardL:                    return PLATFORM_KEY_L;
		case UIKeyboardHIDUsageKeyboardM:                    return PLATFORM_KEY_M;
		case UIKeyboardHIDUsageKeyboardN:                    return PLATFORM_KEY_N;
		case UIKeyboardHIDUsageKeyboardO:                    return PLATFORM_KEY_O;
		case UIKeyboardHIDUsageKeyboardP:                    return PLATFORM_KEY_P;
		case UIKeyboardHIDUsageKeyboardQ:                    return PLATFORM_KEY_Q;
		case UIKeyboardHIDUsageKeyboardR:                    return PLATFORM_KEY_R;
		case UIKeyboardHIDUsageKeyboardS:                    return PLATFORM_KEY_S;
		case UIKeyboardHIDUsageKeyboardT:                    return PLATFORM_KEY_T;
		case UIKeyboardHIDUsageKeyboardU:                    return PLATFORM_KEY_U;
		case UIKeyboardHIDUsageKeyboardV:                    return PLATFORM_KEY_V;
		case UIKeyboardHIDUsageKeyboardW:                    return PLATFORM_KEY_W;
		case UIKeyboardHIDUsageKeyboardX:                    return PLATFORM_KEY_X;
		case UIKeyboardHIDUsageKeyboardY:                    return PLATFORM_KEY_Y;
		case UIKeyboardHIDUsageKeyboardZ:                    return PLATFORM_KEY_Z;
		case UIKeyboardHIDUsageKeyboard0:                    return PLATFORM_KEY_NUM_0;
		case UIKeyboardHIDUsageKeyboard1:                    return PLATFORM_KEY_NUM_1;
		case UIKeyboardHIDUsageKeyboard2:                    return PLATFORM_KEY_NUM_2;
		case UIKeyboardHIDUsageKeyboard3:                    return PLATFORM_KEY_NUM_3;
		case UIKeyboardHIDUsageKeyboard4:                    return PLATFORM_KEY_NUM_4;
		case UIKeyboardHIDUsageKeyboard5:                    return PLATFORM_KEY_NUM_5;
		case UIKeyboardHIDUsageKeyboard6:                    return PLATFORM_KEY_NUM_6;
		case UIKeyboardHIDUsageKeyboard7:                    return PLATFORM_KEY_NUM_7;
		case UIKeyboardHIDUsageKeyboard8:                    return PLATFORM_KEY_NUM_8;
		case UIKeyboardHIDUsageKeyboard9:                    return PLATFORM_KEY_NUM_9;
		case UIKeyboardHIDUsageKeypad0:                      return PLATFORM_KEY_NUMPAD_0;
		case UIKeyboardHIDUsageKeypad1:                      return PLATFORM_KEY_NUMPAD_1;
		case UIKeyboardHIDUsageKeypad2:                      return PLATFORM_KEY_NUMPAD_2;
		case UIKeyboardHIDUsageKeypad3:                      return PLATFORM_KEY_NUMPAD_3;
		case UIKeyboardHIDUsageKeypad4:                      return PLATFORM_KEY_NUMPAD_4;
		case UIKeyboardHIDUsageKeypad5:                      return PLATFORM_KEY_NUMPAD_5;
		case UIKeyboardHIDUsageKeypad6:                      return PLATFORM_KEY_NUMPAD_6;
		case UIKeyboardHIDUsageKeypad7:                      return PLATFORM_KEY_NUMPAD_7;
		case UIKeyboardHIDUsageKeypad8:                      return PLATFORM_KEY_NUMPAD_8;
		case UIKeyboardHIDUsageKeypad9:                      return PLATFORM_KEY_NUMPAD_9;
		case UIKeyboardHIDUsageKeyboardF1:                   return PLATFORM_KEY_F1;
		case UIKeyboardHIDUsageKeyboardF2:                   return PLATFORM_KEY_F2;
		case UIKeyboardHIDUsageKeyboardF3:                   return PLATFORM_KEY_F3;
		case UIKeyboardHIDUsageKeyboardF4:                   return PLATFORM_KEY_F4;
		case UIKeyboardHIDUsageKeyboardF5:                   return PLATFORM_KEY_F5;
		case UIKeyboardHIDUsageKeyboardF6:                   return PLATFORM_KEY_F6;
		case UIKeyboardHIDUsageKeyboardF7:                   return PLATFORM_KEY_F7;
		case UIKeyboardHIDUsageKeyboardF8:                   return PLATFORM_KEY_F8;
		case UIKeyboardHIDUsageKeyboardF9:                   return PLATFORM_KEY_F9;
		case UIKeyboardHIDUsageKeyboardF10:                  return PLATFORM_KEY_F10;
		case UIKeyboardHIDUsageKeyboardF11:                  return PLATFORM_KEY_F11;
		case UIKeyboardHIDUsageKeyboardF12:                  return PLATFORM_KEY_F12;
		case UIKeyboardHIDUsageKeyboardUpArrow:              return PLATFORM_KEY_ARROW_UP;
		case UIKeyboardHIDUsageKeyboardDownArrow:            return PLATFORM_KEY_ARROW_DOWN;
		case UIKeyboardHIDUsageKeyboardLeftArrow:            return PLATFORM_KEY_ARROW_LEFT;
		case UIKeyboardHIDUsageKeyboardRightArrow:           return PLATFORM_KEY_ARROW_RIGHT;
		case UIKeyboardHIDUsageKeyboardLeftShift:            return PLATFORM_KEY_SHIFT_LEFT;
		case UIKeyboardHIDUsageKeyboardRightShift:           return PLATFORM_KEY_SHIFT_RIGHT;
		case UIKeyboardHIDUsageKeyboardLeftControl:          return PLATFORM_KEY_CONTROL_LEFT;
		case UIKeyboardHIDUsageKeyboardRightControl:         return PLATFORM_KEY_CONTROL_RIGHT;
		case UIKeyboardHIDUsageKeyboardLeftAlt:              return PLATFORM_KEY_ALT_LEFT;
		case UIKeyboardHIDUsageKeyboardRightAlt:             return PLATFORM_KEY_ALT_RIGHT;
		case UIKeyboardHIDUsageKeyboardDeleteOrBackspace:    return PLATFORM_KEY_BACKSPACE;
		case UIKeyboardHIDUsageKeyboardTab:                  return PLATFORM_KEY_TAB;
		case UIKeyboardHIDUsageKeyboardReturnOrEnter:
		case UIKeyboardHIDUsageKeyboardReturn:
		case UIKeyboardHIDUsageKeypadEnter:                  return PLATFORM_KEY_ENTER;
		case UIKeyboardHIDUsageKeyboardEscape:               return PLATFORM_KEY_ESCAPE;
		case UIKeyboardHIDUsageKeyboardDeleteForward:        return PLATFORM_KEY_DELETE;
		case UIKeyboardHIDUsageKeyboardInsert:               return PLATFORM_KEY_INSERT;
		case UIKeyboardHIDUsageKeyboardHome:                 return PLATFORM_KEY_HOME;
		case UIKeyboardHIDUsageKeyboardEnd:                  return PLATFORM_KEY_END;
		case UIKeyboardHIDUsageKeyboardPageUp:               return PLATFORM_KEY_PAGE_UP;
		case UIKeyboardHIDUsageKeyboardPageDown:             return PLATFORM_KEY_PAGE_DOWN;
		case UIKeyboardHIDUsageKeyboardSlash:                return PLATFORM_KEY_SLASH;
		case UIKeyboardHIDUsageKeyboardBackslash:            return PLATFORM_KEY_BACKSLASH;
		case UIKeyboardHIDUsageKeyboardOpenBracket:          return PLATFORM_KEY_BRACKET_LEFT;
		case UIKeyboardHIDUsageKeyboardCloseBracket:         return PLATFORM_KEY_BRACKET_RIGHT;
		case UIKeyboardHIDUsageKeyboardGraveAccentAndTilde:  return PLATFORM_KEY_BACKQUOTE;
		case UIKeyboardHIDUsageKeyboardPeriod:               return PLATFORM_KEY_PERIOD;
		case UIKeyboardHIDUsageKeyboardHyphen:               return PLATFORM_KEY_MINUS;
		case UIKeyboardHIDUsageKeyboardEqualSign:            return PLATFORM_KEY_EQUAL;
		case UIKeyboardHIDUsageKeyboardComma:                return PLATFORM_KEY_COMMA;
		case UIKeyboardHIDUsageKeyboardSemicolon:            return PLATFORM_KEY_SEMICOLON;
		case UIKeyboardHIDUsageKeyboardSpacebar:             return PLATFORM_KEY_SPACE;
		default:                                              break;
	}
	return PLATFORM_KEY_COUNT;
}

inline static NSString *
_platform_ios_ns_string(const String &string)
{
	if (string.count == 0)
		return @"";
	if (string.data == nullptr)
		return nil;
	return [[[NSString alloc] initWithBytes:string.data length:string.count encoding:NSUTF8StringEncoding] autorelease];
}

inline static String
_platform_ios_utf8_string(NSString *string)
{
	if (string == nil || [string length] == 0)
		return string_init();

	NSData *data = [string dataUsingEncoding:NSUTF8StringEncoding];
	validate(data != nil, "[PLATFORM][IOS]: Text input contains invalid Unicode.");
	if (data == nil || [data length] == 0)
		return string_init();

	const char *bytes = (const char *)[data bytes];
	return string_from(bytes, bytes + [data length]);
}

inline static bool
_platform_ios_text_equal(const String &lhs, const String &rhs)
{
	if (lhs.count != rhs.count)
		return false;
	return lhs.count == 0 || ::memcmp(lhs.data, rhs.data, lhs.count) == 0;
}

inline static NSUInteger
_platform_ios_utf16_index_from_utf8_offset(const String &text, U32 offset)
{
	U64 byte_count = offset < text.count ? offset : text.count;
	if (byte_count == 0)
		return 0;

	NSString *prefix = [[[NSString alloc] initWithBytes:text.data length:byte_count encoding:NSUTF8StringEncoding] autorelease];
	validate(prefix != nil, "[PLATFORM][IOS]: Text input offset must be on a UTF-8 code-point boundary.");
	return prefix != nil ? [prefix length] : 0;
}

inline static U32
_platform_ios_utf8_offset_from_utf16_index(NSString *text, NSUInteger index)
{
	NSUInteger clamped_index = index < [text length] ? index : [text length];
	if (clamped_index > 0 && clamped_index < [text length])
	{
		unichar previous = [text characterAtIndex:clamped_index - 1];
		unichar next = [text characterAtIndex:clamped_index];
		if (previous >= 0xD800 && previous <= 0xDBFF && next >= 0xDC00 && next <= 0xDFFF)
			--clamped_index;
	}
	NSString *prefix = [text substringToIndex:clamped_index];
	NSUInteger byte_count = [prefix lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
	validate(byte_count <= UINT32_MAX, "[PLATFORM][IOS]: Text input state exceeds Core's offset range.");
	return byte_count <= UINT32_MAX ? (U32)byte_count : UINT32_MAX;
}

inline static NSRange
_platform_ios_text_input_ns_range(Platform_Window_Context *context, U32 start, U32 end)
{
	NSUInteger start_index = _platform_ios_utf16_index_from_utf8_offset(context->text_input_text, start);
	NSUInteger end_index = _platform_ios_utf16_index_from_utf8_offset(context->text_input_text, end);
	if (end_index < start_index)
	{
		NSUInteger swap = start_index;
		start_index = end_index;
		end_index = swap;
	}
	return NSMakeRange(start_index, end_index - start_index);
}

inline static NSRange
_platform_ios_text_input_range_clamp(NSString *text, NSRange range)
{
	NSUInteger text_length = [text length];
	if (range.location == NSNotFound)
		return NSMakeRange(text_length, 0);

	NSUInteger location = range.location < text_length ? range.location : text_length;
	NSUInteger available = text_length - location;
	NSUInteger length = range.length < available ? range.length : available;
	return NSMakeRange(location, length);
}

inline static void
_platform_ios_text_input_events_reset(Array<Platform_Text_Input_Event> &events)
{
	for (U64 i = 0; i < events.count; ++i)
		if (events[i].text.allocator != nullptr)
			string_deinit(events[i].text);
	array_clear(events);
}

inline static void
_platform_ios_text_input_state_reset(Platform_Window_Context *context)
{
	_platform_ios_text_input_events_reset(context->text_input_events);
	if (context->text_input_text.allocator != nullptr)
		string_deinit(context->text_input_text);
	context->text_input = {};
}

inline static void
_platform_ios_text_input_event_push(Platform_Window_Context *context, Platform_Text_Input_Event_Type type, NSString *text = nil)
{
	if (!context->text_input.enabled)
		return;

	Platform_Text_Input_Event event {
		.type = type
	};
	if (text != nil)
		event.text = _platform_ios_utf8_string(text);
	array_push(context->text_input_events, event);
}

inline static void
_platform_ios_text_input_replace(Platform_Window_Context *context, NSRange range, NSString *replacement, NSRange selected_range, bool composing)
{
	NSString *current = _platform_ios_ns_string(context->text_input_text);
	validate(current != nil, "[PLATFORM][IOS]: Text input snapshot must contain valid UTF-8.");
	if (current == nil)
		return;

	range = _platform_ios_text_input_range_clamp(current, range);
	replacement = replacement != nil ? replacement : @"";
	U32 start_byte = _platform_ios_utf8_offset_from_utf16_index(current, range.location);
	U32 end_byte = _platform_ios_utf8_offset_from_utf16_index(current, range.location + range.length);
	String replacement_utf8 = _platform_ios_utf8_string(replacement);
	DEFER(string_deinit(replacement_utf8));

	U64 updated_count = context->text_input_text.count - end_byte + start_byte + replacement_utf8.count;
	validate(updated_count <= UINT32_MAX, "[PLATFORM][IOS]: Text input state exceeds Core's offset range.");
	if (updated_count > UINT32_MAX)
		return;
	String updated = string_with_capacity(updated_count + 2);
	string_resize(updated, updated_count);
	if (start_byte > 0)
		::memcpy(updated.data, context->text_input_text.data, start_byte);
	if (replacement_utf8.count > 0)
		::memcpy(updated.data + start_byte, replacement_utf8.data, replacement_utf8.count);
	U64 suffix_count = context->text_input_text.count - end_byte;
	if (suffix_count > 0)
		::memcpy(updated.data + start_byte + replacement_utf8.count, context->text_input_text.data + end_byte, suffix_count);
	updated.data[updated.count] = '\0';

	if (context->text_input_text.allocator != nullptr)
		string_deinit(context->text_input_text);
	context->text_input_text = updated;

	selected_range = _platform_ios_text_input_range_clamp(replacement, selected_range);
	U32 selected_start = _platform_ios_utf8_offset_from_utf16_index(replacement, selected_range.location);
	U32 selected_end = _platform_ios_utf8_offset_from_utf16_index(replacement, selected_range.location + selected_range.length);
	context->text_input.selection_start = start_byte + selected_start;
	context->text_input.selection_end = start_byte + selected_end;
	if (composing)
	{
		context->text_input.composing_start = start_byte;
		context->text_input.composing_end = start_byte + (U32)replacement_utf8.count;
	}
	else
	{
		context->text_input.composing_start = 0;
		context->text_input.composing_end = 0;
	}
}

@implementation Platform_IOS_Text_Position
@synthesize offset = _offset;

+ (instancetype)
positionWithOffset:(NSUInteger)offset
{
	return [[[self alloc] initWithOffset:offset] autorelease];
}

- (instancetype)
initWithOffset:(NSUInteger)offset
{
	self = [super init];
	if (self != nil)
		_offset = offset;
	return self;
}
@end

@implementation Platform_IOS_Text_Range
@synthesize startOffset = _start_offset;
@synthesize endOffset = _end_offset;

+ (instancetype)
rangeWithStart:(NSUInteger)start end:(NSUInteger)end
{
	return [[[self alloc] initWithStart:start end:end] autorelease];
}

- (instancetype)
initWithStart:(NSUInteger)start end:(NSUInteger)end
{
	self = [super init];
	if (self != nil)
	{
		_start_offset = start < end ? start : end;
		_end_offset = start < end ? end : start;
	}
	return self;
}

- (UITextPosition *)
start
{
	return [Platform_IOS_Text_Position positionWithOffset:_start_offset];
}

- (UITextPosition *)
end
{
	return [Platform_IOS_Text_Position positionWithOffset:_end_offset];
}

- (BOOL)
isEmpty
{
	return _start_offset == _end_offset;
}
@end

inline static Platform_IOS_Touch *
_platform_ios_touch_find(Platform_Window_Context *context, UITouch *native_touch)
{
	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
		if (context->touches[i].native_touch == native_touch)
			return context->touches + i;
	return nullptr;
}

inline static Platform_IOS_Touch *
_platform_ios_touch_get_or_add(Platform_Window_Context *context, UITouch *native_touch)
{
	Platform_IOS_Touch *touch = _platform_ios_touch_find(context, native_touch);
	if (touch != nullptr)
		return touch;

	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
	{
		touch = context->touches + i;
		if (touch->native_touch == nil && !touch->state.pressed && !touch->state.released)
		{
			touch->native_touch = [native_touch retain];
			touch->state.id = (I32)i;
			return touch;
		}
	}
	return nullptr;
}

inline static void
_platform_ios_touches_reset(Platform_Window_Context *context)
{
	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
	{
		Platform_IOS_Touch *touch = context->touches + i;
		[touch->native_touch release];
		*touch = {};
	}
}

inline static void
_platform_ios_touch_position_set(Platform_Touch_State *state, CGPoint location, U32 height, bool reset_delta)
{
	I32 x = (I32)location.x;
	I32 raw_y = (I32)location.y;
	I32 y = height > 0 ? (I32)height - 1 - raw_y : raw_y;
	if (reset_delta)
	{
		state->dx = 0;
		state->dy = 0;
	}
	else
	{
		state->dx += x - state->x;
		state->dy += y - state->y;
	}
	state->x = x;
	state->y = y;
}

@implementation Platform_IOS_Text_Input_View
- (instancetype)
initWithContext:(Platform_Window_Context *)context
{
	self = [super initWithFrame:CGRectZero];
	if (self != nil)
	{
		_context = context;
		_keyboard_type = UIKeyboardTypeDefault;
		_return_key_type = UIReturnKeyDefault;
		_autocapitalization_type = UITextAutocapitalizationTypeNone;
		_autocorrection_type = UITextAutocorrectionTypeDefault;
		_spell_checking_type = UITextSpellCheckingTypeDefault;
	}
	return self;
}

- (void)
dealloc
{
	[_tokenizer release];
	[_marked_text_style release];
	[super dealloc];
}

- (BOOL)
canBecomeFirstResponder
{
	return YES;
}

- (UIView *)
textInputView
{
	UIView *view = [self superview];
	return view != nil ? view : self;
}

- (void)
platformContextClear
{
	[self resignFirstResponder];
	_context = nullptr;
	_input_delegate = nil;
}

- (void)
platformTextInputSet:(const Platform_Text_Input_Desc *)desc
{
	if (_context == nullptr || desc == nullptr)
		return;

	bool text_changed = desc->enabled != _context->text_input.enabled || !_platform_ios_text_equal(desc->text, _context->text_input_text);
	bool selection_changed =
		desc->selection_start != _context->text_input.selection_start ||
		desc->selection_end != _context->text_input.selection_end ||
		desc->composing_start != _context->text_input.composing_start ||
		desc->composing_end != _context->text_input.composing_end;
	bool traits_changed =
		desc->flags != _context->text_input.flags ||
		desc->action != _context->text_input.action;

	if (text_changed)
		[_input_delegate textWillChange:self];
	if (selection_changed)
		[_input_delegate selectionWillChange:self];

	if (desc->enabled)
	{
		validate(desc->text.count <= UINT32_MAX, "[PLATFORM][IOS]: Text input state exceeds Core's offset range.");
		if (text_changed)
		{
			String text = string_copy(desc->text);
			if (_context->text_input_text.allocator != nullptr)
				string_deinit(_context->text_input_text);
			_context->text_input_text = text;
		}
		_context->text_input = *desc;
		_context->text_input.text = {};
	}
	else
	{
		_platform_ios_text_input_state_reset(_context);
	}

	if (selection_changed)
		[_input_delegate selectionDidChange:self];
	if (text_changed)
		[_input_delegate textDidChange:self];

	U32 flags = desc->flags;
	if ((flags & PLATFORM_TEXT_INPUT_FLAG_EMAIL) != 0)
		_keyboard_type = UIKeyboardTypeEmailAddress;
	else if ((flags & PLATFORM_TEXT_INPUT_FLAG_URI) != 0)
		_keyboard_type = UIKeyboardTypeURL;
	else if ((flags & PLATFORM_TEXT_INPUT_FLAG_NUMBER) != 0)
	{
		if ((flags & PLATFORM_TEXT_INPUT_FLAG_SIGNED) != 0)
			_keyboard_type = UIKeyboardTypeNumbersAndPunctuation;
		else if ((flags & PLATFORM_TEXT_INPUT_FLAG_DECIMAL) != 0)
			_keyboard_type = UIKeyboardTypeDecimalPad;
		else
			_keyboard_type = UIKeyboardTypeNumberPad;
	}
	else
		_keyboard_type = UIKeyboardTypeDefault;

	switch (desc->action)
	{
		case PLATFORM_TEXT_INPUT_ACTION_DONE:     _return_key_type = UIReturnKeyDone; break;
		case PLATFORM_TEXT_INPUT_ACTION_GO:       _return_key_type = UIReturnKeyGo; break;
		case PLATFORM_TEXT_INPUT_ACTION_SEARCH:   _return_key_type = UIReturnKeySearch; break;
		case PLATFORM_TEXT_INPUT_ACTION_SEND:     _return_key_type = UIReturnKeySend; break;
		case PLATFORM_TEXT_INPUT_ACTION_NEXT:     _return_key_type = UIReturnKeyNext; break;
		case PLATFORM_TEXT_INPUT_ACTION_PREVIOUS: _return_key_type = UIReturnKeyDefault; break;
		case PLATFORM_TEXT_INPUT_ACTION_NONE:     _return_key_type = UIReturnKeyDefault; break;
	}
	_secure_text_entry = (flags & PLATFORM_TEXT_INPUT_FLAG_PASSWORD) != 0;
	bool suggestions = (flags & (PLATFORM_TEXT_INPUT_FLAG_NO_SUGGESTIONS | PLATFORM_TEXT_INPUT_FLAG_PASSWORD)) == 0;
	_autocorrection_type = suggestions ? UITextAutocorrectionTypeDefault : UITextAutocorrectionTypeNo;
	_spell_checking_type = suggestions ? UITextSpellCheckingTypeDefault : UITextSpellCheckingTypeNo;

	if (traits_changed && [self isFirstResponder])
		[self reloadInputViews];

	Platform_IOS_View *root_view = (Platform_IOS_View *)[self superview];
	[root_view platformInputResponderActivate];
}

- (id<UITextInputDelegate>)
inputDelegate
{
	return _input_delegate;
}

- (void)
setInputDelegate:(id<UITextInputDelegate>)input_delegate
{
	_input_delegate = input_delegate;
}

- (id<UITextInputTokenizer>)
tokenizer
{
	if (_tokenizer == nil)
		_tokenizer = [[UITextInputStringTokenizer alloc] initWithTextInput:self];
	return _tokenizer;
}

- (NSDictionary *)
markedTextStyle
{
	return _marked_text_style;
}

- (void)
setMarkedTextStyle:(NSDictionary *)style
{
	if (_marked_text_style == style)
		return;
	[_marked_text_style release];
	_marked_text_style = [style copy];
}

- (UIKeyboardType)
keyboardType
{
	return _keyboard_type;
}

- (void)
setKeyboardType:(UIKeyboardType)keyboard_type
{
	_keyboard_type = keyboard_type;
}

- (UIReturnKeyType)
returnKeyType
{
	return _return_key_type;
}

- (void)
setReturnKeyType:(UIReturnKeyType)return_key_type
{
	_return_key_type = return_key_type;
}

- (UITextAutocapitalizationType)
autocapitalizationType
{
	return _autocapitalization_type;
}

- (void)
setAutocapitalizationType:(UITextAutocapitalizationType)type
{
	_autocapitalization_type = type;
}

- (UITextAutocorrectionType)
autocorrectionType
{
	return _autocorrection_type;
}

- (void)
setAutocorrectionType:(UITextAutocorrectionType)type
{
	_autocorrection_type = type;
}

- (UITextSpellCheckingType)
spellCheckingType
{
	return _spell_checking_type;
}

- (void)
setSpellCheckingType:(UITextSpellCheckingType)type
{
	_spell_checking_type = type;
}

- (BOOL)
isSecureTextEntry
{
	return _secure_text_entry;
}

- (void)
setSecureTextEntry:(BOOL)secure_text_entry
{
	_secure_text_entry = secure_text_entry;
}

- (BOOL)
hasText
{
	return _context != nullptr && _context->text_input.enabled && _context->text_input_text.count > 0;
}

- (void)
insertText:(NSString *)text
{
	if (_context == nullptr || !_context->text_input.enabled || text == nil)
		return;

	if ([text isEqualToString:@"\n"] && (_context->text_input.flags & PLATFORM_TEXT_INPUT_FLAG_MULTILINE) == 0)
	{
		Platform_Text_Input_Action action = _context->text_input.action != PLATFORM_TEXT_INPUT_ACTION_NONE ? _context->text_input.action : PLATFORM_TEXT_INPUT_ACTION_DONE;
		array_push(_context->text_input_events, Platform_Text_Input_Event {
			.type = PLATFORM_TEXT_INPUT_EVENT_ACTION,
			.action = action
		});
		return;
	}

	bool composing = _context->text_input.composing_start != _context->text_input.composing_end;
	NSRange range = composing ?
		_platform_ios_text_input_ns_range(_context, _context->text_input.composing_start, _context->text_input.composing_end) :
		_platform_ios_text_input_ns_range(_context, _context->text_input.selection_start, _context->text_input.selection_end);
	[_input_delegate textWillChange:self];
	[_input_delegate selectionWillChange:self];
	_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMMIT, text);
	_platform_ios_text_input_replace(_context, range, text, NSMakeRange([text length], 0), false);
	if (composing)
		_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END);
	[_input_delegate selectionDidChange:self];
	[_input_delegate textDidChange:self];
}

- (void)
deleteBackward
{
	if (_context == nullptr || !_context->text_input.enabled)
		return;

	NSString *text = _platform_ios_ns_string(_context->text_input_text);
	if (text == nil)
		return;
	NSRange selection = _platform_ios_text_input_ns_range(_context, _context->text_input.selection_start, _context->text_input.selection_end);
	NSRange delete_range = selection;
	if (delete_range.length == 0 && delete_range.location > 0)
		delete_range = [text rangeOfComposedCharacterSequenceAtIndex:delete_range.location - 1];
	if (delete_range.length == 0)
		return;

	U32 delete_start = _platform_ios_utf8_offset_from_utf16_index(text, delete_range.location);
	U32 delete_end = _platform_ios_utf8_offset_from_utf16_index(text, delete_range.location + delete_range.length);
	U32 selection_start = _platform_ios_utf8_offset_from_utf16_index(text, selection.location);
	U32 selection_end = _platform_ios_utf8_offset_from_utf16_index(text, selection.location + selection.length);
	U32 delete_before = selection_start - delete_start;
	U32 delete_after = delete_end - selection_end;
	bool delete_size_valid = delete_before <= INT_MAX && delete_after <= INT_MAX;
	validate(delete_size_valid, "[PLATFORM][IOS]: Text deletion exceeds Core's event range.");
	if (!delete_size_valid)
		return;
	bool composing = _context->text_input.composing_start != _context->text_input.composing_end;
	[_input_delegate textWillChange:self];
	[_input_delegate selectionWillChange:self];
	array_push(_context->text_input_events, Platform_Text_Input_Event {
		.type = PLATFORM_TEXT_INPUT_EVENT_DELETE_SURROUNDING,
		.delete_before = (I32)delete_before,
		.delete_after = (I32)delete_after
	});
	_platform_ios_text_input_replace(_context, delete_range, @"", NSMakeRange(0, 0), false);
	if (composing)
		_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END);
	[_input_delegate selectionDidChange:self];
	[_input_delegate textDidChange:self];
}

- (NSString *)
textInRange:(UITextRange *)range
{
	if (_context == nullptr || ![range isKindOfClass:[Platform_IOS_Text_Range class]])
		return nil;

	NSString *text = _platform_ios_ns_string(_context->text_input_text);
	Platform_IOS_Text_Range *platform_range = (Platform_IOS_Text_Range *)range;
	NSRange ns_range = _platform_ios_text_input_range_clamp(text, NSMakeRange([platform_range startOffset], [platform_range endOffset] - [platform_range startOffset]));
	return [text substringWithRange:ns_range];
}

- (void)
replaceRange:(UITextRange *)range withText:(NSString *)text
{
	if (_context == nullptr || !_context->text_input.enabled || ![range isKindOfClass:[Platform_IOS_Text_Range class]])
		return;

	Platform_IOS_Text_Range *platform_range = (Platform_IOS_Text_Range *)range;
	NSRange ns_range = NSMakeRange([platform_range startOffset], [platform_range endOffset] - [platform_range startOffset]);
	NSString *current = _platform_ios_ns_string(_context->text_input_text);
	ns_range = _platform_ios_text_input_range_clamp(current, ns_range);
	U32 selection_start = _platform_ios_utf8_offset_from_utf16_index(current, ns_range.location);
	U32 selection_end = _platform_ios_utf8_offset_from_utf16_index(current, ns_range.location + ns_range.length);
	bool composing = _context->text_input.composing_start != _context->text_input.composing_end;
	[_input_delegate textWillChange:self];
	[_input_delegate selectionWillChange:self];
	array_push(_context->text_input_events, Platform_Text_Input_Event {
		.type = PLATFORM_TEXT_INPUT_EVENT_SELECTION,
		.selection_start = selection_start,
		.selection_end = selection_end
	});
	_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMMIT, text);
	_platform_ios_text_input_replace(_context, ns_range, text, NSMakeRange([text length], 0), false);
	if (composing)
		_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END);
	[_input_delegate selectionDidChange:self];
	[_input_delegate textDidChange:self];
}

- (UITextRange *)
selectedTextRange
{
	if (_context == nullptr || !_context->text_input.enabled)
		return nil;
	NSRange range = _platform_ios_text_input_ns_range(_context, _context->text_input.selection_start, _context->text_input.selection_end);
	return [Platform_IOS_Text_Range rangeWithStart:range.location end:range.location + range.length];
}

- (void)
setSelectedTextRange:(UITextRange *)range
{
	if (_context == nullptr || !_context->text_input.enabled || ![range isKindOfClass:[Platform_IOS_Text_Range class]])
		return;

	Platform_IOS_Text_Range *platform_range = (Platform_IOS_Text_Range *)range;
	NSString *text = _platform_ios_ns_string(_context->text_input_text);
	NSRange ns_range = _platform_ios_text_input_range_clamp(text, NSMakeRange([platform_range startOffset], [platform_range endOffset] - [platform_range startOffset]));
	U32 start = _platform_ios_utf8_offset_from_utf16_index(text, ns_range.location);
	U32 end = _platform_ios_utf8_offset_from_utf16_index(text, ns_range.location + ns_range.length);
	if (start == _context->text_input.selection_start && end == _context->text_input.selection_end)
		return;

	[_input_delegate selectionWillChange:self];
	_context->text_input.selection_start = start;
	_context->text_input.selection_end = end;
	array_push(_context->text_input_events, Platform_Text_Input_Event {
		.type = PLATFORM_TEXT_INPUT_EVENT_SELECTION,
		.selection_start = start,
		.selection_end = end
	});
	[_input_delegate selectionDidChange:self];
}

- (UITextRange *)
markedTextRange
{
	if (_context == nullptr || !_context->text_input.enabled || _context->text_input.composing_start == _context->text_input.composing_end)
		return nil;
	NSRange range = _platform_ios_text_input_ns_range(_context, _context->text_input.composing_start, _context->text_input.composing_end);
	return [Platform_IOS_Text_Range rangeWithStart:range.location end:range.location + range.length];
}

- (void)
setMarkedText:(NSString *)marked_text selectedRange:(NSRange)selected_range
{
	if (_context == nullptr || !_context->text_input.enabled)
		return;
	if (marked_text == nil)
	{
		[self unmarkText];
		return;
	}

	bool composing = _context->text_input.composing_start != _context->text_input.composing_end;
	NSRange range = composing ?
		_platform_ios_text_input_ns_range(_context, _context->text_input.composing_start, _context->text_input.composing_end) :
		_platform_ios_text_input_ns_range(_context, _context->text_input.selection_start, _context->text_input.selection_end);
	[_input_delegate textWillChange:self];
	[_input_delegate selectionWillChange:self];
	_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMPOSE, marked_text);
	_platform_ios_text_input_replace(_context, range, marked_text, selected_range, true);
	[_input_delegate selectionDidChange:self];
	[_input_delegate textDidChange:self];
}

- (void)
unmarkText
{
	if (_context == nullptr || !_context->text_input.enabled || _context->text_input.composing_start == _context->text_input.composing_end)
		return;
	[_input_delegate selectionWillChange:self];
	_context->text_input.composing_start = 0;
	_context->text_input.composing_end = 0;
	_platform_ios_text_input_event_push(_context, PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END);
	[_input_delegate selectionDidChange:self];
}

- (UITextPosition *)
beginningOfDocument
{
	return [Platform_IOS_Text_Position positionWithOffset:0];
}

- (UITextPosition *)
endOfDocument
{
	NSString *text = _context != nullptr ? _platform_ios_ns_string(_context->text_input_text) : @"";
	return [Platform_IOS_Text_Position positionWithOffset:[text length]];
}

- (UITextRange *)
textRangeFromPosition:(UITextPosition *)from_position toPosition:(UITextPosition *)to_position
{
	if (![from_position isKindOfClass:[Platform_IOS_Text_Position class]] || ![to_position isKindOfClass:[Platform_IOS_Text_Position class]])
		return nil;
	return [Platform_IOS_Text_Range rangeWithStart:[(Platform_IOS_Text_Position *)from_position offset] end:[(Platform_IOS_Text_Position *)to_position offset]];
}

- (UITextPosition *)
positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset
{
	if (![position isKindOfClass:[Platform_IOS_Text_Position class]])
		return nil;
	NSInteger current = (NSInteger)[(Platform_IOS_Text_Position *)position offset];
	NSInteger target = current + offset;
	Platform_IOS_Text_Position *end = (Platform_IOS_Text_Position *)[self endOfDocument];
	NSUInteger text_length = [end offset];
	if (target < 0 || (NSUInteger)target > text_length)
		return nil;
	return [Platform_IOS_Text_Position positionWithOffset:(NSUInteger)target];
}

- (UITextPosition *)
positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset
{
	NSInteger signed_offset = direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp ? -offset : offset;
	return [self positionFromPosition:position offset:signed_offset];
}

- (NSComparisonResult)
comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other
{
	NSUInteger lhs = [(Platform_IOS_Text_Position *)position offset];
	NSUInteger rhs = [(Platform_IOS_Text_Position *)other offset];
	if (lhs < rhs)
		return NSOrderedAscending;
	if (lhs > rhs)
		return NSOrderedDescending;
	return NSOrderedSame;
}

- (NSInteger)
offsetFromPosition:(UITextPosition *)from_position toPosition:(UITextPosition *)to_position
{
	return (NSInteger)[(Platform_IOS_Text_Position *)to_position offset] - (NSInteger)[(Platform_IOS_Text_Position *)from_position offset];
}

- (UITextPosition *)
positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction
{
	if (![range isKindOfClass:[Platform_IOS_Text_Range class]])
		return nil;
	Platform_IOS_Text_Range *platform_range = (Platform_IOS_Text_Range *)range;
	NSUInteger offset = direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp ? [platform_range startOffset] : [platform_range endOffset];
	return [Platform_IOS_Text_Position positionWithOffset:offset];
}

- (UITextRange *)
characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction
{
	if (_context == nullptr || ![position isKindOfClass:[Platform_IOS_Text_Position class]])
		return nil;
	NSString *text = _platform_ios_ns_string(_context->text_input_text);
	NSUInteger offset = [(Platform_IOS_Text_Position *)position offset];
	if (direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp)
	{
		if (offset == 0)
			return [Platform_IOS_Text_Range rangeWithStart:0 end:0];
		NSRange range = [text rangeOfComposedCharacterSequenceAtIndex:offset - 1];
		return [Platform_IOS_Text_Range rangeWithStart:range.location end:range.location + range.length];
	}
	if (offset >= [text length])
		return [Platform_IOS_Text_Range rangeWithStart:[text length] end:[text length]];
	NSRange range = [text rangeOfComposedCharacterSequenceAtIndex:offset];
	return [Platform_IOS_Text_Range rangeWithStart:range.location end:range.location + range.length];
}

- (NSWritingDirection)
baseWritingDirectionForPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction
{
	unused(position, direction);
	return NSWritingDirectionNatural;
}

- (void)
setBaseWritingDirection:(NSWritingDirection)writing_direction forRange:(UITextRange *)range
{
	unused(writing_direction, range);
}

- (CGRect)
caretRectForPosition:(UITextPosition *)position
{
	unused(position);
	if (_context == nullptr || !_context->text_input.enabled)
		return CGRectZero;
	UIView *view = [self textInputView];
	CGFloat y = [view bounds].size.height - _context->text_input.y - _context->text_input.height;
	return CGRectMake(_context->text_input.x, y, _context->text_input.width, _context->text_input.height);
}

- (CGRect)
firstRectForRange:(UITextRange *)range
{
	return [self caretRectForPosition:[range start]];
}

- (NSArray<UITextSelectionRect *> *)
selectionRectsForRange:(UITextRange *)range
{
	unused(range);
	return [NSArray array];
}

- (UITextPosition *)
closestPositionToPoint:(CGPoint)point
{
	unused(point);
	UITextRange *selection = [self selectedTextRange];
	return selection != nil ? [selection end] : [self beginningOfDocument];
}

- (UITextPosition *)
closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range
{
	unused(point);
	if (![range isKindOfClass:[Platform_IOS_Text_Range class]])
		return nil;
	Platform_IOS_Text_Range *platform_range = (Platform_IOS_Text_Range *)range;
	UITextPosition *position = [self closestPositionToPoint:point];
	NSUInteger offset = [(Platform_IOS_Text_Position *)position offset];
	if (offset < [platform_range startOffset])
		offset = [platform_range startOffset];
	if (offset > [platform_range endOffset])
		offset = [platform_range endOffset];
	return [Platform_IOS_Text_Position positionWithOffset:offset];
}

- (UITextRange *)
characterRangeAtPoint:(CGPoint)point
{
	UITextPosition *position = [self closestPositionToPoint:point];
	return [self characterRangeByExtendingPosition:position inDirection:UITextLayoutDirectionRight];
}
@end

@implementation Platform_IOS_View
- (instancetype)
initWithFrame:(CGRect)frame context:(Platform_Window_Context *)context
{
	self = [super initWithFrame:frame];
	if (self != nil)
	{
		_context = context;
		_text_input_view = [[Platform_IOS_Text_Input_View alloc] initWithContext:context];
		[self addSubview:_text_input_view];
		[_text_input_view release];
		[self setMultipleTouchEnabled:YES];

		if (@available(iOS 13.4, *))
		{
			UIHoverGestureRecognizer *hover = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(platformMouseHover:)];
			[self addGestureRecognizer:hover];
			[hover release];

			UIPanGestureRecognizer *scroll = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(platformMouseScrollGesture:)];
			[scroll setAllowedTouchTypes:@[]];
			[scroll setAllowedScrollTypesMask:UIScrollTypeMaskAll];
			[scroll setCancelsTouchesInView:NO];
			[self addGestureRecognizer:scroll];
			[scroll release];
		}
	}
	return self;
}

- (void)
dealloc
{
	[self platformContextClear];
	[super dealloc];
}

- (void)
platformContextClear
{
	[_text_input_view platformContextClear];
	_context = nullptr;
}

- (void)
platformInputResponderActivate
{
	if (_context != nullptr && _context->text_input.enabled)
	{
		[_text_input_view becomeFirstResponder];
		return;
	}

	[_text_input_view resignFirstResponder];
	[self becomeFirstResponder];
}

- (BOOL)
canBecomeFirstResponder
{
	return YES;
}

- (BOOL)
platformKeyUsage:(UIKeyboardHIDUsage)usage down:(BOOL)down API_AVAILABLE(ios(13.4))
{
	if (_context == nullptr)
		return NO;

	PLATFORM_KEY key = _platform_ios_key_from_hid_usage(usage);
	if (key == PLATFORM_KEY_COUNT)
		return NO;

	_platform_ios_key_state_set(_context->keys + key, down);
	return YES;
}

- (void)
platformMousePosition:(CGPoint)location
{
	if (_context == nullptr)
		return;

	Platform_IOS_Mouse *mouse = &_context->mouse;
	I32 x = (I32)location.x;
	I32 raw_y = (I32)location.y;
	U32 height = (U32)[self bounds].size.height;
	I32 y = height > 0 ? (I32)height - 1 - raw_y : raw_y;
	if (mouse->position_valid)
	{
		mouse->dx += x - mouse->x;
		mouse->dy += y - mouse->y;
	}
	else
	{
		mouse->position_valid = true;
	}
	mouse->x = x;
	mouse->y = y;
}

- (void)
platformMouseButtons:(UIEventButtonMask)button_mask API_AVAILABLE(ios(13.4))
{
	if (_context == nullptr)
		return;

	_platform_ios_key_state_set(_context->keys + PLATFORM_KEY_MOUSE_LEFT, (button_mask & UIEventButtonMaskPrimary) != 0);
	_platform_ios_key_state_set(_context->keys + PLATFORM_KEY_MOUSE_MIDDLE, (button_mask & UIEventButtonMaskForButtonNumber(3)) != 0);
	_platform_ios_key_state_set(_context->keys + PLATFORM_KEY_MOUSE_RIGHT, (button_mask & UIEventButtonMaskSecondary) != 0);
}

- (void)
platformMouseScroll:(CGFloat)delta
{
	if (_context == nullptr || delta == 0.0)
		return;
	_context->mouse.wheel += delta > 0.0 ? 1.0f : -1.0f;
}

- (void)
platformMouseHover:(UIHoverGestureRecognizer *)recognizer API_AVAILABLE(ios(13.4))
{
	switch ([recognizer state])
	{
		case UIGestureRecognizerStateBegan:
		case UIGestureRecognizerStateChanged:
		case UIGestureRecognizerStateEnded:
			[self platformMousePosition:[recognizer locationInView:self]];
			break;
		default:
			break;
	}
}

- (void)
platformMouseScrollGesture:(UIPanGestureRecognizer *)recognizer API_AVAILABLE(ios(13.4))
{
	CGPoint translation = [recognizer translationInView:self];
	if (translation.y != 0.0)
	{
		[self platformMouseScroll:translation.y];
		[recognizer setTranslation:CGPointZero inView:self];
	}
}

- (BOOL)
platformMouseTouch:(UITouch *)touch event:(UIEvent *)event cancelled:(BOOL)cancelled API_AVAILABLE(ios(13.4))
{
	if ([touch type] != UITouchTypeIndirectPointer)
		return NO;

	[self platformMousePosition:[touch locationInView:self]];
	[self platformMouseButtons:cancelled ? 0 : [event buttonMask]];
	return YES;
}

- (void)
platformTouchBegan:(UITouch *)touch location:(CGPoint)location
{
	if (_context == nullptr)
		return;

	Platform_IOS_Touch *platform_touch = _platform_ios_touch_get_or_add(_context, touch);
	if (platform_touch == nullptr)
		return;

	_platform_ios_touch_position_set(&platform_touch->state, location, (U32)[self bounds].size.height, true);
	platform_touch->state.pressed = true;
	platform_touch->state.down = true;
}

- (void)
platformTouchMoved:(UITouch *)touch location:(CGPoint)location
{
	if (_context == nullptr)
		return;

	Platform_IOS_Touch *platform_touch = _platform_ios_touch_find(_context, touch);
	if (platform_touch == nullptr)
		return;

	_platform_ios_touch_position_set(&platform_touch->state, location, (U32)[self bounds].size.height, false);
}

- (void)
platformTouchEnded:(UITouch *)touch location:(CGPoint)location
{
	if (_context == nullptr)
		return;

	Platform_IOS_Touch *platform_touch = _platform_ios_touch_find(_context, touch);
	if (platform_touch == nullptr)
		return;

	_platform_ios_touch_position_set(&platform_touch->state, location, (U32)[self bounds].size.height, false);
	[platform_touch->native_touch release];
	platform_touch->native_touch = nil;
	platform_touch->state.released = true;
	platform_touch->state.down = false;
}

- (void)
touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches)
	{
		if (@available(iOS 13.4, *))
			if ([self platformMouseTouch:touch event:event cancelled:NO])
				continue;
		[self platformTouchBegan:touch location:[touch locationInView:self]];
	}
}

- (void)
touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches)
	{
		if (@available(iOS 13.4, *))
			if ([self platformMouseTouch:touch event:event cancelled:NO])
				continue;
		[self platformTouchMoved:touch location:[touch locationInView:self]];
	}
}

- (void)
touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches)
	{
		if (@available(iOS 13.4, *))
			if ([self platformMouseTouch:touch event:event cancelled:NO])
				continue;
		[self platformTouchEnded:touch location:[touch locationInView:self]];
	}
}

- (void)
touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches)
	{
		if (@available(iOS 13.4, *))
			if ([self platformMouseTouch:touch event:event cancelled:YES])
				continue;
		[self platformTouchEnded:touch location:[touch locationInView:self]];
	}
}

- (void)
pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
	if (@available(iOS 13.4, *))
	{
		for (UIPress *press in presses)
		{
			UIKey *key = [press key];
			if (key != nil)
				[self platformKeyUsage:[key keyCode] down:YES];
		}
	}
	[super pressesBegan:presses withEvent:event];
}

- (void)
pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
	if (@available(iOS 13.4, *))
	{
		for (UIPress *press in presses)
		{
			UIKey *key = [press key];
			if (key != nil)
				[self platformKeyUsage:[key keyCode] down:NO];
		}
	}
	[super pressesEnded:presses withEvent:event];
}

- (void)
pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
	if (@available(iOS 13.4, *))
	{
		for (UIPress *press in presses)
		{
			UIKey *key = [press key];
			if (key != nil)
				[self platformKeyUsage:[key keyCode] down:NO];
		}
	}
	[super pressesCancelled:presses withEvent:event];
}
@end

@implementation Platform_IOS_View_Controller
- (void)
viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
	[(Platform_IOS_View *)[self view] platformInputResponderActivate];
}

- (void)
platformPresentationSet:(U32)flags orientationPolicy:(Platform_Window_Orientation_Policy)orientation_policy
{
	_presentation_flags = flags;
	_orientation_policy = orientation_policy;

	bool edge_to_edge = (flags & (PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN | PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE | PLATFORM_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE)) != 0;
	[self setEdgesForExtendedLayout:edge_to_edge ? UIRectEdgeAll : UIRectEdgeNone];
	[self setExtendedLayoutIncludesOpaqueBars:edge_to_edge];
	[self setNeedsStatusBarAppearanceUpdate];
	[self setNeedsUpdateOfHomeIndicatorAutoHidden];
	[self setNeedsUpdateOfScreenEdgesDeferringSystemGestures];

	if (@available(iOS 16.0, *))
	{
		[self setNeedsUpdateOfSupportedInterfaceOrientations];
		UIWindowScene *scene = [[[self view] window] windowScene];
		if (scene != nil)
		{
			UIWindowSceneGeometryPreferencesIOS *preferences = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:[self supportedInterfaceOrientations]];
			[scene requestGeometryUpdateWithPreferences:preferences errorHandler:nil];
			[preferences release];
		}
	}
	else
	{
		[UIViewController attemptRotationToDeviceOrientation];
	}
}

- (BOOL)
prefersStatusBarHidden
{
	return (_presentation_flags & (PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN | PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE)) != 0;
}

- (BOOL)
prefersHomeIndicatorAutoHidden
{
	return (_presentation_flags & PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE) != 0;
}

- (UIRectEdge)
preferredScreenEdgesDeferringSystemGestures
{
	return (_presentation_flags & PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE) != 0 ? UIRectEdgeAll : UIRectEdgeNone;
}

- (UIInterfaceOrientationMask)
supportedInterfaceOrientations
{
	switch (_orientation_policy)
	{
		case PLATFORM_WINDOW_ORIENTATION_POLICY_PORTRAIT:
			return UIInterfaceOrientationMaskPortrait;
		case PLATFORM_WINDOW_ORIENTATION_POLICY_LANDSCAPE:
		{
			UIWindowScene *scene = [[[self viewIfLoaded] window] windowScene];
			if (scene != nil && [scene interfaceOrientation] == UIInterfaceOrientationLandscapeLeft)
				return UIInterfaceOrientationMaskLandscapeLeft;
			return UIInterfaceOrientationMaskLandscapeRight;
		}
		case PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR:
			return UIInterfaceOrientationMaskAll;
		case PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_PORTRAIT:
			return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
		case PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE:
			return UIInterfaceOrientationMaskLandscape;
		case PLATFORM_WINDOW_ORIENTATION_POLICY_SYSTEM:
			return [super supportedInterfaceOrientations];
	}

	return [super supportedInterfaceOrientations];
}
@end

inline static void
_platform_ios_keep_screen_on_set(bool enabled)
{
	UIApplication *application = [UIApplication sharedApplication];
	if (enabled)
	{
		if (_platform_ios_keep_screen_on_count == 0)
			_platform_ios_idle_timer_disabled = [application isIdleTimerDisabled];
		++_platform_ios_keep_screen_on_count;
		[application setIdleTimerDisabled:YES];
		return;
	}

	validate(_platform_ios_keep_screen_on_count > 0, "[PLATFORM][IOS]: Keep-screen-on reference count underflow.");
	if (_platform_ios_keep_screen_on_count == 0)
		return;

	--_platform_ios_keep_screen_on_count;
	if (_platform_ios_keep_screen_on_count == 0)
		[application setIdleTimerDisabled:_platform_ios_idle_timer_disabled];
}

inline static void
_platform_ios_scene_activation_set_locked(Platform_Window_Context *context, UISceneActivationState activation_state)
{
	switch (activation_state)
	{
		case UISceneActivationStateForegroundActive:
		{
			context->started = true;
			context->paused = false;
			context->focused = true;
			break;
		}
		case UISceneActivationStateForegroundInactive:
		{
			context->started = true;
			context->paused = true;
			context->focused = false;
			break;
		}
		case UISceneActivationStateBackground:
		case UISceneActivationStateUnattached:
		{
			context->started = false;
			context->paused = true;
			context->focused = false;
			break;
		}
	}
}

inline static Platform_Window_Metrics
_platform_ios_window_metrics(UIView *view, UIWindowScene *scene, U32 width, U32 height)
{
	UIEdgeInsets insets = [view safeAreaInsets];
	U32 left = insets.left > 0.0 ? (U32)insets.left : 0;
	U32 top = insets.top > 0.0 ? (U32)insets.top : 0;
	U32 right = insets.right > 0.0 ? (U32)insets.right : 0;
	U32 bottom = insets.bottom > 0.0 ? (U32)insets.bottom : 0;
	if (left > width)
		left = width;
	if (right > width - left)
		right = width - left;
	if (top > height)
		top = height;
	if (bottom > height - top)
		bottom = height - top;

	UIScreen *screen = [scene screen];
	F32 density_scale = screen ? (F32)[screen scale] : 1.0f;
	Platform_Window_Orientation orientation = PLATFORM_WINDOW_ORIENTATION_UNKNOWN;
	if (width > 0 && height > 0)
		orientation = width >= height ? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE : PLATFORM_WINDOW_ORIENTATION_PORTRAIT;

	return Platform_Window_Metrics {
		.content_rect = Platform_Window_Rect {
			.x = (I32)left,
			.y = (I32)bottom,
			.width = width - left - right,
			.height = height - top - bottom
		},
		.safe_area = Platform_Window_Insets {
			.left = left,
			.top = top,
			.right = right,
			.bottom = bottom
		},
		.density_scale = density_scale,
		.dpi_x = 160.0f * density_scale,
		.dpi_y = 160.0f * density_scale,
		.orientation = orientation
	};
}

inline static void
_platform_ios_scene_lifecycle_notification(Platform_Window_Context *context, NSNotification *notification, bool started, bool paused, bool focused)
{
	UIWindowScene *scene = [[notification object] isKindOfClass:[UIWindowScene class]] ? (UIWindowScene *)[notification object] : nil;
	if (scene == nil)
		return;

	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	DEFER(validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context."));

	if (!context->connected || context->scene != scene || context->close_requested)
		return;

	context->started = started;
	context->paused = paused;
	context->focused = focused;
}

inline static void
_platform_ios_scene_disconnect_notification(Platform_Window_Context *context, NSNotification *notification)
{
	UIWindowScene *scene = [[notification object] isKindOfClass:[UIWindowScene class]] ? (UIWindowScene *)[notification object] : nil;
	if (scene == nil)
		return;

	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	if (!context->connected || context->scene != scene)
	{
		validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");
		return;
	}

	UIWindow *window = context->window;
	Platform_IOS_View_Controller *view_controller = context->view_controller;
	Platform_IOS_View *view = context->view;
	bool keep_screen_on = context->keep_screen_on;
	context->scene = nil;
	context->session = nil;
	context->window = nil;
	context->view_controller = nil;
	context->view = nil;
	context->connected = false;
	context->close_requested = true;
	context->started = false;
	context->paused = true;
	context->focused = false;
	context->surface_changed = true;
	context->keep_screen_on = false;
	::memset(context->keys, 0, sizeof(context->keys));
	_platform_ios_touches_reset(context);
	context->mouse = {};
	_platform_ios_text_input_state_reset(context);
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	if (view != nil)
		[view platformContextClear];
	if (keep_screen_on)
		_platform_ios_keep_screen_on_set(false);
	if ([window rootViewController] == view_controller)
		[window setRootViewController:nil];
}

inline static void
_platform_ios_scene_observers_deinit(Platform_Window_Context *context)
{
	NSNotificationCenter *notification_center = [NSNotificationCenter defaultCenter];
	for (U32 i = 0; i < PLATFORM_IOS_SCENE_OBSERVER_COUNT; ++i)
	{
		id observer = context->scene_observers[i];
		if (observer == nil)
			continue;

		[notification_center removeObserver:observer];
		[observer release];
		context->scene_observers[i] = nil;
	}
}

inline static bool
_platform_ios_scene_observer_add(Platform_Window_Context *context, PLATFORM_IOS_SCENE_OBSERVER index, NSString *name, UIWindowScene *scene, bool disconnect, bool started, bool paused, bool focused)
{
	id observer = [[NSNotificationCenter defaultCenter] addObserverForName:name object:scene queue:nil usingBlock:^(NSNotification *notification) {
		if (disconnect)
			_platform_ios_scene_disconnect_notification(context, notification);
		else
			_platform_ios_scene_lifecycle_notification(context, notification, started, paused, focused);
	}];
	if (observer == nil)
		return false;

	context->scene_observers[index] = [observer retain];
	return true;
}

inline static bool
_platform_ios_scene_observers_init(Platform_Window_Context *context, UIWindowScene *scene)
{
	bool initialized =
		_platform_ios_scene_observer_add(context, PLATFORM_IOS_SCENE_OBSERVER_WILL_ENTER_FOREGROUND, UISceneWillEnterForegroundNotification, scene, false, true, true, false) &&
		_platform_ios_scene_observer_add(context, PLATFORM_IOS_SCENE_OBSERVER_DID_ENTER_BACKGROUND, UISceneDidEnterBackgroundNotification, scene, false, false, true, false) &&
		_platform_ios_scene_observer_add(context, PLATFORM_IOS_SCENE_OBSERVER_DID_BECOME_ACTIVE, UISceneDidActivateNotification, scene, false, true, false, true) &&
		_platform_ios_scene_observer_add(context, PLATFORM_IOS_SCENE_OBSERVER_WILL_RESIGN_ACTIVE, UISceneWillDeactivateNotification, scene, false, true, true, false) &&
		_platform_ios_scene_observer_add(context, PLATFORM_IOS_SCENE_OBSERVER_DID_DISCONNECT, UISceneDidDisconnectNotification, scene, true, false, true, false);
	if (!initialized)
		_platform_ios_scene_observers_deinit(context);
	return initialized;
}

inline static void
_platform_ios_scene_session_destroy(UISceneSession *session)
{
	if (session == nil)
		return;

	if ([NSThread isMainThread])
	{
		[[UIApplication sharedApplication] requestSceneSessionDestruction:session options:nil errorHandler:nil];
		[session release];
		return;
	}

	dispatch_async(dispatch_get_main_queue(), ^{
		[[UIApplication sharedApplication] requestSceneSessionDestruction:session options:nil errorHandler:nil];
		[session release];
	});
}

inline static void
_platform_ios_input_reset_transitions(Platform_Input *input)
{
	for (I32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		input->keys[i].pressed = false;
		input->keys[i].released = false;
		input->keys[i].press_count = 0;
		input->keys[i].release_count = 0;
	}

	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
	{
		input->touches[i].pressed = false;
		input->touches[i].released = false;
		input->touches[i].dx = 0;
		input->touches[i].dy = 0;
	}

	input->mouse_dx = 0;
	input->mouse_dy = 0;
	input->mouse_wheel = 0.0f;
	_platform_ios_text_input_events_reset(input->text_input_events);
}

inline static String
_platform_ios_copy_path(NSString *path, bool trailing_slash, memory::Allocator *allocator)
{
	if (path == nil)
		return string_init(allocator);

	const char *path_data = [path fileSystemRepresentation];
	if (path_data == nullptr)
		return string_init(allocator);

	String result = string_from(path_data, allocator);
	if (trailing_slash && !string_is_empty(result) && result[result.count - 1] != '/')
		string_append(result, '/');
	return result;
}

inline static String
_platform_ios_search_path(NSSearchPathDirectory directory, const char *validation_message, memory::Allocator *allocator)
{
	@autoreleasepool
	{
		NSArray<NSString *> *paths = NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES);
		if ([paths count] > 0)
		{
			NSString *path = [paths objectAtIndex:0];
			NSFileManager *file_manager = [NSFileManager defaultManager];
			BOOL is_directory = NO;
			BOOL exists = [file_manager fileExistsAtPath:path isDirectory:&is_directory];
			BOOL ready = exists ? is_directory : [file_manager createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:nil];
			validate(ready, validation_message);
			if (!ready)
				return string_init(allocator);

			String result = _platform_ios_copy_path(path, true, allocator);
			validate(!string_is_empty(result), validation_message);
			return result;
		}
	}

	validate(false, validation_message);
	return string_init(allocator);
}

inline static NSString *
_platform_ios_resource_path(const String &path)
{
	NSBundle *bundle = [NSBundle mainBundle];
	NSURL *resource_url = bundle ? [bundle resourceURL] : nil;
	NSString *resource_root = resource_url ? [resource_url path] : nil;
	if (resource_root == nil)
		return nil;
	if (path.count == 0)
		return resource_root;

	NSString *relative_path = _platform_ios_ns_string(path);
	if (relative_path == nil)
		return nil;

	NSString *standardized_path = [relative_path stringByStandardizingPath];
	if ([standardized_path isAbsolutePath] ||
		[standardized_path isEqualToString:@".."] ||
		[standardized_path hasPrefix:@"../"])
		return nil;
	if ([standardized_path isEqualToString:@"."])
		return resource_root;
	return [resource_root stringByAppendingPathComponent:standardized_path];
}

inline static bool
_platform_ios_extension_matches(const String &file_name, const String &extension_filter)
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

inline static bool
_platform_ios_path_name_is_valid(const String &name)
{
	if (string_is_empty(name) || name == "." || name == "..")
		return false;

	for (U64 i = 0; i < name.count; ++i)
		if (name[i] == '\0' || name[i] == '/' || name[i] == '\\')
			return false;
	return true;
}

inline static String
_platform_ios_path_join(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (string_is_empty(directory) || string_is_empty(name))
		return string_init(allocator);

	String result = string_copy(directory, allocator);
	string_replace(result, '\\', '/');
	if (result[result.count - 1] != '/')
		string_append(result, '/');
	string_append(result, name);
	return result;
}

inline static String
_platform_ios_path_parent(const String &path, memory::Allocator *allocator)
{
	String result = string_copy(path, allocator);
	string_replace(result, '\\', '/');
	while (result.count > 1 && result[result.count - 1] == '/')
		string_resize(result, result.count - 1);

	U64 slash = string_find_last_of(result, '/');
	if (slash == U64(-1))
	{
		string_clear(result);
		string_append(result, '.');
	}
	else if (slash == 0)
	{
		string_resize(result, 1);
	}
	else
	{
		string_resize(result, slash);
	}
	return result;
}

inline static bool
_platform_ios_document_access_init(const String &token, Platform_IOS_Document_Access *access)
{
	access->url = [Platform_IOS_Document_Token copyURLFromToken:token];
	if (access->url == nil)
		return false;
	if (![access->url startAccessingSecurityScopedResource])
	{
		[access->url release];
		access->url = nil;
		return false;
	}
	return true;
}

inline static void
_platform_ios_document_access_deinit(Platform_IOS_Document_Access *access)
{
	if (access->url == nil)
		return;

	[access->url stopAccessingSecurityScopedResource];
	[access->url release];
	access->url = nil;
}

inline static String
_platform_ios_document_token_from_url(NSURL *url, memory::Allocator *allocator)
{
	if (url == nil || ![url startAccessingSecurityScopedResource])
		return string_init(allocator);
	DEFER([url stopAccessingSecurityScopedResource]);
	return [Platform_IOS_Document_Token encodeURL:url allocator:allocator];
}

inline static void
_platform_ios_file_dialog_complete(Platform_IOS_File_Dialog *dialog, NSURL *url)
{
	String token = {};
	if (url != nil && dialog->return_token)
		token = _platform_ios_document_token_from_url(url, memory::heap_allocator());
	bool accepted = url != nil && (!dialog->return_token || !string_is_empty(token));

	validate(::pthread_mutex_lock(&dialog->mutex) == 0, "[PLATFORM][IOS]: Failed to lock file-dialog state.");
	if (!dialog->completed)
	{
		string_deinit(dialog->token);
		dialog->token = token;
		token = {};
		dialog->accepted = accepted;
		dialog->completed = true;
		validate(::pthread_cond_signal(&dialog->condition) == 0, "[PLATFORM][IOS]: Failed to signal file-dialog completion.");
	}
	validate(::pthread_mutex_unlock(&dialog->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock file-dialog state.");
	string_deinit(token);
}

@implementation Platform_IOS_File_Dialog_Controller
+ (NSArray<NSString *> *)documentTypesFromFilters:(const char *)filters
{
	NSMutableArray<NSString *> *document_types = [NSMutableArray array];
	const char *filter = filters;
	while (filter != nullptr && *filter != '\0')
	{
		filter += ::strlen(filter) + 1;
		const char *patterns = filter;
		if (*patterns == '\0')
			break;

		const char *pattern = patterns;
		while (*pattern != '\0')
		{
			while (*pattern == ' ' || *pattern == '\t' || *pattern == ';' || *pattern == ',')
				++pattern;
			while (*pattern == '*' || *pattern == '.')
				++pattern;

			const char *extension = pattern;
			bool valid = true;
			while (*pattern != '\0' && *pattern != ' ' && *pattern != '\t' && *pattern != ';' && *pattern != ',')
			{
				if (*pattern == '*' || *pattern == '?' || *pattern == '/' || *pattern == '\\')
					valid = false;
				++pattern;
			}
			if (!valid || extension == pattern)
				continue;

			NSString *filename_extension = [[[NSString alloc]
				initWithBytes:extension
				length:(NSUInteger)(pattern - extension)
				encoding:NSUTF8StringEncoding] autorelease];
			if (filename_extension == nil)
				continue;

			CFStringRef document_type = UTTypeCreatePreferredIdentifierForTag(
				kUTTagClassFilenameExtension,
				(CFStringRef)filename_extension,
				kUTTypeData);
			if (document_type != nullptr)
			{
				NSString *type = (NSString *)document_type;
				if (![document_types containsObject:type])
					[document_types addObject:type];
				CFRelease(document_type);
			}
		}
		filter = patterns + ::strlen(patterns) + 1;
	}

	if ([document_types count] == 0)
		[document_types addObject:(NSString *)kUTTypeData];
	return document_types;
}

- (instancetype)initWithDocumentTypes:(NSArray<NSString *> *)document_types exportURL:(NSURL *)export_url dialog:(Platform_IOS_File_Dialog *)dialog
{
	if (export_url != nil)
	{
		if (@available(iOS 14.0, *))
			self = [super initForExportingURLs:@[export_url] asCopy:YES];
		else
			self = [super initWithURL:export_url inMode:UIDocumentPickerModeExportToService];
	}
	else
	{
		self = [super initWithDocumentTypes:document_types inMode:UIDocumentPickerModeOpen];
	}
	if (self != nil)
	{
		_dialog = dialog;
		[self setDelegate:self];
		[self setAllowsMultipleSelection:NO];
	}
	return self;
}

- (void)platformCompleteWithURL:(NSURL *)url
{
	Platform_IOS_File_Dialog *dialog = _dialog;
	if (dialog == nullptr)
		return;

	_dialog = nullptr;
	_platform_ios_file_dialog_complete(dialog, url);
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	(void)controller;
	[self platformCompleteWithURL:[urls firstObject]];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
	(void)controller;
	[self platformCompleteWithURL:nil];
}

- (void)presentationControllerDidDismiss:(UIPresentationController *)presentation_controller
{
	(void)presentation_controller;
	[self platformCompleteWithURL:nil];
}

- (void)dealloc
{
	[self platformCompleteWithURL:nil];
	[super dealloc];
}
@end

inline static bool
_platform_ios_document_stat(const String &token, struct stat *path_stat)
{
	Platform_IOS_Document_Access access = {};
	if (!_platform_ios_document_access_init(token, &access))
		return false;
	DEFER(_platform_ios_document_access_deinit(&access));

	__block struct stat coordinated_stat = {};
	__block bool succeeded = false;
	@autoreleasepool
	{
		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *error = nil;
		[coordinator coordinateReadingItemAtURL:access.url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
			const char *coordinated_path = [coordinated_url fileSystemRepresentation];
			if (coordinated_path != nullptr && ::stat(coordinated_path, &coordinated_stat) == 0)
				succeeded = true;
		}];
		[coordinator release];
		if (error != nil)
			succeeded = false;
	}

	if (succeeded)
		*path_stat = coordinated_stat;
	return succeeded;
}

inline static bool
_platform_ios_directory_entry_is_directory(const struct dirent *entry, const String &path)
{
	if (entry->d_type == DT_DIR)
		return true;
	if (entry->d_type != DT_UNKNOWN)
		return false;

	struct stat path_stat = {};
	return ::lstat(path.data, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

inline static I32
_platform_ios_file_descriptor_open(const char *path, Platform_File_Mode mode, I32 flags)
{
	I32 fd = -1;
	do
	{
		if (mode == PLATFORM_FILE_MODE_READ)
			fd = ::open(path, flags);
		else
			fd = ::open(path, flags, S_IRUSR | S_IWUSR);
	}
	while (fd == -1 && errno == EINTR);
	return fd;
}

inline static I64
_platform_ios_file_descriptor_read(I32 fd, void *data, U64 size)
{
	char *cursor = (char *)data;
	U64 bytes_read = 0;
	while (bytes_read < size)
	{
		U64 bytes_to_read = u64_min(size - bytes_read, (U64)SSIZE_MAX);
		I64 result = ::read(fd, cursor + bytes_read, (size_t)bytes_to_read);
		if (result == 0)
			break;
		if (result < 0)
		{
			if (errno == EINTR)
				continue;
			return bytes_read == 0 ? -1 : (I64)bytes_read;
		}
		bytes_read += (U64)result;
	}
	return (I64)bytes_read;
}

inline static I64
_platform_ios_file_descriptor_write(I32 fd, const void *data, U64 size)
{
	const char *cursor = (const char *)data;
	U64 bytes_written = 0;
	while (bytes_written < size)
	{
		U64 bytes_to_write = u64_min(size - bytes_written, (U64)SSIZE_MAX);
		I64 result = ::write(fd, cursor + bytes_written, (size_t)bytes_to_write);
		if (result == 0)
			break;
		if (result < 0)
		{
			if (errno == EINTR)
				continue;
			return bytes_written == 0 ? -1 : (I64)bytes_written;
		}
		bytes_written += (U64)result;
	}
	return (I64)bytes_written;
}

inline static I32
_platform_ios_file_url_open(NSURL *url, Platform_File_Mode mode, I32 flags)
{
	__block I32 fd = -1;
	@autoreleasepool
	{
		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *error = nil;
		if (mode == PLATFORM_FILE_MODE_READ)
		{
			[coordinator coordinateReadingItemAtURL:url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				const char *path = [coordinated_url fileSystemRepresentation];
				if (path != nullptr)
					fd = _platform_ios_file_descriptor_open(path, mode, flags);
			}];
		}
		else
		{
			[coordinator coordinateWritingItemAtURL:url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				const char *path = [coordinated_url fileSystemRepresentation];
				if (path != nullptr)
					fd = _platform_ios_file_descriptor_open(path, mode, flags);
			}];
		}
		[coordinator release];
	}
	return fd;
}

inline static void
_platform_ios_file_deinit(Platform_IOS_File *file)
{
	if (file == nullptr)
		return;

	if (file->fd >= 0)
		validate(::close(file->fd) == 0, "[PLATFORM][IOS]: Failed to close file handle.");
	if (file->security_access)
		[file->security_url stopAccessingSecurityScopedResource];
	[file->security_url release];
	memory::deallocate(file);
}

Platform_File_Handle
platform_file_open(const String &path, Platform_File_Mode mode)
{
	I32 flags = 0;
	switch (mode)
	{
		case PLATFORM_FILE_MODE_READ:       flags = O_RDONLY;                      break;
		case PLATFORM_FILE_MODE_WRITE:      flags = O_WRONLY | O_CREAT | O_TRUNC;  break;
		case PLATFORM_FILE_MODE_READ_WRITE: flags = O_RDWR   | O_CREAT;            break;
		case PLATFORM_FILE_MODE_APPEND:     flags = O_WRONLY | O_CREAT | O_APPEND; break;
		default:
			validate(false, "[PLATFORM][IOS]: File mode is not valid.");
			return PLATFORM_FILE_HANDLE_INVALID;
	}

	Platform_IOS_File *file = memory::allocate_zeroed<Platform_IOS_File>();
	file->fd = -1;
	bool opened = false;
	DEFER(if (!opened) _platform_ios_file_deinit(file));

	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		@autoreleasepool
		{
			file->security_url = [Platform_IOS_Document_Token copyURLFromToken:path];
			if (file->security_url != nil)
				file->security_access = [file->security_url startAccessingSecurityScopedResource];
			if (file->security_access)
				file->fd = _platform_ios_file_url_open(file->security_url, mode, flags);
		}
	}
	else
	{
		file->fd = _platform_ios_file_descriptor_open(path.data, mode, flags);
	}

	if (file->fd < 0)
		return PLATFORM_FILE_HANDLE_INVALID;
	opened = true;
	return (Platform_File_Handle)file;
}

void
platform_file_close(Platform_File_Handle handle)
{
	_platform_ios_file_deinit((Platform_IOS_File *)handle);
}

U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size)
{
	Platform_IOS_File *file = (Platform_IOS_File *)handle;
	__block I64 bytes_read = -1;
	if (file->security_url == nil)
	{
		bytes_read = _platform_ios_file_descriptor_read(file->fd, data, size);
	}
	else
	{
		@autoreleasepool
		{
			NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			NSError *error = nil;
			[coordinator coordinateReadingItemAtURL:file->security_url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				(void)coordinated_url;
				bytes_read = _platform_ios_file_descriptor_read(file->fd, data, size);
			}];
			[coordinator release];
		}
	}
	return bytes_read < 0 ? 0 : (U64)bytes_read;
}

U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size)
{
	Platform_IOS_File *file = (Platform_IOS_File *)handle;
	__block I64 bytes_written = -1;
	if (file->security_url == nil)
	{
		bytes_written = _platform_ios_file_descriptor_write(file->fd, data, size);
	}
	else
	{
		@autoreleasepool
		{
			NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			NSError *error = nil;
			[coordinator coordinateWritingItemAtURL:file->security_url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				(void)coordinated_url;
				bytes_written = _platform_ios_file_descriptor_write(file->fd, data, size);
			}];
			[coordinator release];
		}
	}
	return bytes_written < 0 ? 0 : (U64)bytes_written;
}

bool
platform_file_seek(Platform_File_Handle handle, I64 offset, Platform_File_Seek_Origin origin)
{
	I32 whence = SEEK_SET;
	switch (origin)
	{
		case PLATFORM_FILE_SEEK_ORIGIN_BEGIN:   whence = SEEK_SET; break;
		case PLATFORM_FILE_SEEK_ORIGIN_CURRENT: whence = SEEK_CUR; break;
		case PLATFORM_FILE_SEEK_ORIGIN_END:     whence = SEEK_END; break;
		default:
			validate(false, "[PLATFORM][IOS]: File seek origin is not valid.");
			return false;
	}
	Platform_IOS_File *file = (Platform_IOS_File *)handle;
	__block off_t result = (off_t)-1;
	if (file->security_url == nil)
	{
		result = ::lseek(file->fd, (off_t)offset, whence);
	}
	else
	{
		@autoreleasepool
		{
			NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			NSError *error = nil;
			[coordinator coordinateReadingItemAtURL:file->security_url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				(void)coordinated_url;
				result = ::lseek(file->fd, (off_t)offset, whence);
			}];
			[coordinator release];
		}
	}
	return result != (off_t)-1;
}

U64
platform_file_tell(Platform_File_Handle handle)
{
	Platform_IOS_File *file = (Platform_IOS_File *)handle;
	off_t offset = ::lseek(file->fd, 0, SEEK_CUR);
	return offset == (off_t)-1 ? 0 : (U64)offset;
}

U64
platform_file_size(Platform_File_Handle handle)
{
	Platform_IOS_File *file = (Platform_IOS_File *)handle;
	__block struct stat file_stat = {};
	__block I32 result = -1;
	if (file->security_url == nil)
	{
		result = ::fstat(file->fd, &file_stat);
	}
	else
	{
		@autoreleasepool
		{
			NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			NSError *error = nil;
			[coordinator coordinateReadingItemAtURL:file->security_url options:0 error:&error byAccessor:^(NSURL *coordinated_url) {
				(void)coordinated_url;
				result = ::fstat(file->fd, &file_stat);
			}];
			[coordinator release];
		}
	}
	if (result != 0 || file_stat.st_size <= 0)
		return 0;
	return (U64)file_stat.st_size;
}

bool
platform_path_is_valid(const String &path)
{
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		struct stat path_stat = {};
		return _platform_ios_document_stat(path, &path_stat);
	}

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0;
}

bool
platform_path_is_file(const String &path)
{
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		struct stat path_stat = {};
		return _platform_ios_document_stat(path, &path_stat) && S_ISREG(path_stat.st_mode);
	}

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISREG(path_stat.st_mode);
}

bool
platform_path_is_directory(const String &path)
{
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		struct stat path_stat = {};
		return _platform_ios_document_stat(path, &path_stat) && S_ISDIR(path_stat.st_mode);
	}

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

U64
platform_path_get_file_size(const String &path)
{
	struct stat file_stat = {};
	bool succeeded = string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX) ?
		_platform_ios_document_stat(path, &file_stat) :
		::stat(path.data, &file_stat) == 0;
	if (!succeeded || !S_ISREG(file_stat.st_mode) || file_stat.st_size <= 0)
		return 0;
	return (U64)file_stat.st_size;
}

String
platform_path_get_absolute(const String &path, memory::Allocator *allocator)
{
	if (string_is_empty(path))
		return string_init(allocator);
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return string_copy(path, allocator);

	char absolute_path[PATH_MAX] = {};
	if (::realpath(path.data, absolute_path) != nullptr)
		return string_from(absolute_path, allocator);

	if (path[0] == '/')
	{
		String result = string_copy(path, allocator);
		string_replace(result, '\\', '/');
		return result;
	}

	String result = platform_path_get_current_working_directory(allocator);
	if (string_is_empty(result))
		return result;
	if (result[result.count - 1] != '/')
		string_append(result, '/');
	string_append(result, path);
	string_replace(result, '\\', '/');
	return result;
}

String
platform_path_get_directory(const String &path, memory::Allocator *allocator)
{
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		struct stat path_stat = {};
		if (!_platform_ios_document_stat(path, &path_stat))
			return string_init(allocator);
		if (S_ISDIR(path_stat.st_mode))
			return string_copy(path, allocator);

		String result = string_init(allocator);
		{
			Platform_IOS_Document_Access access = {};
			if (!_platform_ios_document_access_init(path, &access))
				return result;
			DEFER(_platform_ios_document_access_deinit(&access));

			@autoreleasepool
			{
				string_deinit(result);
				result = _platform_ios_document_token_from_url([access.url URLByDeletingLastPathComponent], allocator);
			}
		}

		if (!string_is_empty(result))
		{
			Platform_IOS_Document_Access result_access = {};
			if (_platform_ios_document_access_init(result, &result_access))
			{
				_platform_ios_document_access_deinit(&result_access);
				return result;
			}
			string_deinit(result);
		}
		return string_init(allocator);
	}

	if (!platform_path_is_valid(path))
		return string_init(allocator);

	if (platform_path_is_directory(path))
	{
		String result = string_copy(path, allocator);
		string_replace(result, '\\', '/');
		return result;
	}
	return _platform_ios_path_parent(path, allocator);
}

String
platform_path_get_current_working_directory(memory::Allocator *allocator)
{
	char path[PATH_MAX] = {};
	char *result = ::getcwd(path, sizeof(path));
	validate(result == path, "[PLATFORM][IOS]: Failed to get current working directory.");
	return result == path ? string_from(path, allocator) : string_init(allocator);
}

String
platform_path_get_temp_directory(memory::Allocator *allocator)
{
	String result = {};
	@autoreleasepool
	{
		result = _platform_ios_copy_path(NSTemporaryDirectory(), true, allocator);
	}
	validate(!string_is_empty(result), "[PLATFORM][IOS]: Failed to get temporary directory.");
	return result;
}

String
platform_path_get_app_data_directory(memory::Allocator *allocator)
{
	return _platform_ios_search_path(NSApplicationSupportDirectory, "[PLATFORM][IOS]: Failed to get application support directory.", allocator);
}

String
platform_path_get_cache_directory(memory::Allocator *allocator)
{
	return _platform_ios_search_path(NSCachesDirectory, "[PLATFORM][IOS]: Failed to get cache directory.", allocator);
}

void
platform_path_set_current_working_directory(const String &path)
{
	validate(::chdir(path.data) == 0, "[PLATFORM][IOS]: Failed to set current working directory.");
}

String
platform_path_get_executable_path(memory::Allocator *allocator)
{
	String result = {};
	@autoreleasepool
	{
		NSBundle *bundle = [NSBundle mainBundle];
		NSURL *executable_url = bundle ? [bundle executableURL] : nil;
		result = _platform_ios_copy_path(executable_url ? [executable_url path] : nil, false, allocator);
	}
	validate(!string_is_empty(result), "[PLATFORM][IOS]: Failed to get executable path.");
	return result;
}

String
platform_path_get_current_module_path(memory::Allocator *allocator)
{
	Dl_info info = {};
	if (::dladdr((void *)&platform_path_get_current_module_path, &info) == 0 || info.dli_fname == nullptr)
		return string_init(allocator);

	char absolute_path[PATH_MAX] = {};
	if (::realpath(info.dli_fname, absolute_path) != nullptr)
		return string_from(absolute_path, allocator);
	return string_from(info.dli_fname, allocator);
}

String
platform_path_get_file_name(const String &path, memory::Allocator *allocator)
{
	if (string_is_empty(path))
		return string_init(allocator);
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		Platform_IOS_Document_Access access = {};
		if (!_platform_ios_document_access_init(path, &access))
			return string_init(allocator);
		DEFER(_platform_ios_document_access_deinit(&access));

		String result = {};
		@autoreleasepool
		{
			const char *file_name = [[access.url lastPathComponent] UTF8String];
			result = file_name != nullptr ? string_from(file_name, allocator) : string_init(allocator);
		}
		return result;
	}

	U64 slash = string_find_last_of(path, '/');
	U64 backslash = string_find_last_of(path, '\\');
	U64 separator = backslash != U64(-1) && (slash == U64(-1) || backslash > slash) ? backslash : slash;
	return string_from(path.data + (separator == U64(-1) ? 0 : separator + 1), path.data + path.count, allocator);
}

void
platform_set_current_directory()
{
	String executable_path = platform_path_get_executable_path(memory::temp_allocator());
	String executable_directory = platform_path_get_directory(executable_path, memory::temp_allocator());
	if (!string_is_empty(executable_directory))
		platform_path_set_current_working_directory(executable_directory);
}

String
platform_path_read_file(const String &path, memory::Allocator *allocator)
{
	String content = string_init(allocator);
	Platform_File_Handle file = platform_file_open(path, PLATFORM_FILE_MODE_READ);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return content;
	DEFER(platform_file_close(file));

	U64 file_size = platform_file_size(file);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);
	U64 bytes_read = platform_file_read(file, content.data, content.count);
	if (bytes_read != content.count)
		string_resize(content, bytes_read);
	return content;
}

U64
platform_path_write_file(const String &path, Memory_Block block)
{
	validate(block.data != nullptr || block.size == 0, "[PLATFORM][IOS]: Cannot write a non-empty file from null data.");
	if (block.data == nullptr && block.size > 0)
		return 0;

	Platform_File_Handle file = platform_file_open(path, PLATFORM_FILE_MODE_WRITE);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return 0;
	DEFER(platform_file_close(file));
	return platform_file_write(file, block.data, block.size);
}

inline static bool
_platform_ios_path_copy_file(const char *from, const char *to)
{
	I32 source_file = _platform_ios_file_descriptor_open(from, PLATFORM_FILE_MODE_READ, O_RDONLY);
	if (source_file == -1)
		return false;
	DEFER(validate(::close(source_file) == 0, "[PLATFORM][IOS]: Failed to close copied source file."));

	struct stat source_stat = {};
	if (::fstat(source_file, &source_stat) != 0 || !S_ISREG(source_stat.st_mode))
		return false;

	struct stat destination_stat = {};
	bool destination_existed = ::stat(to, &destination_stat) == 0;
	if (destination_existed &&
		source_stat.st_dev == destination_stat.st_dev &&
		source_stat.st_ino == destination_stat.st_ino)
		return false;

	I32 destination_file = _platform_ios_file_descriptor_open(to, PLATFORM_FILE_MODE_WRITE, O_WRONLY | O_CREAT | O_TRUNC);
	if (destination_file == -1)
		return false;
	bool copy_succeeded = false;
	DEFER({
		validate(::close(destination_file) == 0, "[PLATFORM][IOS]: Failed to close copied destination file.");
		if (!copy_succeeded && !destination_existed)
			::unlink(to);
	});

	char buffer[8192];
	while (true)
	{
		I64 bytes_read = _platform_ios_file_descriptor_read(source_file, buffer, sizeof(buffer));
		if (bytes_read == 0)
		{
			copy_succeeded = true;
			return true;
		}
		if (bytes_read < 0)
			return false;

		I64 bytes_written = _platform_ios_file_descriptor_write(destination_file, buffer, (U64)bytes_read);
		if (bytes_written != bytes_read)
			return false;
	}
}

bool
platform_path_copy_file(const String &from, const String &to)
{
	bool from_token = string_starts_with(from, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	bool to_token = string_starts_with(to, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	if (!from_token && !to_token)
		return _platform_ios_path_copy_file(from.data, to.data);

	Platform_IOS_Document_Access from_access = {};
	Platform_IOS_Document_Access to_access = {};
	if (from_token && !_platform_ios_document_access_init(from, &from_access))
		return false;
	DEFER(_platform_ios_document_access_deinit(&from_access));
	if (to_token && !_platform_ios_document_access_init(to, &to_access))
		return false;
	DEFER(_platform_ios_document_access_deinit(&to_access));

	__block bool copied = false;
	@autoreleasepool
	{
		NSString *from_path = from_token ? nil : _platform_ios_ns_string(from);
		NSString *to_path = to_token ? nil : _platform_ios_ns_string(to);
		NSURL *from_url = from_token ? from_access.url : (from_path != nil ? [NSURL fileURLWithPath:from_path isDirectory:NO] : nil);
		NSURL *to_url = to_token ? to_access.url : (to_path != nil ? [NSURL fileURLWithPath:to_path isDirectory:NO] : nil);
		if (from_url == nil || to_url == nil)
			return false;
		if ([from_url isEqual:to_url])
			return false;

		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *error = nil;
		[coordinator coordinateReadingItemAtURL:from_url options:0 writingItemAtURL:to_url options:0 error:&error byAccessor:^(NSURL *coordinated_from_url, NSURL *coordinated_to_url) {
			const char *coordinated_from = [coordinated_from_url fileSystemRepresentation];
			const char *coordinated_to = [coordinated_to_url fileSystemRepresentation];
			if (coordinated_from != nullptr && coordinated_to != nullptr)
				copied = _platform_ios_path_copy_file(coordinated_from, coordinated_to);
		}];
		[coordinator release];
		if (error != nil)
			copied = false;
	}
	return copied;
}

bool
platform_path_delete_file(const String &path)
{
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
	{
		Platform_IOS_Document_Access access = {};
		if (!_platform_ios_document_access_init(path, &access))
			return false;
		DEFER(_platform_ios_document_access_deinit(&access));

		__block bool deleted = false;
		@autoreleasepool
		{
			NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			NSError *error = nil;
			[coordinator coordinateWritingItemAtURL:access.url options:NSFileCoordinatorWritingForDeleting error:&error byAccessor:^(NSURL *coordinated_url) {
				const char *coordinated_path = [coordinated_url fileSystemRepresentation];
				if (coordinated_path == nullptr)
					return;

				struct stat path_stat = {};
				if (::stat(coordinated_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode))
					deleted = ::unlink(coordinated_path) == 0;
			}];
			[coordinator release];
			if (error != nil)
				deleted = false;
		}
		return deleted;
	}

	return ::unlink(path.data) == 0;
}

inline static void
_platform_ios_document_list_file(Array<String> &files, NSURL *file_url, const String &extension_filter, memory::Allocator *allocator)
{
	NSNumber *is_regular_file = nil;
	if (![file_url getResourceValue:&is_regular_file forKey:NSURLIsRegularFileKey error:nil] || ![is_regular_file boolValue])
		return;

	const char *file_name_data = [[file_url lastPathComponent] UTF8String];
	if (file_name_data == nullptr || !_platform_ios_extension_matches(string_literal(file_name_data), extension_filter))
		return;

	String file_token = _platform_ios_document_token_from_url(file_url, allocator);
	if (!string_is_empty(file_token))
		array_push(files, file_token);
	else
		string_deinit(file_token);
}

inline static Array<String>
_platform_ios_document_list_files(const String &directory, const String &extension_filter, bool recursive, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);
	Platform_IOS_Document_Access access = {};
	if (!_platform_ios_document_access_init(directory, &access))
		return files;
	DEFER(_platform_ios_document_access_deinit(&access));

	Array<String> *files_ref = &files;
	const String *extension_filter_ref = &extension_filter;
	__block bool succeeded = false;
	@autoreleasepool
	{
		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *coordination_error = nil;
		[coordinator coordinateReadingItemAtURL:access.url options:0 error:&coordination_error byAccessor:^(NSURL *coordinated_url) {
			NSFileManager *file_manager = [NSFileManager defaultManager];
			NSArray<NSURLResourceKey> *resource_keys = @[NSURLIsRegularFileKey];
			if (!recursive)
			{
				NSError *listing_error = nil;
				NSArray<NSURL *> *children = [file_manager contentsOfDirectoryAtURL:coordinated_url includingPropertiesForKeys:resource_keys options:0 error:&listing_error];
				if (children == nil || listing_error != nil)
					return;

				for (NSURL *child_url in children)
					_platform_ios_document_list_file(*files_ref, child_url, *extension_filter_ref, allocator);
				succeeded = true;
				return;
			}

			__block bool enumeration_succeeded = true;
			NSDirectoryEnumerator<NSURL *> *enumerator = [file_manager enumeratorAtURL:coordinated_url includingPropertiesForKeys:resource_keys options:0 errorHandler:^BOOL(NSURL *url, NSError *error) {
				(void)url;
				(void)error;
				enumeration_succeeded = false;
				return NO;
			}];
			if (enumerator == nil)
				return;

			for (NSURL *child_url in enumerator)
				_platform_ios_document_list_file(*files_ref, child_url, *extension_filter_ref, allocator);
			succeeded = enumeration_succeeded;
		}];
		[coordinator release];
		if (coordination_error != nil)
			succeeded = false;
	}

	if (!succeeded)
	{
		for (U64 i = 0; i < files.count; ++i)
			string_deinit(files[i]);
		array_deinit(files);
		return array_init<String>(allocator);
	}
	return files;
}

Array<String>
platform_path_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	if (string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_list_files(directory, extension_filter, false, allocator);

	Array<String> files = array_init<String>(allocator);
	if (!platform_path_is_directory(directory))
		return files;

	DIR *dir = ::opendir(directory.data);
	if (dir == nullptr)
		return files;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][IOS]: Failed to close directory."));

	while (true)
	{
		errno = 0;
		struct dirent *entry = ::readdir(dir);
		if (entry == nullptr)
		{
			validate(errno == 0, "[PLATFORM][IOS]: Failed to read directory.");
			break;
		}

		String file_name = string_literal(entry->d_name);
		if (file_name == "." || file_name == "..")
			continue;

		String child_path = _platform_ios_path_join(directory, file_name, memory::temp_allocator());
		if (_platform_ios_directory_entry_is_directory(entry, child_path) || !platform_path_is_file(child_path))
			continue;
		if (_platform_ios_extension_matches(file_name, extension_filter))
			array_push(files, string_copy(file_name, allocator));
	}
	return files;
}

inline static void
_platform_ios_path_list_files_recursive(Array<String> &files, const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	DIR *dir = ::opendir(directory.data);
	if (dir == nullptr)
		return;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][IOS]: Failed to close recursive directory."));

	while (true)
	{
		errno = 0;
		struct dirent *entry = ::readdir(dir);
		if (entry == nullptr)
		{
			validate(errno == 0, "[PLATFORM][IOS]: Failed to read recursive directory.");
			break;
		}

		String file_name = string_literal(entry->d_name);
		if (file_name == "." || file_name == "..")
			continue;

		String child_path = _platform_ios_path_join(directory, file_name, memory::temp_allocator());
		if (_platform_ios_directory_entry_is_directory(entry, child_path))
		{
			_platform_ios_path_list_files_recursive(files, child_path, extension_filter, allocator);
			continue;
		}
		if (platform_path_is_file(child_path) && _platform_ios_extension_matches(file_name, extension_filter))
			array_push(files, string_copy(child_path, allocator));
	}
}

Array<String>
platform_path_list_files_recursive(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	if (string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_list_files(directory, extension_filter, true, allocator);

	Array<String> files = array_init<String>(allocator);
	if (platform_path_is_directory(directory))
		_platform_ios_path_list_files_recursive(files, directory, extension_filter, allocator);
	return files;
}

inline static String
_platform_ios_document_create(const String &directory, const String &name, bool create_directory, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_IOS_Document_Access access = {};
	if (!_platform_ios_document_access_init(directory, &access))
		return result;
	DEFER(_platform_ios_document_access_deinit(&access));

	String *result_ref = &result;
	__block bool succeeded = false;
	@autoreleasepool
	{
		NSString *name_string = _platform_ios_ns_string(name);
		if (name_string == nil)
			return result;

		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *coordination_error = nil;
		[coordinator coordinateWritingItemAtURL:access.url options:0 error:&coordination_error byAccessor:^(NSURL *coordinated_directory_url) {
			const char *coordinated_directory = [coordinated_directory_url fileSystemRepresentation];
			struct stat directory_stat = {};
			if (coordinated_directory == nullptr || ::stat(coordinated_directory, &directory_stat) != 0 || !S_ISDIR(directory_stat.st_mode))
				return;

			NSURL *child_url = [coordinated_directory_url URLByAppendingPathComponent:name_string isDirectory:create_directory];
			const char *child_path = [child_url fileSystemRepresentation];
			if (child_path == nullptr)
				return;

			bool created = false;
			if (create_directory)
			{
				created = ::mkdir(child_path, S_IRWXU) == 0;
			}
			else
			{
				I32 file = _platform_ios_file_descriptor_open(child_path, PLATFORM_FILE_MODE_WRITE, O_WRONLY | O_CREAT | O_EXCL);
				if (file >= 0)
				{
					created = ::close(file) == 0;
					validate(created, "[PLATFORM][IOS]: Failed to close created document file.");
					if (!created)
						::unlink(child_path);
				}
			}
			if (!created)
				return;

			String child_token = _platform_ios_document_token_from_url(child_url, allocator);
			if (string_is_empty(child_token))
			{
				string_deinit(child_token);
				bool rolled_back = (create_directory ? ::rmdir(child_path) : ::unlink(child_path)) == 0;
				validate(rolled_back, "[PLATFORM][IOS]: Failed to roll back document creation without a result token.");
				return;
			}

			string_deinit(*result_ref);
			*result_ref = child_token;
			succeeded = true;
		}];
		[coordinator release];
		if (coordination_error != nil)
			succeeded = false;
	}

	if (!succeeded)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

inline static String
_platform_ios_document_rename(const String &path, const String &name, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_IOS_Document_Access access = {};
	if (!_platform_ios_document_access_init(path, &access))
		return result;
	DEFER(_platform_ios_document_access_deinit(&access));

	String *result_ref = &result;
	__block bool succeeded = false;
	@autoreleasepool
	{
		NSString *name_string = _platform_ios_ns_string(name);
		if (name_string == nil)
			return result;
		if ([[access.url lastPathComponent] isEqualToString:name_string])
		{
			string_deinit(result);
			return string_copy(path, allocator);
		}
		NSURL *destination_url = [[access.url URLByDeletingLastPathComponent] URLByAppendingPathComponent:name_string];
		if (destination_url == nil)
			return result;

		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *coordination_error = nil;
		[coordinator coordinateWritingItemAtURL:access.url options:NSFileCoordinatorWritingForMoving writingItemAtURL:destination_url options:0 error:&coordination_error byAccessor:^(NSURL *coordinated_url, NSURL *coordinated_destination_url) {
			NSError *move_error = nil;
			if (![[NSFileManager defaultManager] moveItemAtURL:coordinated_url toURL:coordinated_destination_url error:&move_error] || move_error != nil)
				return;

			String destination_token = _platform_ios_document_token_from_url(coordinated_destination_url, allocator);
			if (string_is_empty(destination_token))
			{
				string_deinit(destination_token);
				NSError *rollback_error = nil;
				bool rolled_back = [[NSFileManager defaultManager] moveItemAtURL:coordinated_destination_url toURL:coordinated_url error:&rollback_error] && rollback_error == nil;
				validate(rolled_back, "[PLATFORM][IOS]: Failed to roll back a document rename without a result token.");
				return;
			}

			string_deinit(*result_ref);
			*result_ref = destination_token;
			succeeded = true;
		}];
		[coordinator release];
		if (coordination_error != nil)
			succeeded = false;
	}

	if (!succeeded)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

inline static String
_platform_ios_document_move(const String &path, const String &directory, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	bool path_token = string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	bool directory_token = string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX);
	Platform_IOS_Document_Access path_access = {};
	Platform_IOS_Document_Access directory_access = {};
	if (path_token && !_platform_ios_document_access_init(path, &path_access))
		return result;
	DEFER(_platform_ios_document_access_deinit(&path_access));
	if (directory_token && !_platform_ios_document_access_init(directory, &directory_access))
		return result;
	DEFER(_platform_ios_document_access_deinit(&directory_access));

	String *result_ref = &result;
	__block bool succeeded = false;
	@autoreleasepool
	{
		NSString *path_string = path_token ? nil : _platform_ios_ns_string(path);
		NSString *directory_string = directory_token ? nil : _platform_ios_ns_string(directory);
		NSURL *source_url = path_token ? path_access.url : (path_string != nil ? [NSURL fileURLWithPath:path_string] : nil);
		NSURL *directory_url = directory_token ? directory_access.url : (directory_string != nil ? [NSURL fileURLWithPath:directory_string isDirectory:YES] : nil);
		NSString *file_name = [source_url lastPathComponent];
		NSURL *destination_url = file_name != nil ? [directory_url URLByAppendingPathComponent:file_name] : nil;
		if (source_url == nil || directory_url == nil || destination_url == nil)
			return result;
		if ([source_url isEqual:destination_url])
		{
			string_deinit(result);
			if (directory_token)
				return path_token ? string_copy(path, allocator) : _platform_ios_document_token_from_url(source_url, allocator);
			return _platform_ios_copy_path([source_url path], false, allocator);
		}

		NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
		NSError *coordination_error = nil;
		[coordinator coordinateWritingItemAtURL:source_url options:NSFileCoordinatorWritingForMoving writingItemAtURL:destination_url options:0 error:&coordination_error byAccessor:^(NSURL *coordinated_source_url, NSURL *coordinated_destination_url) {
			NSError *move_error = nil;
			if (![[NSFileManager defaultManager] moveItemAtURL:coordinated_source_url toURL:coordinated_destination_url error:&move_error] || move_error != nil)
				return;

			String destination_result = directory_token ?
				_platform_ios_document_token_from_url(coordinated_destination_url, allocator) :
				_platform_ios_copy_path([coordinated_destination_url path], false, allocator);
			if (string_is_empty(destination_result))
			{
				string_deinit(destination_result);
				NSError *rollback_error = nil;
				bool rolled_back = [[NSFileManager defaultManager] moveItemAtURL:coordinated_destination_url toURL:coordinated_source_url error:&rollback_error] && rollback_error == nil;
				validate(rolled_back, "[PLATFORM][IOS]: Failed to roll back a document move without a result path.");
				return;
			}

			string_deinit(*result_ref);
			*result_ref = destination_result;
			succeeded = true;
		}];
		[coordinator release];
		if (coordination_error != nil)
			succeeded = false;
	}

	if (!succeeded)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	return result;
}

String
platform_path_create_file(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (!_platform_ios_path_name_is_valid(name))
		return string_init(allocator);
	if (string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_create(directory, name, false, allocator);
	if (!platform_path_is_directory(directory))
		return string_init(allocator);

	String result = _platform_ios_path_join(directory, name, allocator);
	if (string_is_empty(result))
		return result;

	I32 file = -1;
	do
	{
		file = ::open(result.data, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	}
	while (file == -1 && errno == EINTR);
	if (file == -1)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	validate(::close(file) == 0, "[PLATFORM][IOS]: Failed to close created file.");
	return result;
}

String
platform_path_create_directory(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (!_platform_ios_path_name_is_valid(name))
		return string_init(allocator);
	if (string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_create(directory, name, true, allocator);
	if (!platform_path_is_directory(directory))
		return string_init(allocator);

	String result = _platform_ios_path_join(directory, name, allocator);
	if (string_is_empty(result))
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
	if (!_platform_ios_path_name_is_valid(name))
		return string_init(allocator);
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_rename(path, name, allocator);
	if (!platform_path_is_valid(path))
		return string_init(allocator);

	String directory = _platform_ios_path_parent(path, memory::temp_allocator());
	String result = _platform_ios_path_join(directory, name, allocator);
	if (string_is_empty(result))
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
	if (!platform_path_is_valid(path) || !platform_path_is_directory(directory))
		return string_init(allocator);
	if (string_starts_with(path, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX) || string_starts_with(directory, PLATFORM_IOS_DOCUMENT_TOKEN_PREFIX))
		return _platform_ios_document_move(path, directory, allocator);

	String normalized_path = string_copy(path, memory::temp_allocator());
	string_replace(normalized_path, '\\', '/');
	while (normalized_path.count > 1 && normalized_path[normalized_path.count - 1] == '/')
		string_resize(normalized_path, normalized_path.count - 1);

	String name = platform_path_get_file_name(normalized_path, memory::temp_allocator());
	String result = _platform_ios_path_join(directory, name, allocator);
	if (string_is_empty(result))
		return result;

	if (::rename(path.data, result.data) == 0)
		return result;
	if (errno != EXDEV || !platform_path_is_file(path))
	{
		string_deinit(result);
		return string_init(allocator);
	}

	bool destination_existed = platform_path_is_valid(result);
	if (platform_path_copy_file(path, result) && platform_path_delete_file(path))
		return result;

	if (!destination_existed && platform_path_is_file(result))
		platform_path_delete_file(result);
	string_deinit(result);
	return string_init(allocator);
}

String
platform_environment_variable_get(const String &name, memory::Allocator *allocator)
{
	if (string_is_empty(name))
		return string_init(allocator);

	const char *value = ::getenv(name.data);
	if (value == nullptr || value[0] == '\0')
		return string_init(allocator);
	return string_from(value, allocator);
}

String
platform_resource_read(const String &path, memory::Allocator *allocator)
{
	String content = string_init(allocator);
	if (path.count == 0)
		return content;

	@autoreleasepool
	{
		NSString *resource_path = _platform_ios_resource_path(path);
		if (resource_path == nil)
			return content;

		NSData *data = [NSData dataWithContentsOfFile:resource_path];
		if (data == nil)
			return content;

		U64 data_count = [data length];
		string_resize(content, data_count);
		if (data_count > 0)
			::memcpy(content.data, [data bytes], data_count);
	}
	return content;
}

Array<String>
platform_resource_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);

	@autoreleasepool
	{
		NSString *directory_path = _platform_ios_resource_path(directory);
		if (directory_path == nil)
			return files;

		NSFileManager *file_manager = [NSFileManager defaultManager];
		NSArray<NSString *> *file_names = [file_manager contentsOfDirectoryAtPath:directory_path error:nil];
		for (NSString *file_name_string in file_names)
		{
			NSString *file_path = [directory_path stringByAppendingPathComponent:file_name_string];
			BOOL is_directory = NO;
			if (![file_manager fileExistsAtPath:file_path isDirectory:&is_directory] || is_directory)
				continue;

			const char *file_name_data = [file_name_string fileSystemRepresentation];
			if (file_name_data == nullptr)
				continue;

			String file_name = string_literal(file_name_data);
			if (_platform_ios_extension_matches(file_name, extension_filter))
				array_push(files, string_copy(file_name, allocator));
		}
	}
	return files;
}

inline static String
_platform_ios_file_dialog_run(
	Platform_Window &window,
	PLATFORM_IOS_FILE_DIALOG_MODE mode,
	const char *filters,
	const String &export_path,
	bool *accepted,
	memory::Allocator *allocator)
{
	bool return_token = mode != PLATFORM_IOS_FILE_DIALOG_MODE_EXPORT;
	String result = return_token ? string_init(allocator) : String{};
	if (accepted != nullptr)
		*accepted = false;
	bool valid = window.ctx != nullptr && ![NSThread isMainThread];
	validate(valid, "[PLATFORM][IOS]: Document dialog requires an initialized window and a non-main calling thread.");
	if (!valid)
		return result;

	Platform_IOS_File_Dialog dialog = {};
	dialog.token = return_token ? string_init(memory::heap_allocator()) : String{};
	dialog.return_token = return_token;
	DEFER(string_deinit(dialog.token));

	I32 mutex_result = ::pthread_mutex_init(&dialog.mutex, nullptr);
	validate(mutex_result == 0, "[PLATFORM][IOS]: Failed to initialize file-dialog mutex.");
	if (mutex_result != 0)
		return result;
	DEFER(validate(::pthread_mutex_destroy(&dialog.mutex) == 0, "[PLATFORM][IOS]: Failed to destroy file-dialog mutex."));

	I32 condition_result = ::pthread_cond_init(&dialog.condition, nullptr);
	validate(condition_result == 0, "[PLATFORM][IOS]: Failed to initialize file-dialog condition.");
	if (condition_result != 0)
		return result;
	DEFER(validate(::pthread_cond_destroy(&dialog.condition) == 0, "[PLATFORM][IOS]: Failed to destroy file-dialog condition."));

	NSArray<NSString *> *document_types = nil;
	NSURL *export_url = nil;
	@autoreleasepool
	{
		switch (mode)
		{
			case PLATFORM_IOS_FILE_DIALOG_MODE_OPEN:
				document_types = [[Platform_IOS_File_Dialog_Controller documentTypesFromFilters:filters] retain];
				break;
			case PLATFORM_IOS_FILE_DIALOG_MODE_DIRECTORY:
				document_types = [[NSArray alloc] initWithObjects:(NSString *)kUTTypeFolder, nil];
				break;
			case PLATFORM_IOS_FILE_DIALOG_MODE_EXPORT:
			{
				NSString *path = _platform_ios_ns_string(export_path);
				if (path != nil)
					export_url = [[NSURL alloc] initFileURLWithPath:path isDirectory:NO];
				break;
			}
		}
	}
	bool picker_input_valid = mode == PLATFORM_IOS_FILE_DIALOG_MODE_EXPORT ? export_url != nil : document_types != nil;
	validate(picker_input_valid, "[PLATFORM][IOS]: Failed to initialize document-picker input.");
	if (!picker_input_valid)
	{
		[document_types release];
		[export_url release];
		return result;
	}

	Platform_Window_Context *context = window.ctx;
	Platform_IOS_View_Controller *presenter = nil;
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	if (context->connected && context->focused && !context->close_requested && context->view_controller != nil)
		presenter = [context->view_controller retain];
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");
	if (presenter == nil)
	{
		[document_types release];
		[export_url release];
		return result;
	}

	Platform_IOS_File_Dialog *dialog_ptr = &dialog;
	dispatch_async(dispatch_get_main_queue(), ^{
		@autoreleasepool
		{
			UIViewController *active_presenter = presenter;
			while ([active_presenter presentedViewController] != nil)
				active_presenter = [active_presenter presentedViewController];

			UIWindow *presentation_window = [[active_presenter viewIfLoaded] window];
			UIWindowScene *presentation_scene = [presentation_window windowScene];
			bool can_present =
				presentation_window != nil &&
				presentation_scene != nil &&
				[presentation_scene activationState] == UISceneActivationStateForegroundActive &&
				![active_presenter isBeingPresented] &&
				![active_presenter isBeingDismissed] &&
				![active_presenter isKindOfClass:[UIDocumentPickerViewController class]];
			if (!can_present)
			{
				_platform_ios_file_dialog_complete(dialog_ptr, nil);
			}
			else
			{
				Platform_IOS_File_Dialog_Controller *picker = [[Platform_IOS_File_Dialog_Controller alloc]
					initWithDocumentTypes:document_types
					exportURL:export_url
					dialog:dialog_ptr];
				if (picker == nil)
				{
					_platform_ios_file_dialog_complete(dialog_ptr, nil);
				}
				else
				{
					[active_presenter presentViewController:picker animated:YES completion:nil];
					[[picker presentationController] setDelegate:picker];
				}
				[picker release];
			}

			[document_types release];
			[export_url release];
			[presenter release];
		}
	});

	validate(::pthread_mutex_lock(&dialog.mutex) == 0, "[PLATFORM][IOS]: Failed to lock file-dialog state.");
	while (!dialog.completed)
		validate(::pthread_cond_wait(&dialog.condition, &dialog.mutex) == 0, "[PLATFORM][IOS]: Failed to wait for file-dialog completion.");
	if (!string_is_empty(dialog.token))
	{
		string_deinit(result);
		result = string_copy(dialog.token, allocator);
	}
	if (accepted != nullptr)
		*accepted = dialog.accepted;
	validate(::pthread_mutex_unlock(&dialog.mutex) == 0, "[PLATFORM][IOS]: Failed to unlock file-dialog state.");
	return result;
}

String
platform_file_dialog_open(Platform_Window &window, const char *filters, memory::Allocator *allocator)
{
	return _platform_ios_file_dialog_run(window, PLATFORM_IOS_FILE_DIALOG_MODE_OPEN, filters, {}, nullptr, allocator);
}

bool
platform_file_dialog_save(Platform_Window &window, const String &suggested_name, Slice<const U8> data, const char *filters)
{
	bool valid = window.ctx != nullptr && ![NSThread isMainThread];
	validate(valid, "[PLATFORM][IOS]: File save dialog requires an initialized window and a non-main calling thread.");
	validate(data.data != nullptr || data.count == 0, "[PLATFORM][IOS]: File save data is invalid.");
	if (!valid || (data.data == nullptr && data.count > 0))
		return false;

	String name = string_is_empty(suggested_name) ? string_literal("Untitled") : suggested_name;
	bool name_valid = _platform_ios_path_name_is_valid(name);
	validate(name_valid, "[PLATFORM][IOS]: File save suggested name must be a valid filename.");
	if (!name_valid)
		return false;

	String temp_directory = platform_path_get_temp_directory(memory::temp_allocator());
	String staging_directory = _platform_ios_path_join(temp_directory, string_literal("core-save-XXXXXX"), memory::temp_allocator());
	if (string_is_empty(staging_directory) || ::mkdtemp(staging_directory.data) == nullptr)
		return false;

	String staging_path = _platform_ios_path_join(staging_directory, name, memory::temp_allocator());
	DEFER({
		bool file_removed = ::unlink(staging_path.data) == 0;
		if (!file_removed)
			file_removed = errno == ENOENT;
		validate(file_removed, "[PLATFORM][IOS]: Failed to remove the staged save file.");

		bool directory_removed = ::rmdir(staging_directory.data) == 0;
		if (!directory_removed)
			directory_removed = errno == ENOENT;
		validate(directory_removed, "[PLATFORM][IOS]: Failed to remove the staged save directory.");
	});
	if (string_is_empty(staging_path))
		return false;

	I32 staging_file = _platform_ios_file_descriptor_open(staging_path.data, PLATFORM_FILE_MODE_WRITE, O_WRONLY | O_CREAT | O_EXCL);
	if (staging_file < 0)
		return false;

	I64 bytes_written = _platform_ios_file_descriptor_write(staging_file, data.data, data.count);
	I32 close_result = ::close(staging_file);
	if (bytes_written < 0 || (U64)bytes_written != data.count || close_result != 0)
		return false;

	bool accepted = false;
	(void)_platform_ios_file_dialog_run(
		window,
		PLATFORM_IOS_FILE_DIALOG_MODE_EXPORT,
		filters,
		staging_path,
		&accepted,
		memory::temp_allocator());
	return accepted;
}

String
platform_directory_dialog_open(Platform_Window &window, memory::Allocator *allocator)
{
	return _platform_ios_file_dialog_run(window, PLATFORM_IOS_FILE_DIALOG_MODE_DIRECTORY, nullptr, {}, nullptr, allocator);
}

inline static NSString *
_platform_ios_clipboard_type_from_media_type(const String &media_type)
{
	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || media_type == "text/plain")
		return @"public.utf8-plain-text";
	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG)
		return @"public.png";
	if (media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY)
		return @"public.data";
	if (media_type.count == 0 || media_type.data == nullptr)
		return nil;

	NSString *type = [[NSString alloc] initWithBytes:media_type.data length:media_type.count encoding:NSUTF8StringEncoding];
	return [type autorelease];
}

inline static String
_platform_ios_clipboard_media_type_from_type(NSString *type, memory::Allocator *allocator)
{
	if ([type isEqualToString:@"public.utf8-plain-text"] || [type isEqualToString:@"public.plain-text"])
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator);
	if ([type isEqualToString:@"public.png"])
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG, allocator);
	if ([type isEqualToString:@"public.data"])
		return string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY, allocator);

	const char *type_data = [type UTF8String];
	return type_data != nullptr ? string_from(type_data, allocator) : string_init(allocator);
}

inline static bool
_platform_ios_clipboard_media_type_exists(const Array<String> &media_types, const String &media_type)
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
	@autoreleasepool
	{
		for (NSString *type in [[UIPasteboard generalPasteboard] pasteboardTypes])
		{
			String media_type = _platform_ios_clipboard_media_type_from_type(type, memory::temp_allocator());
			if (media_type.count > 0 && !_platform_ios_clipboard_media_type_exists(media_types, media_type))
				array_push(media_types, string_copy(media_type, allocator));
		}
	}
	return media_types;
}

bool
platform_window_clipboard_item_read(Platform_Window &, const String &media_type, Array<U8> &data)
{
	validate(data.allocator != nullptr, "[PLATFORM][IOS]: Clipboard read requires initialized output data.");
	array_clear(data);

	@autoreleasepool
	{
		NSString *type = _platform_ios_clipboard_type_from_media_type(media_type);
		if (type == nil)
			return false;

		NSData *clipboard_data = [[UIPasteboard generalPasteboard] dataForPasteboardType:type];
		if (clipboard_data == nil)
			return false;

		array_resize(data, [clipboard_data length]);
		if (data.count > 0)
			::memcpy(data.data, [clipboard_data bytes], data.count);
	}
	return true;
}

bool
platform_window_clipboard_item_write(Platform_Window &, const Platform_Clipboard_Item_Desc *items, U32 item_count)
{
	if (items == nullptr || item_count == 0)
		return false;
	for (U32 i = 0; i < item_count; ++i)
		if (items[i].media_type.count == 0 || items[i].media_type.data == nullptr || (items[i].data.count > 0 && items[i].data.data == nullptr))
			return false;

	@autoreleasepool
	{
		NSMutableDictionary<NSString *, NSData *> *item = [NSMutableDictionary dictionaryWithCapacity:item_count];
		for (U32 i = 0; i < item_count; ++i)
		{
			NSString *type = _platform_ios_clipboard_type_from_media_type(items[i].media_type);
			if (type == nil)
				return false;

			NSData *item_data = items[i].data.count > 0 ?
				[NSData dataWithBytes:items[i].data.data length:items[i].data.count] :
				[NSData data];
			[item setObject:item_data forKey:type];
		}

		[[UIPasteboard generalPasteboard] setItems:@[item] options:@{}];
	}
	return true;
}

Platform_Api
platform_api_init(const char *filepath)
{
	(void)filepath;
	validate(false, "[PLATFORM][IOS]: Runtime API hot reload is not supported on iOS.");
	return Platform_Api{};
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self != nullptr)
		*self = {};
}

void *
platform_api_load(Platform_Api *self)
{
	(void)self;
	validate(false, "[PLATFORM][IOS]: Runtime API hot reload is not supported on iOS.");
	return nullptr;
}

inline static bool
_platform_ios_virtual_memory_block_is_valid(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	return page_size > 0 &&
		block.data != nullptr &&
		block.size > 0 &&
		(uintptr_t)block.data % page_size == 0 &&
		block.size % page_size == 0;
}

U64
platform_virtual_memory_get_page_size()
{
	long page_size = ::sysconf(_SC_PAGESIZE);
	bool valid = page_size > 0 && u64_is_power_of_two((U64)page_size);
	validate(valid, "[PLATFORM][IOS]: Failed to get a valid virtual-memory page size.");
	return valid ? (U64)page_size : 0;
}

U64
platform_virtual_memory_page_align(U64 size)
{
	if (size == 0)
		return 0;

	U64 page_size = platform_virtual_memory_get_page_size();
	if (page_size == 0)
		return 0;

	bool valid = size <= U64_MAX - (page_size - 1);
	validate(valid, "[PLATFORM][IOS]: Virtual-memory page alignment would overflow.");
	return valid ? u64_align_up(size, page_size) : 0;
}

Memory_Block
platform_virtual_memory_reserve(U64 size)
{
	U64 aligned_size = platform_virtual_memory_page_align(size);
	if (aligned_size == 0 || aligned_size > SIZE_MAX)
		return Memory_Block{};

	void *data = ::mmap(nullptr, (size_t)aligned_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (data == MAP_FAILED)
		return Memory_Block{};
	return Memory_Block{.data = data, .size = aligned_size};
}

bool
platform_virtual_memory_commit(Memory_Block block)
{
	bool valid = _platform_ios_virtual_memory_block_is_valid(block);
	validate(valid, "[PLATFORM][IOS]: Virtual-memory commit requires a non-empty page-aligned block.");
	return valid && ::mprotect(block.data, (size_t)block.size, PROT_READ | PROT_WRITE) == 0;
}

bool
platform_virtual_memory_decommit(Memory_Block block)
{
	bool valid = _platform_ios_virtual_memory_block_is_valid(block);
	validate(valid, "[PLATFORM][IOS]: Virtual-memory decommit requires a non-empty page-aligned block.");
	if (!valid)
		return false;

	I32 advise_result = ::madvise(block.data, (size_t)block.size, MADV_DONTNEED);
	I32 protect_result = ::mprotect(block.data, (size_t)block.size, PROT_NONE);
	return advise_result == 0 && protect_result == 0;
}

void
platform_virtual_memory_release(Memory_Block block)
{
	if (block.data == nullptr)
		return;

	bool valid = _platform_ios_virtual_memory_block_is_valid(block);
	validate(valid, "[PLATFORM][IOS]: Virtual-memory release requires a page-aligned block.");
	if (!valid)
		return;

	I32 result = ::munmap(block.data, (size_t)block.size);
	validate(result == 0, "[PLATFORM][IOS]: Failed to release virtual memory.");
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

inline static void *
_platform_thread_main_routine(void *thread)
{
	Platform_Thread *self = (Platform_Thread *)thread;
	if (!string_is_empty(self->name))
		platform_thread_set_current_name(self->name.data);
	self->function(self->data);
	return nullptr;
}

Platform_Thread *
platform_thread_init(Platform_Thread_Desc desc)
{
	validate(desc.function != nullptr, "[PLATFORM][IOS]: Thread function is not valid.");

	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->function = desc.function;
	self->data = desc.data;
	if (desc.name != nullptr)
		self->name = string_from(desc.name);
	validate(::pthread_create(&self->handle, nullptr, _platform_thread_main_routine, self) == 0, "[PLATFORM][IOS]: Failed to create thread.");
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

	validate(::pthread_join(self->handle, nullptr) == 0, "[PLATFORM][IOS]: Failed to join thread.");
	self->joined = true;
}

void
platform_thread_sleep(U32 milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	::nanosleep(&ts, nullptr);
}

void
platform_thread_set_current_name(const char *name)
{
	if (name == nullptr || name[0] == '\0')
		return;

	validate(::pthread_setname_np(name) == 0, "[PLATFORM][IOS]: Failed to set thread name.");
}

struct Platform_Mutex
{
	pthread_mutex_t handle;
};

Platform_Mutex *
platform_mutex_init()
{
	Platform_Mutex *self = memory::allocate_zeroed<Platform_Mutex>();
	validate(::pthread_mutex_init(&self->handle, nullptr) == 0, "[PLATFORM][IOS]: Failed to initialize mutex.");
	return self;
}

void
platform_mutex_deinit(Platform_Mutex *self)
{
	validate(::pthread_mutex_destroy(&self->handle) == 0, "[PLATFORM][IOS]: Failed to destroy mutex.");
	memory::deallocate(self);
}

void
platform_mutex_lock(Platform_Mutex *self)
{
	validate(::pthread_mutex_lock(&self->handle) == 0, "[PLATFORM][IOS]: Failed to lock mutex.");
}

void
platform_mutex_unlock(Platform_Mutex *self)
{
	validate(::pthread_mutex_unlock(&self->handle) == 0, "[PLATFORM][IOS]: Failed to unlock mutex.");
}

struct Platform_Condition_Variable
{
	pthread_cond_t handle;
};

Platform_Condition_Variable *
platform_condition_variable_init()
{
	Platform_Condition_Variable *self = memory::allocate_zeroed<Platform_Condition_Variable>();
	validate(::pthread_cond_init(&self->handle, nullptr) == 0, "[PLATFORM][IOS]: Failed to initialize condition variable.");
	return self;
}

void
platform_condition_variable_deinit(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_destroy(&self->handle) == 0, "[PLATFORM][IOS]: Failed to destroy condition variable.");
	memory::deallocate(self);
}

void
platform_condition_variable_wait(Platform_Condition_Variable *self, Platform_Mutex *mutex)
{
	validate(::pthread_cond_wait(&self->handle, &mutex->handle) == 0, "[PLATFORM][IOS]: Failed to wait for condition variable.");
}

void
platform_condition_variable_signal(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_signal(&self->handle) == 0, "[PLATFORM][IOS]: Failed to signal condition variable.");
}

void
platform_condition_variable_broadcast(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_broadcast(&self->handle) == 0, "[PLATFORM][IOS]: Failed to broadcast condition variable.");
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
	validate(::pthread_mutex_init(&self->mutex, nullptr) == 0, "[PLATFORM][IOS]: Failed to initialize semaphore mutex.");
	validate(::pthread_cond_init(&self->condition_variable, nullptr) == 0, "[PLATFORM][IOS]: Failed to initialize semaphore condition variable.");
	self->count = initial_count;
	return self;
}

void
platform_semaphore_deinit(Platform_Semaphore *self)
{
	validate(::pthread_cond_destroy(&self->condition_variable) == 0, "[PLATFORM][IOS]: Failed to destroy semaphore condition variable.");
	validate(::pthread_mutex_destroy(&self->mutex) == 0, "[PLATFORM][IOS]: Failed to destroy semaphore mutex.");
	memory::deallocate(self);
}

void
platform_semaphore_wait(Platform_Semaphore *self)
{
	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][IOS]: Failed to lock semaphore mutex.");
	while (self->count == 0)
		validate(::pthread_cond_wait(&self->condition_variable, &self->mutex) == 0, "[PLATFORM][IOS]: Failed to wait for semaphore condition variable.");
	--self->count;
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock semaphore mutex.");
}

void
platform_semaphore_signal(Platform_Semaphore *self, U32 count)
{
	if (count == 0)
		return;

	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][IOS]: Failed to lock semaphore mutex.");
	self->count += count;
	if (count == 1)
		validate(::pthread_cond_signal(&self->condition_variable) == 0, "[PLATFORM][IOS]: Failed to signal semaphore condition variable.");
	else
		validate(::pthread_cond_broadcast(&self->condition_variable) == 0, "[PLATFORM][IOS]: Failed to broadcast semaphore condition variable.");
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock semaphore mutex.");
}

Platform_Window
platform_window_init(Platform_Window_Desc)
{
	Platform_Window_Context *context = memory::allocate_zeroed<Platform_Window_Context>();
	I32 mutex_result = ::pthread_mutex_init(&context->mutex, nullptr);
	validate(mutex_result == 0, "[PLATFORM][IOS]: Failed to initialize window context mutex.");
	if (mutex_result != 0)
	{
		memory::deallocate(context);
		return Platform_Window{};
	}
	context->paused = true;
	context->text_input_events = array_init<Platform_Text_Input_Event>(memory::heap_allocator());

	Platform_Window self {
		.ctx = context,
		.width = 0,
		.height = 0,
		.metrics = {},
		.presentation = {},
		.input = {},
		.text_input = {},
		.close_requested = false,
		.focused = false,
		.started = false,
		.paused = true,
		.low_memory = false,
		.save_state_requested = false,
		.surface_valid = false,
		.surface_changed = false
	};
	self.input.text_input_events = array_init<Platform_Text_Input_Event>(memory::heap_allocator());
	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	if (self == nullptr)
		return;
	bool main_thread = [NSThread isMainThread];
	validate(main_thread, "[PLATFORM][IOS]: Window deinitialization must run on the main thread.");
	if (!main_thread)
		return;

	Platform_Window_Context *context = self->ctx;
	if (context != nullptr)
	{
		_platform_ios_scene_observers_deinit(context);
		platform_window_close(self);

		validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
		UIWindow *window = context->window;
		Platform_IOS_View_Controller *view_controller = context->view_controller;
		Platform_IOS_View *view = context->view;
		bool keep_screen_on = context->keep_screen_on;
		context->scene = nil;
		context->session = nil;
		context->window = nil;
		context->view_controller = nil;
		context->view = nil;
		context->connected = false;
		context->started = false;
		context->paused = true;
		context->focused = false;
		context->keep_screen_on = false;
		::memset(context->keys, 0, sizeof(context->keys));
		_platform_ios_touches_reset(context);
		context->mouse = {};
		_platform_ios_text_input_state_reset(context);
		validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

		if (view != nil)
			[view platformContextClear];
		if (keep_screen_on)
			_platform_ios_keep_screen_on_set(false);
		if ([window rootViewController] == view_controller)
			[window setRootViewController:nil];

		array_deinit(context->text_input_events);
		validate(::pthread_mutex_destroy(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to destroy window context mutex.");
		memory::deallocate(context);
	}

	self->ctx = nullptr;
	self->width = 0;
	self->height = 0;
	self->metrics = {};
	self->presentation = {};
	if (self->input.text_input_events.allocator != nullptr)
	{
		_platform_ios_text_input_events_reset(self->input.text_input_events);
		array_deinit(self->input.text_input_events);
	}
	self->input = {};
	self->text_input = {};
	self->close_requested = true;
	self->focused = false;
	self->started = false;
	self->paused = true;
	self->low_memory = false;
	self->save_state_requested = false;
	self->surface_valid = false;
	self->surface_changed = false;
}

bool
platform_window_poll(Platform_Window *self)
{
	if (self == nullptr || self->ctx == nullptr)
		return false;
	bool main_thread = [NSThread isMainThread];
	validate(main_thread, "[PLATFORM][IOS]: Window polling must run on the main thread.");
	if (!main_thread)
		return false;

	_platform_ios_input_reset_transitions(&self->input);

	Platform_Window_Context *context = self->ctx;
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	UIWindowScene *scene = context->scene;
	UIView *view = context->view;
	bool connected = context->connected;
	bool close_requested = context->close_requested;
	bool focused = context->focused;
	bool started = context->started;
	bool paused = context->paused;
	bool surface_changed = context->surface_changed;
	context->surface_changed = false;
	for (I32 i = 0; i < PLATFORM_KEY_COUNT; ++i)
	{
		Platform_Key_State *key = context->keys + i;
		self->input.keys[i] = *key;
		key->pressed = false;
		key->released = false;
		key->press_count = 0;
		key->release_count = 0;
	}
	self->input.mouse_x = context->mouse.x;
	self->input.mouse_y = context->mouse.y;
	self->input.mouse_dx = context->mouse.dx;
	self->input.mouse_dy = context->mouse.dy;
	self->input.mouse_wheel = context->mouse.wheel;
	context->mouse.dx = 0;
	context->mouse.dy = 0;
	context->mouse.wheel = 0.0f;
	for (U64 i = 0; i < context->text_input_events.count; ++i)
	{
		array_push(self->input.text_input_events, context->text_input_events[i]);
		context->text_input_events[i] = {};
	}
	array_clear(context->text_input_events);
	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
	{
		Platform_IOS_Touch *touch = context->touches + i;
		self->input.touches[i] = touch->state;
		touch->state.pressed = false;
		touch->state.released = false;
		touch->state.dx = 0;
		touch->state.dy = 0;
		if (!touch->state.down)
			*touch = {};
	}
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	U32 width = 0;
	U32 height = 0;
	Platform_Window_Metrics metrics = {};
	if (connected && scene != nil && view != nil)
	{
		CGRect bounds = [view bounds];
		width = bounds.size.width > 0.0 ? (U32)bounds.size.width : 0;
		height = bounds.size.height > 0.0 ? (U32)bounds.size.height : 0;
		metrics = _platform_ios_window_metrics(view, scene, width, height);
	}

	bool surface_valid = connected && !close_requested && width > 0 && height > 0;
	bool metrics_changed =
		self->metrics.content_rect.x != metrics.content_rect.x ||
		self->metrics.content_rect.y != metrics.content_rect.y ||
		self->metrics.content_rect.width != metrics.content_rect.width ||
		self->metrics.content_rect.height != metrics.content_rect.height ||
		self->metrics.safe_area.left != metrics.safe_area.left ||
		self->metrics.safe_area.top != metrics.safe_area.top ||
		self->metrics.safe_area.right != metrics.safe_area.right ||
		self->metrics.safe_area.bottom != metrics.safe_area.bottom ||
		self->metrics.density_scale != metrics.density_scale ||
		self->metrics.dpi_x != metrics.dpi_x ||
		self->metrics.dpi_y != metrics.dpi_y ||
		self->metrics.orientation != metrics.orientation;
	surface_changed = surface_changed || self->width != width || self->height != height || metrics_changed || self->surface_valid != surface_valid;

	self->width = width;
	self->height = height;
	self->metrics = metrics;
	self->close_requested = close_requested;
	self->focused = focused;
	self->started = started;
	self->paused = paused;
	self->low_memory = false;
	self->save_state_requested = false;
	self->surface_valid = surface_valid;
	self->surface_changed = surface_changed;
	return !close_requested;
}

Platform_Window_Native_Handles
platform_window_get_native_handles(Platform_Window *self)
{
	if (self == nullptr || self->ctx == nullptr)
		return Platform_Window_Native_Handles {};

	Platform_Window_Context *context = self->ctx;
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	DEFER(validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context."));

	if (!context->connected)
		return Platform_Window_Native_Handles{};

	return Platform_Window_Native_Handles {
		.window = context->window,
		.context = context->view
	};
}

void
platform_window_set_title(Platform_Window *self, const char *title)
{
	unused(self, title);
}

void
platform_window_close(Platform_Window *self)
{
	if (self == nullptr)
		return;

	Platform_Window_Context *context = self->ctx;
	if (context == nullptr)
	{
		self->close_requested = true;
		self->focused = false;
		self->started = false;
		self->paused = true;
		self->surface_valid = false;
		return;
	}

	UISceneSession *session = nil;
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	if (!context->close_requested)
	{
		context->close_requested = true;
		context->started = false;
		context->paused = true;
		context->focused = false;
		context->surface_changed = true;
		if (context->session != nil)
			session = [context->session retain];
	}
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	self->close_requested = true;
	self->focused = false;
	self->started = false;
	self->paused = true;
	self->surface_changed = self->surface_changed || self->surface_valid;
	self->surface_valid = false;
	_platform_ios_scene_session_destroy(session);
}

void
platform_window_presentation_set(Platform_Window &window, const Platform_Window_Presentation_Desc &desc)
{
	bool main_thread = [NSThread isMainThread];
	validate(main_thread, "[PLATFORM][IOS]: Window presentation must be updated on the main thread.");
	if (!main_thread)
		return;

	Platform_IOS_View_Controller *view_controller = nil;
	bool keep_screen_on_enable = false;
	bool keep_screen_on_disable = false;
	Platform_Window_Context *context = window.ctx;
	if (context != nullptr)
	{
		validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
		if (context->connected)
		{
			bool keep_screen_on = (desc.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0;
			keep_screen_on_enable = keep_screen_on && !context->keep_screen_on;
			keep_screen_on_disable = !keep_screen_on && context->keep_screen_on;
			context->keep_screen_on = keep_screen_on;
			context->surface_changed = true;
			view_controller = context->view_controller;
		}
		validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");
	}

	if (keep_screen_on_enable)
		_platform_ios_keep_screen_on_set(true);
	else if (keep_screen_on_disable)
		_platform_ios_keep_screen_on_set(false);
	if (view_controller != nil)
		[view_controller platformPresentationSet:desc.flags orientationPolicy:desc.orientation_policy];

	window.presentation = desc;
	window.surface_changed = true;
}

void
platform_window_text_input_set(Platform_Window &window, const Platform_Text_Input_Desc &desc)
{
	if (window.ctx == nullptr)
		return;

	bool main_thread = [NSThread isMainThread];
	bool storage_valid = desc.text.count == 0 || desc.text.data != nullptr;
	bool size_valid = desc.text.count <= UINT32_MAX;
	I32 action = (I32)desc.action;
	bool action_valid = action >= PLATFORM_TEXT_INPUT_ACTION_NONE && action <= PLATFORM_TEXT_INPUT_ACTION_PREVIOUS;
	bool ranges_valid =
		desc.selection_start <= desc.text.count &&
		desc.selection_end <= desc.text.count &&
		desc.composing_start <= desc.text.count &&
		desc.composing_end <= desc.text.count;
	bool boundaries_valid = false;
	if (storage_valid && size_valid && ranges_valid)
	{
		boundaries_valid =
			(desc.selection_start == desc.text.count || (((U8)desc.text.data[desc.selection_start] & 0xC0) != 0x80)) &&
			(desc.selection_end == desc.text.count || (((U8)desc.text.data[desc.selection_end] & 0xC0) != 0x80)) &&
			(desc.composing_start == desc.text.count || (((U8)desc.text.data[desc.composing_start] & 0xC0) != 0x80)) &&
			(desc.composing_end == desc.text.count || (((U8)desc.text.data[desc.composing_end] & 0xC0) != 0x80));
	}
	NSString *validated_text = desc.text.count > 0 && desc.text.data != nullptr && size_valid ?
		[[NSString alloc] initWithBytes:desc.text.data length:desc.text.count encoding:NSUTF8StringEncoding] :
		[@"" retain];
	DEFER([validated_text release]);
	bool valid =
		main_thread &&
		validated_text != nil &&
		storage_valid &&
		size_valid &&
		action_valid &&
		ranges_valid &&
		boundaries_valid;
	validate(valid, "[PLATFORM][IOS]: Text input state requires valid UTF-8 storage, ranges, and main-thread access.");
	if (!valid)
		return;

	Platform_Text_Input_Desc text_input = desc;
	window.text_input = text_input;
	window.text_input.text = {};

	Platform_Window_Context *context = window.ctx;
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	Platform_IOS_Text_Input_View *text_input_view = context->view != nil ? context->view->_text_input_view : nil;
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	if (text_input_view != nil)
	{
		[text_input_view platformTextInputSet:&text_input];
		return;
	}

	if (text_input.enabled)
	{
		String text = string_copy(text_input.text);
		if (context->text_input_text.allocator != nullptr)
			string_deinit(context->text_input_text);
		context->text_input_text = text;
		context->text_input = text_input;
		context->text_input.text = {};
	}
	else
	{
		_platform_ios_text_input_state_reset(context);
	}
}

extern "C" CORE_API PLATFORM_WINDOW_NATIVE_CONNECT_RESULT
platform_window_native_connect(Platform_Window *self, void *context, void *native_window)
{
	if (self == nullptr || self->ctx == nullptr)
		return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;

	bool main_thread = [NSThread isMainThread];
	UIWindowScene *window_scene = [(id)context isKindOfClass:[UIWindowScene class]] ? (UIWindowScene *)context : nil;
	UIWindow *window = [(id)native_window isKindOfClass:[UIWindow class]] ? (UIWindow *)native_window : nil;
	UISceneSession *session = window_scene ? [window_scene session] : nil;
	bool valid =
		main_thread &&
		window_scene != nil &&
		session != nil &&
		window != nil &&
		[window windowScene] == window_scene &&
		[window rootViewController] == nil;
	validate(valid, "[PLATFORM][IOS]: Scene connection requires a UIWindowScene and its UIWindow without a root view controller.");
	if (!valid)
		return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;

	Platform_Window_Context *ctx = self->ctx;
	validate(::pthread_mutex_lock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	bool available = !ctx->connected;
	validate(available, "[PLATFORM][IOS]: Platform window is already bound to a scene.");
	if (!available)
	{
		validate(::pthread_mutex_unlock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");
		return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;
	}
	validate(::pthread_mutex_unlock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	Platform_IOS_View_Controller *view_controller = [[Platform_IOS_View_Controller alloc] init];
	Platform_IOS_View *view = [[Platform_IOS_View alloc] initWithFrame:CGRectZero context:ctx];
	[view_controller setView:view];
	[view release];
	[window setRootViewController:view_controller];
	[view_controller release];
	bool view_valid = view != nil && [view_controller view] == view;
	validate(view_valid, "[PLATFORM][IOS]: Failed to create the Core root view.");
	if (!view_valid)
	{
		if (view != nil)
			[view platformContextClear];
		[window setRootViewController:nil];
		return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;
	}

	validate(::pthread_mutex_lock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
	ctx->scene = window_scene;
	ctx->session = session;
	ctx->window = window;
	ctx->view_controller = view_controller;
	ctx->view = view;
	ctx->connected = true;
	ctx->close_requested = false;
	ctx->surface_changed = true;
	ctx->keep_screen_on = (self->presentation.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0;
	_platform_ios_scene_activation_set_locked(ctx, [window_scene activationState]);
	bool started = ctx->started;
	bool paused = ctx->paused;
	bool focused = ctx->focused;
	validate(::pthread_mutex_unlock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");

	bool observers_initialized = _platform_ios_scene_observers_init(ctx, window_scene);
	validate(observers_initialized, "[PLATFORM][IOS]: Failed to observe UIWindowScene lifecycle.");
	if (!observers_initialized)
	{
		validate(::pthread_mutex_lock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to lock window context.");
		ctx->scene = nil;
		ctx->session = nil;
		ctx->window = nil;
		ctx->view_controller = nil;
		ctx->view = nil;
		ctx->connected = false;
		ctx->started = false;
		ctx->paused = true;
		ctx->focused = false;
		ctx->surface_changed = false;
		ctx->keep_screen_on = false;
		validate(::pthread_mutex_unlock(&ctx->mutex) == 0, "[PLATFORM][IOS]: Failed to unlock window context.");
		[view platformContextClear];
		[window setRootViewController:nil];
		return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;
	}

	[view_controller platformPresentationSet:self->presentation.flags orientationPolicy:self->presentation.orientation_policy];
	Platform_Text_Input_Desc text_input = ctx->text_input;
	text_input.text = ctx->text_input_text;
	[view->_text_input_view platformTextInputSet:&text_input];
	if ((self->presentation.flags & PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON) != 0)
		_platform_ios_keep_screen_on_set(true);

	self->close_requested = false;
	self->started = started;
	self->paused = paused;
	self->focused = focused;
	CGRect bounds = [view bounds];
	self->width = bounds.size.width > 0.0 ? (U32)bounds.size.width : 0;
	self->height = bounds.size.height > 0.0 ? (U32)bounds.size.height : 0;
	self->metrics = _platform_ios_window_metrics(view, window_scene, self->width, self->height);
	self->surface_valid = self->width > 0 && self->height > 0;
	self->surface_changed = true;
	return PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_CONNECTED;
}

U64
platform_query_microseconds()
{
	struct timespec time = {};
	I32 result = ::clock_gettime(CLOCK_MONOTONIC, &time);
	validate(result == 0, "[PLATFORM][IOS]: Failed to query monotonic time.");
	if (result != 0)
		return 0;
	return (U64)time.tv_sec * 1000000 + (U64)time.tv_nsec / 1000;
}

U32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	if (callstack == nullptr || frame_count == 0)
		return 0;

	bool valid = frame_count <= (U32)INT_MAX;
	validate(valid, "[PLATFORM][IOS]: Callstack frame count exceeds backtrace capacity.");
	if (!valid)
		return 0;

	::memset(callstack, 0, frame_count * sizeof(*callstack));
	I32 captured_count = ::backtrace(callstack, (I32)frame_count);
	return captured_count > 0 ? (U32)captured_count : 0;
#else
	return 0;
#endif
}

#if DEBUG
inline static void
_platform_ios_callstack_copy_string(char *dst, U64 dst_size, const char *src)
{
	if (dst_size == 0)
		return;

	U64 i = 0;
	if (src != nullptr)
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
	if (callstack == nullptr || frames == nullptr || frame_count == 0)
		return;

	bool valid = frame_count <= (U32)INT_MAX;
	validate(valid, "[PLATFORM][IOS]: Callstack frame count exceeds symbol-resolution capacity.");
	if (!valid)
		return;

	char **symbols = ::backtrace_symbols(callstack, (I32)frame_count);
	for (U32 i = 0; i < frame_count; ++i)
	{
		Platform_Callstack_Frame *frame = frames + i;
		*frame = {};
		frame->address = callstack[i];

		Dl_info info = {};
		if (::dladdr(callstack[i], &info) != 0)
		{
			if (info.dli_sname != nullptr)
			{
				frame->symbol_found = true;
				_platform_ios_callstack_copy_string(frame->symbol, PLATFORM_CALLSTACK_SYMBOL_LENGTH, info.dli_sname);
			}
			if (info.dli_fname != nullptr)
				_platform_ios_callstack_copy_string(frame->file, PLATFORM_CALLSTACK_FILE_LENGTH, info.dli_fname);
		}

		if (!frame->symbol_found && symbols != nullptr)
		{
			frame->symbol_found = true;
			_platform_ios_callstack_copy_string(frame->symbol, PLATFORM_CALLSTACK_SYMBOL_LENGTH, symbols[i]);
		}
	}
	if (symbols != nullptr)
		::free(symbols);
#endif
}