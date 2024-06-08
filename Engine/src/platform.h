#ifndef PLATFORM_H
#define PLATFORM_H

#include "coal_miner.h"

//region Important Structures

typedef struct Point { int x; int y; } Point;
typedef struct Size { unsigned int width; unsigned int height; } Size;

typedef struct Window
{
	const char *title;                  // Window text title const pointer
	unsigned int flags;                 // Configuration flags (bit based), keeps window state
	bool ready;                         // Check if window has been initialized successfully
	bool shouldClose;                   // Check if window set for closing
	bool fullscreen;                    // Check if fullscreen mode is enabled
	bool resizedLastFrame;              // Check if window has been resized last frame
	bool eventWaiting;                  // Wait for events before ending frame
	bool usingFbo;                      // Using FBO (RenderTexture) for rendering instead of default framebuffer

	Point position;                     // Window position (required on fullscreen toggle)
	Point previousPosition;             // Window previous position (required on borderless windowed toggle)
	Size display;                       // Display width and height (monitor, device-screen, LCD, ...)
	Size screen;                        // Screen width and height (used render area)
	Size previousScreen;                // Screen previous width and height (required on borderless windowed toggle)
	Size currentFbo;                    // Current render width and height (depends on active fbo)
	Size render;                        // Framebuffer width and height (render area, including black bars if required)
	Point renderOffset;                 // Offset from render area (must be divided by 2)
	Size screenMin;                     // Screen minimum width and height (for resizable window)
	Size screenMax;                     // Screen maximum width and height (for resizable window)
	M4x4 screenScale;                 // Matrix to scale screen (framebuffer rendering)

	void* platformHandle;

} Window;

typedef struct Input
{
	struct Keyboard
	{
		int exitKey;                    // Default exit key
		char currentKeyState[MAX_KEYBOARD_KEYS]; // Registers current frame key state
		char previousKeyState[MAX_KEYBOARD_KEYS]; // Registers previous frame key state

		// NOTE: Since key press logic involves comparing prev vs cur key state, we need to handle key repeats specially
		char keyRepeatInFrame[MAX_KEYBOARD_KEYS]; // Registers key repeats for current frame.

		int keyPressedQueue[MAX_KEY_PRESSED_QUEUE]; // Input keys queue
		int keyPressedQueueCount;       // Input keys queue count

		int charPressedQueue[MAX_CHAR_PRESSED_QUEUE]; // Input characters queue (unicode)
		int charPressedQueueCount;      // Input characters queue count

	} Keyboard;
	struct Mouse
	{
		V2 offset;                 // Mouse offset
		V2 scale;                  // Mouse scaling
		V2 currentPosition;        // Mouse position on screen
		V2 previousPosition;       // Previous mouse position

		int cursor;                     // Tracks current mouse cursor
		bool cursorHidden;              // Track if cursor is hidden
		bool cursorOnScreen;            // Tracks if cursor is inside client area

		char currentButtonState[MAX_MOUSE_BUTTONS];     // Registers current mouse button state
		char previousButtonState[MAX_MOUSE_BUTTONS];    // Registers previous mouse button state
		V2 currentWheelMove;       // Registers current mouse wheel variation
		V2 previousWheelMove;      // Registers previous mouse wheel variation

	} Mouse;
	struct Touch
	{
		int pointCount;                             // Number of touch points active
		int pointId[MAX_TOUCH_POINTS];              // Point identifiers
		V2 position[MAX_TOUCH_POINTS];         // Touch position on screen
		char currentTouchState[MAX_TOUCH_POINTS];   // Registers current touch state
		char previousTouchState[MAX_TOUCH_POINTS];  // Registers previous touch state

	} Touch;
	struct Gamepad
	{
		int lastButtonPressed;          // Register last gamepad button pressed
		int axisCount[MAX_GAMEPADS];                  // Register number of available gamepad axis
		bool ready[MAX_GAMEPADS];       // Flag to know if gamepad is ready
		char name[MAX_GAMEPADS][64];    // Gamepad name holder
		char currentButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];     // Current gamepad buttons state
		char previousButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];    // Previous gamepad buttons state
		float axisState[MAX_GAMEPADS][MAX_GAMEPAD_AXIS];                // Gamepad axis state

	} Gamepad;
} Input;

//endregion

bool init_platform(Window* window, Input* input);
void terminate_platform();

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

void set_window_icon(Image image);
void set_window_icons(Image* images, int count);

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
