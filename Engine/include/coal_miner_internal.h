#ifndef COAL_MINER_INTERNAL_H
#define COAL_MINER_INTERNAL_H

#include "coal_miner.h"
#include "coal_math.h"

extern void coal_run();
extern void coal_minimize_window();
extern void coal_maximize_window();
extern void coal_toggle_fullscreen();
extern void coal_toggle_borderless_windowed();
extern void coal_restore_window();

extern void coal_set_window_title(const char *title);
extern void coal_set_window_position(V2i position);
extern void coal_set_window_monitor(int monitor);
extern void coal_set_window_min_size(V2i size);
extern void coal_set_window_max_size(V2i size);
extern void coal_set_window_size(V2i size);
extern void coal_set_window_opacity(float opacity);
extern void coal_set_window_focused(void);

extern void coal_set_window_icon(Image image);
extern void coal_set_window_icons(Image* images, int count);

extern V2 coal_get_window_position(void);
extern V2 coal_get_window_scale_dpi(void);
extern void coal_set_clipboard_text(const char *text);
extern const char *coal_get_clipboard_text(void);
extern void coal_show_cursor(void);
extern void coal_hide_cursor(void);
extern void coal_enable_cursor(void);
extern void coal_disable_cursor(void);
extern void coal_set_mouse_position(V2i position);
extern void coal_set_mouse_cursor(int cursor);

#endif //COAL_MINER_INTERNAL_H