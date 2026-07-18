#include <core/defer.h>
#include <core/platform/platform.h>
#include <core/tester.h>

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <UIKit/UIKit.h>
#include <XCTest/XCTest.h>

extern "C" Platform_Window *
core_ios_test_window();

inline static void
_core_ios_metal_layer_resize(CAMetalLayer *layer, UIView *view)
{
	CGRect bounds = [view bounds];
	CGFloat scale = [[[view window] screen] scale];
	[layer setContentsScale:scale];
	[layer setFrame:bounds];
	[layer setDrawableSize:CGSizeMake(bounds.size.width * scale, bounds.size.height * scale)];
}

inline static bool
_core_ios_metal_render(CAMetalLayer *layer, id<MTLCommandQueue> command_queue, MTLClearColor clear_color)
{
	id<CAMetalDrawable> drawable = [layer nextDrawable];
	if (drawable == nil)
		return false;

	MTLRenderPassDescriptor *render_pass = [MTLRenderPassDescriptor renderPassDescriptor];
	MTLRenderPassColorAttachmentDescriptor *color = [[render_pass colorAttachments] objectAtIndexedSubscript:0];
	[color setTexture:[drawable texture]];
	[color setLoadAction:MTLLoadActionClear];
	[color setStoreAction:MTLStoreActionStore];
	[color setClearColor:clear_color];

	id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
	if (command_buffer == nil)
		return false;
	id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass];
	if (encoder == nil)
		return false;

	[encoder endEncoding];
	[command_buffer presentDrawable:drawable];
	[command_buffer commit];
	[command_buffer waitUntilCompleted];
	return [command_buffer status] == MTLCommandBufferStatusCompleted && [command_buffer error] == nil;
}

inline static void
_core_ios_wait_for_orientation(XCTestCase *test, UIView *view, bool landscape)
{
	NSPredicate *predicate = [NSPredicate predicateWithBlock:^BOOL(id object, NSDictionary *) {
		CGRect bounds = [(UIView *)object bounds];
		return landscape ? bounds.size.width > bounds.size.height : bounds.size.height > bounds.size.width;
	}];
	[test expectationForPredicate:predicate evaluatedWithObject:view handler:nil];
	[test waitForExpectationsWithTimeout:10.0 handler:nil];
}

@interface Platform_IOS_Document_Token : NSObject
+ (String)encodeBookmarkData:(NSData *)bookmark_data allocator:(memory::Allocator *)allocator;
+ (String)encodeURL:(NSURL *)url allocator:(memory::Allocator *)allocator;
+ (NSData *)copyBookmarkDataFromToken:(const String &)token;
+ (NSURL *)copyURLFromToken:(const String &)token;
@end

@interface Platform_IOS_File_Dialog_Controller : UIDocumentPickerViewController
+ (NSArray<NSString *> *)documentTypesFromFilters:(const char *)filters;
@end

@interface UIView (Core_IOSTest_Input)
- (BOOL)platformKeyUsage:(UIKeyboardHIDUsage)usage down:(BOOL)down API_AVAILABLE(ios(13.4));
- (void)platformMousePosition:(CGPoint)location;
- (void)platformMouseButtons:(UIEventButtonMask)buttonMask API_AVAILABLE(ios(13.4));
- (void)platformMouseScroll:(CGFloat)delta;
- (void)platformTouchBegan:(UITouch *)touch location:(CGPoint)location;
- (void)platformTouchMoved:(UITouch *)touch location:(CGPoint)location;
- (void)platformTouchEnded:(UITouch *)touch location:(CGPoint)location;
@end

@interface Core_IOSTests : XCTestCase
@end

@implementation Core_IOSTests
- (void)
testDocumentTokenEncoding
{
	const U8 bookmark_bytes[] = {0xFB, 0xFF, 0x00, 0x01, 0x02};
	NSData *bookmark_data = [NSData dataWithBytes:bookmark_bytes length:sizeof(bookmark_bytes)];
	String token = [Platform_IOS_Document_Token encodeBookmarkData:bookmark_data allocator:memory::heap_allocator()];
	XCTAssertTrue(token == "core-document://v1/-_8AAQI");

	NSData *decoded_data = [Platform_IOS_Document_Token copyBookmarkDataFromToken:token];
	XCTAssertNotNil(decoded_data);
	XCTAssertEqual([decoded_data length], sizeof(bookmark_bytes));
	if (decoded_data != nil && [decoded_data length] == sizeof(bookmark_bytes))
	{
		const U8 *decoded_bytes = (const U8 *)[decoded_data bytes];
		for (U64 i = 0; i < count_of(bookmark_bytes); ++i)
			XCTAssertEqual(decoded_bytes[i], bookmark_bytes[i]);
	}
	[decoded_data release];
	NSURL *invalid_bookmark_url = [Platform_IOS_Document_Token copyURLFromToken:token];
	XCTAssertNil(invalid_bookmark_url);
	[invalid_bookmark_url release];

	const char *invalid_tokens[] = {
		"core-document://v1/",
		"core-document://v1/A",
		"core-document://v1/AB",
		"core-document://v1/AA=",
		"core-document://v1/AA+",
		"core-document://v2/AA"
	};
	for (U64 i = 0; i < count_of(invalid_tokens); ++i)
	{
		NSData *invalid_data = [Platform_IOS_Document_Token copyBookmarkDataFromToken:string_literal(invalid_tokens[i])];
		XCTAssertNil(invalid_data);
		[invalid_data release];
	}

	NSString *bookmark_path = [NSTemporaryDirectory() stringByAppendingPathComponent:@"core-document-token-test.bin"];
	NSURL *bookmark_url = [NSURL fileURLWithPath:bookmark_path isDirectory:NO];
	XCTAssertTrue([bookmark_data writeToURL:bookmark_url atomically:YES]);
	NSError *bookmark_error = nil;
	NSData *file_bookmark = [bookmark_url bookmarkDataWithOptions:NSURLBookmarkCreationMinimalBookmark includingResourceValuesForKeys:nil relativeToURL:nil error:&bookmark_error];
	XCTAssertNotNil(file_bookmark);
	XCTAssertNil(bookmark_error);
	if (file_bookmark != nil)
	{
		String file_token = [Platform_IOS_Document_Token encodeBookmarkData:file_bookmark allocator:memory::heap_allocator()];
		NSURL *resolved_url = [Platform_IOS_Document_Token copyURLFromToken:file_token];
		XCTAssertNotNil(resolved_url);
		XCTAssertEqualObjects([resolved_url URLByStandardizingPath], [bookmark_url URLByStandardizingPath]);
		[resolved_url release];
		string_deinit(file_token);
	}
	String url_token = [Platform_IOS_Document_Token encodeURL:bookmark_url allocator:memory::heap_allocator()];
	XCTAssertFalse(string_is_empty(url_token));
	NSURL *url_token_url = [Platform_IOS_Document_Token copyURLFromToken:url_token];
	XCTAssertNotNil(url_token_url);
	XCTAssertEqualObjects([url_token_url URLByStandardizingPath], [bookmark_url URLByStandardizingPath]);
	[url_token_url release];
	string_deinit(url_token);
	XCTAssertTrue([[NSFileManager defaultManager] removeItemAtURL:bookmark_url error:nil]);
	string_deinit(token);
}

- (void)
testDocumentDialogExportsAndFilters
{
	NSArray<NSString *> *image_types = [Platform_IOS_File_Dialog_Controller
		documentTypesFromFilters:"Images (*.png;*.jpg)\0*.png;*.jpg;*.png\0"];
	XCTAssertEqual([image_types count], (NSUInteger)2);
	XCTAssertTrue([image_types containsObject:@"public.png"]);
	XCTAssertTrue([image_types containsObject:@"public.jpeg"]);

	NSArray<NSString *> *all_types = [Platform_IOS_File_Dialog_Controller
		documentTypesFromFilters:"All files (*.*)\0*.*\0"];
	XCTAssertEqual([all_types count], (NSUInteger)1);
	XCTAssertEqualObjects([all_types firstObject], @"public.data");

	NSArray<NSString *> *unfiltered_types = [Platform_IOS_File_Dialog_Controller documentTypesFromFilters:nullptr];
	XCTAssertEqual([unfiltered_types count], (NSUInteger)1);
	XCTAssertEqualObjects([unfiltered_types firstObject], @"public.data");

	auto open_dialog = &platform_file_dialog_open;
	XCTAssertTrue(open_dialog != nullptr);
	auto directory_dialog = &platform_directory_dialog_open;
	XCTAssertTrue(directory_dialog != nullptr);
	bool (*save_dialog)(Platform_Window &, const String &, Slice<const U8>, const char *) = &platform_file_dialog_save;
	XCTAssertTrue(save_dialog != nullptr);
}

- (void)
testBundledResource
{
	String resource = platform_resource_read("platform_resource.txt", memory::temp_allocator());
	XCTAssertTrue(resource == "core-ios-resource");
}

- (void)
testSceneLifecycle
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	XCTAssertTrue(window->ctx != nullptr);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(window->surface_valid);
	XCTAssertTrue(window->surface_changed);

	UIScene *scene = [[[UIApplication sharedApplication] connectedScenes] anyObject];
	XCTAssertTrue(scene != nil);
	if (scene == nil)
		return;
	UIWindowScene *window_scene = (UIWindowScene *)scene;
	UIWindow *ui_window = [[window_scene windows] firstObject];
	UIView *view = [[ui_window rootViewController] view];
	XCTAssertNotNil(ui_window);
	XCTAssertNotNil(view);
	if (ui_window == nil || view == nil)
		return;
	Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
	XCTAssertEqual(native_handles.window, (void *)ui_window);
	XCTAssertEqual(native_handles.context, (void *)view);
	CGRect bounds = [view bounds];
	UIEdgeInsets insets = [view safeAreaInsets];
	XCTAssertEqual(window->width, bounds.size.width > 0.0 ? (U32)bounds.size.width : 0);
	XCTAssertEqual(window->height, bounds.size.height > 0.0 ? (U32)bounds.size.height : 0);
	XCTAssertEqual(window->metrics.safe_area.left, insets.left > 0.0 ? (U32)insets.left : 0);
	XCTAssertEqual(window->metrics.safe_area.top, insets.top > 0.0 ? (U32)insets.top : 0);
	XCTAssertEqual(window->metrics.safe_area.right, insets.right > 0.0 ? (U32)insets.right : 0);
	XCTAssertEqual(window->metrics.safe_area.bottom, insets.bottom > 0.0 ? (U32)insets.bottom : 0);
	XCTAssertEqualWithAccuracy(window->metrics.density_scale, (F32)[[window_scene screen] scale], 0.001f);
	XCTAssertEqualWithAccuracy(window->metrics.dpi_x, 160.0f * window->metrics.density_scale, 0.001f);
	XCTAssertEqualWithAccuracy(window->metrics.dpi_y, 160.0f * window->metrics.density_scale, 0.001f);
	XCTAssertEqual(window->metrics.content_rect.x, (I32)window->metrics.safe_area.left);
	XCTAssertEqual(window->metrics.content_rect.y, (I32)window->metrics.safe_area.bottom);
	XCTAssertEqual(window->metrics.content_rect.width + window->metrics.safe_area.left + window->metrics.safe_area.right, window->width);
	XCTAssertEqual(window->metrics.content_rect.height + window->metrics.safe_area.top + window->metrics.safe_area.bottom, window->height);
	XCTAssertEqual(window->metrics.orientation, window->width >= window->height ? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE : PLATFORM_WINDOW_ORIENTATION_PORTRAIT);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertFalse(window->surface_changed);

	switch ([scene activationState])
	{
		case UISceneActivationStateForegroundActive:
		{
			XCTAssertTrue(window->started);
			XCTAssertFalse(window->paused);
			XCTAssertTrue(window->focused);
			break;
		}
		case UISceneActivationStateForegroundInactive:
		{
			XCTAssertTrue(window->started);
			XCTAssertTrue(window->paused);
			XCTAssertFalse(window->focused);
			break;
		}
		case UISceneActivationStateBackground:
		case UISceneActivationStateUnattached:
		{
			XCTAssertFalse(window->started);
			XCTAssertTrue(window->paused);
			XCTAssertFalse(window->focused);
			break;
		}
	}
}

- (void)
testTouchInput
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
	UIView *view = (UIView *)native_handles.context;
	XCTAssertNotNil(view);
	XCTAssertTrue([view isMultipleTouchEnabled]);
	if (view == nil)
		return;

	NSObject *touch_token = [[NSObject alloc] init];
	UITouch *touch = (UITouch *)touch_token;
	U32 height = (U32)[view bounds].size.height;
	[view platformTouchBegan:touch location:CGPointMake(12.0, 18.0)];
	XCTAssertTrue(platform_window_poll(window));
	Platform_Touch_State *state = window->input.touches;
	XCTAssertEqual(state->id, 0);
	XCTAssertEqual(state->x, 12);
	XCTAssertEqual(state->y, (I32)height - 19);
	XCTAssertEqual(state->dx, 0);
	XCTAssertEqual(state->dy, 0);
	XCTAssertTrue(state->pressed);
	XCTAssertFalse(state->released);
	XCTAssertTrue(state->down);

	height = (U32)[view bounds].size.height;
	I32 previous_y = state->y;
	[view platformTouchMoved:touch location:CGPointMake(17.0, 24.0)];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(state->x, 17);
	XCTAssertEqual(state->y, (I32)height - 25);
	XCTAssertEqual(state->dx, 5);
	XCTAssertEqual(state->dy, state->y - previous_y);
	XCTAssertFalse(state->pressed);
	XCTAssertFalse(state->released);
	XCTAssertTrue(state->down);

	height = (U32)[view bounds].size.height;
	previous_y = state->y;
	[view platformTouchEnded:touch location:CGPointMake(20.0, 25.0)];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(state->x, 20);
	XCTAssertEqual(state->y, (I32)height - 26);
	XCTAssertEqual(state->dx, 3);
	XCTAssertEqual(state->dy, state->y - previous_y);
	XCTAssertFalse(state->pressed);
	XCTAssertTrue(state->released);
	XCTAssertFalse(state->down);

	XCTAssertTrue(platform_window_poll(window));
	XCTAssertFalse(state->pressed);
	XCTAssertFalse(state->released);
	XCTAssertFalse(state->down);
	[touch_token release];

	touch_token = [[NSObject alloc] init];
	touch = (UITouch *)touch_token;
	[view platformTouchBegan:touch location:CGPointMake(30.0, 30.0)];
	[view platformTouchEnded:touch location:CGPointMake(31.0, 31.0)];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(state->pressed);
	XCTAssertTrue(state->released);
	XCTAssertFalse(state->down);
	XCTAssertEqual(state->dx, 1);
	XCTAssertEqual(state->dy, -1);
	[touch_token release];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertFalse(state->pressed);
	XCTAssertFalse(state->released);
}

- (void)
testConsumerMetalRendering
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(window->surface_valid);
	XCTAssertTrue(window->started);
	XCTAssertFalse(window->paused);
	XCTAssertTrue(window->focused);
	if (!window->surface_valid || !window->started || window->paused || !window->focused)
		return;

	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	XCTAssertNotNil(device);
	if (device == nil)
		return;
	id<MTLCommandQueue> command_queue = [device newCommandQueue];
	XCTAssertNotNil(command_queue);
	if (command_queue == nil)
		return;
	DEFER([command_queue release]);

	Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
	UIView *view = (UIView *)native_handles.context;
	XCTAssertNotNil(view);
	if (view == nil)
		return;

	CAMetalLayer *metal_layer = [[CAMetalLayer alloc] init];
	XCTAssertNotNil(metal_layer);
	if (metal_layer == nil)
		return;
	DEFER([metal_layer removeFromSuperlayer]; [metal_layer release]);
	[metal_layer setDevice:device];
	[metal_layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
	[metal_layer setFramebufferOnly:YES];
	_core_ios_metal_layer_resize(metal_layer, view);
	[[view layer] addSublayer:metal_layer];
	XCTAssertTrue(_core_ios_metal_render(metal_layer, command_queue, MTLClearColorMake(0.10, 0.20, 0.30, 1.0)));

	Platform_Window_Presentation_Desc original_presentation = window->presentation;
	bool original_landscape = [view bounds].size.width > [view bounds].size.height;
	bool rotated_landscape = !original_landscape;
	Platform_Window_Presentation_Desc rotated_presentation = original_presentation;
	rotated_presentation.orientation_policy = rotated_landscape
		? PLATFORM_WINDOW_ORIENTATION_POLICY_LANDSCAPE
		: PLATFORM_WINDOW_ORIENTATION_POLICY_PORTRAIT;
	platform_window_presentation_set(*window, rotated_presentation);
	_core_ios_wait_for_orientation(self, view, rotated_landscape);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(window->surface_changed);
	XCTAssertEqual(window->metrics.orientation, rotated_landscape
		? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE
		: PLATFORM_WINDOW_ORIENTATION_PORTRAIT);

	native_handles = platform_window_get_native_handles(window);
	view = (UIView *)native_handles.context;
	XCTAssertEqual([metal_layer superlayer], [view layer]);
	_core_ios_metal_layer_resize(metal_layer, view);
	XCTAssertTrue(_core_ios_metal_render(metal_layer, command_queue, MTLClearColorMake(0.30, 0.20, 0.10, 1.0)));

	UIWindowScene *scene = [[view window] windowScene];
	XCTAssertNotNil(scene);
	NSNotificationCenter *notification_center = [NSNotificationCenter defaultCenter];
	[notification_center postNotificationName:UISceneWillDeactivateNotification object:scene];
	[notification_center postNotificationName:UISceneDidEnterBackgroundNotification object:scene];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertFalse(window->started);
	XCTAssertTrue(window->paused);
	XCTAssertFalse(window->focused);

	native_handles = platform_window_get_native_handles(window);
	view = (UIView *)native_handles.context;
	scene = [[view window] windowScene];
	XCTAssertNotNil(scene);
	[notification_center postNotificationName:UISceneWillEnterForegroundNotification object:scene];
	[notification_center postNotificationName:UISceneDidActivateNotification object:scene];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(window->started);
	XCTAssertFalse(window->paused);
	XCTAssertTrue(window->focused);
	native_handles = platform_window_get_native_handles(window);
	view = (UIView *)native_handles.context;
	XCTAssertEqual([metal_layer superlayer], [view layer]);
	_core_ios_metal_layer_resize(metal_layer, view);
	XCTAssertTrue(_core_ios_metal_render(metal_layer, command_queue, MTLClearColorMake(0.20, 0.30, 0.10, 1.0)));

	Platform_Window_Presentation_Desc restored_orientation = original_presentation;
	restored_orientation.orientation_policy = original_landscape
		? PLATFORM_WINDOW_ORIENTATION_POLICY_LANDSCAPE
		: PLATFORM_WINDOW_ORIENTATION_POLICY_PORTRAIT;
	platform_window_presentation_set(*window, restored_orientation);
	_core_ios_wait_for_orientation(self, view, original_landscape);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertTrue(window->surface_changed);
	native_handles = platform_window_get_native_handles(window);
	view = (UIView *)native_handles.context;
	XCTAssertEqual([metal_layer superlayer], [view layer]);
	_core_ios_metal_layer_resize(metal_layer, view);
	XCTAssertTrue(_core_ios_metal_render(metal_layer, command_queue, MTLClearColorMake(0.10, 0.30, 0.20, 1.0)));
	platform_window_presentation_set(*window, original_presentation);
}

- (void)
testMouseInput
{
	if (@available(iOS 13.4, *))
	{
		Platform_Window *window = core_ios_test_window();
		XCTAssertTrue(window != nullptr);
		if (window == nullptr)
			return;

		Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
		UIView *view = (UIView *)native_handles.context;
		XCTAssertNotNil(view);
		if (view == nil)
			return;

		bool hover_registered = false;
		bool scroll_registered = false;
		for (UIGestureRecognizer *recognizer in [view gestureRecognizers])
		{
			hover_registered |= [recognizer isKindOfClass:[UIHoverGestureRecognizer class]];
			if ([recognizer isKindOfClass:[UIPanGestureRecognizer class]])
			{
				UIPanGestureRecognizer *pan = (UIPanGestureRecognizer *)recognizer;
				scroll_registered |= [pan allowedScrollTypesMask] == UIScrollTypeMaskAll && [[pan allowedTouchTypes] count] == 0;
			}
		}
		XCTAssertTrue(hover_registered);
		XCTAssertTrue(scroll_registered);
		XCTAssertEqualObjects([[NSBundle mainBundle] objectForInfoDictionaryKey:@"UIApplicationSupportsIndirectInputEvents"], [NSNumber numberWithBool:YES]);

		U32 height = (U32)[view bounds].size.height;
		[view platformMousePosition:CGPointMake(12.0, 18.0)];
		XCTAssertTrue(platform_window_poll(window));
		XCTAssertEqual(window->input.mouse_x, 12);
		XCTAssertEqual(window->input.mouse_y, (I32)height - 19);

		UIEventButtonMask primary_and_middle = UIEventButtonMaskPrimary | UIEventButtonMaskForButtonNumber(3);
		[view platformMouseButtons:primary_and_middle];
		[view platformMouseButtons:primary_and_middle];
		[view platformMousePosition:CGPointMake(17.0, 24.0)];
		[view platformMousePosition:CGPointMake(20.0, 25.0)];
		[view platformMouseScroll:10.0];
		[view platformMouseScroll:5.0];
		[view platformMouseScroll:-2.0];
		XCTAssertTrue(platform_window_poll(window));
		XCTAssertEqual(window->input.mouse_x, 20);
		XCTAssertEqual(window->input.mouse_y, (I32)height - 26);
		XCTAssertEqual(window->input.mouse_dx, 8);
		XCTAssertEqual(window->input.mouse_dy, -7);
		XCTAssertEqual(window->input.mouse_wheel, 1.0f);
		Platform_Key_State *left = window->input.keys + PLATFORM_KEY_MOUSE_LEFT;
		Platform_Key_State *middle = window->input.keys + PLATFORM_KEY_MOUSE_MIDDLE;
		Platform_Key_State *right = window->input.keys + PLATFORM_KEY_MOUSE_RIGHT;
		XCTAssertTrue(left->pressed);
		XCTAssertTrue(left->down);
		XCTAssertEqual(left->press_count, 1);
		XCTAssertTrue(middle->pressed);
		XCTAssertTrue(middle->down);
		XCTAssertEqual(middle->press_count, 1);
		XCTAssertFalse(right->pressed);
		XCTAssertFalse(right->down);

		[view platformMouseButtons:UIEventButtonMaskSecondary];
		XCTAssertTrue(platform_window_poll(window));
		XCTAssertTrue(left->released);
		XCTAssertFalse(left->down);
		XCTAssertTrue(middle->released);
		XCTAssertFalse(middle->down);
		XCTAssertTrue(right->pressed);
		XCTAssertTrue(right->down);

		[view platformMouseButtons:0];
		[view platformMouseScroll:-1.0];
		XCTAssertTrue(platform_window_poll(window));
		XCTAssertTrue(right->released);
		XCTAssertFalse(right->down);
		XCTAssertEqual(window->input.mouse_wheel, -1.0f);

		XCTAssertTrue(platform_window_poll(window));
		XCTAssertFalse(left->pressed);
		XCTAssertFalse(left->released);
		XCTAssertFalse(middle->pressed);
		XCTAssertFalse(middle->released);
		XCTAssertFalse(right->pressed);
		XCTAssertFalse(right->released);
		XCTAssertEqual(window->input.mouse_dx, 0);
		XCTAssertEqual(window->input.mouse_dy, 0);
		XCTAssertEqual(window->input.mouse_wheel, 0.0f);
	}
}

- (void)
testPhysicalKeyboardInput
{
	if (@available(iOS 13.4, *))
	{
		Platform_Window *window = core_ios_test_window();
		XCTAssertTrue(window != nullptr);
		if (window == nullptr)
			return;

		Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
		UIView *view = (UIView *)native_handles.context;
		XCTAssertNotNil(view);
		XCTAssertTrue([view canBecomeFirstResponder]);
		if (view == nil)
			return;

		XCTAssertFalse([view platformKeyUsage:(UIKeyboardHIDUsage)0 down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardA down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardA down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardLeftShift down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeypad7 down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeypad7 down:NO]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardF12 down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardF12 down:NO]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardSemicolon down:YES]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardSemicolon down:NO]);
		XCTAssertTrue(platform_window_poll(window));

		Platform_Key_State *a = window->input.keys + PLATFORM_KEY_A;
		XCTAssertTrue(a->pressed);
		XCTAssertFalse(a->released);
		XCTAssertTrue(a->down);
		XCTAssertEqual(a->press_count, 1);
		XCTAssertEqual(a->release_count, 0);

		Platform_Key_State *shift = window->input.keys + PLATFORM_KEY_SHIFT_LEFT;
		XCTAssertTrue(shift->pressed);
		XCTAssertFalse(shift->released);
		XCTAssertTrue(shift->down);
		XCTAssertEqual(shift->press_count, 1);

		Platform_Key_State *numpad = window->input.keys + PLATFORM_KEY_NUMPAD_7;
		XCTAssertTrue(numpad->pressed);
		XCTAssertTrue(numpad->released);
		XCTAssertFalse(numpad->down);
		XCTAssertEqual(numpad->press_count, 1);
		XCTAssertEqual(numpad->release_count, 1);

		Platform_Key_State *function = window->input.keys + PLATFORM_KEY_F12;
		XCTAssertTrue(function->pressed);
		XCTAssertTrue(function->released);
		XCTAssertFalse(function->down);

		Platform_Key_State *symbol = window->input.keys + PLATFORM_KEY_SEMICOLON;
		XCTAssertTrue(symbol->pressed);
		XCTAssertTrue(symbol->released);
		XCTAssertFalse(symbol->down);

		XCTAssertTrue(platform_window_poll(window));
		XCTAssertFalse(a->pressed);
		XCTAssertFalse(a->released);
		XCTAssertTrue(a->down);
		XCTAssertEqual(a->press_count, 0);
		XCTAssertFalse(shift->pressed);
		XCTAssertTrue(shift->down);
		XCTAssertFalse(numpad->pressed);
		XCTAssertFalse(numpad->released);

		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardA down:NO]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardA down:NO]);
		XCTAssertTrue([view platformKeyUsage:UIKeyboardHIDUsageKeyboardLeftShift down:NO]);
		XCTAssertTrue(platform_window_poll(window));
		XCTAssertFalse(a->pressed);
		XCTAssertTrue(a->released);
		XCTAssertFalse(a->down);
		XCTAssertEqual(a->release_count, 1);
		XCTAssertTrue(shift->released);
		XCTAssertFalse(shift->down);

		XCTAssertTrue(platform_window_poll(window));
		XCTAssertFalse(a->pressed);
		XCTAssertFalse(a->released);
		XCTAssertFalse(a->down);
	}
}

- (void)
testTextInput
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
	UIView *view = (UIView *)native_handles.context;
	XCTAssertNotNil(view);
	if (view == nil)
		return;

	UIView<UITextInput> *text_input = nil;
	for (UIView *subview in [view subviews])
	{
		if ([subview conformsToProtocol:@protocol(UITextInput)])
		{
			text_input = (UIView<UITextInput> *)subview;
			break;
		}
	}
	XCTAssertNotNil(text_input);
	if (text_input == nil)
		return;

	String initial_text = string_literal("A\xC3\xA9\xF0\x9F\x99\x82" "Z");
	Platform_Text_Input_Desc desc {
		.x = 12,
		.y = 18,
		.width = 3,
		.height = 20,
		.flags = PLATFORM_TEXT_INPUT_FLAG_EMAIL | PLATFORM_TEXT_INPUT_FLAG_NO_SUGGESTIONS,
		.action = PLATFORM_TEXT_INPUT_ACTION_SEARCH,
		.text = initial_text,
		.selection_start = 7,
		.selection_end = 7,
		.enabled = true
	};
	platform_window_text_input_set(*window, desc);
	XCTAssertTrue([text_input isFirstResponder]);
	XCTAssertEqual([text_input keyboardType], UIKeyboardTypeEmailAddress);
	XCTAssertEqual([text_input returnKeyType], UIReturnKeySearch);
	XCTAssertEqual([text_input autocorrectionType], UITextAutocorrectionTypeNo);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 0);

	UITextPosition *begin = [text_input beginningOfDocument];
	UITextPosition *end = [text_input endOfDocument];
	UITextRange *all = [text_input textRangeFromPosition:begin toPosition:end];
	XCTAssertEqualObjects([text_input textInRange:all], @"Aé🙂Z");
	XCTAssertEqual([text_input offsetFromPosition:begin toPosition:end], 5);
	UITextRange *selection = [text_input selectedTextRange];
	XCTAssertEqual([text_input offsetFromPosition:begin toPosition:[selection start]], 4);
	CGRect caret = [text_input caretRectForPosition:[selection end]];
	XCTAssertEqual(caret.origin.x, 12.0);
	XCTAssertEqual(caret.origin.y, [view bounds].size.height - 38.0);
	XCTAssertEqual(caret.size.width, 3.0);
	XCTAssertEqual(caret.size.height, 20.0);

	[text_input setMarkedText:@"漢" selectedRange:NSMakeRange(1, 0)];
	XCTAssertNotNil([text_input markedTextRange]);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 1);
	XCTAssertEqual(window->input.text_input_events[0].type, PLATFORM_TEXT_INPUT_EVENT_COMPOSE);
	XCTAssertTrue(window->input.text_input_events[0].text == "\xE6\xBC\xA2");

	[text_input insertText:@"字"];
	XCTAssertNil([text_input markedTextRange]);
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 2);
	XCTAssertEqual(window->input.text_input_events[0].type, PLATFORM_TEXT_INPUT_EVENT_COMMIT);
	XCTAssertTrue(window->input.text_input_events[0].text == "\xE5\xAD\x97");
	XCTAssertEqual(window->input.text_input_events[1].type, PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END);

	String committed_text = string_literal("A\xC3\xA9\xF0\x9F\x99\x82\xE5\xAD\x97" "Z");
	desc.text = committed_text;
	desc.selection_start = 10;
	desc.selection_end = 10;
	platform_window_text_input_set(*window, desc);
	[text_input deleteBackward];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 1);
	XCTAssertEqual(window->input.text_input_events[0].type, PLATFORM_TEXT_INPUT_EVENT_DELETE_SURROUNDING);
	XCTAssertEqual(window->input.text_input_events[0].delete_before, 3);
	XCTAssertEqual(window->input.text_input_events[0].delete_after, 0);

	desc.text = initial_text;
	desc.selection_start = 7;
	desc.selection_end = 7;
	platform_window_text_input_set(*window, desc);
	[text_input insertText:@"\n"];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 1);
	XCTAssertEqual(window->input.text_input_events[0].type, PLATFORM_TEXT_INPUT_EVENT_ACTION);
	XCTAssertEqual(window->input.text_input_events[0].action, PLATFORM_TEXT_INPUT_ACTION_SEARCH);

	UITextPosition *selection_start = [text_input positionFromPosition:[text_input beginningOfDocument] offset:1];
	UITextPosition *selection_end = [text_input positionFromPosition:[text_input beginningOfDocument] offset:4];
	[text_input setSelectedTextRange:[text_input textRangeFromPosition:selection_start toPosition:selection_end]];
	XCTAssertTrue(platform_window_poll(window));
	XCTAssertEqual(window->input.text_input_events.count, 1);
	XCTAssertEqual(window->input.text_input_events[0].type, PLATFORM_TEXT_INPUT_EVENT_SELECTION);
	XCTAssertEqual(window->input.text_input_events[0].selection_start, 1);
	XCTAssertEqual(window->input.text_input_events[0].selection_end, 7);

	desc.enabled = false;
	desc.text = {};
	desc.selection_start = 0;
	desc.selection_end = 0;
	platform_window_text_input_set(*window, desc);
	XCTAssertFalse([text_input isFirstResponder]);
	XCTAssertTrue([view isFirstResponder]);
	XCTAssertFalse(window->text_input.enabled);
}

- (void)
testClipboard
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	String text = string_literal("Core iOS clipboard \xF0\x9F\x93\x8B");
	U8 binary[] = {0, 1, 2, 127, 255};
	U8 custom[] = {9, 8, 7};
	Platform_Clipboard_Item_Desc items[] = {
		{
			.media_type = string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8),
			.data = slice_from((const U8 *)text.data, text.count)
		},
		{
			.media_type = string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY),
			.data = slice_from((const U8 *)binary, count_of(binary))
		},
		{
			.media_type = string_literal("application/x-core-test"),
			.data = slice_from((const U8 *)custom, count_of(custom))
		}
	};
	XCTAssertFalse(platform_window_clipboard_item_write(*window, nullptr, 0));
	XCTAssertTrue(platform_window_clipboard_item_write(*window, items, (U32)count_of(items)));

	Array<String> media_types = platform_window_clipboard_query_media_types(*window);
	bool found_text = false;
	bool found_binary = false;
	bool found_custom = false;
	for (U64 i = 0; i < media_types.count; ++i)
	{
		found_text |= media_types[i] == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8;
		found_binary |= media_types[i] == PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY;
		found_custom |= media_types[i] == "application/x-core-test";
	}
	XCTAssertTrue(found_text);
	XCTAssertTrue(found_binary);
	XCTAssertTrue(found_custom);

	Array<U8> data = array_init<U8>();
	XCTAssertTrue(platform_window_clipboard_item_read(*window, string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8), data));
	XCTAssertEqual(data.count, text.count);
	for (U64 i = 0; i < data.count && i < text.count; ++i)
		XCTAssertEqual(data[i], (U8)text[i]);

	XCTAssertTrue(platform_window_clipboard_item_read(*window, string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY), data));
	XCTAssertEqual(data.count, count_of(binary));
	for (U64 i = 0; i < data.count && i < count_of(binary); ++i)
		XCTAssertEqual(data[i], binary[i]);

	XCTAssertTrue(platform_window_clipboard_item_read(*window, string_literal("application/x-core-test"), data));
	XCTAssertEqual(data.count, count_of(custom));
	for (U64 i = 0; i < data.count && i < count_of(custom); ++i)
		XCTAssertEqual(data[i], custom[i]);

	XCTAssertFalse(platform_window_clipboard_item_read(*window, string_literal("application/x-core-missing"), data));
	XCTAssertEqual(data.count, 0);
	array_deinit(data);
	for (U64 i = 0; i < media_types.count; ++i)
		string_deinit(media_types[i]);
	array_deinit(media_types);
}

- (void)
testWindowPresentation
{
	Platform_Window *window = core_ios_test_window();
	XCTAssertTrue(window != nullptr);
	if (window == nullptr)
		return;

	Platform_Window_Native_Handles native_handles = platform_window_get_native_handles(window);
	UIWindow *ui_window = (UIWindow *)native_handles.window;
	UIViewController *view_controller = [ui_window rootViewController];
	XCTAssertNotNil(ui_window);
	XCTAssertNotNil(view_controller);
	if (ui_window == nil || view_controller == nil)
		return;

	UIApplication *application = [UIApplication sharedApplication];
	BOOL idle_timer_disabled = [application isIdleTimerDisabled];
	Platform_Window_Presentation_Desc presentation {
		.flags = PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN |
			PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE |
			PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON |
			PLATFORM_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE,
		.orientation_policy = PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE
	};
	platform_window_presentation_set(*window, presentation);

	XCTAssertEqual(window->presentation.flags, presentation.flags);
	XCTAssertEqual(window->presentation.orientation_policy, presentation.orientation_policy);
	XCTAssertTrue([view_controller prefersStatusBarHidden]);
	XCTAssertTrue([view_controller prefersHomeIndicatorAutoHidden]);
	XCTAssertEqual([view_controller preferredScreenEdgesDeferringSystemGestures], UIRectEdgeAll);
	XCTAssertEqual([view_controller edgesForExtendedLayout], UIRectEdgeAll);
	XCTAssertEqual([view_controller supportedInterfaceOrientations], UIInterfaceOrientationMaskLandscape);
	XCTAssertTrue([application isIdleTimerDisabled]);

	Platform_Window_Presentation_Desc restored = {};
	platform_window_presentation_set(*window, restored);
	XCTAssertFalse([view_controller prefersStatusBarHidden]);
	XCTAssertFalse([view_controller prefersHomeIndicatorAutoHidden]);
	XCTAssertEqual([view_controller preferredScreenEdgesDeferringSystemGestures], UIRectEdgeNone);
	XCTAssertEqual([view_controller edgesForExtendedLayout], UIRectEdgeNone);
	XCTAssertEqual([application isIdleTimerDisabled], idle_timer_disabled);
}

- (void)
testCore
{
	XCTAssertTrue(tester_run(tester()));
}
@end