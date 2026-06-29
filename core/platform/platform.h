#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/memory/memory.h"
#include "core/containers/string.h"

CORE_API String
platform_file_read(const String &file_path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_file_read(const char *file_path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_file_read(string_literal(file_path), allocator);
}

// ============================================================
// Handle-based file I/O (used by File_Stream)
// ============================================================

typedef void *Platform_File_Handle;
#define PLATFORM_FILE_HANDLE_INVALID nullptr

enum Platform_File_Mode
{
	PLATFORM_FILE_MODE_READ,
	PLATFORM_FILE_MODE_WRITE,
	PLATFORM_FILE_MODE_READ_WRITE,
	PLATFORM_FILE_MODE_APPEND,
};

enum Platform_File_Seek_Origin
{
	PLATFORM_FILE_SEEK_ORIGIN_BEGIN,
	PLATFORM_FILE_SEEK_ORIGIN_CURRENT,
	PLATFORM_FILE_SEEK_ORIGIN_END,
};

CORE_API Platform_File_Handle
platform_file_open(const String &path, Platform_File_Mode mode);

inline static Platform_File_Handle
platform_file_open(const char *path, Platform_File_Mode mode)
{
	return platform_file_open(string_literal(path), mode);
}

CORE_API void
platform_file_close(Platform_File_Handle handle);

CORE_API U64
platform_file_read(Platform_File_Handle handle, void *data, U64 size);

CORE_API U64
platform_file_write(Platform_File_Handle handle, const void *data, U64 size);

CORE_API bool
platform_file_seek(Platform_File_Handle handle, I64 offset, Platform_File_Seek_Origin origin);

CORE_API U64
platform_file_tell(Platform_File_Handle handle);

CORE_API U64
platform_file_size(Platform_File_Handle handle);

CORE_API bool
platform_path_is_valid(const String &path);

inline static bool
platform_path_is_valid(const char *path)
{
	return platform_path_is_valid(string_literal(path));
}

CORE_API bool
platform_path_is_file(const String &path);

inline static bool
platform_path_is_file(const char *path)
{
	return platform_path_is_file(string_literal(path));
}

CORE_API bool
platform_path_is_directory(const String &path);

inline static bool
platform_path_is_directory(const char *path)
{
	return platform_path_is_directory(string_literal(path));
}

// TODO: Rename to get_full_path?
CORE_API String
platform_path_get_absolute(const String &path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_get_absolute(const char *path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_get_absolute(string_literal(path), allocator);
}

CORE_API String
platform_path_get_directory(const String &path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_get_directory(const char *path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_get_directory(string_literal(path), allocator);
}

CORE_API String
platform_path_get_current_working_directory(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_path_get_temp_directory(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_path_get_app_data_directory(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_path_get_cache_directory(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_environment_variable_get(const String &name, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_environment_variable_get(const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_environment_variable_get(string_literal(name), allocator);
}

CORE_API void
platform_path_set_current_working_directory(const String &path);

inline static void
platform_path_set_current_working_directory(const char *path)
{
	platform_path_set_current_working_directory(string_literal(path));
}

CORE_API String
platform_path_get_executable_path(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_path_get_current_module_path(memory::Allocator *allocator = memory::heap_allocator());

CORE_API String
platform_path_get_file_name(const String &path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_get_file_name(const char *path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_get_file_name(string_literal(path), allocator);
}

CORE_API String
platform_path_read_file(const String &path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_read_file(const char *path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_read_file(string_literal(path), allocator);
}

CORE_API U64
platform_path_write_file(const String &path, Memory_Block block);

inline static U64
platform_path_write_file(const String &path, const String &content)
{
	return platform_path_write_file(path, Memory_Block{(void *)content.data, content.count});
}

inline static U64
platform_path_write_file(const String &path, const char *content)
{
	return platform_path_write_file(path, string_literal(content));
}

inline static U64
platform_path_write_file(const char *path, const String &content)
{
	return platform_path_write_file(string_literal(path), Memory_Block{(void *)content.data, content.count});
}

inline static U64
platform_path_write_file(const char *path, const char *content)
{
	return platform_path_write_file(string_literal(path), string_literal(content));
}

CORE_API Array<String>
platform_path_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator());

inline static Array<String>
platform_path_list_files(const String &directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files(directory, string_literal(extension_filter), allocator);
}

inline static Array<String>
platform_path_list_files(const char *directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files(string_literal(directory), extension_filter, allocator);
}

inline static Array<String>
platform_path_list_files(const char *directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files(string_literal(directory), string_literal(extension_filter), allocator);
}

CORE_API Array<String>
platform_path_list_files_recursive(const String &directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator());

inline static Array<String>
platform_path_list_files_recursive(const String &directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files_recursive(directory, string_literal(extension_filter), allocator);
}

inline static Array<String>
platform_path_list_files_recursive(const char *directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files_recursive(string_literal(directory), extension_filter, allocator);
}

inline static Array<String>
platform_path_list_files_recursive(const char *directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_list_files_recursive(string_literal(directory), string_literal(extension_filter), allocator);
}

CORE_API String
platform_path_create_file(const String &directory, const String &name, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_create_file(const String &directory, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_file(directory, string_literal(name), allocator);
}

inline static String
platform_path_create_file(const char *directory, const String &name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_file(string_literal(directory), name, allocator);
}

inline static String
platform_path_create_file(const char *directory, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_file(string_literal(directory), string_literal(name), allocator);
}

CORE_API String
platform_path_create_directory(const String &directory, const String &name, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_create_directory(const String &directory, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_directory(directory, string_literal(name), allocator);
}

inline static String
platform_path_create_directory(const char *directory, const String &name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_directory(string_literal(directory), name, allocator);
}

inline static String
platform_path_create_directory(const char *directory, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_create_directory(string_literal(directory), string_literal(name), allocator);
}

CORE_API String
platform_path_rename(const String &path, const String &name, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_rename(const String &path, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_rename(path, string_literal(name), allocator);
}

inline static String
platform_path_rename(const char *path, const String &name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_rename(string_literal(path), name, allocator);
}

inline static String
platform_path_rename(const char *path, const char *name, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_rename(string_literal(path), string_literal(name), allocator);
}

CORE_API String
platform_path_move(const String &path, const String &directory, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_path_move(const String &path, const char *directory, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_move(path, string_literal(directory), allocator);
}

inline static String
platform_path_move(const char *path, const String &directory, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_move(string_literal(path), directory, allocator);
}

inline static String
platform_path_move(const char *path, const char *directory, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_path_move(string_literal(path), string_literal(directory), allocator);
}

CORE_API String
platform_resource_read(const String &path, memory::Allocator *allocator = memory::heap_allocator());

inline static String
platform_resource_read(const char *path, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_resource_read(string_literal(path), allocator);
}

CORE_API Array<String>
platform_resource_list_files(const String &directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator());

inline static Array<String>
platform_resource_list_files(const String &directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_resource_list_files(directory, string_literal(extension_filter), allocator);
}

inline static Array<String>
platform_resource_list_files(const char *directory, const String &extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_resource_list_files(string_literal(directory), extension_filter, allocator);
}

inline static Array<String>
platform_resource_list_files(const char *directory, const char *extension_filter, memory::Allocator *allocator = memory::heap_allocator())
{
	return platform_resource_list_files(string_literal(directory), string_literal(extension_filter), allocator);
}

#ifdef __cplusplus
extern "C" {
#endif

#define SECOND_TO_MILLISECOND      1000.0f
#define MILLISOCEND_TO_SECOND      0.001f
#define MICROSECOND_TO_MILLISECOND 0.001f

typedef enum PLATFORM_API_STATE
{
	PLATFORM_API_STATE_INIT,
	PLATFORM_API_STATE_DEINIT,
	PLATFORM_API_STATE_LOAD
} PLATFORM_API_STATE;

typedef void * (*platform_api_proc)(void *api, PLATFORM_API_STATE state);

typedef struct Platform_Api
{
	char filepath[4096];
	void *handle;
	void *api;
	I64 last_write_time;
} Platform_Api;

typedef enum PLATFORM_KEY
{
	// Mouse.
	PLATFORM_KEY_MOUSE_LEFT,
	PLATFORM_KEY_MOUSE_MIDDLE,
	PLATFORM_KEY_MOUSE_RIGHT,
	PLATFORM_KEY_MOUSE_WHEEL_UP,
	PLATFORM_KEY_MOUSE_WHEEL_DOWN,
	// Keyboard.
	PLATFORM_KEY_A,
	PLATFORM_KEY_B,
	PLATFORM_KEY_C,
	PLATFORM_KEY_D,
	PLATFORM_KEY_E,
	PLATFORM_KEY_F,
	PLATFORM_KEY_G,
	PLATFORM_KEY_H,
	PLATFORM_KEY_I,
	PLATFORM_KEY_J,
	PLATFORM_KEY_K,
	PLATFORM_KEY_L,
	PLATFORM_KEY_M,
	PLATFORM_KEY_N,
	PLATFORM_KEY_O,
	PLATFORM_KEY_P,
	PLATFORM_KEY_Q,
	PLATFORM_KEY_R,
	PLATFORM_KEY_S,
	PLATFORM_KEY_T,
	PLATFORM_KEY_U,
	PLATFORM_KEY_V,
	PLATFORM_KEY_W,
	PLATFORM_KEY_X,
	PLATFORM_KEY_Y,
	PLATFORM_KEY_Z,
	PLATFORM_KEY_NUM_0,
	PLATFORM_KEY_NUM_1,
	PLATFORM_KEY_NUM_2,
	PLATFORM_KEY_NUM_3,
	PLATFORM_KEY_NUM_4,
	PLATFORM_KEY_NUM_5,
	PLATFORM_KEY_NUM_6,
	PLATFORM_KEY_NUM_7,
	PLATFORM_KEY_NUM_8,
	PLATFORM_KEY_NUM_9,
	PLATFORM_KEY_NUMPAD_0,
	PLATFORM_KEY_NUMPAD_1,
	PLATFORM_KEY_NUMPAD_2,
	PLATFORM_KEY_NUMPAD_3,
	PLATFORM_KEY_NUMPAD_4,
	PLATFORM_KEY_NUMPAD_5,
	PLATFORM_KEY_NUMPAD_6,
	PLATFORM_KEY_NUMPAD_7,
	PLATFORM_KEY_NUMPAD_8,
	PLATFORM_KEY_NUMPAD_9,
	PLATFORM_KEY_F1,
	PLATFORM_KEY_F2,
	PLATFORM_KEY_F3,
	PLATFORM_KEY_F4,
	PLATFORM_KEY_F5,
	PLATFORM_KEY_F6,
	PLATFORM_KEY_F7,
	PLATFORM_KEY_F8,
	PLATFORM_KEY_F9,
	PLATFORM_KEY_F10,
	PLATFORM_KEY_F11,
	PLATFORM_KEY_F12,
	PLATFORM_KEY_ARROW_UP,
	PLATFORM_KEY_ARROW_DOWN,
	PLATFORM_KEY_ARROW_LEFT,
	PLATFORM_KEY_ARROW_RIGHT,
	PLATFORM_KEY_SHIFT_LEFT,
	PLATFORM_KEY_SHIFT_RIGHT,
	PLATFORM_KEY_CONTROL_LEFT,
	PLATFORM_KEY_CONTROL_RIGHT,
	PLATFORM_KEY_ALT_LEFT,
	PLATFORM_KEY_ALT_RIGHT,
	PLATFORM_KEY_BACKSPACE,
	PLATFORM_KEY_TAB,
	PLATFORM_KEY_ENTER,
	PLATFORM_KEY_ESCAPE,
	PLATFORM_KEY_DELETE,
	PLATFORM_KEY_INSERT,
	PLATFORM_KEY_HOME,
	PLATFORM_KEY_END,
	PLATFORM_KEY_PAGE_UP,
	PLATFORM_KEY_PAGE_DOWN,
	PLATFORM_KEY_SLASH,
	PLATFORM_KEY_BACKSLASH,
	PLATFORM_KEY_BRACKET_LEFT,
	PLATFORM_KEY_BRACKET_RIGHT,
	PLATFORM_KEY_BACKQUOTE,
	PLATFORM_KEY_PERIOD,
	PLATFORM_KEY_MINUS,
	PLATFORM_KEY_EQUAL,
	PLATFORM_KEY_COMMA,
	PLATFORM_KEY_SEMICOLON,
	PLATFORM_KEY_SPACE,
	PLATFORM_KEY_COUNT
} PLATFORM_KEY;

typedef struct Platform_Key_State
{
	bool pressed;  // Once we press.
	bool released; // Once we release.
	bool down;

	I32 press_count;
	I32 release_count;
} Platform_Key_State;

#define PLATFORM_TOUCH_MAX_COUNT 10

typedef struct Platform_Touch_State
{
	I32 id;
	I32 x, y;
	I32 dx, dy;
	bool pressed;
	bool released;
	bool down;
} Platform_Touch_State;

typedef enum Platform_Text_Input_Event_Type
{
	PLATFORM_TEXT_INPUT_EVENT_COMMIT,
	PLATFORM_TEXT_INPUT_EVENT_COMPOSE,
	PLATFORM_TEXT_INPUT_EVENT_COMPOSE_END,
	PLATFORM_TEXT_INPUT_EVENT_COMPOSE_REGION,
	PLATFORM_TEXT_INPUT_EVENT_DELETE_SURROUNDING,
	PLATFORM_TEXT_INPUT_EVENT_SELECTION,
	PLATFORM_TEXT_INPUT_EVENT_ACTION
} Platform_Text_Input_Event_Type;

typedef enum Platform_Text_Input_Action
{
	PLATFORM_TEXT_INPUT_ACTION_NONE,
	PLATFORM_TEXT_INPUT_ACTION_DONE,
	PLATFORM_TEXT_INPUT_ACTION_GO,
	PLATFORM_TEXT_INPUT_ACTION_SEARCH,
	PLATFORM_TEXT_INPUT_ACTION_SEND,
	PLATFORM_TEXT_INPUT_ACTION_NEXT,
	PLATFORM_TEXT_INPUT_ACTION_PREVIOUS
} Platform_Text_Input_Action;

typedef enum Platform_Text_Input_Flag
{
	PLATFORM_TEXT_INPUT_FLAG_MULTILINE      = 1 << 0,
	PLATFORM_TEXT_INPUT_FLAG_PASSWORD       = 1 << 1,
	PLATFORM_TEXT_INPUT_FLAG_NUMBER         = 1 << 2,
	PLATFORM_TEXT_INPUT_FLAG_DECIMAL        = 1 << 3,
	PLATFORM_TEXT_INPUT_FLAG_SIGNED         = 1 << 4,
	PLATFORM_TEXT_INPUT_FLAG_EMAIL          = 1 << 5,
	PLATFORM_TEXT_INPUT_FLAG_URI            = 1 << 6,
	PLATFORM_TEXT_INPUT_FLAG_NO_SUGGESTIONS = 1 << 7
} Platform_Text_Input_Flag;

typedef struct Platform_Text_Input_Event
{
	Platform_Text_Input_Event_Type type;
	Platform_Text_Input_Action action;
	String text;
	I32 delete_before;
	I32 delete_after;
	U32 selection_start;
	U32 selection_end;
	U32 composing_start;
	U32 composing_end;
} Platform_Text_Input_Event;

typedef struct Platform_Text_Input_Desc
{
	I32 x, y;
	U32 width, height;
	U32 flags;
	Platform_Text_Input_Action action;
	String text;
	U32 selection_start;
	U32 selection_end;
	U32 composing_start;
	U32 composing_end;
	bool enabled;
} Platform_Text_Input_Desc;

typedef struct Platform_Input
{
	I32 mouse_x, mouse_y;
	I32 mouse_dx, mouse_dy;
	F32 mouse_wheel;
	Platform_Key_State keys[PLATFORM_KEY_COUNT];
	Platform_Touch_State touches[PLATFORM_TOUCH_MAX_COUNT];
	Array<Platform_Text_Input_Event> text_input_events;
} Platform_Input;

typedef enum Platform_Window_Orientation
{
	PLATFORM_WINDOW_ORIENTATION_UNKNOWN,
	PLATFORM_WINDOW_ORIENTATION_PORTRAIT,
	PLATFORM_WINDOW_ORIENTATION_LANDSCAPE
} Platform_Window_Orientation;

typedef struct Platform_Window_Rect
{
	I32 x, y;
	U32 width, height;
} Platform_Window_Rect;

typedef struct Platform_Window_Insets
{
	U32 left, top, right, bottom;
} Platform_Window_Insets;

typedef struct Platform_Window_Metrics
{
	Platform_Window_Rect content_rect;
	Platform_Window_Insets safe_area;
	F32 density_scale;
	F32 dpi_x, dpi_y;
	Platform_Window_Orientation orientation;
} Platform_Window_Metrics;

typedef enum Platform_Window_Presentation_Flag
{
	PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN     = 1 << 0,
	PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE      = 1 << 1,
	PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON = 1 << 2,
	PLATFORM_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE   = 1 << 3
} Platform_Window_Presentation_Flag;

typedef enum Platform_Window_Orientation_Policy
{
	PLATFORM_WINDOW_ORIENTATION_POLICY_SYSTEM,
	PLATFORM_WINDOW_ORIENTATION_POLICY_PORTRAIT,
	PLATFORM_WINDOW_ORIENTATION_POLICY_LANDSCAPE,
	PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR,
	PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_PORTRAIT,
	PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE
} Platform_Window_Orientation_Policy;

typedef struct Platform_Window_Presentation_Desc
{
	U32 flags;
	Platform_Window_Orientation_Policy orientation_policy;
} Platform_Window_Presentation_Desc;

typedef struct Platform_Window
{
	void *handle; // TODO: Rename to context.
	U32 width, height;
	Platform_Window_Metrics metrics;
	Platform_Window_Presentation_Desc presentation;
	Platform_Input input;
	Platform_Text_Input_Desc text_input;
	U16 text_input_pending_surrogate;
	bool close_requested; // Window should stop running.
	bool focused;         // Window is the active input target when the platform can report it.
	bool started;         // Window/app is in the started/visible lifecycle; true on desktop.
	bool paused;          // App/window is paused by the platform; false on desktop.
	bool low_memory;      // Transient platform memory pressure signal for this poll.
	bool save_state_requested; // Transient platform request to persist app state for this poll.
	bool surface_valid;   // Native render surface exists.
	bool surface_changed; // Native render surface or size changed since the previous poll.
} Platform_Window;

typedef struct Platform_Window_Native_Handles
{
	void *window;
	void *context;
} Platform_Window_Native_Handles;

#define PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8 "text/plain;charset=utf-8"
#define PLATFORM_CLIPBOARD_MEDIA_TYPE_IMAGE_PNG "image/png"
#define PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY "application/octet-stream"

typedef struct Platform_Clipboard_Item
{
	String media_type;
	Array<U8> data;
} Platform_Clipboard_Item;

CORE_API Platform_Api
platform_api_init(const char *filepath);

CORE_API void
platform_api_deinit(Platform_Api *self);

CORE_API void *
platform_api_load(Platform_Api *self);


CORE_API U64
platform_virtual_memory_get_page_size();

CORE_API U64
platform_virtual_memory_page_align(U64 size);

CORE_API Memory_Block
platform_virtual_memory_reserve(U64 size);

CORE_API bool
platform_virtual_memory_commit(Memory_Block block);

CORE_API bool
platform_virtual_memory_decommit(Memory_Block block);

CORE_API void
platform_virtual_memory_release(Memory_Block block);


CORE_API U32
platform_get_logical_processor_count();


struct Platform_Thread;

using Platform_Thread_Function = void (*)(void *);

struct Platform_Thread_Desc
{
	Platform_Thread_Function function;
	void *data;
};

CORE_API Platform_Thread *
platform_thread_init(Platform_Thread_Desc desc);

CORE_API void
platform_thread_deinit(Platform_Thread *self);

CORE_API void
platform_thread_join(Platform_Thread *self);

CORE_API void
platform_thread_sleep(U32 milliseconds);


CORE_API Platform_Window
platform_window_init(U32 width, U32 height, const char *title);

CORE_API void
platform_window_deinit(Platform_Window *self);

/**
 * @brief Poll events from the supplied platform window.
 * @param self a pointer to the platform window to poll.
 * Updates Platform_Window input, size, close, focus, lifecycle, memory pressure, state-save, and surface fields.
 * @return 'false' when window is closed.
 */
CORE_API bool
platform_window_poll(Platform_Window *self);

/**
 * @brief Gets native handles for the supplied platform window.
 * Returned handles are borrowed and should not be cached across frames.
 */
CORE_API Platform_Window_Native_Handles
platform_window_get_native_handles(Platform_Window *self);

CORE_API void
platform_window_set_title(Platform_Window *self, const char *title);

CORE_API void
platform_window_close(Platform_Window *self);

CORE_API void
platform_window_presentation_set(Platform_Window &window, const Platform_Window_Presentation_Desc &desc);

CORE_API void
platform_window_text_input_set(Platform_Window &window, const Platform_Text_Input_Desc &desc);

/**
 * @brief Sets current working directory to process directory.
 */
CORE_API void
platform_set_current_directory();

CORE_API bool
platform_file_exists(const char *filepath);

CORE_API U64
platform_file_size(const char *filepath);

CORE_API U64
platform_file_read(const char *filepath, Memory_Block block);

CORE_API U64
platform_file_write(const char *filepath, Memory_Block block);

CORE_API bool
platform_file_copy(const char *from, const char *to);

CORE_API bool
platform_file_delete(const char *filepath);

/**
 * @brief Opens a file dialog.
 * @param filters a pair of null-terminated strings, that specify what to filter in the file dialog; for example, if you want to filter by models you can use "Models (*.obj)\0*.obj\0".
 * @return the selected path, or empty string on cancel, failure, or unsupported platforms.
 * On Android, this may be a content URI usable with Core file APIs.
 */
CORE_API String
platform_file_dialog_open(const char *filters, memory::Allocator *allocator = memory::heap_allocator());

/**
 * @brief Opens a file dialog for saving.
 * @param filters a pair of null-terminated strings, that specify what to filter in the file dialog; for example, if you want to filter by models you can use "Models (*.obj)\0*.obj\0".
 * @return the selected path, or empty string on cancel, failure, or unsupported platforms.
 * On Android, this may be a content URI usable with Core file APIs.
 */
CORE_API String
platform_file_dialog_save(const char *filters, memory::Allocator *allocator = memory::heap_allocator());

/**
 * @brief Opens a directory dialog.
 * @return the selected directory path, or empty string on cancel, failure, or unsupported platforms.
 * On Android, this may be a tree content URI usable with Core file APIs.
 */
CORE_API String
platform_directory_dialog_open(memory::Allocator *allocator = memory::heap_allocator());

CORE_API Array<String>
platform_window_clipboard_query_media_types(Platform_Window &window, memory::Allocator *allocator = memory::heap_allocator());

CORE_API Platform_Clipboard_Item
platform_window_clipboard_item_read(Platform_Window &window, const String &media_type, memory::Allocator *allocator = memory::heap_allocator());

CORE_API bool
platform_window_clipboard_item_write(Platform_Window &window, const Platform_Clipboard_Item *items, U32 item_count);

CORE_API U64
platform_query_microseconds(void);

CORE_API void
platform_sleep_set_period(U32 period);

CORE_API void
platform_sleep(U32 milliseconds);

CORE_API U32
platform_callstack_capture(void **callstack, U32 frame_count);

#define PLATFORM_CALLSTACK_SYMBOL_LENGTH 256
#define PLATFORM_CALLSTACK_FILE_LENGTH   512

typedef struct Platform_Callstack_Frame
{
	void *address;
	char symbol[PLATFORM_CALLSTACK_SYMBOL_LENGTH];
	char file[PLATFORM_CALLSTACK_FILE_LENGTH];
	U32 line;
	bool symbol_found;
	bool line_found;
} Platform_Callstack_Frame;

CORE_API void
platform_callstack_resolve(void **callstack, Platform_Callstack_Frame *frames, U32 frame_count);

#ifdef __cplusplus
}

inline static void
platform_clipboard_item_deinit(Platform_Clipboard_Item &item)
{
	string_deinit(item.media_type);
	array_deinit(item.data);
}
#endif