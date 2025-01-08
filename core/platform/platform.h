#pragma once

#include "core/export.h"
#include "core/defines.h"

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
	i64 last_write_time;
} Platform_Api;

typedef struct Platform_Memory
{
	u8 *ptr;
	u64 size;
} Platform_Memory;

typedef struct Platform_Allocator
{
	u8 *ptr;
	u64 size;
	u64 used;
} Platform_Allocator;

typedef struct Platform_Thread Platform_Thread;

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

	i32 press_count;
	i32 release_count;
} Platform_Key_State;

typedef struct Platform_Input
{
	i32 mouse_x, mouse_y;
	i32 mouse_dx, mouse_dy;
	f32 mouse_wheel;
	Platform_Key_State keys[PLATFORM_KEY_COUNT];
} Platform_Input;

typedef struct Platform_Window
{
	void *handle; // TODO: Rename to context.
	u32 width, height;
	Platform_Input input;
} Platform_Window;

typedef struct Glyph
{
	i32 codepoint;
	i32 yadvance;
	u32 width;
	u32 height;
	f32 uv_min_x;
	f32 uv_min_y;
	f32 uv_max_x;
	f32 uv_max_y;
} Glyph;

typedef struct Platform_Font
{
	// Font data.
	i32 ascent;
	i32 descent;
	i32 line_spacing;
	u32 whitespace_width;
	u32 max_glyph_height;
	i32 *kerning_table;

	// Font glyphs.
	Glyph *glyphs;
	u32 glyph_count;

	// Font atlas.
	u8 *atlas;
	u32 atlas_width;
	u32 atlas_height;
} Platform_Font;


CORE_API Platform_Api
platform_api_init(const char *filepath);

CORE_API void
platform_api_deinit(Platform_Api *self);

CORE_API void *
platform_api_load(Platform_Api *self);

CORE_API Platform_Memory
platform_virtual_memory_reserve(void *address, u64 size);

CORE_API void
platform_virtual_memory_commit(Platform_Memory memory);

CORE_API void
platform_virtual_memory_decommit(Platform_Memory memory);

CORE_API void
platform_virtual_memory_release(Platform_Memory memory);

// TODO: Deprecate.
// {
	CORE_API Platform_Allocator
	platform_allocator_init(u64 size_in_bytes);

	CORE_API void
	platform_allocator_deinit(Platform_Allocator *self);

	CORE_API Platform_Memory
	platform_allocator_alloc(Platform_Allocator *self, u64 size_in_bytes);

	CORE_API void
	platform_allocator_clear(Platform_Allocator *self);
// }

CORE_API Platform_Thread *
platform_thread_init();

CORE_API void
platform_thread_deinit(Platform_Thread *self);

CORE_API void
platform_thread_run(Platform_Thread *self, void (*function)(void *), void *user_data);


CORE_API Platform_Window
platform_window_init(u32 width, u32 height, const char *title);

CORE_API void
platform_window_deinit(Platform_Window *self);

/**
 * @brief Poll events from the supplied platform window.
 * @param self a pointer to the platform window to be po.
 * @return 'false' when window is closed.
 */
CORE_API bool
platform_window_poll(Platform_Window *self);

CORE_API void
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_connection);

CORE_API void
platform_window_set_title(Platform_Window *self, const char *title);

CORE_API void
platform_window_close(Platform_Window *self);

/**
 * @brief Sets current working directory to process directory.
 */
CORE_API void
platform_set_current_directory();

CORE_API bool
platform_file_exists(const char *filepath);

CORE_API u64
platform_file_size(const char *filepath);

CORE_API u64
platform_file_read(const char *filepath, Platform_Memory mem);

CORE_API u64
platform_file_write(const char *filepath, Platform_Memory mem);

CORE_API bool
platform_file_copy(const char *from, const char *to);

CORE_API bool
platform_file_delete(const char *filepath);

/**
 * @brief Opens a file dialog.
 * @param path is the buffer that will store the path of the selected file.
 * @param path_length is the size of the 'path' buffer in bytes.
 * @param filters a pair of null-terminated strings, that specify what to filter in the file dialog; for example, if you want to filter by models you can use "Models (*.obj)\0*.obj\0".
 * @return 'true' on file select success, otherwise 'false'.
 * Note that in case the path was larger than the supplied buffer, the dialog will return 'false'.
 */
CORE_API bool
platform_file_dialog_open(char *path, u32 path_length, const char *filters);

/**
 * @brief Opens a file dialog for saving.
 * @param path is the buffer that will store the path of the specified file name.
 * @param path_length is the size of the 'path' buffer in bytes.
 * @param filters a pair of null-terminated strings, that specify what to filter in the file dialog; for example, if you want to filter by models you can use "Models (*.obj)\0*.obj\0".
 * @return 'true' on file select success, otherwise 'false'.
 * Note that in case the path was larger than the supplied buffer, the dialog will return 'false'.
 */
CORE_API bool
platform_file_dialog_save(char *path, u32 path_length, const char *filters);


CORE_API u64
platform_query_microseconds(void);

CORE_API void
platform_sleep_set_period(u32 period);

CORE_API void
platform_sleep(u32 milliseconds);

CORE_API u32
platform_callstack_capture(void **callstack, u32 frame_count);

CORE_API void
platform_callstack_log(void **callstack, u32 frame_count);

/**
 * @brief Loads the font at the specified path, and extracts information about glyphs from it.
 * @param filepath is the full path of the font resource to be loaded.
 * @param face_name is the name of the font's face to be loaded (Font files may contain more than one face).
 * @param font_height is the desired height to rasterize the font in.
 * @param origin_top_left is a flag used to flip the rasterization 'Y' axis. The default origin is bottom left.
 * @return a font structure that holds information about the loaded font and each glyph in the range '!' to '~'.
 * The font atlas stores only the alpha channel of the font glyphs.
 */
CORE_API Platform_Font
platform_font_init(const char *filepath, const char *face_name, u32 font_height, bool origin_top_left);

/**
 * @brief Frees resources held by a previously loaded 'Font' structure.
 * @param font is the pointer to the font structure to be freed.
 */
CORE_API void
platform_font_deinit(Platform_Font *font);

#ifdef __cplusplus
}
#endif