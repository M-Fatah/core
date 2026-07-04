#include "core/platform/platform.h"

#include "core/validate.h"
#include "core/defer.h"
#include "core/math/u64.h"
#include "core/memory/allocator.h"

#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/rect.h>
#include <android/native_window.h>
#include <jni.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <unwind.h>
#include <sys/mman.h>
#include <sys/stat.h>

enum
{
	PLATFORM_ANDROID_LOOPER_INPUT = 1
};

struct Platform_Thread
{
	pthread_t handle;
	Platform_Thread_Function function;
	void *data;
	String name;
	bool joined;
};

struct Platform_Context
{
	ANativeActivity *activity;
	AAssetManager *asset_manager;
	JavaVM *vm;
	jobject activity_object;
	jclass activity_class;
	jmethodID file_dialog_open_method;
	jmethodID file_dialog_save_method;
	jmethodID directory_dialog_open_method;
	jmethodID app_data_directory_method;
	jmethodID cache_directory_method;
	jmethodID content_open_fd_method;
	jmethodID content_size_method;
	jmethodID content_delete_method;
	jmethodID content_is_directory_method;
	jmethodID content_display_name_method;
	jmethodID content_list_files_method;
	jmethodID content_list_files_recursive_method;
	jmethodID content_create_file_method;
	jmethodID content_create_directory_method;
	jmethodID content_rename_method;
	jmethodID content_move_method;
	jmethodID clipboard_query_media_types_method;
	jmethodID clipboard_read_item_method;
	jmethodID clipboard_write_items_method;
	jmethodID window_presentation_set_method;
	jmethodID text_input_set_method;
	AInputQueue *input_queue;
	ANativeWindow *native_window;
	ANativeWindow *native_window_lease;
	ALooper *looper;
	pthread_mutex_t mutex;
	pthread_cond_t file_dialog_condition;
	Array<Platform_Text_Input_Event> text_input_events;
	U32 width;
	U32 height;
	Platform_Window_Metrics metrics;
	Platform_Window_Presentation_Desc presentation;
	I32 content_rect_left;
	I32 content_rect_top;
	I32 content_rect_right;
	I32 content_rect_bottom;
	I32 text_input_x;
	I32 text_input_y;
	U32 text_input_width;
	U32 text_input_height;
	U32 text_input_flags;
	Platform_Text_Input_Action text_input_action;
	U32 text_input_selection_start;
	U32 text_input_selection_end;
	U32 text_input_composing_start;
	U32 text_input_composing_end;
	String text_input_text;
	U64 file_dialog_next_id;
	U64 file_dialog_active_id;
	String file_dialog_path;
	bool input_queue_attached;
	bool close_requested;
	bool started;
	bool paused;
	bool focused;
	bool low_memory;
	bool save_state_requested;
	bool surface_changed;
	bool content_rect_valid;
	bool activity_recreation_pending;
	bool window_deinitialized;
	bool window_deinit_in_progress;
	bool file_dialog_waiting;
	bool file_dialog_completed;
	bool file_dialog_accepted;
	bool text_input_active;
};

static Platform_Context *_platform_android_context = nullptr;

inline static void
_platform_android_context_try_deinit(Platform_Context *context, bool window_deinit_finished = false);

inline static void
_platform_android_window_presentation_set(Platform_Context *context, const Platform_Window_Presentation_Desc &desc);

inline static void
_platform_android_text_input_set(Platform_Context *context, const Platform_Text_Input_Desc &desc);

inline static Platform_Context *
_platform_android_context_get()
{
	validate(_platform_android_context != nullptr, "[PLATFORM][ANDROID]: Android context is not initialized.");
	return _platform_android_context;
}

inline static void
_platform_copy_string(char *dst, U64 dst_size, const char *src)
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

inline static bool
_platform_android_is_content_uri(const char *path)
{
	return path != nullptr && ::strncmp(path, "content://", 10) == 0;
}

inline static bool
_platform_extension_matches(const String &file_name, const String &extension_filter)
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

inline static Platform_File_Handle
_platform_file_handle_from_fd(I32 fd)
{
	return (Platform_File_Handle)(intptr_t)(fd + 1);
}

inline static I32
_platform_file_handle_to_fd(Platform_File_Handle handle)
{
	return (I32)((intptr_t)handle - 1);
}

inline static I64
_platform_android_read(I32 fd, void *data, U64 size)
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
			return -1;
		}

		bytes_read += (U64)result;
	}
	return (I64)bytes_read;
}

inline static I64
_platform_android_write(I32 fd, const void *data, U64 size)
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
			return -1;
		}

		bytes_written += (U64)result;
	}
	return (I64)bytes_written;
}

inline static Platform_Context *
_platform_android_context_from_activity(ANativeActivity *activity)
{
	return activity ? (Platform_Context *)activity->instance : nullptr;
}

inline static void
_platform_android_context_lock(Platform_Context *context)
{
	validate(::pthread_mutex_lock(&context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to lock context mutex.");
}

inline static void
_platform_android_context_unlock(Platform_Context *context)
{
	validate(::pthread_mutex_unlock(&context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to unlock context mutex.");
}

inline static JNIEnv *
_platform_android_jni_env(Platform_Context *context, bool *needs_detach)
{
	*needs_detach = false;
	if (context->vm == nullptr)
		return nullptr;

	JNIEnv *env = nullptr;
	I32 result = context->vm->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (result == JNI_OK)
		return env;

	if (result != JNI_EDETACHED)
		return nullptr;

	if (context->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
		return nullptr;

	*needs_detach = true;
	return env;
}

inline static void
_platform_android_jni_detach(Platform_Context *context, bool needs_detach)
{
	if (needs_detach && context->vm)
		context->vm->DetachCurrentThread();
}

inline static bool
_platform_android_jni_clear_exception(JNIEnv *env)
{
	if (env == nullptr || !env->ExceptionCheck())
		return false;

	env->ExceptionClear();
	return true;
}

inline static jmethodID
_platform_android_jni_get_method(JNIEnv *env, jclass klass, const char *name, const char *signature)
{
	jmethodID method = env->GetMethodID(klass, name, signature);
	_platform_android_jni_clear_exception(env);
	return method;
}

inline static bool
_platform_android_jni_bridge_init(Platform_Context *context, ANativeActivity *activity)
{
	JavaVM *vm = activity->vm;
	JNIEnv *env = activity->env;
	if (env == nullptr || activity->clazz == nullptr)
		return false;

	jobject activity_object = env->NewGlobalRef(activity->clazz);
	if (_platform_android_jni_clear_exception(env) || activity_object == nullptr)
		return false;
	DEFER(if (activity_object) env->DeleteGlobalRef(activity_object););

	jclass activity_class = env->GetObjectClass(activity->clazz);
	if (_platform_android_jni_clear_exception(env) || activity_class == nullptr)
		return false;
	DEFER(env->DeleteLocalRef(activity_class););

	jclass activity_class_global = (jclass)env->NewGlobalRef(activity_class);
	if (_platform_android_jni_clear_exception(env) || activity_class_global == nullptr)
		return false;
	DEFER(if (activity_class_global) env->DeleteGlobalRef(activity_class_global););

	jmethodID file_dialog_open_method = _platform_android_jni_get_method(env, activity_class_global, "coreOpenFileDialog", "(JLjava/lang/String;)V");
	jmethodID file_dialog_save_method = _platform_android_jni_get_method(env, activity_class_global, "coreSaveFileDialog", "(JLjava/lang/String;)V");
	jmethodID directory_dialog_open_method = _platform_android_jni_get_method(env, activity_class_global, "coreOpenDirectoryDialog", "(J)V");
	jmethodID app_data_directory_method = _platform_android_jni_get_method(env, activity_class_global, "coreAppDataDirectory", "()Ljava/lang/String;");
	jmethodID cache_directory_method = _platform_android_jni_get_method(env, activity_class_global, "coreCacheDirectory", "()Ljava/lang/String;");
	jmethodID content_open_fd_method = _platform_android_jni_get_method(env, activity_class_global, "coreOpenContentFd", "(Ljava/lang/String;Ljava/lang/String;)I");
	jmethodID content_size_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentSize", "(Ljava/lang/String;)J");
	jmethodID content_delete_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentDelete", "(Ljava/lang/String;)Z");
	jmethodID content_is_directory_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentIsDirectory", "(Ljava/lang/String;)Z");
	jmethodID content_display_name_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentDisplayName", "(Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_list_files_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentListFiles", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_list_files_recursive_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentListFilesRecursive", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_create_file_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentCreateFile", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_create_directory_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentCreateDirectory", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_rename_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentRename", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID content_move_method = _platform_android_jni_get_method(env, activity_class_global, "coreContentMove", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID clipboard_query_media_types_method = _platform_android_jni_get_method(env, activity_class_global, "coreClipboardQueryMediaTypes", "()Ljava/lang/String;");
	jmethodID clipboard_read_item_method = _platform_android_jni_get_method(env, activity_class_global, "coreClipboardReadItem", "(Ljava/lang/String;)[B");
	jmethodID clipboard_write_items_method = _platform_android_jni_get_method(env, activity_class_global, "coreClipboardWriteItems", "([Ljava/lang/String;[[B)Z");
	jmethodID window_presentation_set_method = _platform_android_jni_get_method(env, activity_class_global, "coreWindowPresentationSet", "(II)V");
	jmethodID text_input_set_method = _platform_android_jni_get_method(env, activity_class_global, "coreTextInputSet", "(ZIIIIII[BIIII)V");

	bool methods_valid =
		file_dialog_open_method &&
		file_dialog_save_method &&
		directory_dialog_open_method &&
		app_data_directory_method &&
		cache_directory_method &&
		content_open_fd_method &&
		content_size_method &&
		content_delete_method &&
		content_is_directory_method &&
		content_display_name_method &&
		content_list_files_method &&
		content_list_files_recursive_method &&
		content_create_file_method &&
		content_create_directory_method &&
		content_rename_method &&
		content_move_method &&
		clipboard_query_media_types_method &&
		clipboard_read_item_method &&
		clipboard_write_items_method &&
		window_presentation_set_method &&
		text_input_set_method;
	if (!methods_valid)
		return false;

	jobject old_activity_object = nullptr;
	jclass old_activity_class = nullptr;

	_platform_android_context_lock(context);
	old_activity_object = context->activity_object;
	old_activity_class = context->activity_class;
	context->vm = vm;
	context->activity_object = activity_object;
	context->activity_class = activity_class_global;
	context->file_dialog_open_method = file_dialog_open_method;
	context->file_dialog_save_method = file_dialog_save_method;
	context->directory_dialog_open_method = directory_dialog_open_method;
	context->app_data_directory_method = app_data_directory_method;
	context->cache_directory_method = cache_directory_method;
	context->content_open_fd_method = content_open_fd_method;
	context->content_size_method = content_size_method;
	context->content_delete_method = content_delete_method;
	context->content_is_directory_method = content_is_directory_method;
	context->content_display_name_method = content_display_name_method;
	context->content_list_files_method = content_list_files_method;
	context->content_list_files_recursive_method = content_list_files_recursive_method;
	context->content_create_file_method = content_create_file_method;
	context->content_create_directory_method = content_create_directory_method;
	context->content_rename_method = content_rename_method;
	context->content_move_method = content_move_method;
	context->clipboard_query_media_types_method = clipboard_query_media_types_method;
	context->clipboard_read_item_method = clipboard_read_item_method;
	context->clipboard_write_items_method = clipboard_write_items_method;
	context->window_presentation_set_method = window_presentation_set_method;
	context->text_input_set_method = text_input_set_method;
	_platform_android_context_unlock(context);

	activity_object = nullptr;
	activity_class_global = nullptr;
	if (old_activity_object)
		env->DeleteGlobalRef(old_activity_object);
	if (old_activity_class)
		env->DeleteGlobalRef(old_activity_class);
	return true;
}

inline static void
_platform_android_jni_bridge_deinit(Platform_Context *context)
{
	jobject activity_object = nullptr;
	jclass activity_class = nullptr;

	_platform_android_context_lock(context);
	activity_object = context->activity_object;
	activity_class = context->activity_class;
	context->activity_object = nullptr;
	context->activity_class = nullptr;
	context->file_dialog_open_method = nullptr;
	context->file_dialog_save_method = nullptr;
	context->directory_dialog_open_method = nullptr;
	context->app_data_directory_method = nullptr;
	context->cache_directory_method = nullptr;
	context->content_open_fd_method = nullptr;
	context->content_size_method = nullptr;
	context->content_delete_method = nullptr;
	context->content_is_directory_method = nullptr;
	context->content_display_name_method = nullptr;
	context->content_list_files_method = nullptr;
	context->content_list_files_recursive_method = nullptr;
	context->content_create_file_method = nullptr;
	context->content_create_directory_method = nullptr;
	context->content_rename_method = nullptr;
	context->content_move_method = nullptr;
	context->clipboard_query_media_types_method = nullptr;
	context->clipboard_read_item_method = nullptr;
	context->clipboard_write_items_method = nullptr;
	context->window_presentation_set_method = nullptr;
	context->text_input_set_method = nullptr;
	_platform_android_context_unlock(context);

	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env)
	{
		if (activity_object)
			env->DeleteGlobalRef(activity_object);
		if (activity_class)
			env->DeleteGlobalRef(activity_class);
	}

	_platform_android_jni_detach(context, needs_detach);
}

inline static jobject
_platform_android_activity_ref_get(Platform_Context *context, JNIEnv *env, jmethodID *method, jmethodID *method_out)
{
	*method_out = nullptr;
	jobject activity_object = nullptr;

	_platform_android_context_lock(context);
	if (context->activity && context->activity_object && method && *method)
	{
		activity_object = env->NewLocalRef(context->activity_object);
		*method_out = *method;
	}
	_platform_android_context_unlock(context);

	if (_platform_android_jni_clear_exception(env) || activity_object == nullptr)
	{
		*method_out = nullptr;
		return nullptr;
	}
	return activity_object;
}

inline static Platform_Window_Orientation
_platform_android_window_orientation(U32 width, U32 height)
{
	if (width == 0 || height == 0)
		return PLATFORM_WINDOW_ORIENTATION_UNKNOWN;
	return width >= height ? PLATFORM_WINDOW_ORIENTATION_LANDSCAPE : PLATFORM_WINDOW_ORIENTATION_PORTRAIT;
}

inline static I32
_platform_android_i32_clamp(I32 value, I32 min_value, I32 max_value)
{
	if (value < min_value)
		return min_value;
	if (value > max_value)
		return max_value;
	return value;
}

inline static void
_platform_android_metrics_update_locked(Platform_Context *context)
{
	I32 width = (I32)context->width;
	I32 height = (I32)context->height;
	I32 left = 0;
	I32 top = 0;
	I32 right = width;
	I32 bottom = height;
	if (context->content_rect_valid && width > 0 && height > 0)
	{
		left = _platform_android_i32_clamp(context->content_rect_left, 0, width);
		top = _platform_android_i32_clamp(context->content_rect_top, 0, height);
		right = _platform_android_i32_clamp(context->content_rect_right, left, width);
		bottom = _platform_android_i32_clamp(context->content_rect_bottom, top, height);
	}

	I32 density = ACONFIGURATION_DENSITY_MEDIUM;
	I32 orientation = ACONFIGURATION_ORIENTATION_ANY;
	if (context->asset_manager)
	{
		AConfiguration *configuration = ::AConfiguration_new();
		if (configuration)
		{
			::AConfiguration_fromAssetManager(configuration, context->asset_manager);
			density = ::AConfiguration_getDensity(configuration);
			orientation = ::AConfiguration_getOrientation(configuration);
			::AConfiguration_delete(configuration);
		}
	}

	if (density <= 0 || density == ACONFIGURATION_DENSITY_DEFAULT || density == ACONFIGURATION_DENSITY_ANY || density == ACONFIGURATION_DENSITY_NONE)
		density = ACONFIGURATION_DENSITY_MEDIUM;

	Platform_Window_Orientation window_orientation = PLATFORM_WINDOW_ORIENTATION_UNKNOWN;
	if (orientation == ACONFIGURATION_ORIENTATION_PORT)
		window_orientation = PLATFORM_WINDOW_ORIENTATION_PORTRAIT;
	else if (orientation == ACONFIGURATION_ORIENTATION_LAND)
		window_orientation = PLATFORM_WINDOW_ORIENTATION_LANDSCAPE;
	else
		window_orientation = _platform_android_window_orientation(context->width, context->height);

	context->metrics = Platform_Window_Metrics {
		.content_rect = Platform_Window_Rect {
			.x = left,
			.y = height - bottom,
			.width = width > 0 ? (U32)(right - left) : 0,
			.height = height > 0 ? (U32)(bottom - top) : 0
		},
		.safe_area = Platform_Window_Insets {
			.left = width > 0 ? (U32)left : 0,
			.top = height > 0 ? (U32)top : 0,
			.right = width > 0 ? (U32)(width - right) : 0,
			.bottom = height > 0 ? (U32)(height - bottom) : 0
		},
		.density_scale = (F32)density / 160.0f,
		.dpi_x = (F32)density,
		.dpi_y = (F32)density,
		.orientation = window_orientation
	};
}

inline static void
_platform_android_window_lease_release_locked(Platform_Context *context)
{
	if (context->native_window_lease)
	{
		::ANativeWindow_release(context->native_window_lease);
		context->native_window_lease = nullptr;
	}
}

inline static void
_platform_android_window_set_locked(Platform_Context *context, ANativeWindow *window)
{
	if (context->native_window == window)
		return;

	context->surface_changed = true;
	if (context->native_window)
		::ANativeWindow_release(context->native_window);

	context->native_window = window;
	if (context->native_window)
	{
		::ANativeWindow_acquire(context->native_window);
		context->width  = (U32)::ANativeWindow_getWidth(context->native_window);
		context->height = (U32)::ANativeWindow_getHeight(context->native_window);
	}
	else
	{
		context->width = 0;
		context->height = 0;
		context->content_rect_valid = false;
	}
	_platform_android_metrics_update_locked(context);
}

inline static void
_platform_android_input_queue_attach_locked(Platform_Context *context)
{
	if (context->input_queue == nullptr || context->looper == nullptr || context->input_queue_attached)
		return;

	::AInputQueue_attachLooper(context->input_queue, context->looper, PLATFORM_ANDROID_LOOPER_INPUT, nullptr, nullptr);
	context->input_queue_attached = true;
}

inline static void
_platform_android_input_queue_detach_locked(Platform_Context *context)
{
	if (context->input_queue == nullptr || !context->input_queue_attached)
		return;

	::AInputQueue_detachLooper(context->input_queue);
	context->input_queue_attached = false;
}

inline static void
_platform_android_file_dialog_cancel_locked(Platform_Context *context)
{
	if (!context->file_dialog_waiting)
		return;

	if (context->file_dialog_path.capacity > 0)
		string_deinit(context->file_dialog_path);
	context->file_dialog_path = {};

	context->file_dialog_accepted = false;
	context->file_dialog_completed = true;
	validate(::pthread_cond_signal(&context->file_dialog_condition) == 0, "[PLATFORM][ANDROID]: Failed to signal file dialog condition.");
}

inline static void
_platform_android_text_input_events_reset(Array<Platform_Text_Input_Event> &events)
{
	for (U64 i = 0; i < events.count; ++i)
	{
		string_deinit(events[i].text);
		events[i] = {};
	}
	events.count = 0;
}

inline static void
_platform_android_on_resume(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->paused = false;
	}
}

inline static void
_platform_android_on_start(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->started = true;
	}
}

inline static void
_platform_android_on_stop(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->started = false;
		context->focused = false;
	}
}

inline static void
_platform_android_on_low_memory(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->low_memory = true;
	}
}

inline static void *
_platform_android_on_save_instance_state(ANativeActivity *activity, size_t *out_size)
{
	if (out_size)
		*out_size = 0;

	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->save_state_requested = true;
	}
	return nullptr;
}

inline static void
_platform_android_on_pause(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->paused = true;
	}
}

inline static void
_platform_android_on_destroy(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		bool recreating = context->activity_recreation_pending && !context->window_deinitialized && !context->window_deinit_in_progress;
		if (!recreating)
			context->close_requested = true;
		context->started = false;
		context->paused = true;
		context->focused = false;
		context->activity = nullptr;
		activity->instance = nullptr;
		_platform_android_input_queue_detach_locked(context);
		context->input_queue = nullptr;
		_platform_android_window_lease_release_locked(context);
		_platform_android_window_set_locked(context, nullptr);
		_platform_android_file_dialog_cancel_locked(context);
	}
	_platform_android_context_try_deinit(context);
}

inline static void
_platform_android_on_window_focus_changed(ANativeActivity *activity, int has_focus)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->focused = has_focus != 0;
	}
}

inline static void
_platform_android_on_native_window_created(ANativeActivity *activity, ANativeWindow *window)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		_platform_android_window_set_locked(context, window);
	}
}

inline static void
_platform_android_on_native_window_resized(ANativeActivity *activity, ANativeWindow *window)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->native_window == window)
		{
			context->width = (U32)::ANativeWindow_getWidth(context->native_window);
			context->height = (U32)::ANativeWindow_getHeight(context->native_window);
			context->content_rect_valid = false;
			context->surface_changed = true;
			_platform_android_metrics_update_locked(context);
		}
	}
}

inline static void
_platform_android_on_native_window_redraw_needed(ANativeActivity *activity, ANativeWindow *window)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->native_window == window)
			context->surface_changed = true;
	}
}

inline static void
_platform_android_on_native_window_destroyed(ANativeActivity *activity, ANativeWindow *window)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->native_window == window)
			_platform_android_window_set_locked(context, nullptr);
	}
}

inline static void
_platform_android_on_content_rect_changed(ANativeActivity *activity, const ARect *rect)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (rect)
		{
			context->content_rect_left = rect->left;
			context->content_rect_top = rect->top;
			context->content_rect_right = rect->right;
			context->content_rect_bottom = rect->bottom;
			context->content_rect_valid = true;
		}
		else
		{
			context->content_rect_valid = false;
		}
		context->surface_changed = true;
		_platform_android_metrics_update_locked(context);
	}
}

inline static void
_platform_android_on_configuration_changed(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->native_window)
		{
			U32 width = (U32)::ANativeWindow_getWidth(context->native_window);
			U32 height = (U32)::ANativeWindow_getHeight(context->native_window);
			if (context->width != width || context->height != height)
			{
				context->width = width;
				context->height = height;
			}
		}
		context->surface_changed = true;
		_platform_android_metrics_update_locked(context);
	}
}

inline static void
_platform_android_on_input_queue_created(ANativeActivity *activity, AInputQueue *queue)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		_platform_android_input_queue_detach_locked(context);
		context->input_queue = queue;
	}
}

inline static void
_platform_android_on_input_queue_destroyed(ANativeActivity *activity, AInputQueue *queue)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->input_queue == queue)
		{
			_platform_android_input_queue_detach_locked(context);
			context->input_queue = nullptr;
		}
	}
}

inline static PLATFORM_KEY
_platform_key_from_android_keycode(I32 keycode)
{
	switch (keycode)
	{
		case AKEYCODE_A:             return PLATFORM_KEY_A;
		case AKEYCODE_B:             return PLATFORM_KEY_B;
		case AKEYCODE_C:             return PLATFORM_KEY_C;
		case AKEYCODE_D:             return PLATFORM_KEY_D;
		case AKEYCODE_E:             return PLATFORM_KEY_E;
		case AKEYCODE_F:             return PLATFORM_KEY_F;
		case AKEYCODE_G:             return PLATFORM_KEY_G;
		case AKEYCODE_H:             return PLATFORM_KEY_H;
		case AKEYCODE_I:             return PLATFORM_KEY_I;
		case AKEYCODE_J:             return PLATFORM_KEY_J;
		case AKEYCODE_K:             return PLATFORM_KEY_K;
		case AKEYCODE_L:             return PLATFORM_KEY_L;
		case AKEYCODE_M:             return PLATFORM_KEY_M;
		case AKEYCODE_N:             return PLATFORM_KEY_N;
		case AKEYCODE_O:             return PLATFORM_KEY_O;
		case AKEYCODE_P:             return PLATFORM_KEY_P;
		case AKEYCODE_Q:             return PLATFORM_KEY_Q;
		case AKEYCODE_R:             return PLATFORM_KEY_R;
		case AKEYCODE_S:             return PLATFORM_KEY_S;
		case AKEYCODE_T:             return PLATFORM_KEY_T;
		case AKEYCODE_U:             return PLATFORM_KEY_U;
		case AKEYCODE_V:             return PLATFORM_KEY_V;
		case AKEYCODE_W:             return PLATFORM_KEY_W;
		case AKEYCODE_X:             return PLATFORM_KEY_X;
		case AKEYCODE_Y:             return PLATFORM_KEY_Y;
		case AKEYCODE_Z:             return PLATFORM_KEY_Z;
		case AKEYCODE_0:             return PLATFORM_KEY_NUM_0;
		case AKEYCODE_1:             return PLATFORM_KEY_NUM_1;
		case AKEYCODE_2:             return PLATFORM_KEY_NUM_2;
		case AKEYCODE_3:             return PLATFORM_KEY_NUM_3;
		case AKEYCODE_4:             return PLATFORM_KEY_NUM_4;
		case AKEYCODE_5:             return PLATFORM_KEY_NUM_5;
		case AKEYCODE_6:             return PLATFORM_KEY_NUM_6;
		case AKEYCODE_7:             return PLATFORM_KEY_NUM_7;
		case AKEYCODE_8:             return PLATFORM_KEY_NUM_8;
		case AKEYCODE_9:             return PLATFORM_KEY_NUM_9;
		case AKEYCODE_NUMPAD_0:      return PLATFORM_KEY_NUMPAD_0;
		case AKEYCODE_NUMPAD_1:      return PLATFORM_KEY_NUMPAD_1;
		case AKEYCODE_NUMPAD_2:      return PLATFORM_KEY_NUMPAD_2;
		case AKEYCODE_NUMPAD_3:      return PLATFORM_KEY_NUMPAD_3;
		case AKEYCODE_NUMPAD_4:      return PLATFORM_KEY_NUMPAD_4;
		case AKEYCODE_NUMPAD_5:      return PLATFORM_KEY_NUMPAD_5;
		case AKEYCODE_NUMPAD_6:      return PLATFORM_KEY_NUMPAD_6;
		case AKEYCODE_NUMPAD_7:      return PLATFORM_KEY_NUMPAD_7;
		case AKEYCODE_NUMPAD_8:      return PLATFORM_KEY_NUMPAD_8;
		case AKEYCODE_NUMPAD_9:      return PLATFORM_KEY_NUMPAD_9;
		case AKEYCODE_F1:            return PLATFORM_KEY_F1;
		case AKEYCODE_F2:            return PLATFORM_KEY_F2;
		case AKEYCODE_F3:            return PLATFORM_KEY_F3;
		case AKEYCODE_F4:            return PLATFORM_KEY_F4;
		case AKEYCODE_F5:            return PLATFORM_KEY_F5;
		case AKEYCODE_F6:            return PLATFORM_KEY_F6;
		case AKEYCODE_F7:            return PLATFORM_KEY_F7;
		case AKEYCODE_F8:            return PLATFORM_KEY_F8;
		case AKEYCODE_F9:            return PLATFORM_KEY_F9;
		case AKEYCODE_F10:           return PLATFORM_KEY_F10;
		case AKEYCODE_F11:           return PLATFORM_KEY_F11;
		case AKEYCODE_F12:           return PLATFORM_KEY_F12;
		case AKEYCODE_DPAD_UP:       return PLATFORM_KEY_ARROW_UP;
		case AKEYCODE_DPAD_DOWN:     return PLATFORM_KEY_ARROW_DOWN;
		case AKEYCODE_DPAD_LEFT:     return PLATFORM_KEY_ARROW_LEFT;
		case AKEYCODE_DPAD_RIGHT:    return PLATFORM_KEY_ARROW_RIGHT;
		case AKEYCODE_SHIFT_LEFT:    return PLATFORM_KEY_SHIFT_LEFT;
		case AKEYCODE_SHIFT_RIGHT:   return PLATFORM_KEY_SHIFT_RIGHT;
		case AKEYCODE_CTRL_LEFT:     return PLATFORM_KEY_CONTROL_LEFT;
		case AKEYCODE_CTRL_RIGHT:    return PLATFORM_KEY_CONTROL_RIGHT;
		case AKEYCODE_ALT_LEFT:      return PLATFORM_KEY_ALT_LEFT;
		case AKEYCODE_ALT_RIGHT:     return PLATFORM_KEY_ALT_RIGHT;
		case AKEYCODE_DEL:           return PLATFORM_KEY_BACKSPACE;
		case AKEYCODE_TAB:           return PLATFORM_KEY_TAB;
		case AKEYCODE_ENTER:         return PLATFORM_KEY_ENTER;
		case AKEYCODE_ESCAPE:        return PLATFORM_KEY_ESCAPE;
		case AKEYCODE_BACK:          return PLATFORM_KEY_ESCAPE;
		case AKEYCODE_FORWARD_DEL:   return PLATFORM_KEY_DELETE;
		case AKEYCODE_INSERT:        return PLATFORM_KEY_INSERT;
		case AKEYCODE_MOVE_HOME:     return PLATFORM_KEY_HOME;
		case AKEYCODE_MOVE_END:      return PLATFORM_KEY_END;
		case AKEYCODE_PAGE_UP:       return PLATFORM_KEY_PAGE_UP;
		case AKEYCODE_PAGE_DOWN:     return PLATFORM_KEY_PAGE_DOWN;
		case AKEYCODE_SLASH:         return PLATFORM_KEY_SLASH;
		case AKEYCODE_BACKSLASH:     return PLATFORM_KEY_BACKSLASH;
		case AKEYCODE_LEFT_BRACKET:  return PLATFORM_KEY_BRACKET_LEFT;
		case AKEYCODE_RIGHT_BRACKET: return PLATFORM_KEY_BRACKET_RIGHT;
		case AKEYCODE_GRAVE:         return PLATFORM_KEY_BACKQUOTE;
		case AKEYCODE_PERIOD:        return PLATFORM_KEY_PERIOD;
		case AKEYCODE_MINUS:         return PLATFORM_KEY_MINUS;
		case AKEYCODE_EQUALS:        return PLATFORM_KEY_EQUAL;
		case AKEYCODE_COMMA:         return PLATFORM_KEY_COMMA;
		case AKEYCODE_SEMICOLON:     return PLATFORM_KEY_SEMICOLON;
		case AKEYCODE_SPACE:         return PLATFORM_KEY_SPACE;
		case AKEYCODE_BUTTON_A:      return PLATFORM_KEY_ENTER;
		case AKEYCODE_BUTTON_B:      return PLATFORM_KEY_ESCAPE;
		case AKEYCODE_BUTTON_START:  return PLATFORM_KEY_ENTER;
	}
	return PLATFORM_KEY_COUNT;
}

inline static void
_platform_key_set(Platform_Input *input, PLATFORM_KEY key, bool down, bool repeat = false)
{
	if (key == PLATFORM_KEY_COUNT)
		return;

	Platform_Key_State *state = input->keys + key;
	if (down)
	{
		if (!state->down && !repeat)
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

inline static Platform_Touch_State *
_platform_touch_find(Platform_Input *input, I32 id)
{
	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
		if (input->touches[i].down && input->touches[i].id == id)
			return input->touches + i;
	return nullptr;
}

inline static Platform_Touch_State *
_platform_touch_get_or_add(Platform_Input *input, I32 id)
{
	Platform_Touch_State *touch = _platform_touch_find(input, id);
	if (touch)
		return touch;

	for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
	{
		if (!input->touches[i].down)
		{
			input->touches[i].id = id;
			return input->touches + i;
		}
	}
	return nullptr;
}

inline static void
_platform_mouse_set_position(Platform_Window *window, F32 raw_x, F32 raw_y)
{
	I32 x = (I32)raw_x;
	I32 y = window->height > 0 ? (I32)window->height - 1 - (I32)raw_y : (I32)raw_y;
	window->input.mouse_dx = x - window->input.mouse_x;
	window->input.mouse_dy = y - window->input.mouse_y;
	window->input.mouse_x = x;
	window->input.mouse_y = y;
}

inline static void
_platform_touch_set_position(Platform_Window *window, Platform_Touch_State *touch, F32 raw_x, F32 raw_y)
{
	I32 x = (I32)raw_x;
	I32 y = window->height > 0 ? (I32)window->height - 1 - (I32)raw_y : (I32)raw_y;
	touch->dx = x - touch->x;
	touch->dy = y - touch->y;
	touch->x = x;
	touch->y = y;
}

inline static void
_platform_android_handle_touch(Platform_Window *window, AInputEvent *event)
{
	I32 action = ::AMotionEvent_getAction(event);
	I32 action_masked = action & AMOTION_EVENT_ACTION_MASK;
	I32 action_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
	U64 pointer_count = (U64)::AMotionEvent_getPointerCount(event);

	if (action_masked == AMOTION_EVENT_ACTION_CANCEL)
	{
		for (U32 i = 0; i < PLATFORM_TOUCH_MAX_COUNT; ++i)
		{
			if (window->input.touches[i].down)
			{
				window->input.touches[i].released = true;
				window->input.touches[i].down = false;
			}
		}
		_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_LEFT, false);
		return;
	}

	if (action_masked == AMOTION_EVENT_ACTION_MOVE)
	{
		for (U64 i = 0; i < pointer_count; ++i)
		{
			I32 id = ::AMotionEvent_getPointerId(event, i);
			Platform_Touch_State *touch = _platform_touch_get_or_add(&window->input, id);
			if (touch == nullptr)
				continue;

			_platform_touch_set_position(window, touch, ::AMotionEvent_getX(event, i), ::AMotionEvent_getY(event, i));
			touch->down = true;
			if (i == 0)
				_platform_mouse_set_position(window, ::AMotionEvent_getX(event, i), ::AMotionEvent_getY(event, i));
		}
		return;
	}

	if (action_index < 0 || (U64)action_index >= pointer_count)
		return;

	I32 id = ::AMotionEvent_getPointerId(event, action_index);
	Platform_Touch_State *touch = _platform_touch_get_or_add(&window->input, id);
	if (touch == nullptr)
		return;

	_platform_touch_set_position(window, touch, ::AMotionEvent_getX(event, action_index), ::AMotionEvent_getY(event, action_index));
	if (action_index == 0)
		_platform_mouse_set_position(window, ::AMotionEvent_getX(event, action_index), ::AMotionEvent_getY(event, action_index));

	switch (action_masked)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
		{
			touch->pressed = true;
			touch->down = true;
			if (action_index == 0)
				_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_LEFT, true);
			break;
		}
		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
		{
			touch->released = true;
			touch->down = false;
			if (action_index == 0)
				_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_LEFT, false);
			break;
		}
	}
}

inline static void
_platform_android_handle_mouse(Platform_Window *window, AInputEvent *event)
{
	_platform_mouse_set_position(window, ::AMotionEvent_getX(event, 0), ::AMotionEvent_getY(event, 0));

	I32 action = ::AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
	if (action == AMOTION_EVENT_ACTION_SCROLL)
		window->input.mouse_wheel += ::AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, 0);

	I32 button_state = ::AMotionEvent_getButtonState(event);
	_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_LEFT, (button_state & AMOTION_EVENT_BUTTON_PRIMARY) != 0);
	_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_RIGHT, (button_state & AMOTION_EVENT_BUTTON_SECONDARY) != 0);
	_platform_key_set(&window->input, PLATFORM_KEY_MOUSE_MIDDLE, (button_state & AMOTION_EVENT_BUTTON_TERTIARY) != 0);
}

inline static I32
_platform_android_handle_input_event(Platform_Window *window, AInputEvent *event)
{
	I32 event_type = ::AInputEvent_getType(event);
	if (event_type == AINPUT_EVENT_TYPE_KEY)
	{
		I32 action = ::AKeyEvent_getAction(event);
		if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP)
			return 0;

		I32 keycode = ::AKeyEvent_getKeyCode(event);
		PLATFORM_KEY key = _platform_key_from_android_keycode(keycode);
		_platform_key_set(&window->input, key, action == AKEY_EVENT_ACTION_DOWN, ::AKeyEvent_getRepeatCount(event) > 0);

		if (keycode == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP)
		{
			Platform_Context *context = (Platform_Context *)window->handle;
			context->close_requested = true;
		}

		return key != PLATFORM_KEY_COUNT;
	}

	if (event_type == AINPUT_EVENT_TYPE_MOTION)
	{
		I32 source = ::AInputEvent_getSource(event);
		if ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE)
		{
			_platform_android_handle_mouse(window, event);
			return 1;
		}

		if ((source & AINPUT_SOURCE_TOUCHSCREEN) == AINPUT_SOURCE_TOUCHSCREEN || (source & AINPUT_SOURCE_STYLUS) == AINPUT_SOURCE_STYLUS || (source & AINPUT_SOURCE_TOUCHPAD) == AINPUT_SOURCE_TOUCHPAD)
		{
			_platform_android_handle_touch(window, event);
			return 1;
		}

		if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK || (source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD)
			return 0;
	}

	return 0;
}

inline static void
_platform_android_process_input_queue(Platform_Context *context, Platform_Window *window)
{
	AInputEvent *event = nullptr;
	while (::AInputQueue_getEvent(context->input_queue, &event) >= 0)
	{
		if (::AInputQueue_preDispatchEvent(context->input_queue, event))
			continue;

		I32 handled = _platform_android_handle_input_event(window, event);
		::AInputQueue_finishEvent(context->input_queue, event, handled);
	}
}

inline static void
_platform_input_reset_transitions(Platform_Input *input)
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
	_platform_android_text_input_events_reset(input->text_input_events);
}

inline static void
_platform_android_activity_callbacks_set(ANativeActivity *activity)
{
	activity->callbacks->onStart = _platform_android_on_start;
	activity->callbacks->onResume = _platform_android_on_resume;
	activity->callbacks->onSaveInstanceState = _platform_android_on_save_instance_state;
	activity->callbacks->onPause = _platform_android_on_pause;
	activity->callbacks->onStop = _platform_android_on_stop;
	activity->callbacks->onDestroy = _platform_android_on_destroy;
	activity->callbacks->onWindowFocusChanged = _platform_android_on_window_focus_changed;
	activity->callbacks->onNativeWindowCreated = _platform_android_on_native_window_created;
	activity->callbacks->onNativeWindowResized = _platform_android_on_native_window_resized;
	activity->callbacks->onNativeWindowRedrawNeeded = _platform_android_on_native_window_redraw_needed;
	activity->callbacks->onNativeWindowDestroyed = _platform_android_on_native_window_destroyed;
	activity->callbacks->onContentRectChanged = _platform_android_on_content_rect_changed;
	activity->callbacks->onInputQueueCreated = _platform_android_on_input_queue_created;
	activity->callbacks->onInputQueueDestroyed = _platform_android_on_input_queue_destroyed;
	activity->callbacks->onConfigurationChanged = _platform_android_on_configuration_changed;
	activity->callbacks->onLowMemory = _platform_android_on_low_memory;
}

inline static bool
_platform_android_context_init(ANativeActivity *activity)
{
	validate(activity != nullptr, "[PLATFORM][ANDROID]: NativeActivity is required.");
	validate(activity->callbacks != nullptr, "[PLATFORM][ANDROID]: NativeActivity callbacks are required.");
	if (activity == nullptr || activity->callbacks == nullptr)
		return false;

	if (_platform_android_context)
	{
		Platform_Context *context = _platform_android_context;
		_platform_android_context_lock(context);
		bool can_rebind = context->activity == nullptr && context->activity_recreation_pending && !context->window_deinitialized && !context->window_deinit_in_progress;
		_platform_android_context_unlock(context);
		validate(can_rebind, "[PLATFORM][ANDROID]: Android context is already initialized.");
		if (!can_rebind)
			return false;

		bool bridge_initialized = _platform_android_jni_bridge_init(context, activity);
		validate(bridge_initialized, "[PLATFORM][ANDROID]: Core generated Java bridge is required. Use core.android.CoreNativeActivity in the Android manifest.");
		if (!bridge_initialized)
		{
			_platform_android_jni_bridge_deinit(context);
			_platform_android_context_lock(context);
			context->activity_recreation_pending = false;
			context->close_requested = true;
			_platform_android_context_unlock(context);
			return false;
		}

		_platform_android_context_lock(context);
		context->activity = activity;
		context->asset_manager = activity->assetManager;
		context->close_requested = false;
		context->started = true;
		context->paused = false;
		context->focused = false;
		context->surface_changed = true;
		context->activity_recreation_pending = false;
		activity->instance = context;
		_platform_android_activity_callbacks_set(activity);
		_platform_android_metrics_update_locked(context);
		Platform_Window_Presentation_Desc presentation_desc = context->presentation;
		Platform_Text_Input_Desc text_input_desc {
			.x = context->text_input_x,
			.y = context->text_input_y,
			.width = context->text_input_width,
			.height = context->text_input_height,
			.flags = context->text_input_flags,
			.action = context->text_input_action,
			.text = string_copy(context->text_input_text, memory::temp_allocator()),
			.selection_start = context->text_input_selection_start,
			.selection_end = context->text_input_selection_end,
			.composing_start = context->text_input_composing_start,
			.composing_end = context->text_input_composing_end,
			.enabled = context->text_input_active
		};
		_platform_android_context_unlock(context);
		_platform_android_window_presentation_set(context, presentation_desc);
		if (text_input_desc.enabled)
			_platform_android_text_input_set(context, text_input_desc);
		return false;
	}

	Platform_Context *context = memory::allocate_zeroed<Platform_Context>();
	context->activity = activity;
	context->asset_manager = activity->assetManager;
	context->started = true;
	context->text_input_events = array_init<Platform_Text_Input_Event>();
	validate(::pthread_mutex_init(&context->mutex, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize context mutex.");
	validate(::pthread_cond_init(&context->file_dialog_condition, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize file dialog condition.");
	bool bridge_initialized = _platform_android_jni_bridge_init(context, activity);
	validate(bridge_initialized, "[PLATFORM][ANDROID]: Core generated Java bridge is required. Use core.android.CoreNativeActivity in the Android manifest.");
	if (!bridge_initialized)
	{
		array_deinit(context->text_input_events);
		validate(::pthread_cond_destroy(&context->file_dialog_condition) == 0, "[PLATFORM][ANDROID]: Failed to destroy file dialog condition.");
		validate(::pthread_mutex_destroy(&context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to destroy context mutex.");
		memory::deallocate(context);
		return false;
	}

	activity->instance = context;
	_platform_android_activity_callbacks_set(activity);

	_platform_android_context = context;
	return true;
}

inline static void
_platform_android_context_try_deinit(Platform_Context *context, bool window_deinit_finished)
{
	if (context == nullptr)
		return;

	validate(context == _platform_android_context, "[PLATFORM][ANDROID]: Android context mismatch during deinit.");

	_platform_android_context_lock(context);
	if (window_deinit_finished)
		context->window_deinit_in_progress = false;

	if (context->activity || !context->window_deinitialized || context->window_deinit_in_progress)
	{
		_platform_android_context_unlock(context);
		return;
	}

	_platform_android_input_queue_detach_locked(context);
	_platform_android_file_dialog_cancel_locked(context);
	_platform_android_window_lease_release_locked(context);
	_platform_android_window_set_locked(context, nullptr);
	_platform_android_context = nullptr;
	_platform_android_context_unlock(context);

	_platform_android_jni_bridge_deinit(context);
	_platform_android_text_input_events_reset(context->text_input_events);
	array_deinit(context->text_input_events);
	if (context->text_input_text.capacity > 0)
		string_deinit(context->text_input_text);
	validate(::pthread_cond_destroy(&context->file_dialog_condition) == 0, "[PLATFORM][ANDROID]: Failed to destroy file dialog condition.");
	validate(::pthread_mutex_destroy(&context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to destroy context mutex.");
	memory::deallocate(context);
}

extern "C" CORE_API bool
platform_android_native_activity_on_create(void *native_activity, void *saved_state, U64 saved_state_size)
{
	unused(saved_state, saved_state_size);
	return _platform_android_context_init((ANativeActivity *)native_activity);
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeActivityDestroying(JNIEnv *, jclass, jboolean changing_configurations, jboolean finishing)
{
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		context->activity_recreation_pending = changing_configurations && !finishing;
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeMemoryPressure(JNIEnv *, jclass)
{
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		context->low_memory = true;
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeFileDialogResult(JNIEnv *env, jclass, jlong request_id, jboolean accepted, jstring uri)
{
	const char *uri_chars = nullptr;
	if (accepted && uri)
		uri_chars = env->GetStringUTFChars(uri, nullptr);
	bool result_accepted = accepted && uri_chars != nullptr;

	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->file_dialog_waiting && context->file_dialog_active_id == (U64)request_id)
		{
			if (context->file_dialog_path.capacity > 0)
				string_deinit(context->file_dialog_path);
			context->file_dialog_path = result_accepted ? string_from(uri_chars) : String {};
			context->file_dialog_accepted = result_accepted;
			context->file_dialog_completed = true;
			validate(::pthread_cond_signal(&context->file_dialog_condition) == 0, "[PLATFORM][ANDROID]: Failed to signal file dialog condition.");
		}
		_platform_android_context_unlock(context);
	}

	if (uri_chars)
		env->ReleaseStringUTFChars(uri, uri_chars);
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputCommit(JNIEnv *env, jclass, jbyteArray text)
{
	if (text == nullptr)
		return;

	jsize text_size = env->GetArrayLength(text);
	jbyte *text_bytes = env->GetByteArrayElements(text, nullptr);
	if (text_bytes)
	{
		const char *text_begin = (const char *)text_bytes;
		Platform_Text_Input_Event event {
			.type = PLATFORM_TEXT_INPUT_EVENT_COMMIT,
			.text = string_from(text_begin, text_begin + text_size)
		};
		Platform_Context *context = _platform_android_context;
		if (context)
		{
			_platform_android_context_lock(context);
			if (context->text_input_active)
				array_push(context->text_input_events, event);
			else
				string_deinit(event.text);
			_platform_android_context_unlock(context);
		}
		else
		{
			string_deinit(event.text);
		}
		env->ReleaseByteArrayElements(text, text_bytes, JNI_ABORT);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputCompose(JNIEnv *env, jclass, jbyteArray text)
{
	if (text == nullptr)
		return;

	jsize text_size = env->GetArrayLength(text);
	jbyte *text_bytes = env->GetByteArrayElements(text, nullptr);
	if (text_bytes)
	{
		const char *text_begin = (const char *)text_bytes;
		Platform_Text_Input_Event event {
			.type = PLATFORM_TEXT_INPUT_EVENT_COMPOSE,
			.text = string_from(text_begin, text_begin + text_size)
		};
		Platform_Context *context = _platform_android_context;
		if (context)
		{
			_platform_android_context_lock(context);
			if (context->text_input_active)
				array_push(context->text_input_events, event);
			else
				string_deinit(event.text);
			_platform_android_context_unlock(context);
		}
		else
		{
			string_deinit(event.text);
		}
		env->ReleaseByteArrayElements(text, text_bytes, JNI_ABORT);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputComposeEnd(JNIEnv *, jclass)
{
	Platform_Text_Input_Event event {
		.type = PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END
	};
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->text_input_active)
			array_push(context->text_input_events, event);
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputComposeRegion(JNIEnv *, jclass, jint start, jint end)
{
	Platform_Text_Input_Event event {
		.type = PLATFORM_TEXT_INPUT_EVENT_COMPOSE_REGION,
		.composing_start = (U32)start,
		.composing_end = (U32)end
	};
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->text_input_active)
			array_push(context->text_input_events, event);
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputDelete(JNIEnv *, jclass, jint before, jint after)
{
	Platform_Text_Input_Event event {
		.type = PLATFORM_TEXT_INPUT_EVENT_DELETE_SURROUNDING,
		.delete_before = before,
		.delete_after = after
	};
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->text_input_active)
			array_push(context->text_input_events, event);
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputSelection(JNIEnv *, jclass, jint start, jint end)
{
	Platform_Text_Input_Event event {
		.type = PLATFORM_TEXT_INPUT_EVENT_SELECTION,
		.selection_start = (U32)start,
		.selection_end = (U32)end
	};
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->text_input_active)
			array_push(context->text_input_events, event);
		_platform_android_context_unlock(context);
	}
}

extern "C" JNIEXPORT void JNICALL
Java_core_android_CoreNativeActivity_nativeTextInputAction(JNIEnv *, jclass, jint action)
{
	Platform_Text_Input_Action platform_action = PLATFORM_TEXT_INPUT_ACTION_NONE;
	if (action >= PLATFORM_TEXT_INPUT_ACTION_NONE && action <= PLATFORM_TEXT_INPUT_ACTION_PREVIOUS)
		platform_action = (Platform_Text_Input_Action)action;
	Platform_Text_Input_Event event {
		.type = PLATFORM_TEXT_INPUT_EVENT_ACTION,
		.action = platform_action
	};
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		_platform_android_context_lock(context);
		if (context->text_input_active)
			array_push(context->text_input_events, event);
		_platform_android_context_unlock(context);
	}
}

inline static String
_platform_android_activity_string_method(Platform_Context *context, jmethodID *method, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	if (context)
	{
		bool needs_detach = false;
		JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
		if (env)
		{
			jmethodID method_ref = nullptr;
			jobject activity_object = _platform_android_activity_ref_get(context, env, method, &method_ref);
			if (activity_object == nullptr)
			{
				_platform_android_jni_detach(context, needs_detach);
				return result;
			}

			jstring value_string = (jstring)env->CallObjectMethod(activity_object, method_ref);
			bool call_failed = _platform_android_jni_clear_exception(env);
			if (!call_failed && value_string)
			{
				const char *value_chars = env->GetStringUTFChars(value_string, nullptr);
				if (value_chars)
				{
					string_deinit(result);
					result = string_from(value_chars, allocator);
					env->ReleaseStringUTFChars(value_string, value_chars);
				}
			}
			if (value_string)
				env->DeleteLocalRef(value_string);
			env->DeleteLocalRef(activity_object);
			_platform_android_jni_detach(context, needs_detach);
		}
	}
	return result;
}

inline static I32
_platform_android_content_open_fd(const char *uri, const char *mode)
{
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return -1;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->content_open_fd_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return -1;
	}

	jstring uri_string = env->NewStringUTF(uri);
	jstring mode_string = env->NewStringUTF(mode);
	I32 fd = -1;
	if (uri_string && mode_string)
	{
		fd = env->CallIntMethod(activity_object, method, uri_string, mode_string);
		if (_platform_android_jni_clear_exception(env))
			fd = -1;
	}
	else
	{
		_platform_android_jni_clear_exception(env);
	}

	if (uri_string)
		env->DeleteLocalRef(uri_string);
	if (mode_string)
		env->DeleteLocalRef(mode_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return fd;
}

inline static I64
_platform_android_content_size(const char *uri)
{
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return -1;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->content_size_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return -1;
	}

	jstring uri_string = env->NewStringUTF(uri);
	I64 size = -1;
	if (uri_string)
	{
		size = (I64)env->CallLongMethod(activity_object, method, uri_string);
		if (_platform_android_jni_clear_exception(env))
			size = -1;
		env->DeleteLocalRef(uri_string);
	}
	else
	{
		_platform_android_jni_clear_exception(env);
	}

	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return size;
}

inline static bool
_platform_android_content_delete(const char *uri)
{
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return false;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->content_delete_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return false;
	}

	jstring uri_string = env->NewStringUTF(uri);
	bool result = false;
	if (uri_string)
	{
		result = env->CallBooleanMethod(activity_object, method, uri_string);
		if (_platform_android_jni_clear_exception(env))
			result = false;
		env->DeleteLocalRef(uri_string);
	}
	else
	{
		_platform_android_jni_clear_exception(env);
	}

	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

inline static bool
_platform_android_content_is_directory(const char *uri)
{
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return false;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->content_is_directory_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return false;
	}

	jstring uri_string = env->NewStringUTF(uri);
	bool result = false;
	if (uri_string)
	{
		result = env->CallBooleanMethod(activity_object, method, uri_string);
		if (_platform_android_jni_clear_exception(env))
			result = false;
		env->DeleteLocalRef(uri_string);
	}
	else
	{
		_platform_android_jni_clear_exception(env);
	}

	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

inline static String
_platform_android_content_display_name(const char *uri, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return result;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->content_display_name_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jstring uri_string = env->NewStringUTF(uri);
	if (uri_string == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jstring name_string = (jstring)env->CallObjectMethod(activity_object, method, uri_string);
	env->DeleteLocalRef(uri_string);
	if (_platform_android_jni_clear_exception(env) || name_string == nullptr)
	{
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	const char *name_chars = env->GetStringUTFChars(name_string, nullptr);
	if (name_chars)
	{
		string_deinit(result);
		result = string_from(name_chars, allocator);
		env->ReleaseStringUTFChars(name_string, name_chars);
	}
	env->DeleteLocalRef(name_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

inline static Array<String>
_platform_android_content_list_files_with_method(const char *uri, const String &extension_filter, jmethodID *method, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return files;

	jmethodID method_ref = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, method, &method_ref);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return files;
	}

	jstring uri_string = env->NewStringUTF(uri);
	jstring extension_string = env->NewStringUTF(extension_filter.data ? extension_filter.data : "");
	if (uri_string == nullptr || extension_string == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		if (uri_string)
			env->DeleteLocalRef(uri_string);
		if (extension_string)
			env->DeleteLocalRef(extension_string);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return files;
	}

	jstring files_string = (jstring)env->CallObjectMethod(activity_object, method_ref, uri_string, extension_string);
	env->DeleteLocalRef(uri_string);
	env->DeleteLocalRef(extension_string);
	if (_platform_android_jni_clear_exception(env) || files_string == nullptr)
	{
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return files;
	}

	const char *files_chars = env->GetStringUTFChars(files_string, nullptr);
	if (files_chars)
	{
		const char *begin = files_chars;
		for (const char *at = files_chars; ; ++at)
		{
			if (*at == '\n' || *at == '\0')
			{
				if (at != begin)
					array_push(files, string_from(begin, at, allocator));
				if (*at == '\0')
					break;
				begin = at + 1;
			}
		}
		env->ReleaseStringUTFChars(files_string, files_chars);
	}
	env->DeleteLocalRef(files_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return files;
}

inline static Array<String>
_platform_android_content_list_files(const char *uri, const String &extension_filter, memory::Allocator *allocator)
{
	Platform_Context *context = _platform_android_context_get();
	return _platform_android_content_list_files_with_method(uri, extension_filter, &context->content_list_files_method, allocator);
}

inline static Array<String>
_platform_android_content_list_files_recursive(const char *uri, const String &extension_filter, memory::Allocator *allocator)
{
	Platform_Context *context = _platform_android_context_get();
	return _platform_android_content_list_files_with_method(uri, extension_filter, &context->content_list_files_recursive_method, allocator);
}

inline static String
_platform_android_content_call_uri_string_method(const char *uri, const String &value, jmethodID *method, memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return result;

	jmethodID method_ref = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, method, &method_ref);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jstring uri_string = env->NewStringUTF(uri);
	jstring value_string = env->NewStringUTF(value.data ? value.data : "");
	if (uri_string == nullptr || value_string == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		if (uri_string)
			env->DeleteLocalRef(uri_string);
		if (value_string)
			env->DeleteLocalRef(value_string);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jstring result_string = (jstring)env->CallObjectMethod(activity_object, method_ref, uri_string, value_string);
	env->DeleteLocalRef(uri_string);
	env->DeleteLocalRef(value_string);
	if (_platform_android_jni_clear_exception(env) || result_string == nullptr)
	{
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	const char *result_chars = env->GetStringUTFChars(result_string, nullptr);
	if (result_chars)
	{
		string_deinit(result);
		result = string_from(result_chars, allocator);
		env->ReleaseStringUTFChars(result_string, result_chars);
	}
	env->DeleteLocalRef(result_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

inline static void
_platform_android_file_dialog_extension(char *extension, U64 extension_size, const char *filters)
{
	if (extension_size == 0)
		return;

	extension[0] = '\0';
	if (filters == nullptr || filters[0] == '\0')
		return;

	const char *pattern = filters;
	while (*pattern != '\0')
		++pattern;
	++pattern;

	while (*pattern == '*' || *pattern == '.' || *pattern == ' ')
		++pattern;

	U64 count = 0;
	while (pattern[count] != '\0' && pattern[count] != ';' && pattern[count] != ',' && count + 1 < extension_size)
	{
		extension[count] = pattern[count];
		++count;
	}
	extension[count] = '\0';
}

inline static String
_platform_android_file_dialog_run(const char *filters, bool save, memory::Allocator *allocator)
{
	String result = string_init(allocator);

	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return result;

	jmethodID *method = save ? &context->file_dialog_save_method : &context->file_dialog_open_method;
	jmethodID method_ref = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, method, &method_ref);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	char extension[64] = {};
	_platform_android_file_dialog_extension(extension, sizeof(extension), filters);
	jstring extension_string = env->NewStringUTF(extension);
	if (extension_string == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	U64 request_id = 0;
	_platform_android_context_lock(context);
	if (context->activity == nullptr || context->file_dialog_waiting)
	{
		_platform_android_context_unlock(context);
		env->DeleteLocalRef(extension_string);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	request_id = ++context->file_dialog_next_id;
	if (request_id == 0)
		request_id = ++context->file_dialog_next_id;
	context->file_dialog_active_id = request_id;
	if (context->file_dialog_path.capacity > 0)
		string_deinit(context->file_dialog_path);
	context->file_dialog_path = {};
	context->file_dialog_waiting = true;
	context->file_dialog_completed = false;
	context->file_dialog_accepted = false;
	_platform_android_context_unlock(context);

	env->CallVoidMethod(activity_object, method_ref, (jlong)request_id, extension_string);
	bool call_failed = _platform_android_jni_clear_exception(env);
	env->DeleteLocalRef(extension_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);

	if (call_failed)
	{
		_platform_android_context_lock(context);
		if (context->file_dialog_waiting && context->file_dialog_active_id == request_id)
		{
			context->file_dialog_waiting = false;
			context->file_dialog_completed = false;
			context->file_dialog_accepted = false;
			context->file_dialog_active_id = 0;
			if (context->file_dialog_path.capacity > 0)
				string_deinit(context->file_dialog_path);
			context->file_dialog_path = {};
		}
		_platform_android_context_unlock(context);
		return result;
	}

	_platform_android_context_lock(context);
	while (context->file_dialog_waiting && !context->file_dialog_completed)
		validate(::pthread_cond_wait(&context->file_dialog_condition, &context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to wait for file dialog condition.");

	bool accepted = context->file_dialog_completed && context->file_dialog_accepted;
	if (accepted)
	{
		string_deinit(result);
		result = string_copy(context->file_dialog_path, allocator);
	}
	context->file_dialog_waiting = false;
	context->file_dialog_completed = false;
	context->file_dialog_accepted = false;
	context->file_dialog_active_id = 0;
	if (context->file_dialog_path.capacity > 0)
		string_deinit(context->file_dialog_path);
	context->file_dialog_path = {};
	_platform_android_context_unlock(context);
	return result;
}

inline static String
_platform_android_directory_dialog_run(memory::Allocator *allocator)
{
	String result = string_init(allocator);

	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return result;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->directory_dialog_open_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	U64 request_id = 0;
	_platform_android_context_lock(context);
	if (context->activity == nullptr || context->file_dialog_waiting)
	{
		_platform_android_context_unlock(context);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	request_id = ++context->file_dialog_next_id;
	if (request_id == 0)
		request_id = ++context->file_dialog_next_id;
	context->file_dialog_active_id = request_id;
	if (context->file_dialog_path.capacity > 0)
		string_deinit(context->file_dialog_path);
	context->file_dialog_path = {};
	context->file_dialog_waiting = true;
	context->file_dialog_completed = false;
	context->file_dialog_accepted = false;
	_platform_android_context_unlock(context);

	env->CallVoidMethod(activity_object, method, (jlong)request_id);
	bool call_failed = _platform_android_jni_clear_exception(env);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);

	if (call_failed)
	{
		_platform_android_context_lock(context);
		if (context->file_dialog_waiting && context->file_dialog_active_id == request_id)
		{
			context->file_dialog_waiting = false;
			context->file_dialog_completed = false;
			context->file_dialog_accepted = false;
			context->file_dialog_active_id = 0;
			if (context->file_dialog_path.capacity > 0)
				string_deinit(context->file_dialog_path);
			context->file_dialog_path = {};
		}
		_platform_android_context_unlock(context);
		return result;
	}

	_platform_android_context_lock(context);
	while (context->file_dialog_waiting && !context->file_dialog_completed)
		validate(::pthread_cond_wait(&context->file_dialog_condition, &context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to wait for directory dialog condition.");

	bool accepted = context->file_dialog_completed && context->file_dialog_accepted;
	if (accepted)
	{
		string_deinit(result);
		result = string_copy(context->file_dialog_path, allocator);
	}
	context->file_dialog_waiting = false;
	context->file_dialog_completed = false;
	context->file_dialog_accepted = false;
	context->file_dialog_active_id = 0;
	if (context->file_dialog_path.capacity > 0)
		string_deinit(context->file_dialog_path);
	context->file_dialog_path = {};
	_platform_android_context_unlock(context);
	return result;
}

inline static String
_platform_android_file_read_handle(Platform_File_Handle handle, U64 size, memory::Allocator *allocator)
{
	String content = string_init(allocator);
	if (size > 0)
	{
		string_resize(content, size);
		U64 bytes_read = platform_file_read(handle, content.data, content.count);
		if (bytes_read != content.count)
			string_resize(content, bytes_read);
		return content;
	}

	char buffer[8192];
	while (true)
	{
		U64 bytes_read = platform_file_read(handle, buffer, sizeof(buffer));
		if (bytes_read == 0)
			break;

		U64 old_count = content.count;
		string_resize(content, old_count + bytes_read);
		::memcpy(content.data + old_count, buffer, bytes_read);
	}

	return content;
}

String
platform_resource_read(const String &path, memory::Allocator *allocator)
{
	String content = string_init(allocator);
	Platform_Context *context = _platform_android_context_get();
	if (context->asset_manager == nullptr || path.count == 0)
		return content;

	AAsset *asset = ::AAssetManager_open(context->asset_manager, path.data, AASSET_MODE_BUFFER);
	if (asset == nullptr)
		return content;
	DEFER(::AAsset_close(asset););

	U64 asset_size = (U64)::AAsset_getLength64(asset);
	if (asset_size == 0)
		return content;

	string_resize(content, asset_size);

	U64 bytes_read = 0;
	while (bytes_read < asset_size)
	{
		I32 read_result = ::AAsset_read(asset, content.data + bytes_read, asset_size - bytes_read);
		if (read_result <= 0)
		{
			string_resize(content, bytes_read);
			break;
		}
		bytes_read += (U64)read_result;
	}

	return content;
}

Array<String>
platform_resource_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	Array<String> files = array_init<String>(allocator);
	Platform_Context *context = _platform_android_context_get();
	if (context->asset_manager == nullptr)
		return files;

	AAssetDir *asset_dir = ::AAssetManager_openDir(context->asset_manager, directory.data);
	if (asset_dir == nullptr)
		return files;
	DEFER(::AAssetDir_close(asset_dir););

	const char *file_name = nullptr;
	while ((file_name = ::AAssetDir_getNextFileName(asset_dir)) != nullptr)
	{
		String file_name_string = string_from(file_name, memory::temp_allocator());
		if (_platform_extension_matches(file_name_string, extension_filter))
			array_push(files, string_copy(file_name_string, allocator));
	}

	return files;
}

bool
platform_path_is_valid(const String &path)
{
	if (_platform_android_is_content_uri(path.data))
		return _platform_android_content_is_directory(path.data) || platform_file_exists(path.data);

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0;
}

bool
platform_path_is_file(const String &path)
{
	if (_platform_android_is_content_uri(path.data))
		return !_platform_android_content_is_directory(path.data) && platform_file_exists(path.data);

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISREG(path_stat.st_mode);
}

bool
platform_path_is_directory(const String &path)
{
	if (_platform_android_is_content_uri(path.data))
		return _platform_android_content_is_directory(path.data);

	struct stat path_stat = {};
	return ::stat(path.data, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

String
platform_path_get_absolute(const String &path, memory::Allocator *allocator)
{
	if (_platform_android_is_content_uri(path.data))
		return string_copy(path, allocator);

	char buffer[PATH_MAX] = {};
	if (::realpath(path.data, buffer))
		return string_from(buffer, allocator);

	String full_path = platform_path_get_current_working_directory(allocator);
	if (full_path.count > 0 && full_path[full_path.count - 1] != '/')
		string_append(full_path, '/');
	string_append(full_path, path);
	return full_path;
}

String
platform_path_get_directory(const String &path, memory::Allocator *allocator)
{
	if (!platform_path_is_valid(path))
		return string_literal("");

	if (_platform_android_is_content_uri(path.data))
		return platform_path_is_directory(path) ? string_copy(path, allocator) : string_init(allocator);

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
	validate(::getcwd(buffer, PATH_MAX) != nullptr, "[PLATFORM][ANDROID]: Failed to get current working directory.");
	return string_from(buffer, allocator);
}

String
platform_path_get_temp_directory(memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		string_deinit(result);
		result = _platform_android_activity_string_method(context, &context->cache_directory_method, allocator);
	}

	if (result.count == 0)
	{
		if (result.capacity > 0)
			string_deinit(result);
		result = platform_environment_variable_get("TMPDIR", allocator);
		if (result.count == 0)
			result = string_from("/data/local/tmp/", allocator);
	}

	if (result.count > 0 && result[result.count - 1] != '/')
		string_append(result, '/');
	return result;
}

String
platform_path_get_app_data_directory(memory::Allocator *allocator)
{
	Platform_Context *context = _platform_android_context;
	String result = context ? _platform_android_activity_string_method(context, &context->app_data_directory_method, allocator) : string_init(allocator);
	if (result.count == 0)
	{
		if (result.capacity > 0)
			string_deinit(result);
		result = {};
		if (context)
		{
			_platform_android_context_lock(context);
			if (context->activity && context->activity->internalDataPath)
				result = string_from(context->activity->internalDataPath, allocator);
			else if (context->activity && context->activity->externalDataPath)
				result = string_from(context->activity->externalDataPath, allocator);
			_platform_android_context_unlock(context);
		}
	}

	if (result.count == 0)
		return platform_path_get_temp_directory(allocator);

	if (result[result.count - 1] != '/')
		string_append(result, '/');
	return result;
}

String
platform_path_get_cache_directory(memory::Allocator *allocator)
{
	String result = string_init(allocator);
	Platform_Context *context = _platform_android_context;
	if (context)
	{
		string_deinit(result);
		result = _platform_android_activity_string_method(context, &context->cache_directory_method, allocator);
	}

	if (result.count == 0)
	{
		if (result.capacity > 0)
			string_deinit(result);
		result = platform_environment_variable_get("TMPDIR", allocator);
		if (result.count == 0)
			result = string_from("/data/local/tmp/", allocator);
	}

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
	validate(::chdir(path.data) == 0, "[PLATFORM][ANDROID]: Failed to set current working directory.");
}

String
platform_path_get_executable_path(memory::Allocator *allocator)
{
	char module_path_relative[PATH_MAX + 1] = {};
	char module_path_absolute[PATH_MAX + 1] = {};

	I64 module_path_relative_length = ::readlink("/proc/self/exe", module_path_relative, sizeof(module_path_relative) - 1);
	validate(module_path_relative_length != -1 && module_path_relative_length < (I64)sizeof(module_path_relative), "[PLATFORM][ANDROID]: Failed to get executable path.");

	char *path_absolute = ::realpath(module_path_relative, module_path_absolute);
	validate(path_absolute == module_path_absolute, "[PLATFORM][ANDROID]: Failed to get executable absolute path.");
	return string_from(path_absolute, allocator);
}

String
platform_path_get_current_module_path(memory::Allocator *allocator)
{
	Dl_info info = {};
	if (::dladdr((void *)&platform_path_get_current_module_path, &info) == 0 || info.dli_fname == nullptr)
		return string_literal("");

	char path_absolute[PATH_MAX + 1] = {};
	char *absolute = ::realpath(info.dli_fname, path_absolute);
	if (absolute)
		return string_from(absolute, allocator);
	return string_from(info.dli_fname, allocator);
}

String
platform_path_get_file_name(const String &path, memory::Allocator *allocator)
{
	if (_platform_android_is_content_uri(path.data))
		return _platform_android_content_display_name(path.data, allocator);

	String path_temp = string_copy(path, memory::temp_allocator());
	string_replace(path_temp, "\\", "/");
	Array<String> splits = string_split(path_temp, "/", true, memory::temp_allocator());
	return string_copy(array_back(splits), allocator);
}

String
platform_path_read_file(const String &path, memory::Allocator *allocator)
{
	Platform_File_Handle file = platform_file_open(path, PLATFORM_FILE_MODE_READ);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return string_init(allocator);
	DEFER(platform_file_close(file));

	return _platform_android_file_read_handle(file, platform_file_size(path.data), allocator);
}

U64
platform_path_write_file(const String &path, Memory_Block block)
{
	return platform_file_write(path.data, block);
}

inline static String
_platform_android_path_join(const String &directory, const String &name, memory::Allocator *allocator)
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
_platform_android_path_parent(const String &path, memory::Allocator *allocator)
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

	if (_platform_android_is_content_uri(directory.data))
		return _platform_android_content_list_files(directory.data, extension_filter, allocator);

	String directory_temp = string_copy(directory, memory::temp_allocator());
	string_replace(directory_temp, "\\", "/");

	if (!platform_path_is_directory(directory_temp))
		return files;

	DIR *dir = ::opendir(directory_temp.data);
	if (!dir)
		return files;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][ANDROID]: Failed to close directory."););

	struct dirent *entry = nullptr;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		if (entry->d_type == DT_DIR)
			continue;

		String file_name = string_from(entry->d_name, memory::temp_allocator());
		if (!_platform_extension_matches(file_name, extension_filter))
			continue;

		array_push(files, string_copy(file_name, allocator));
	}

	return files;
}

inline static void
_platform_android_path_list_files_recursive(Array<String> &files, const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	DIR *dir = ::opendir(directory.data);
	if (!dir)
		return;
	DEFER(validate(::closedir(dir) == 0, "[PLATFORM][ANDROID]: Failed to close recursive directory."););

	struct dirent *entry = nullptr;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		String file_name = string_from(entry->d_name, memory::temp_allocator());
		if (file_name == "." || file_name == "..")
			continue;

		String child_path = _platform_android_path_join(directory, file_name, memory::temp_allocator());
		if (platform_path_is_directory(child_path))
		{
			_platform_android_path_list_files_recursive(files, child_path, extension_filter, allocator);
			continue;
		}

		if (_platform_extension_matches(file_name, extension_filter))
			array_push(files, string_copy(child_path, allocator));
	}
}

Array<String>
platform_path_list_files_recursive(const String &directory, const String &extension_filter, memory::Allocator *allocator)
{
	if (_platform_android_is_content_uri(directory.data))
		return _platform_android_content_list_files_recursive(directory.data, extension_filter, allocator);

	Array<String> files = array_init<String>(allocator);
	String directory_temp = string_copy(directory, memory::temp_allocator());
	string_replace(directory_temp, "\\", "/");
	_platform_android_path_list_files_recursive(files, directory_temp, extension_filter, allocator);
	return files;
}

String
platform_path_create_file(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (_platform_android_is_content_uri(directory.data))
	{
		Platform_Context *context = _platform_android_context_get();
		return _platform_android_content_call_uri_string_method(directory.data, name, &context->content_create_file_method, allocator);
	}

	String result = _platform_android_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	I32 file_handle = ::open(result.data, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
	if (file_handle == -1)
	{
		string_deinit(result);
		return string_init(allocator);
	}
	validate(::close(file_handle) == 0, "[PLATFORM][ANDROID]: Failed to close created file.");
	return result;
}

String
platform_path_create_directory(const String &directory, const String &name, memory::Allocator *allocator)
{
	if (_platform_android_is_content_uri(directory.data))
	{
		Platform_Context *context = _platform_android_context_get();
		return _platform_android_content_call_uri_string_method(directory.data, name, &context->content_create_directory_method, allocator);
	}

	String result = _platform_android_path_join(directory, name, allocator);
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
	if (_platform_android_is_content_uri(path.data))
	{
		Platform_Context *context = _platform_android_context_get();
		return _platform_android_content_call_uri_string_method(path.data, name, &context->content_rename_method, allocator);
	}

	String directory = _platform_android_path_parent(path, memory::temp_allocator());
	String result = _platform_android_path_join(directory, name, allocator);
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
	bool path_is_content = _platform_android_is_content_uri(path.data);
	bool directory_is_content = _platform_android_is_content_uri(directory.data);
	if (path_is_content && directory_is_content)
	{
		Platform_Context *context = _platform_android_context_get();
		return _platform_android_content_call_uri_string_method(path.data, directory, &context->content_move_method, allocator);
	}

	String name = platform_path_get_file_name(path, memory::temp_allocator());
	String result = directory_is_content ? platform_path_create_file(directory, name, allocator) : _platform_android_path_join(directory, name, allocator);
	if (result.count == 0)
		return result;

	if (!path_is_content && !directory_is_content && ::rename(path.data, result.data) == 0)
		return result;

	if (platform_path_is_file(path) && platform_file_copy(path.data, result.data) && platform_file_delete(path.data))
		return result;

	if (platform_path_is_file(result))
		platform_file_delete(result.data);
	string_deinit(result);
	return string_init(allocator);
}

Platform_Api
platform_api_init(const char *filepath)
{
	Platform_Api self = {};

	char path[PATH_MAX] = {};
	I32 path_length = ::snprintf(path, sizeof(path), "%s.so", filepath);
	validate(path_length > 0 && path_length < (I32)sizeof(path), "[PLATFORM][ANDROID]: Library path is too long.");

	self.handle = ::dlopen(path, RTLD_NOW);
	if (self.handle == nullptr)
		self.handle = ::dlopen(filepath, RTLD_NOW);
	validate(self.handle != nullptr, "[PLATFORM][ANDROID]: Failed to load library.");

	platform_api_proc proc = (platform_api_proc)::dlsym(self.handle, "platform_api");
	validate(proc != nullptr, "[PLATFORM][ANDROID]: Failed to get proc platform_api.");

	self.api = proc(nullptr, PLATFORM_API_STATE_INIT);
	validate(self.api != nullptr, "[PLATFORM][ANDROID]: Failed to get api.");

	_platform_copy_string(self.filepath, sizeof(self.filepath), filepath);
	return self;
}

void
platform_api_deinit(Platform_Api *self)
{
	if (self->api)
	{
		platform_api_proc proc = (platform_api_proc)::dlsym(self->handle, "platform_api");
		validate(proc != nullptr, "[PLATFORM][ANDROID]: Failed to get proc platform_api.");
		self->api = proc(self->api, PLATFORM_API_STATE_DEINIT);
	}

	if (self->handle)
		::dlclose(self->handle);
}

void *
platform_api_load(Platform_Api *self)
{
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
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][ANDROID]: Cannot commit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][ANDROID]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][ANDROID]: Virtual memory block size is not page-aligned.");

	return ::mprotect(block.data, block.size, PROT_READ | PROT_WRITE) == 0;
}

bool
platform_virtual_memory_decommit(Memory_Block block)
{
	U64 page_size = platform_virtual_memory_get_page_size();
	validate(block.data != nullptr && block.size > 0, "[PLATFORM][ANDROID]: Cannot decommit an empty virtual memory block.");
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][ANDROID]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][ANDROID]: Virtual memory block size is not page-aligned.");

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
	validate(((U64)block.data & (page_size - 1)) == 0, "[PLATFORM][ANDROID]: Virtual memory block address is not page-aligned.");
	validate(block.size == platform_virtual_memory_page_align(block.size), "[PLATFORM][ANDROID]: Virtual memory block size is not page-aligned.");

	[[maybe_unused]] I32 result = ::munmap(block.data, block.size);
	validate(result == 0, "[PLATFORM][ANDROID]: Failed to release virtual memory.");
}

U32
platform_get_logical_processor_count()
{
	long count = ::sysconf(_SC_NPROCESSORS_ONLN);
	return count > 0 ? (U32)count : U32(1);
}

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
	validate(desc.function != nullptr, "[PLATFORM][ANDROID]: Thread function is not valid.");

	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->function = desc.function;
	self->data = desc.data;
	if (desc.name != nullptr)
		self->name = string_from(desc.name);
	validate(::pthread_create(&self->handle, nullptr, _platform_thread_main_routine, self) == 0, "[PLATFORM][ANDROID]: Failed to create thread.");
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

	validate(::pthread_join(self->handle, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to join thread.");
	self->joined = true;
}

void
platform_thread_sleep(U32 milliseconds)
{
	struct timespec ts = {};
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	::nanosleep(&ts, nullptr);
}

void
platform_thread_set_current_name(const char *name)
{
	if (name == nullptr || name[0] == '\0')
		return;

	validate(::pthread_setname_np(::pthread_self(), name) == 0, "[PLATFORM][ANDROID]: Failed to set thread name.");
}

struct Platform_Mutex
{
	pthread_mutex_t handle;
};

Platform_Mutex *
platform_mutex_init()
{
	Platform_Mutex *self = memory::allocate_zeroed<Platform_Mutex>();
	validate(::pthread_mutex_init(&self->handle, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize mutex.");
	return self;
}

void
platform_mutex_deinit(Platform_Mutex *self)
{
	validate(::pthread_mutex_destroy(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to destroy mutex.");
	memory::deallocate(self);
}

void
platform_mutex_lock(Platform_Mutex *self)
{
	validate(::pthread_mutex_lock(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to lock mutex.");
}

void
platform_mutex_unlock(Platform_Mutex *self)
{
	validate(::pthread_mutex_unlock(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to unlock mutex.");
}

struct Platform_Condition_Variable
{
	pthread_cond_t handle;
};

Platform_Condition_Variable *
platform_condition_variable_init()
{
	Platform_Condition_Variable *self = memory::allocate_zeroed<Platform_Condition_Variable>();
	validate(::pthread_cond_init(&self->handle, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize condition variable.");
	return self;
}

void
platform_condition_variable_deinit(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_destroy(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to destroy condition variable.");
	memory::deallocate(self);
}

void
platform_condition_variable_wait(Platform_Condition_Variable *self, Platform_Mutex *mutex)
{
	validate(::pthread_cond_wait(&self->handle, &mutex->handle) == 0, "[PLATFORM][ANDROID]: Failed to wait for condition variable.");
}

void
platform_condition_variable_signal(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_signal(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to signal condition variable.");
}

void
platform_condition_variable_broadcast(Platform_Condition_Variable *self)
{
	validate(::pthread_cond_broadcast(&self->handle) == 0, "[PLATFORM][ANDROID]: Failed to broadcast condition variable.");
}

Platform_Window
platform_window_init(U32, U32, const char *)
{
	Platform_Context *context = _platform_android_context_get();

	_platform_android_context_lock(context);
	DEFER(_platform_android_context_unlock(context););

	U32 width = context->native_window ? (U32)::ANativeWindow_getWidth(context->native_window) : context->width;
	U32 height = context->native_window ? (U32)::ANativeWindow_getHeight(context->native_window) : context->height;
	context->width = width;
	context->height = height;
	_platform_android_metrics_update_locked(context);

	Platform_Window self {
		.handle = context,
		.width = width,
		.height = height,
		.metrics = context->metrics,
		.presentation = context->presentation,
		.input = {},
		.text_input = {},
		.close_requested = context->close_requested,
		.focused = context->focused,
		.started = context->started,
		.paused = context->paused,
		.low_memory = false,
		.save_state_requested = false,
		.surface_valid = context->native_window != nullptr,
		.surface_changed = context->native_window != nullptr
	};
	self.text_input.enabled = context->text_input_active;
	self.input.text_input_events = array_init<Platform_Text_Input_Event>(memory::heap_allocator());
	return self;
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	ANativeActivity *activity = nullptr;
	if (context)
	{
		_platform_android_context_lock(context);
		context->window_deinit_in_progress = true;
		context->window_deinitialized = true;
		context->close_requested = true;
		context->text_input_active = false;
		if (context->text_input_text.capacity > 0)
			string_deinit(context->text_input_text);
		context->text_input_text = {};
		_platform_android_input_queue_detach_locked(context);
		_platform_android_window_lease_release_locked(context);
		_platform_android_window_set_locked(context, nullptr);
		if (context->activity)
			activity = context->activity;
		_platform_android_context_unlock(context);
	}

	self->handle = nullptr;
	self->width = 0;
	self->height = 0;
	self->metrics = {};
	self->presentation = {};
	_platform_android_text_input_events_reset(self->input.text_input_events);
	array_deinit(self->input.text_input_events);
	self->input = {};
	self->text_input = {};
	self->close_requested = true;
	self->started = false;
	self->low_memory = false;
	self->save_state_requested = false;
	self->surface_valid = false;
	self->surface_changed = true;

	if (activity)
		::ANativeActivity_finish(activity);

	_platform_android_context_try_deinit(context, true);
}

bool
platform_window_poll(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	if (context == nullptr)
		return false;

	bool surface_changed = self->surface_changed;
	self->surface_changed = false;

	_platform_input_reset_transitions(&self->input);

	_platform_android_context_lock(context);
	_platform_android_window_lease_release_locked(context);
	if (context->looper == nullptr)
	{
		context->looper = ::ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
		validate(context->looper != nullptr, "[PLATFORM][ANDROID]: Failed to prepare looper.");
	}
	_platform_android_input_queue_attach_locked(context);
	_platform_android_context_unlock(context);

	I32 events = 0;
	void *data = nullptr;
	I32 ident = 0;
	while ((ident = ::ALooper_pollOnce(0, nullptr, &events, &data)) >= 0)
	{
		if (ident == PLATFORM_ANDROID_LOOPER_INPUT)
		{
			_platform_android_context_lock(context);
			if (context->input_queue)
				_platform_android_process_input_queue(context, self);
			_platform_android_context_unlock(context);
		}
	}

	bool close_requested = false;
	bool started = false;
	bool paused = false;
	bool focused = false;
	bool low_memory = false;
	bool save_state_requested = false;
	bool surface_valid = false;
	ANativeActivity *activity = nullptr;
	_platform_android_context_lock(context);
	if (context->native_window)
	{
		U32 width = (U32)::ANativeWindow_getWidth(context->native_window);
		U32 height = (U32)::ANativeWindow_getHeight(context->native_window);
		if (context->width != width || context->height != height)
		{
			context->width = width;
			context->height = height;
			context->surface_changed = true;
			_platform_android_metrics_update_locked(context);
		}
	}
	self->width = context->width;
	self->height = context->height;
	self->metrics = context->metrics;
	self->presentation = context->presentation;
	self->text_input = Platform_Text_Input_Desc {
		.x = context->text_input_x,
		.y = context->text_input_y,
		.width = context->text_input_width,
		.height = context->text_input_height,
		.flags = context->text_input_flags,
		.action = context->text_input_action,
		.selection_start = context->text_input_selection_start,
		.selection_end = context->text_input_selection_end,
		.composing_start = context->text_input_composing_start,
		.composing_end = context->text_input_composing_end,
		.enabled = context->text_input_active
	};
	for (U64 i = 0; i < context->text_input_events.count; ++i)
	{
		array_push(self->input.text_input_events, context->text_input_events[i]);
		context->text_input_events[i] = {};
	}
	context->text_input_events.count = 0;
	close_requested = context->close_requested;
	started = context->started;
	paused = context->paused;
	focused = context->focused;
	low_memory = context->low_memory;
	save_state_requested = context->save_state_requested;
	context->low_memory = false;
	context->save_state_requested = false;
	surface_valid = context->native_window != nullptr;
	surface_changed = surface_changed || context->surface_changed;
	context->surface_changed = false;
	if (close_requested)
		activity = context->activity;
	_platform_android_context_unlock(context);

	if (activity)
		::ANativeActivity_finish(activity);

	self->close_requested = close_requested;
	self->focused = focused;
	self->started = started;
	self->paused = paused;
	self->low_memory = low_memory;
	self->save_state_requested = save_state_requested;
	self->surface_valid = surface_valid;
	self->surface_changed = surface_changed;
	return !close_requested;
}

Platform_Window_Native_Handles
platform_window_get_native_handles(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (context->native_window_lease == nullptr && context->native_window)
		{
			context->native_window_lease = context->native_window;
			::ANativeWindow_acquire(context->native_window_lease);
		}
		return Platform_Window_Native_Handles {
			.window = context->native_window_lease,
			.context = context->activity
		};
	}

	return Platform_Window_Native_Handles {};
}

void
platform_window_set_title(Platform_Window *, const char *)
{

}

void
platform_window_close(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	if (context == nullptr)
		return;

	ANativeActivity *activity = nullptr;
	_platform_android_context_lock(context);
	context->close_requested = true;
	activity = context->activity;
	_platform_android_context_unlock(context);

	self->close_requested = true;
	self->started = false;
	self->surface_valid = false;
	if (activity)
		::ANativeActivity_finish(activity);
}

inline static void
_platform_android_window_presentation_set(Platform_Context *context, const Platform_Window_Presentation_Desc &desc)
{
	if (context == nullptr)
		return;

	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->window_presentation_set_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return;
	}

	env->CallVoidMethod(activity_object, method, (jint)desc.flags, (jint)desc.orientation_policy);
	_platform_android_jni_clear_exception(env);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
}

void
platform_window_presentation_set(Platform_Window &window, const Platform_Window_Presentation_Desc &desc)
{
	Platform_Context *context = (Platform_Context *)window.handle;
	if (context)
	{
		_platform_android_context_lock(context);
		context->presentation = desc;
		_platform_android_context_unlock(context);
		_platform_android_window_presentation_set(context, desc);
	}
	window.presentation = desc;
	window.surface_changed = true;
}

inline static void
_platform_android_text_input_set(Platform_Context *context, const Platform_Text_Input_Desc &desc)
{
	if (context == nullptr)
		return;
	validate(desc.text.count <= (U64)INT_MAX, "[PLATFORM][ANDROID]: Text input state is too large for JNI.");

	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->text_input_set_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return;
	}

	jbyteArray text_array = env->NewByteArray((jsize)desc.text.count);
	if (text_array == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return;
	}

	if (desc.text.count > 0)
		env->SetByteArrayRegion(text_array, 0, (jsize)desc.text.count, (const jbyte *)desc.text.data);
	if (_platform_android_jni_clear_exception(env))
	{
		env->DeleteLocalRef(text_array);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return;
	}

	env->CallVoidMethod(activity_object, method, (jboolean)desc.enabled, (jint)desc.x, (jint)desc.y, (jint)desc.width, (jint)desc.height, (jint)desc.flags, (jint)desc.action, text_array, (jint)desc.selection_start, (jint)desc.selection_end, (jint)desc.composing_start, (jint)desc.composing_end);
	_platform_android_jni_clear_exception(env);
	env->DeleteLocalRef(text_array);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
}

void
platform_window_text_input_set(Platform_Window &window, const Platform_Text_Input_Desc &desc)
{
	Platform_Context *context = (Platform_Context *)window.handle;
	if (context == nullptr)
		return;

	Platform_Text_Input_Desc text_input_desc = desc;
	window.text_input = text_input_desc;
	window.text_input.text = {};
	if (text_input_desc.enabled)
	{
		_platform_android_context_lock(context);
		context->text_input_active = true;
		context->text_input_x = text_input_desc.x;
		context->text_input_y = text_input_desc.y;
		context->text_input_width = text_input_desc.width;
		context->text_input_height = text_input_desc.height;
		context->text_input_flags = text_input_desc.flags;
		context->text_input_action = text_input_desc.action;
		context->text_input_selection_start = text_input_desc.selection_start;
		context->text_input_selection_end = text_input_desc.selection_end;
		context->text_input_composing_start = text_input_desc.composing_start;
		context->text_input_composing_end = text_input_desc.composing_end;
		if (context->text_input_text.capacity > 0)
			string_deinit(context->text_input_text);
		context->text_input_text = string_copy(text_input_desc.text);
		_platform_android_context_unlock(context);
	}
	else
	{
		_platform_android_context_lock(context);
		context->text_input_active = false;
		context->text_input_x = 0;
		context->text_input_y = 0;
		context->text_input_width = 0;
		context->text_input_height = 0;
		context->text_input_flags = 0;
		context->text_input_action = PLATFORM_TEXT_INPUT_ACTION_NONE;
		context->text_input_selection_start = 0;
		context->text_input_selection_end = 0;
		context->text_input_composing_start = 0;
		context->text_input_composing_end = 0;
		if (context->text_input_text.capacity > 0)
			string_deinit(context->text_input_text);
		context->text_input_text = {};
		_platform_android_text_input_events_reset(context->text_input_events);
		_platform_android_context_unlock(context);
	}

	_platform_android_text_input_set(context, text_input_desc);
}

void
platform_set_current_directory()
{
	String executable_path = platform_path_get_executable_path(memory::temp_allocator());
	String executable_directory = platform_path_get_directory(executable_path, memory::temp_allocator());
	if (executable_directory.count > 0)
		platform_path_set_current_working_directory(executable_directory);
}

bool
platform_file_exists(const char *filepath)
{
	if (_platform_android_is_content_uri(filepath))
	{
		Platform_File_Handle file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ);
		if (file == PLATFORM_FILE_HANDLE_INVALID)
			return false;
		platform_file_close(file);
		return true;
	}

	struct stat file_stat = {};
	return ::stat(filepath, &file_stat) == 0;
}

U64
platform_file_size(const char *filepath)
{
	if (_platform_android_is_content_uri(filepath))
	{
		I64 size = _platform_android_content_size(filepath);
		return size > 0 ? (U64)size : 0;
	}

	struct stat file_stat = {};
	if (::stat(filepath, &file_stat) == 0)
		return file_stat.st_size;
	return 0;
}

String
platform_file_read(const String &file_path, memory::Allocator *allocator)
{
	return platform_path_read_file(file_path, allocator);
}

U64
platform_file_read(const char *filepath, Memory_Block block)
{
	Platform_File_Handle file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return 0;
	DEFER(platform_file_close(file));

	return platform_file_read(file, block.data, block.size);
}

U64
platform_file_write(const char *filepath, Memory_Block block)
{
	Platform_File_Handle file = platform_file_open(filepath, PLATFORM_FILE_MODE_WRITE);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return 0;
	DEFER(platform_file_close(file));

	return platform_file_write(file, block.data, block.size);
}

Platform_File_Handle
platform_file_open(const String &path, Platform_File_Mode mode)
{
	if (_platform_android_is_content_uri(path.data))
	{
		const char *content_mode = "r";
		switch (mode)
		{
			case PLATFORM_FILE_MODE_READ:       content_mode = "r";  break;
			case PLATFORM_FILE_MODE_WRITE:      content_mode = "wt"; break;
			case PLATFORM_FILE_MODE_READ_WRITE: content_mode = "rw"; break;
			case PLATFORM_FILE_MODE_APPEND:     content_mode = "wa"; break;
		}

		I32 content_fd = _platform_android_content_open_fd(path.data, content_mode);
		return content_fd == -1 ? PLATFORM_FILE_HANDLE_INVALID : _platform_file_handle_from_fd(content_fd);
	}

	I32 flags = 0;
	switch (mode)
	{
		case PLATFORM_FILE_MODE_READ:       flags = O_RDONLY;                      break;
		case PLATFORM_FILE_MODE_WRITE:      flags = O_WRONLY | O_CREAT | O_TRUNC;  break;
		case PLATFORM_FILE_MODE_READ_WRITE: flags = O_RDWR   | O_CREAT;            break;
		case PLATFORM_FILE_MODE_APPEND:     flags = O_WRONLY | O_CREAT | O_APPEND; break;
	}

	I32 fd = -1;
	if (mode == PLATFORM_FILE_MODE_READ)
		fd = ::open(path.data, flags);
	else
		fd = ::open(path.data, flags, S_IRWXU);
	return fd == -1 ? PLATFORM_FILE_HANDLE_INVALID : _platform_file_handle_from_fd(fd);
}

void
platform_file_close(Platform_File_Handle handle)
{
	validate(!handle || ::close(_platform_file_handle_to_fd(handle)) == 0, "[PLATFORM][ANDROID]: Failed to close file handle.");
}

U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size)
{
	I64 bytes_read = _platform_android_read(_platform_file_handle_to_fd(handle), data, size);
	return bytes_read < 0 ? 0 : (U64)bytes_read;
}

U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size)
{
	I64 bytes_written = _platform_android_write(_platform_file_handle_to_fd(handle), data, size);
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
	}
	return ::lseek(_platform_file_handle_to_fd(handle), (off_t)offset, whence) != (off_t)-1;
}

U64
platform_file_tell(Platform_File_Handle handle)
{
	off_t offset = ::lseek(_platform_file_handle_to_fd(handle), 0, SEEK_CUR);
	return offset == (off_t)-1 ? 0 : (U64)offset;
}

U64
platform_file_size(Platform_File_Handle handle)
{
	struct stat st = {};
	if (::fstat(_platform_file_handle_to_fd(handle), &st) != 0)
		return 0;
	return (U64)st.st_size;
}

bool
platform_file_copy(const char *from, const char *to)
{
	Platform_File_Handle src_file = platform_file_open(from, PLATFORM_FILE_MODE_READ);
	if (src_file == PLATFORM_FILE_HANDLE_INVALID)
		return false;
	DEFER(platform_file_close(src_file));

	Platform_File_Handle dst_file = platform_file_open(to, PLATFORM_FILE_MODE_WRITE);
	if (dst_file == PLATFORM_FILE_HANDLE_INVALID)
		return false;
	DEFER(platform_file_close(dst_file));

	char buffer[8192];
	while (true)
	{
		U64 bytes_read = platform_file_read(src_file, buffer, sizeof(buffer));
		if (bytes_read == 0)
			break;

		U64 bytes_written = platform_file_write(dst_file, buffer, bytes_read);
		if (bytes_written != bytes_read)
			return false;
	}

	return true;
}

bool
platform_file_delete(const char *filepath)
{
	if (_platform_android_is_content_uri(filepath))
		return _platform_android_content_delete(filepath);

	return ::unlink(filepath) == 0;
}

String
platform_file_dialog_open(const char *filters, memory::Allocator *allocator)
{
	return _platform_android_file_dialog_run(filters, false, allocator);
}

String
platform_file_dialog_save(const char *filters, memory::Allocator *allocator)
{
	return _platform_android_file_dialog_run(filters, true, allocator);
}

String
platform_directory_dialog_open(memory::Allocator *allocator)
{
	return _platform_android_directory_dialog_run(allocator);
}

inline static bool
_platform_android_clipboard_media_type_is_text(const String &media_type)
{
	return media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 || media_type == "text/plain";
}

Array<String>
platform_window_clipboard_query_media_types(Platform_Window &, memory::Allocator *allocator)
{
	Array<String> media_types = array_init<String>(allocator);
	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return media_types;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->clipboard_query_media_types_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return media_types;
	}

	jstring media_types_string = (jstring)env->CallObjectMethod(activity_object, method);
	if (_platform_android_jni_clear_exception(env) || media_types_string == nullptr)
	{
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return media_types;
	}

	const char *media_types_chars = env->GetStringUTFChars(media_types_string, nullptr);
	if (media_types_chars)
	{
		const char *begin = media_types_chars;
		for (const char *at = media_types_chars; ; ++at)
		{
			if (*at == '\n' || *at == '\0')
			{
				if (at != begin)
					array_push(media_types, string_from(begin, at, allocator));
				if (*at == '\0')
					break;
				begin = at + 1;
			}
		}
		env->ReleaseStringUTFChars(media_types_string, media_types_chars);
	}
	env->DeleteLocalRef(media_types_string);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return media_types;
}

Platform_Clipboard_Item
platform_window_clipboard_item_read(Platform_Window &, const String &media_type, memory::Allocator *allocator)
{
	Platform_Clipboard_Item result {
		.media_type = string_init(allocator),
		.data = array_init<U8>(allocator)
	};

	Platform_Context *context = _platform_android_context_get();
	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return result;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->clipboard_read_item_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jstring media_type_string = env->NewStringUTF(media_type.data ? media_type.data : "");
	if (media_type_string == nullptr)
	{
		_platform_android_jni_clear_exception(env);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jbyteArray data_array = (jbyteArray)env->CallObjectMethod(activity_object, method, media_type_string);
	env->DeleteLocalRef(media_type_string);
	if (_platform_android_jni_clear_exception(env) || data_array == nullptr)
	{
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return result;
	}

	jsize data_count = env->GetArrayLength(data_array);
	string_deinit(result.media_type);
	result.media_type = _platform_android_clipboard_media_type_is_text(media_type) ? string_from(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8, allocator) : string_copy(media_type, allocator);
	array_resize(result.data, (U64)data_count);
	if (data_count > 0)
		env->GetByteArrayRegion(data_array, 0, data_count, (jbyte *)result.data.data);
	env->DeleteLocalRef(data_array);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

bool
platform_window_clipboard_item_write(Platform_Window &, const Platform_Clipboard_Item *items, U32 item_count)
{
	Platform_Context *context = _platform_android_context_get();
	if (items == nullptr || item_count == 0 || item_count > (U32)INT_MAX)
		return false;

	bool needs_detach = false;
	JNIEnv *env = _platform_android_jni_env(context, &needs_detach);
	if (env == nullptr)
		return false;

	jmethodID method = nullptr;
	jobject activity_object = _platform_android_activity_ref_get(context, env, &context->clipboard_write_items_method, &method);
	if (activity_object == nullptr)
	{
		_platform_android_jni_detach(context, needs_detach);
		return false;
	}

	jclass string_class = env->FindClass("java/lang/String");
	jclass byte_array_class = env->FindClass("[B");
	jobjectArray media_type_array = nullptr;
	jobjectArray data_item_array = nullptr;
	if (string_class && byte_array_class)
	{
		media_type_array = env->NewObjectArray((jsize)item_count, string_class, nullptr);
		data_item_array = env->NewObjectArray((jsize)item_count, byte_array_class, nullptr);
	}
	if (_platform_android_jni_clear_exception(env) || media_type_array == nullptr || data_item_array == nullptr)
	{
		if (string_class)
			env->DeleteLocalRef(string_class);
		if (byte_array_class)
			env->DeleteLocalRef(byte_array_class);
		if (media_type_array)
			env->DeleteLocalRef(media_type_array);
		if (data_item_array)
			env->DeleteLocalRef(data_item_array);
		env->DeleteLocalRef(activity_object);
		_platform_android_jni_detach(context, needs_detach);
		return false;
	}

	bool arrays_valid = true;
	for (U32 i = 0; i < item_count; ++i)
	{
		if (items[i].media_type.count == 0 || items[i].data.count > (U64)INT_MAX)
		{
			arrays_valid = false;
			break;
		}

		jstring media_type_string = env->NewStringUTF(items[i].media_type.data);
		jbyteArray data_array = env->NewByteArray((jsize)items[i].data.count);
		if (media_type_string == nullptr || data_array == nullptr)
		{
			_platform_android_jni_clear_exception(env);
			if (media_type_string)
				env->DeleteLocalRef(media_type_string);
			if (data_array)
				env->DeleteLocalRef(data_array);
			arrays_valid = false;
			break;
		}

		if (items[i].data.count > 0)
			env->SetByteArrayRegion(data_array, 0, (jsize)items[i].data.count, (const jbyte *)items[i].data.data);
		if (_platform_android_jni_clear_exception(env))
		{
			env->DeleteLocalRef(media_type_string);
			env->DeleteLocalRef(data_array);
			arrays_valid = false;
			break;
		}

		env->SetObjectArrayElement(media_type_array, (jsize)i, media_type_string);
		env->SetObjectArrayElement(data_item_array, (jsize)i, data_array);
		env->DeleteLocalRef(media_type_string);
		env->DeleteLocalRef(data_array);
		if (_platform_android_jni_clear_exception(env))
		{
			arrays_valid = false;
			break;
		}
	}

	bool result = false;
	if (arrays_valid)
		result = env->CallBooleanMethod(activity_object, method, media_type_array, data_item_array);
	if (_platform_android_jni_clear_exception(env))
		result = false;

	env->DeleteLocalRef(string_class);
	env->DeleteLocalRef(byte_array_class);
	env->DeleteLocalRef(media_type_array);
	env->DeleteLocalRef(data_item_array);
	env->DeleteLocalRef(activity_object);
	_platform_android_jni_detach(context, needs_detach);
	return result;
}

U64
platform_query_microseconds()
{
	struct timespec time = {};
	[[maybe_unused]] I32 result = ::clock_gettime(CLOCK_MONOTONIC, &time);
	validate(result == 0, "[PLATFORM][ANDROID]: Failed to query clock.");
	return (U64)time.tv_sec * 1000000 + (U64)time.tv_nsec / 1000;
}

void
platform_sleep_set_period(U32)
{

}

void
platform_sleep(U32 milliseconds)
{
	struct timespec ts = {};
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	::nanosleep(&ts, nullptr);
}

#if DEBUG
struct Platform_Android_Callstack_State
{
	void **callstack;
	U32 frame_count;
	U32 max_frame_count;
};

inline static _Unwind_Reason_Code
_platform_android_callstack_trace(struct _Unwind_Context *context, void *user_data)
{
	Platform_Android_Callstack_State *state = (Platform_Android_Callstack_State *)user_data;
	if (state->frame_count >= state->max_frame_count)
		return _URC_END_OF_STACK;

	uintptr_t pc = _Unwind_GetIP(context);
	if (pc != 0)
		state->callstack[state->frame_count++] = (void *)pc;

	return _URC_NO_REASON;
}
#endif

U32
platform_callstack_capture([[maybe_unused]] void **callstack, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	::memset(callstack, 0, frame_count * sizeof(*callstack));
	Platform_Android_Callstack_State state = {
		.callstack = callstack,
		.frame_count = 0,
		.max_frame_count = frame_count
	};
	_Unwind_Backtrace(_platform_android_callstack_trace, &state);
	return state.frame_count;
#else
	return 0;
#endif
}

void
platform_callstack_resolve([[maybe_unused]] void **callstack, [[maybe_unused]] Platform_Callstack_Frame *frames, [[maybe_unused]] U32 frame_count)
{
#if DEBUG
	for (U32 i = 0; i < frame_count; ++i)
	{
		Platform_Callstack_Frame *frame = frames + i;
		frame->address = callstack[i];
		frame->line = 0;
		frame->symbol_found = false;
		frame->line_found = false;
		frame->symbol[0] = '\0';
		frame->file[0] = '\0';

		Dl_info info = {};
		if (::dladdr(callstack[i], &info) != 0)
		{
			if (info.dli_sname)
			{
				frame->symbol_found = true;
				_platform_copy_string(frame->symbol, PLATFORM_CALLSTACK_SYMBOL_LENGTH, info.dli_sname);
			}
			if (info.dli_fname)
				_platform_copy_string(frame->file, PLATFORM_CALLSTACK_FILE_LENGTH, info.dli_fname);
		}
	}
#endif
}