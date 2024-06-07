#ifndef COAL_MINER_H
#define COAL_MINER_H

#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include "config.h"

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

typedef struct M4x4
{
	float m0, m4, m8, m12;  // Matrix first row (4 components)
	float m1, m5, m9, m13;  // Matrix second row (4 components)
	float m2, m6, m10, m14; // Matrix third row (4 components)
	float m3, m7, m11, m15; // Matrix fourth row (4 components)
} M4x4;

typedef struct V2
{
	float x, y;
}V2;

typedef struct V3
{
	float x, y, z;
}V3;

typedef struct V4
{
	float x, y, z, w;
}V4;

typedef V4 Quaternion;

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

// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
typedef enum KeyboardKey
{
	KEY_NULL            = 0,        // Key: NULL, used for no key pressed
	// Alphanumeric keys
	KEY_APOSTROPHE      = 39,       // Key: '
	KEY_COMMA           = 44,       // Key: ,
	KEY_MINUS           = 45,       // Key: -
	KEY_PERIOD          = 46,       // Key: .
	KEY_SLASH           = 47,       // Key: /
	KEY_ZERO            = 48,       // Key: 0
	KEY_ONE             = 49,       // Key: 1
	KEY_TWO             = 50,       // Key: 2
	KEY_THREE           = 51,       // Key: 3
	KEY_FOUR            = 52,       // Key: 4
	KEY_FIVE            = 53,       // Key: 5
	KEY_SIX             = 54,       // Key: 6
	KEY_SEVEN           = 55,       // Key: 7
	KEY_EIGHT           = 56,       // Key: 8
	KEY_NINE            = 57,       // Key: 9
	KEY_SEMICOLON       = 59,       // Key: ;
	KEY_EQUAL           = 61,       // Key: =
	KEY_A               = 65,       // Key: A | a
	KEY_B               = 66,       // Key: B | b
	KEY_C               = 67,       // Key: C | c
	KEY_D               = 68,       // Key: D | d
	KEY_E               = 69,       // Key: E | e
	KEY_F               = 70,       // Key: F | f
	KEY_G               = 71,       // Key: G | g
	KEY_H               = 72,       // Key: H | h
	KEY_I               = 73,       // Key: I | i
	KEY_J               = 74,       // Key: J | j
	KEY_K               = 75,       // Key: K | k
	KEY_L               = 76,       // Key: L | l
	KEY_M               = 77,       // Key: M | m
	KEY_N               = 78,       // Key: N | n
	KEY_O               = 79,       // Key: O | o
	KEY_P               = 80,       // Key: P | p
	KEY_Q               = 81,       // Key: Q | q
	KEY_R               = 82,       // Key: R | r
	KEY_S               = 83,       // Key: S | s
	KEY_T               = 84,       // Key: T | t
	KEY_U               = 85,       // Key: U | u
	KEY_V               = 86,       // Key: V | v
	KEY_W               = 87,       // Key: W | w
	KEY_X               = 88,       // Key: X | x
	KEY_Y               = 89,       // Key: Y | y
	KEY_Z               = 90,       // Key: Z | z
	KEY_LEFT_BRACKET    = 91,       // Key: [
	KEY_BACKSLASH       = 92,       // Key: '\'
	KEY_RIGHT_BRACKET   = 93,       // Key: ]
	KEY_GRAVE           = 96,       // Key: `
	// Function keys
	KEY_SPACE           = 32,       // Key: Space
	KEY_ESCAPE          = 256,      // Key: Esc
	KEY_ENTER           = 257,      // Key: Enter
	KEY_TAB             = 258,      // Key: Tab
	KEY_BACKSPACE       = 259,      // Key: Backspace
	KEY_INSERT          = 260,      // Key: Ins
	KEY_DELETE          = 261,      // Key: Del
	KEY_RIGHT           = 262,      // Key: Cursor right
	KEY_LEFT            = 263,      // Key: Cursor left
	KEY_DOWN            = 264,      // Key: Cursor down
	KEY_UP              = 265,      // Key: Cursor up
	KEY_PAGE_UP         = 266,      // Key: Page up
	KEY_PAGE_DOWN       = 267,      // Key: Page down
	KEY_HOME            = 268,      // Key: Home
	KEY_END             = 269,      // Key: End
	KEY_CAPS_LOCK       = 280,      // Key: Caps lock
	KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
	KEY_NUM_LOCK        = 282,      // Key: Num lock
	KEY_PRINT_SCREEN    = 283,      // Key: Print screen
	KEY_PAUSE           = 284,      // Key: Pause
	KEY_F1              = 290,      // Key: F1
	KEY_F2              = 291,      // Key: F2
	KEY_F3              = 292,      // Key: F3
	KEY_F4              = 293,      // Key: F4
	KEY_F5              = 294,      // Key: F5
	KEY_F6              = 295,      // Key: F6
	KEY_F7              = 296,      // Key: F7
	KEY_F8              = 297,      // Key: F8
	KEY_F9              = 298,      // Key: F9
	KEY_F10             = 299,      // Key: F10
	KEY_F11             = 300,      // Key: F11
	KEY_F12             = 301,      // Key: F12
	KEY_LEFT_SHIFT      = 340,      // Key: Shift left
	KEY_LEFT_CONTROL    = 341,      // Key: Control left
	KEY_LEFT_ALT        = 342,      // Key: Alt left
	KEY_LEFT_SUPER      = 343,      // Key: Super left
	KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
	KEY_RIGHT_CONTROL   = 345,      // Key: Control right
	KEY_RIGHT_ALT       = 346,      // Key: Alt right
	KEY_RIGHT_SUPER     = 347,      // Key: Super right
	KEY_KB_MENU         = 348,      // Key: KB menu
	// Keypad keys
	KEY_KP_0            = 320,      // Key: Keypad 0
	KEY_KP_1            = 321,      // Key: Keypad 1
	KEY_KP_2            = 322,      // Key: Keypad 2
	KEY_KP_3            = 323,      // Key: Keypad 3
	KEY_KP_4            = 324,      // Key: Keypad 4
	KEY_KP_5            = 325,      // Key: Keypad 5
	KEY_KP_6            = 326,      // Key: Keypad 6
	KEY_KP_7            = 327,      // Key: Keypad 7
	KEY_KP_8            = 328,      // Key: Keypad 8
	KEY_KP_9            = 329,      // Key: Keypad 9
	KEY_KP_DECIMAL      = 330,      // Key: Keypad .
	KEY_KP_DIVIDE       = 331,      // Key: Keypad /
	KEY_KP_MULTIPLY     = 332,      // Key: Keypad *
	KEY_KP_SUBTRACT     = 333,      // Key: Keypad -
	KEY_KP_ADD          = 334,      // Key: Keypad +
	KEY_KP_ENTER        = 335,      // Key: Keypad Enter
	KEY_KP_EQUAL        = 336,      // Key: Keypad =
	// Android key buttons
	KEY_BACK            = 4,        // Key: Android back button
	KEY_MENU            = 82,       // Key: Android menu button
	KEY_VOLUME_UP       = 24,       // Key: Android volume up button
	KEY_VOLUME_DOWN     = 25        // Key: Android volume down button
} KeyboardKey;

// Add backwards compatibility support for deprecated names
#define MOUSE_LEFT_BUTTON   MOUSE_BUTTON_LEFT
#define MOUSE_RIGHT_BUTTON  MOUSE_BUTTON_RIGHT
#define MOUSE_MIDDLE_BUTTON MOUSE_BUTTON_MIDDLE

// Mouse buttons
typedef enum MouseButton
{
	MOUSE_BUTTON_LEFT    = 0,       // Mouse button left
	MOUSE_BUTTON_RIGHT   = 1,       // Mouse button right
	MOUSE_BUTTON_MIDDLE  = 2,       // Mouse button middle (pressed wheel)
	MOUSE_BUTTON_SIDE    = 3,       // Mouse button side (advanced mouse device)
	MOUSE_BUTTON_EXTRA   = 4,       // Mouse button extra (advanced mouse device)
	MOUSE_BUTTON_FORWARD = 5,       // Mouse button forward (advanced mouse device)
	MOUSE_BUTTON_BACK    = 6,       // Mouse button back (advanced mouse device)
} MouseButton;

// Mouse cursor
typedef enum MouseCursor
{
	MOUSE_CURSOR_DEFAULT       = 0,     // Default pointer shape
	MOUSE_CURSOR_ARROW         = 1,     // Arrow shape
	MOUSE_CURSOR_IBEAM         = 2,     // Text writing cursor shape
	MOUSE_CURSOR_CROSSHAIR     = 3,     // Cross shape
	MOUSE_CURSOR_POINTING_HAND = 4,     // Pointing hand cursor
	MOUSE_CURSOR_RESIZE_EW     = 5,     // Horizontal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NS     = 6,     // Vertical resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NWSE   = 7,     // Top-left to bottom-right diagonal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NESW   = 8,     // The top-right to bottom-left diagonal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_ALL    = 9,     // The omnidirectional resize/move cursor shape
	MOUSE_CURSOR_NOT_ALLOWED   = 10     // The operation-not-allowed shape
} MouseCursor;

// Gamepad buttons
typedef enum GamepadButton
{
	GAMEPAD_BUTTON_UNKNOWN = 0,         // Unknown button, just for error checking
	GAMEPAD_BUTTON_LEFT_FACE_UP,        // Gamepad left DPAD up button
	GAMEPAD_BUTTON_LEFT_FACE_RIGHT,     // Gamepad left DPAD right button
	GAMEPAD_BUTTON_LEFT_FACE_DOWN,      // Gamepad left DPAD down button
	GAMEPAD_BUTTON_LEFT_FACE_LEFT,      // Gamepad left DPAD left button
	GAMEPAD_BUTTON_RIGHT_FACE_UP,       // Gamepad right button up (i.e. PS3: Triangle, Xbox: Y)
	GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,    // Gamepad right button right (i.e. PS3: Square, Xbox: X)
	GAMEPAD_BUTTON_RIGHT_FACE_DOWN,     // Gamepad right button down (i.e. PS3: Cross, Xbox: A)
	GAMEPAD_BUTTON_RIGHT_FACE_LEFT,     // Gamepad right button left (i.e. PS3: Circle, Xbox: B)
	GAMEPAD_BUTTON_LEFT_TRIGGER_1,      // Gamepad top/back trigger left (first), it could be a trailing button
	GAMEPAD_BUTTON_LEFT_TRIGGER_2,      // Gamepad top/back trigger left (second), it could be a trailing button
	GAMEPAD_BUTTON_RIGHT_TRIGGER_1,     // Gamepad top/back trigger right (one), it could be a trailing button
	GAMEPAD_BUTTON_RIGHT_TRIGGER_2,     // Gamepad top/back trigger right (second), it could be a trailing button
	GAMEPAD_BUTTON_MIDDLE_LEFT,         // Gamepad center buttons, left one (i.e. PS3: Select)
	GAMEPAD_BUTTON_MIDDLE,              // Gamepad center buttons, middle one (i.e. PS3: PS, Xbox: XBOX)
	GAMEPAD_BUTTON_MIDDLE_RIGHT,        // Gamepad center buttons, right one (i.e. PS3: Start)
	GAMEPAD_BUTTON_LEFT_THUMB,          // Gamepad joystick pressed button left
	GAMEPAD_BUTTON_RIGHT_THUMB          // Gamepad joystick pressed button right
} GamepadButton;

// Gamepad axis
typedef enum GamepadAxis
{
	GAMEPAD_AXIS_LEFT_X        = 0,     // Gamepad left stick X axis
	GAMEPAD_AXIS_LEFT_Y        = 1,     // Gamepad left stick Y axis
	GAMEPAD_AXIS_RIGHT_X       = 2,     // Gamepad right stick X axis
	GAMEPAD_AXIS_RIGHT_Y       = 3,     // Gamepad right stick Y axis
	GAMEPAD_AXIS_LEFT_TRIGGER  = 4,     // Gamepad back trigger left, pressure level: [1..-1]
	GAMEPAD_AXIS_RIGHT_TRIGGER = 5      // Gamepad back trigger right, pressure level: [1..-1]
} GamepadAxis;

#endif //COAL_MINER_H
