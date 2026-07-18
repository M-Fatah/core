#include <core/platform/platform.h>
#include <core/validate.h>

#include <UIKit/UIKit.h>

inline static Platform_Window *_core_ios_test_window = nullptr;

@interface Core_IOSTest_Scene_Delegate : UIResponder<UIWindowSceneDelegate>
{
	UIWindow *_window;
	Platform_Window _platform_window;
	bool _platform_window_initialized;
}

@property(nonatomic, retain) UIWindow *window;

- (void)
deinitPlatformWindow;
@end

@implementation Core_IOSTest_Scene_Delegate
@synthesize window = _window;

- (void)
scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connection_options
{
	(void)connection_options;

	bool valid_scene = [scene isKindOfClass:[UIWindowScene class]] && [scene session] == session;
	validate(valid_scene, "[UNITTEST][IOS]: Test host requires a UIWindowScene.");
	if (!valid_scene)
		return;

	UIWindowScene *window_scene = (UIWindowScene *)scene;
	UIWindow *window = [[UIWindow alloc] initWithWindowScene:window_scene];

	_platform_window = platform_window_init(Platform_Window_Desc {});
	_platform_window_initialized = _platform_window.ctx != nullptr;

	PLATFORM_WINDOW_NATIVE_CONNECT_RESULT connect_result = _platform_window_initialized
		? platform_window_native_connect(&_platform_window, window_scene, window)
		: PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED;
	validate(connect_result == PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_CONNECTED, "[UNITTEST][IOS]: Failed to bind the test Platform_Window to its UIWindowScene.");
	if (connect_result != PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_CONNECTED)
	{
		if (_platform_window_initialized)
			platform_window_deinit(&_platform_window);
		_platform_window_initialized = false;
		[window release];
		return;
	}

	self.window = window;
	if (_core_ios_test_window == nullptr)
		_core_ios_test_window = &_platform_window;
	[window makeKeyAndVisible];
	[window release];
}

- (void)
sceneDidDisconnect:(UIScene *)scene
{
	(void)scene;
	[self deinitPlatformWindow];
	self.window = nil;
}

- (void)
deinitPlatformWindow
{
	if (!_platform_window_initialized)
		return;

	if (_core_ios_test_window == &_platform_window)
		_core_ios_test_window = nullptr;
	platform_window_deinit(&_platform_window);
	_platform_window_initialized = false;
}

- (void)
dealloc
{
	[self deinitPlatformWindow];
	[_window release];
	[super dealloc];
}
@end

@interface Core_IOSTest_Host_Delegate : UIResponder<UIApplicationDelegate>
@end

@implementation Core_IOSTest_Host_Delegate
- (BOOL)
application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launch_options
{
	(void)application;
	(void)launch_options;
	return YES;
}

- (UISceneConfiguration *)
application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connecting_scene_session options:(UISceneConnectionOptions *)options
{
	(void)application;
	(void)options;

	UISceneConfiguration *configuration = [[[UISceneConfiguration alloc] initWithName:@"Core Unit Tests" sessionRole:[connecting_scene_session role]] autorelease];
	configuration.sceneClass = [UIWindowScene class];
	configuration.delegateClass = [Core_IOSTest_Scene_Delegate class];
	return configuration;
}
@end

extern "C" __attribute__((visibility("default"))) Platform_Window *
core_ios_test_window()
{
	return _core_ios_test_window;
}

I32
main(I32 argc, char **argv)
{
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([Core_IOSTest_Host_Delegate class]));
	}
}