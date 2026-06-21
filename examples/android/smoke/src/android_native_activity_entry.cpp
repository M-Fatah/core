#include "core/defines.h"
#include "core/validate.h"

#include <android/native_activity.h>
#include <pthread.h>

extern "C" void
platform_android_native_activity_on_create(void *native_activity, void *saved_state, U64 saved_state_size);

extern "C" void
core_android_smoke_main();

inline static void *
_android_smoke_thread_main(void *)
{
	core_android_smoke_main();
	return nullptr;
}

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved_state, size_t saved_state_size)
{
	platform_android_native_activity_on_create(activity, saved_state, (U64)saved_state_size);

	pthread_t thread = {};
	validate(::pthread_create(&thread, nullptr, _android_smoke_thread_main, nullptr) == 0, "[ANDROID_SMOKE]: Failed to create app thread.");
	validate(::pthread_detach(thread) == 0, "[ANDROID_SMOKE]: Failed to detach app thread.");
}