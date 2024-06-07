#ifndef PLATFORM_H
#define PLATFORM_H

#include "coal_miner.h"

bool init_platform(Window* window, Input* input);
void setup_viewport(int width, int height);

//region Window
void toggle_fullscreen(void);
void toggle_borderless_windowed(void);
void maximize_window(void);
void minimize_window(void);
void restore_window(void);
void set_window_state(unsigned int flags);
void clear_window_state(unsigned int flags);

void set_window_title(const char *title);
void set_window_position(int x, int y);
void set_window_monitor(int monitor);
void set_window_min_size(int width, int height);
void set_window_max_size(int width, int height);
void set_window_size(int width, int height);
void set_window_opacity(float opacity);
void set_window_focused(void);

void *get_window_handle(void);
V2 get_window_position(void);
V2 get_window_scale_dpi(void);
void set_clipboard_text(const char *text);
const char *get_clipboard_text(void);
void show_cursor(void);
void hide_cursor(void);
void enable_cursor(void);
void disable_cursor(void);
void swap_screen_buffer(void);
double get_time(void);
int set_gamepad_mappings(const char *mappings);
void set_mouse_position(int x, int y);
void set_mouse_cursor(int cursor);
void poll_input_events(void);
//endregion

//region Monitor

int get_monitor_count(void);
int get_current_monitor(void);
V2 get_monitor_position(int monitor);
int get_monitor_width(int monitor);
int get_monitor_height(int monitor);
int get_monitor_physical_width(int monitor);
int get_monitor_physical_height(int monitor);
int get_monitor_refresh_rate(int monitor);
const char *get_monitor_name(int monitor);

//endregion

#endif //PLATFORM_H
