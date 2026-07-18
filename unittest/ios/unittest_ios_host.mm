#include <UIKit/UIKit.h>

@interface Core_IOSTest_Scene_Delegate : UIResponder<UIWindowSceneDelegate>
{
	UIWindow *_window;
}

@property(nonatomic, retain) UIWindow *window;
@end

@implementation Core_IOSTest_Scene_Delegate
@synthesize window = _window;

- (void)
scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connection_options
{
	(void)connection_options;

	bool valid_scene = [scene isKindOfClass:[UIWindowScene class]] && [scene session] == session;
	if (!valid_scene)
		return;

	UIWindowScene *window_scene = (UIWindowScene *)scene;
	UIWindow *window = [[UIWindow alloc] initWithWindowScene:window_scene];
	self.window = window;
	[window release];
}

- (void)
sceneDidDisconnect:(UIScene *)scene
{
	(void)scene;
	self.window = nil;
}

- (void)
dealloc
{
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

int
main(int argc, char **argv)
{
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([Core_IOSTest_Host_Delegate class]));
	}
}