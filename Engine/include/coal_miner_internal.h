#ifndef COAL_MINER_INTERNAL_H
#define COAL_MINER_INTERNAL_H

#include "coal_miner.h"

//region Necessary Structures

typedef enum ConfigFlags
{
	FLAG_VSYNC_HINT         = 0x00000040,   // Set to try enabling V-Sync on GPU
	FLAG_FULLSCREEN_MODE    = 0x00000002,   // Set to run program in fullscreen
	FLAG_WINDOW_RESIZABLE   = 0x00000004,   // Set to allow resizable window
	FLAG_WINDOW_UNDECORATED = 0x00000008,   // Set to disable window decoration (frame and buttons)
	FLAG_WINDOW_HIDDEN      = 0x00000080,   // Set to hide window
	FLAG_WINDOW_MINIMIZED   = 0x00000200,   // Set to minimize window (iconify)
	FLAG_WINDOW_MAXIMIZED   = 0x00000400,   // Set to maximize window (expanded to monitor)
	FLAG_WINDOW_UNFOCUSED   = 0x00000800,   // Set to window non focused
	FLAG_WINDOW_TOPMOST     = 0x00001000,   // Set to window always on top
	FLAG_WINDOW_ALWAYS_RUN  = 0x00000100,   // Set to allow windows running while minimized
	FLAG_WINDOW_TRANSPARENT = 0x00000010,   // Set to allow transparent framebuffer
	FLAG_WINDOW_HIGHDPI     = 0x00002000,   // Set to support HighDPI
	FLAG_WINDOW_MOUSE_PASSTHROUGH = 0x00004000, // Set to support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
	FLAG_BORDERLESS_WINDOWED_MODE = 0x00008000, // Set to run program in borderless windowed mode
	FLAG_MSAA_4X_HINT       = 0x00000020,   // Set to try enabling MSAA 4X
	FLAG_INTERLACED_HINT    = 0x00010000    // Set to try enabling interlaced video format (for V3D)
} ConfigFlags;

typedef struct EngineData
{
	unsigned int configFlags;
	unsigned int windowWidth;
	unsigned int windowHeight;
	const char* title;
	const char* iconLocation;

	void (*awakeCallback)();

	void (*updateCallback)();
	void (*renderCallback)();

	void (*frameEndCallback)();
	void (*appCloseCallback)();

}EngineData;

//endregion

extern EngineData DEFAULT_ENGINE_DATA;

extern void coal_run(EngineData data);
extern void cm_minimize_window();
extern void cm_maximize_window();
extern void cm_toggle_fullscreen();
extern void cm_toggle_borderless_windowed();
extern void cm_restore_window();

extern void cm_set_window_title(const char *title);
extern void cm_set_window_position(const ivec2 position);
extern void cm_set_window_monitor(int monitor);
extern void cm_set_window_min_size(const ivec2 size);
extern void cm_set_window_max_size(const ivec2 size);
extern void cm_set_window_size(const ivec2 size);
extern void cm_set_window_opacity(float opacity);
extern void cm_set_window_focused(void);

extern void cm_set_window_icon(Image image);
extern void cm_set_window_icons(Image* images, int count);

extern void cm_get_window_position(float* dest);
extern void cm_get_window_scale_dpi(float* dest);
extern void cm_set_clipboard_text(const char *text);
extern const char *cm_get_clipboard_text(void);
extern void cm_show_cursor(void);
extern void cm_hide_cursor(void);
extern void cm_enable_cursor(void);
extern void cm_disable_cursor(void);
extern void cm_set_mouse_position(const ivec2 position);
extern void cm_set_mouse_cursor(int cursor);

#endif //COAL_MINER_INTERNAL_H