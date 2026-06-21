#include "core/platform/platform.h"

#include "core/validate.h"
#include "core/defer.h"
#include "core/math/u64.h"
#include "core/memory/memory.h"

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <dlfcn.h>
#include <dirent.h>
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

struct Platform_Task
{
	void (*function)(void *);
	void *user_data;
};

struct Platform_Thread
{
	pthread_t handle;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	bool is_running;
	bool has_task;
	Platform_Task task;
};

struct Platform_Context
{
	ANativeActivity *activity;
	AAssetManager *asset_manager;
	AInputQueue *input_queue;
	ANativeWindow *native_window;
	ALooper *looper;
	memory::Allocator *allocator;
	pthread_mutex_t mutex;
	U32 width;
	U32 height;
	bool input_queue_attached;
	bool started;
	bool resumed;
	bool has_focus;
	bool destroy_requested;
	bool finish_requested;
};

static Platform_Context *_platform_android_context = nullptr;

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

inline static void
_platform_android_window_set_locked(Platform_Context *context, ANativeWindow *window)
{
	if (context->native_window == window)
		return;

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
	}
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
_platform_android_on_resume(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->resumed = true;
	}
}

inline static void
_platform_android_on_pause(ANativeActivity *activity)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->resumed = false;
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
		context->destroy_requested = true;
	}
}

inline static void *
_platform_android_on_save_instance_state(ANativeActivity *, size_t *out_size)
{
	if (out_size)
		*out_size = 0;
	return nullptr;
}

inline static void
_platform_android_on_window_focus_changed(ANativeActivity *activity, int has_focus)
{
	Platform_Context *context = _platform_android_context_from_activity(activity);
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		context->has_focus = has_focus != 0;
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
			context->width  = (U32)::ANativeWindow_getWidth(context->native_window);
			context->height = (U32)::ANativeWindow_getHeight(context->native_window);
		}
	}
}

inline static void
_platform_android_on_low_memory(ANativeActivity *)
{

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
			context->destroy_requested = true;
			context->finish_requested = true;
		}

		return key != PLATFORM_KEY_COUNT;
	}

	if (event_type == AINPUT_EVENT_TYPE_MOTION)
	{
		I32 source = ::AInputEvent_getSource(event);
		if ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE)
			_platform_android_handle_mouse(window, event);
		else
			_platform_android_handle_touch(window, event);
		return 1;
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
}

inline static Platform_Context *
_platform_android_context_init(ANativeActivity *activity, void *saved_state, U64 saved_state_size, memory::Allocator *allocator)
{
	unused(saved_state, saved_state_size);
	validate(_platform_android_context == nullptr, "[PLATFORM][ANDROID]: Android context is already initialized.");
	validate(activity != nullptr, "[PLATFORM][ANDROID]: NativeActivity is required.");
	validate(activity->callbacks != nullptr, "[PLATFORM][ANDROID]: NativeActivity callbacks are required.");

	allocator = allocator ? allocator : memory::heap_allocator();
	Platform_Context *context = (Platform_Context *)memory::allocate_zeroed(allocator, sizeof(Platform_Context), alignof(Platform_Context)).data;
	context->activity = activity;
	context->asset_manager = activity->assetManager;
	context->allocator = allocator;
	validate(::pthread_mutex_init(&context->mutex, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize context mutex.");

	activity->instance = context;
	activity->callbacks->onStart = _platform_android_on_start;
	activity->callbacks->onResume = _platform_android_on_resume;
	activity->callbacks->onSaveInstanceState = _platform_android_on_save_instance_state;
	activity->callbacks->onPause = _platform_android_on_pause;
	activity->callbacks->onStop = _platform_android_on_stop;
	activity->callbacks->onDestroy = _platform_android_on_destroy;
	activity->callbacks->onWindowFocusChanged = _platform_android_on_window_focus_changed;
	activity->callbacks->onNativeWindowCreated = _platform_android_on_native_window_created;
	activity->callbacks->onNativeWindowDestroyed = _platform_android_on_native_window_destroyed;
	activity->callbacks->onInputQueueCreated = _platform_android_on_input_queue_created;
	activity->callbacks->onInputQueueDestroyed = _platform_android_on_input_queue_destroyed;
	activity->callbacks->onConfigurationChanged = _platform_android_on_configuration_changed;
	activity->callbacks->onLowMemory = _platform_android_on_low_memory;

	_platform_android_context = context;
	return context;
}

inline static void
_platform_android_context_deinit(Platform_Context *context)
{
	if (context == nullptr)
		return;

	validate(context == _platform_android_context, "[PLATFORM][ANDROID]: Android context mismatch during deinit.");

	_platform_android_context_lock(context);
	_platform_android_input_queue_detach_locked(context);
	_platform_android_window_set_locked(context, nullptr);
	_platform_android_context_unlock(context);

	if (context->activity)
		context->activity->instance = nullptr;

	validate(::pthread_mutex_destroy(&context->mutex) == 0, "[PLATFORM][ANDROID]: Failed to destroy context mutex.");

	memory::Allocator *allocator = context->allocator ? context->allocator : memory::heap_allocator();
	memory::deallocate(allocator, Memory_Block{context, sizeof(Platform_Context)});
	_platform_android_context = nullptr;
}

extern "C" CORE_API void
platform_android_native_activity_on_create(void *native_activity, void *saved_state, U64 saved_state_size)
{
	_platform_android_context_init((ANativeActivity *)native_activity, saved_state, saved_state_size, memory::heap_allocator());
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
	String result = platform_environment_variable_get("TMPDIR", allocator);
	if (result.count == 0)
		result = string_from("/data/local/tmp/", allocator);
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
	String path_temp = string_copy(path, memory::temp_allocator());
	string_replace(path_temp, "\\", "/");
	Array<String> splits = string_split(path_temp, "/", true, memory::temp_allocator());
	return string_copy(array_back(splits), allocator);
}

String
platform_path_read_file(const String &path, memory::Allocator *allocator)
{
	String content = string_init(allocator);

	I32 file_handle = ::open(path.data, O_RDONLY);
	if (file_handle == -1)
		return content;
	DEFER(validate(::close(file_handle) == 0, "[PLATFORM][ANDROID]: Failed to close file handle."););

	U64 file_size = platform_file_size(path.data);
	if (file_size == 0)
		return content;

	string_resize(content, file_size);
	I64 bytes_read = ::read(file_handle, content.data, content.count);
	if (bytes_read == -1)
		return content;

	validate((I64)content.count == bytes_read, "[PLATFORM][ANDROID]: File read size mismatch.");
	return content;
}

U64
platform_path_write_file(const String &path, Memory_Block block)
{
	I32 file_handle = ::open(path.data, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (file_handle == -1)
		return 0;
	DEFER(validate(::close(file_handle) == 0, "[PLATFORM][ANDROID]: Failed to close file handle."););

	I64 bytes_written = ::write(file_handle, block.data, block.size);
	if (bytes_written == -1)
		return 0;
	return (U64)bytes_written;
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

static void *
_platform_thread_main_routine(void *user_data)
{
	Platform_Thread *self = (Platform_Thread *)user_data;

	while (true)
	{
		validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to lock thread mutex.");
		while (self->is_running && !self->has_task)
			validate(::pthread_cond_wait(&self->condition, &self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to wait for thread condition.");

		if (!self->is_running)
		{
			validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to unlock thread mutex.");
			break;
		}

		Platform_Task task = self->task;
		self->task = {};
		self->has_task = false;
		validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to unlock thread mutex.");

		if (task.function)
			task.function(task.user_data);
	}

	return nullptr;
}

Platform_Thread *
platform_thread_init()
{
	Platform_Thread *self = memory::allocate_zeroed<Platform_Thread>();
	self->is_running = true;
	validate(::pthread_mutex_init(&self->mutex, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize thread mutex.");
	validate(::pthread_cond_init(&self->condition, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to initialize thread condition.");
	validate(::pthread_create(&self->handle, nullptr, _platform_thread_main_routine, self) == 0, "[PLATFORM][ANDROID]: Failed to create thread.");
	return self;
}

void
platform_thread_deinit(Platform_Thread *self)
{
	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to lock thread mutex.");
	self->is_running = false;
	validate(::pthread_cond_signal(&self->condition) == 0, "[PLATFORM][ANDROID]: Failed to signal thread condition.");
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to unlock thread mutex.");

	validate(::pthread_join(self->handle, nullptr) == 0, "[PLATFORM][ANDROID]: Failed to join thread.");
	validate(::pthread_cond_destroy(&self->condition) == 0, "[PLATFORM][ANDROID]: Failed to destroy thread condition.");
	validate(::pthread_mutex_destroy(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to destroy thread mutex.");
	memory::deallocate(self);
}

void
platform_thread_run(Platform_Thread *self, void (*function)(void *), void *user_data)
{
	validate(::pthread_mutex_lock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to lock thread mutex.");
	self->task = Platform_Task {
		.function = function,
		.user_data = user_data
	};
	self->has_task = true;
	validate(::pthread_cond_signal(&self->condition) == 0, "[PLATFORM][ANDROID]: Failed to signal thread condition.");
	validate(::pthread_mutex_unlock(&self->mutex) == 0, "[PLATFORM][ANDROID]: Failed to unlock thread mutex.");
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

	return Platform_Window {
		.handle = context,
		.width = width,
		.height = height,
		.input = {}
	};
}

void
platform_window_deinit(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	self->handle = nullptr;
	self->width = 0;
	self->height = 0;
	self->input = {};
	_platform_android_context_deinit(context);
}

bool
platform_window_poll(Platform_Window *self)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	if (context == nullptr)
		return false;

	_platform_input_reset_transitions(&self->input);

	_platform_android_context_lock(context);
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

	bool destroy_requested = false;
	bool finish_requested = false;
	ANativeActivity *activity = nullptr;
	_platform_android_context_lock(context);
	if (context->native_window)
	{
		context->width = (U32)::ANativeWindow_getWidth(context->native_window);
		context->height = (U32)::ANativeWindow_getHeight(context->native_window);
	}
	self->width = context->width;
	self->height = context->height;
	destroy_requested = context->destroy_requested;
	finish_requested = context->finish_requested;
	context->finish_requested = false;
	activity = context->activity;
	_platform_android_context_unlock(context);

	if (finish_requested && activity)
		::ANativeActivity_finish(activity);

	return !destroy_requested;
}

void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_connection)
{
	Platform_Context *context = (Platform_Context *)self->handle;
	if (context)
	{
		_platform_android_context_lock(context);
		DEFER(_platform_android_context_unlock(context););
		if (native_handle)
			*native_handle = context->native_window;
		if (native_connection)
			*native_connection = context->activity;
		return;
	}

	if (native_handle)
		*native_handle = nullptr;
	if (native_connection)
		*native_connection = nullptr;
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
	context->destroy_requested = true;
	activity = context->activity;
	_platform_android_context_unlock(context);

	if (activity)
		::ANativeActivity_finish(activity);
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
	return platform_path_read_file(file_path, allocator);
}

U64
platform_file_read(const char *filepath, Memory_Block block)
{
	I32 file_handle = ::open(filepath, O_RDONLY);
	if (file_handle == -1)
		return 0;
	DEFER(validate(::close(file_handle) == 0, "[PLATFORM][ANDROID]: Failed to close file handle."););

	I64 bytes_read = ::read(file_handle, block.data, block.size);
	if (bytes_read == -1)
		return 0;
	return (U64)bytes_read;
}

U64
platform_file_write(const char *filepath, Memory_Block block)
{
	I32 file_handle = ::open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (file_handle == -1)
		return 0;
	DEFER(validate(::close(file_handle) == 0, "[PLATFORM][ANDROID]: Failed to close file handle."););

	I64 bytes_written = ::write(file_handle, block.data, block.size);
	if (bytes_written == -1)
		return 0;
	return (U64)bytes_written;
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
	if (handle)
		validate(::close(_platform_file_handle_to_fd(handle)) == 0, "[PLATFORM][ANDROID]: Failed to close file handle.");
}

U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size)
{
	I64 bytes_read = ::read(_platform_file_handle_to_fd(handle), data, size);
	return bytes_read < 0 ? 0 : (U64)bytes_read;
}

U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size)
{
	I64 bytes_written = ::write(_platform_file_handle_to_fd(handle), data, size);
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
	return (U64)::lseek(_platform_file_handle_to_fd(handle), 0, SEEK_CUR);
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
	I32 src_file = ::open(from, O_RDONLY);
	if (src_file < 0)
		return false;

	I32 dst_file = ::open(to, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
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

bool
platform_file_dialog_open(char *path, U32 path_length, const char *)
{
	if (path && path_length > 0)
		::memset(path, 0, path_length);
	return false;
}

bool
platform_file_dialog_save(char *path, U32 path_length, const char *)
{
	if (path && path_length > 0)
		::memset(path, 0, path_length);
	return false;
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

struct Platform_Android_Callstack_State
{
	void **callstack;
	U32 frame_count;
	U32 max_frame_count;
};

static _Unwind_Reason_Code
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