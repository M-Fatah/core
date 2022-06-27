#pragma once

#include "core/export.h"
#include "core/defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECOND_TO_MILLISECOND      1000.0f
#define MILLISOCEND_TO_SECOND      0.001f
#define MICROSECOND_TO_MILLISECOND 0.001f

typedef struct Platform_Api
{
	const char *filepath;
	void *handle;
	void *api;
	u64 last_write_time;
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

typedef enum PLATFORM_KEY
{
	PLATFORM_KEY_MOUSE_LEFT,
	PLATFORM_KEY_MOUSE_MIDDLE,
	PLATFORM_KEY_MOUSE_RIGHT,
	PLATFORM_KEY_W,
	PLATFORM_KEY_S,
	PLATFORM_KEY_A,
	PLATFORM_KEY_D,
	PLATFORM_KEY_E,
	PLATFORM_KEY_Q,
	PLATFORM_KEY_ARROW_UP,
	PLATFORM_KEY_ARROW_DOWN,
	PLATFORM_KEY_ARROW_LEFT,
	PLATFORM_KEY_ARROW_RIGHT,
	PLATFORM_KEY_ESC,

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
	void *handle;
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


CORE_API Platform_Allocator
platform_allocator_init(u64 size_in_bytes);

CORE_API void
platform_allocator_deinit(Platform_Allocator *self);

CORE_API Platform_Memory
platform_allocator_alloc(Platform_Allocator *self, u64 size_in_bytes);

CORE_API void
platform_allocator_clear(Platform_Allocator *self);


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
platform_window_get_native_handles(Platform_Window *self, void **native_handle, void **native_display);

CORE_API void
platform_window_set_title(Platform_Window *self, const char *title);

CORE_API void
platform_window_close(Platform_Window *self);

/**
 * @brief Sets current working directory to process directory.
 */
CORE_API void
platform_set_current_directory();


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