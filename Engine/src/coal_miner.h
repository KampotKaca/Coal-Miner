#ifndef COAL_MINER_H
#define COAL_MINER_H

#include <stdbool.h>
#include <stdio.h>

#define COAL_PROFILE_MAJOR 4
#define COAL_PROFILE_MINOR 3
#define COAL_PROFILE 0x00032001

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

//region Structures
typedef struct Point { int x; int y; } Point;
typedef struct Size { unsigned int width; unsigned int height; } Size;

typedef struct Window
{
	const char *title;                  // Window text title const pointer
	unsigned int flags;                 // Configuration flags (bit based), keeps window state
	bool ready;                         // Check if window has been initialized successfully
	bool shouldClose;                   // Check if window set for closing
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
//		Matrix screenScale;                 // Matrix to scale screen (framebuffer rendering)

	char **dropFilePaths;               // Store dropped files paths pointers (provided by GLFW)
	unsigned int dropFileCount;         // Count dropped files strings
	void* platformHandle;

} Window;
//endregion

#endif //COAL_MINER_H
