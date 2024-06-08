#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
#define GLFW_INCLUDE_NONE       // Disable the standard OpenGL header inclusion on GLFW3

#if !defined(GLFW_MOUSE_PASSTHROUGH)
#define GLFW_MOUSE_PASSTHROUGH      0x0002000D
#endif

#include "coal_miner_internal.h"
#include "platform.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

Window* WINDOW_ptr;
Input* INPUT_ptr;

void setup_viewport(int width, int height)
{
	WINDOW_ptr->render.width = width;
	WINDOW_ptr->render.height = height;

	glViewport(WINDOW_ptr->renderOffset.x / 2, WINDOW_ptr->renderOffset.y / 2,
	           (int)WINDOW_ptr->render.width, (int)WINDOW_ptr->render.height);
}

static void glfw_error_callback(int error, const char *description)
{
	printf("GLFW_Error: %i Description: %s", error, description);
}

static void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
	// Reset viewport and projection matrix for new size
	setup_viewport(width, height);

	WINDOW_ptr->currentFbo.width = width;
	WINDOW_ptr->currentFbo.height = height;
	WINDOW_ptr->resizedLastFrame = true;

	if (WINDOW_ptr->fullscreen) return;

	// Set current screen size
#if defined(__APPLE__)
	WINDOW->screen.width = width;
    WINDOW->screen.height = height;
#else
	if ((WINDOW_ptr->flags & FLAG_WINDOW_HIGHDPI) > 0)
	{
		V2 windowScaleDPI = get_window_scale_dpi();

		WINDOW_ptr->screen.width = (unsigned int)((float)width / windowScaleDPI.x);
		WINDOW_ptr->screen.height = (unsigned int)((float)height / windowScaleDPI.y);
	}
	else
	{
		WINDOW_ptr->screen.width = width;
		WINDOW_ptr->screen.height = height;
	}
#endif

	// NOTE: Postprocessing texture is not scaled to new size
}

// GLFW3 WindowIconify Callback, runs when window is minimized/restored
static void WindowIconifyCallback(GLFWwindow *window, int iconified)
{
	if (iconified) WINDOW_ptr->flags |= FLAG_WINDOW_MINIMIZED;  // The window was iconified
	else WINDOW_ptr->flags &= ~FLAG_WINDOW_MINIMIZED;           // The window was restored
}

// GLFW3 WindowMaximize Callback, runs when window is maximized/restored
static void WindowMaximizeCallback(GLFWwindow *window, int maximized)
{
	if (maximized) WINDOW_ptr->flags |= FLAG_WINDOW_MAXIMIZED;  // The window was maximized
	else WINDOW_ptr->flags &= ~FLAG_WINDOW_MAXIMIZED;           // The window was restored
}

// GLFW3 WindowFocus Callback, runs when window get/lose focus
static void WindowFocusCallback(GLFWwindow *window, int focused)
{
	if (focused) WINDOW_ptr->flags &= ~FLAG_WINDOW_UNFOCUSED;   // The window was focused
	else WINDOW_ptr->flags |= FLAG_WINDOW_UNFOCUSED;            // The window lost focus
}

// GLFW3 Keyboard Callback, runs on key pressed
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key < 0) return;    // Security check, macOS fn key generates -1

	// WARNING: GLFW could return GLFW_REPEAT, we need to consider it as 1
	// to work properly with our implementation (IsKeyDown/IsKeyUp checks)
	if (action == GLFW_RELEASE) INPUT_ptr->Keyboard.currentKeyState[key] = 0;
	else if(action == GLFW_PRESS) INPUT_ptr->Keyboard.currentKeyState[key] = 1;
	else if(action == GLFW_REPEAT) INPUT_ptr->Keyboard.keyRepeatInFrame[key] = 1;

	// WARNING: Check if CAPS/NUM key modifiers are enabled and force down state for those keys
	if (((key == KEY_CAPS_LOCK) && ((mods & GLFW_MOD_CAPS_LOCK) > 0)) ||
	    ((key == KEY_NUM_LOCK) && ((mods & GLFW_MOD_NUM_LOCK) > 0))) INPUT_ptr->Keyboard.currentKeyState[key] = 1;

	// Check if there is space available in the key queue
	if ((INPUT_ptr->Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) && (action == GLFW_PRESS))
	{
		// Add character to the queue
		INPUT_ptr->Keyboard.keyPressedQueue[INPUT_ptr->Keyboard.keyPressedQueueCount] = key;
		INPUT_ptr->Keyboard.keyPressedQueueCount++;
	}

	// Check the exit key to set close window
	if ((key == INPUT_ptr->Keyboard.exitKey) && (action == GLFW_PRESS)) glfwSetWindowShouldClose(WINDOW_ptr->platformHandle, GLFW_TRUE);
}

// GLFW3 Char Key Callback, runs on key down (gets equivalent unicode char value)
static void CharCallback(GLFWwindow *window, unsigned int key)
{
	//TRACELOG(LOG_DEBUG, "Char Callback: KEY:%i(%c)", key, key);

	// NOTE: Registers any key down considering OS keyboard layout but
	// does not detect action events, those should be managed by user...
	// Ref: https://github.com/glfw/glfw/issues/668#issuecomment-166794907
	// Ref: https://www.glfw.org/docs/latest/input_guide.html#input_char

	// Check if there is space available in the queue
	if (INPUT_ptr->Keyboard.charPressedQueueCount < MAX_CHAR_PRESSED_QUEUE)
	{
		// Add character to the queue
		INPUT_ptr->Keyboard.charPressedQueue[INPUT_ptr->Keyboard.charPressedQueueCount] = (int)key;
		INPUT_ptr->Keyboard.charPressedQueueCount++;
	}
}

// GLFW3 Mouse Button Callback, runs on mouse button pressed
static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	// WARNING: GLFW could only return GLFW_PRESS (1) or GLFW_RELEASE (0) for now,
	// but future releases may add more actions (i.e. GLFW_REPEAT)
	INPUT_ptr->Mouse.currentButtonState[button] = (char)action;
}

// GLFW3 Cursor Position Callback, runs on mouse move
static void MouseCursorPosCallback(GLFWwindow *window, double x, double y)
{
	INPUT_ptr->Mouse.currentPosition.x = (float)x;
	INPUT_ptr->Mouse.currentPosition.y = (float)y;
	INPUT_ptr->Touch.position[0] = INPUT_ptr->Mouse.currentPosition;
}

// GLFW3 Scrolling Callback, runs on mouse wheel
static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	INPUT_ptr->Mouse.currentWheelMove = (V2){(float)xoffset, (float)yoffset };
}

// GLFW3 CursorEnter Callback, when cursor enters the window
static void CursorEnterCallback(GLFWwindow *window, int enter)
{
	if (enter) INPUT_ptr->Mouse.cursorOnScreen = true;
	else INPUT_ptr->Mouse.cursorOnScreen = false;
}

// GLFW3 Joystick Connected/Disconnected Callback
static void JoystickCallback(int jid, int event)
{
	if (event == GLFW_CONNECTED)
	{
		strcpy_s(INPUT_ptr->Gamepad.name[jid], sizeof(INPUT_ptr->Gamepad.name[jid]), glfwGetJoystickName(jid));
//		strcpy(INPUT_ptr->Gamepad.name[jid], glfwGetJoystickName(jid));
	}
	else if (event == GLFW_DISCONNECTED)
	{
		memset(INPUT_ptr->Gamepad.name[jid], 0, 64);
	}
}

// Compute framebuffer size relative to screen size and display size
// NOTE: Global variables WINDOW->render.width/WINDOW->render.height and WINDOW->renderOffset.x/WINDOW->renderOffset.y can be modified
void setup_framebuffer(Window* window)
{
	// Calculate WINDOW->render.width and WINDOW->render.height, we have the display size (input params) and the desired screen size (global var)
	if ((window->screen.width > window->display.width) || (window->screen.height > window->display.height))
	{
		printf("DISPLAY: Downscaling required: Screen size (%ix%i) is bigger than display size (%ix%i)", window->screen.width, window->screen.height, window->display.width, window->display.height);

		// Downscaling to fit display with border-bars
		float widthRatio = (float)window->display.width / (float)window->screen.width;
		float heightRatio = (float)window->display.height / (float)window->screen.height;

		if (widthRatio <= heightRatio)
		{
			window->render.width = window->display.width;
			window->render.height = (int)roundf((float)window->screen.height * widthRatio);
			window->renderOffset.x = 0;
			window->renderOffset.y = (int)(window->display.height - window->render.height);
		}
		else
		{
			window->render.width = (int)roundf((float)window->screen.width * heightRatio);
			window->render.height = window->display.height;
			window->renderOffset.x = (int)(window->display.width - window->render.width);
			window->renderOffset.y = 0;
		}

		// Screen scaling required
		float scaleRatio = (float)window->render.width/(float)window->screen.width;
		window->screenScale = matrix_scale(scaleRatio, scaleRatio, 1.0f);

		// NOTE: We render to full display resolution!
		// We just need to calculate above parameters for downscale matrix and offsets
		window->render.width = window->display.width;
		window->render.height = window->display.height;

		printf("DISPLAY: Downscale matrix generated, content will be rendered at (%ix%i)", window->render.width, window->render.height);
	}
	else if ((window->screen.width < window->display.width) || (window->screen.height < window->display.height))
	{
		// Required screen size is smaller than display size
		printf("DISPLAY: Upscaling required: Screen size (%ix%i) smaller than display size (%ix%i)", window->screen.width, window->screen.height, window->display.width, window->display.height);

		if ((window->screen.width == 0) || (window->screen.height == 0))
		{
			window->screen.width = window->display.width;
			window->screen.height = window->display.height;
		}

		// Upscaling to fit display with border-bars
		float displayRatio = (float)window->display.width / (float)window->display.height;
		float screenRatio = (float)window->screen.width / (float)window->screen.height;

		if (displayRatio <= screenRatio)
		{
			window->render.width = window->screen.width;
			window->render.height = (int)roundf((float)window->screen.width/displayRatio);
			window->renderOffset.x = 0;
			window->renderOffset.y = (int)(window->render.height - window->screen.height);
		}
		else
		{
			window->render.width = (int)roundf((float)window->screen.height*displayRatio);
			window->render.height = window->screen.height;
			window->renderOffset.x = (int)(window->render.width - window->screen.width);
			window->renderOffset.y = 0;
		}
	}
	else
	{
		window->render.width = window->screen.width;
		window->render.height = window->screen.height;
		window->renderOffset.x = 0;
		window->renderOffset.y = 0;
	}
}

bool init_platform(Window* window, Input* input)
{
	WINDOW_ptr = window;
	INPUT_ptr = input;

	glfwSetErrorCallback(glfw_error_callback);

	int result = glfwInit();

#if defined(__APPLE__)
	glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

	if (result == GLFW_FALSE) { printf("Failed to initialize GLFW"); return false; }
	glfwDefaultWindowHints();

	//region flags
	if ((WINDOW_ptr->flags & FLAG_FULLSCREEN_MODE) > 0)
	{
		printf("Window Is Fullscreen");
		WINDOW_ptr->fullscreen = true;
	}

	if ((WINDOW_ptr->flags & FLAG_WINDOW_HIDDEN) > 0) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Visible window
	else glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);     // Window initially hidden

	if ((WINDOW_ptr->flags & FLAG_WINDOW_UNDECORATED) > 0) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Border and buttons on Window
	else glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);   // Decorated window

	if ((WINDOW_ptr->flags & FLAG_WINDOW_RESIZABLE) > 0) glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Resizable window
	else glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // Avoid window being resizable

	// Disable FLAG_WINDOW_MINIMIZED, not supported on initialization
	if ((WINDOW_ptr->flags & FLAG_WINDOW_MINIMIZED) > 0) WINDOW_ptr->flags &= ~FLAG_WINDOW_MINIMIZED;

	// Disable FLAG_WINDOW_MAXIMIZED, not supported on initialization
	if ((WINDOW_ptr->flags & FLAG_WINDOW_MAXIMIZED) > 0) WINDOW_ptr->flags &= ~FLAG_WINDOW_MAXIMIZED;

	if ((WINDOW_ptr->flags & FLAG_WINDOW_UNFOCUSED) > 0) glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
	else glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

	if ((WINDOW_ptr->flags & FLAG_WINDOW_TOPMOST) > 0) glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	else glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

	// NOTE: Some GLFW flags are not supported on HTML5
	if ((WINDOW_ptr->flags & FLAG_WINDOW_TRANSPARENT) > 0) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);     // Transparent framebuffer
	else glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);  // Opaque framebuffer

	if ((WINDOW_ptr->flags & FLAG_WINDOW_HIGHDPI) > 0)
	{
		// Resize window content area based on the monitor content scale.
		// NOTE: This hint only has an effect on platforms where screen coordinates and pixels always map 1:1 such as Windows and X11.
		// On platforms like macOS the resolution of the framebuffer is changed independently of the window size.
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);   // Scale content area based on the monitor content scale where window is placed on
#if defined(__APPLE__)
		glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif
	}
	else glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

	// Mouse passthrough
	if ((WINDOW_ptr->flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) > 0) glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
	else glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);

	if (WINDOW_ptr->flags & FLAG_MSAA_4X_HINT)
		glfwWindowHint(GLFW_SAMPLES, 4);   // Tries to enable multisampling x4 (MSAA), default is 0
	//endregion

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, COAL_PROFILE_MAJOR);          // Choose OpenGL major version (just hint)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, COAL_PROFILE_MINOR);          // Choose OpenGL minor version (just hint)
	glfwWindowHint(GLFW_OPENGL_PROFILE, COAL_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);

	glfwSetJoystickCallback(NULL);

	// Find monitor resolution
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	if (!monitor)
	{
		printf("GLFW: Failed to get primary monitor");
		return false;
	}

	// Set screen width/height to the display width/height if they are 0
	if (window->screen.width == 0) window->screen.width = window->display.width;
	if (window->screen.height == 0) window->screen.height = window->display.height;

	if (window->fullscreen)
	{
		// remember center for switching from fullscreen to window
		if ((window->screen.height == window->display.height) && (window->screen.width == window->display.width))
		{
			// If screen width/height equal to the display, we can't calculate the window pos for toggling full-screened/windowed.
			// Toggling full-screened/windowed with pos(0, 0) can cause problems in some platforms, such as X11.
			window->position.x = (int) window->display.width / 4;
			window->position.y = (int) window->display.height / 4;
		} else
		{
			window->position.x = (int) (window->display.width / 2 - window->screen.width / 2);
			window->position.y = (int) (window->display.height / 2 - window->screen.height / 2);
		}

		if (window->position.x < 0) window->position.x = 0;
		if (window->position.y < 0) window->position.y = 0;

		// Obtain recommended WINDOW->display.width/WINDOW->display.height from a valid videomode for the monitor
		int count = 0;
		const GLFWvidmode *modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);

		// Get closest video mode to desired WINDOW->screen.width/WINDOW->screen.height
		for (int i = 0; i < count; i++)
		{
			if ((unsigned int) modes[i].width >= window->screen.width)
			{
				if ((unsigned int) modes[i].height >= window->screen.height)
				{
					window->display.width = modes[i].width;
					window->display.height = modes[i].height;
					break;
				}
			}
		}

		// NOTE: ISSUE: Closest videomode could not match monitor aspect-ratio, for example,
		// for a desired screen size of 800x450 (16:9), closest supported videomode is 800x600 (4:3),
		// framebuffer is rendered correctly but once displayed on a 16:9 monitor, it gets stretched
		// by the sides to fit all monitor space...

		// Try to setup the most appropriate fullscreen framebuffer for the requested screenWidth/screenHeight
		// It considers device display resolution mode and setups a framebuffer with black bars if required (render size/offset)
		// Modified global variables: WINDOW->screen.width/WINDOW->screen.height - WINDOW->render.width/WINDOW->render.height - WINDOW->renderOffset.x/WINDOW->renderOffset.y - WINDOW->screenScale
		// TODO: It is a quite cumbersome solution to display size vs requested size, it should be reviewed or removed...
		// HighDPI monitors are properly considered in a following similar function: SetupViewport()
		setup_framebuffer(window);

		window->platformHandle = glfwCreateWindow((int)window->display.width, (int)window->display.height, (window->title != 0) ? window->title : " ", glfwGetPrimaryMonitor(), NULL);
	}
	else
	{
		// If we are windowed fullscreen, ensures that window does not minimize when focus is lost
		if ((window->screen.height == window->display.height) && (window->screen.width == window->display.width))
		{
			glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
		}

		// No-fullscreen window creation
		window->platformHandle = glfwCreateWindow((int)window->screen.width, (int)window->screen.height, (window->title != 0) ? window->title : " ", NULL, NULL);

		if (window->platformHandle)
		{
			window->render.width = window->screen.width;
			window->render.height = window->screen.height;
		}
	}

	if (!window->platformHandle)
	{
		glfwTerminate();
		printf("GLFW: Failed to initialize Window");
		return false;
	}

	glfwMakeContextCurrent(window->platformHandle);
	result = glfwGetError(NULL);

	// Check context activation
	if ((result != GLFW_NO_WINDOW_CONTEXT) && (result != GLFW_PLATFORM_ERROR))
	{
		window->ready = true;

		glfwSwapInterval(0);        // No V-Sync by default

		// Try to enable GPU V-Sync, so frames are limited to screen refresh rate (60Hz -> 60 FPS)
		// NOTE: V-Sync can be enabled by graphic driver configuration, it doesn't need
		// to be activated on web platforms since VSync is enforced there.
		if (window->flags & FLAG_VSYNC_HINT)
		{
			// WARNING: It seems to hit a critical render path in Intel HD Graphics
			glfwSwapInterval(1);
		}

		int fbWidth = (int)window->screen.width;
		int fbHeight = (int)window->screen.height;

		if ((window->flags & FLAG_WINDOW_HIGHDPI) > 0)
		{
			// NOTE: On APPLE platforms system should manage window/input scaling and also framebuffer scaling.
			// Framebuffer scaling should be activated with: glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#if defined(PLATFORM_MAC)
			glfwGetFramebufferSize(window->platformHandle, &fbWidth, &fbHeight);

			// Screen scaling matrix is required in case desired screen area is different from display area
			window->screenScale = matrix_scale((float)fbWidth / (float)window->screen.width, (float)fbHeight / (float)window->screen.height, 1.0f);

			// Mouse input scaling for the new screen size
			INPUT_ptr->Mouse.scale = (V2){(float)window->screen.width / (float)fbWidth, (float)window->screen.height / (float)fbHeight };
#endif
		}

		window->render.width = fbWidth;
		window->render.height = fbHeight;
		window->currentFbo.width = fbWidth;
		window->currentFbo.height = fbHeight;

	}
	else return false;

	if ((window->flags & FLAG_WINDOW_MINIMIZED) > 0) minimize_window();

	// If graphic device is no properly initialized, we end program
	if (!window->ready) return false;
	else set_window_position((int)(get_monitor_width(get_current_monitor()) / 2 - WINDOW_ptr->screen.width / 2),
							 (int)(get_monitor_height(get_current_monitor()) / 2 - WINDOW_ptr->screen.height / 2));

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Unable to load glad!!!");
		return false;
	}

	// Load OpenGL extensions
	// NOTE: GL procedures address loader is required to load extensions
	//----------------------------------------------------------------------------

	// Initialize input events callbacks
	//----------------------------------------------------------------------------
	// Set window callback events
	glfwSetWindowSizeCallback(window->platformHandle, WindowSizeCallback);      // NOTE: Resizing not allowed by default!
	glfwSetWindowMaximizeCallback(window->platformHandle, WindowMaximizeCallback);
	glfwSetWindowIconifyCallback(window->platformHandle, WindowIconifyCallback);
	glfwSetWindowFocusCallback(window->platformHandle, WindowFocusCallback);

	// Set input callback events
	glfwSetKeyCallback(window->platformHandle, KeyCallback);
	glfwSetCharCallback(window->platformHandle, CharCallback);
	glfwSetMouseButtonCallback(window->platformHandle, MouseButtonCallback);
	glfwSetCursorPosCallback(window->platformHandle, MouseCursorPosCallback);   // Track mouse position changes
	glfwSetScrollCallback(window->platformHandle, MouseScrollCallback);
	glfwSetCursorEnterCallback(window->platformHandle, CursorEnterCallback);
	glfwSetJoystickCallback(JoystickCallback);

	glfwSetInputMode(window->platformHandle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);    // Enable lock keys modifiers (CAPS, NUM)

	// Retrieve gamepad names
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (glfwJoystickPresent(i))
		{
			strcpy_s(input->Gamepad.name[i], sizeof(input->Gamepad.name[i]), glfwGetJoystickName(i));
//			strcpy(input->Gamepad.name[i], glfwGetJoystickName(i));
		}
	}

	return true;
}

void terminate_platform()
{
	glfwDestroyWindow(WINDOW_ptr->platformHandle);
	glfwTerminate();
}

//region Window

void toggle_fullscreen(void)
{
	if (!WINDOW_ptr->fullscreen)
	{
		// Store previous window position (in case we exit fullscreen)
		glfwGetWindowPos(WINDOW_ptr->platformHandle, &WINDOW_ptr->position.x, &WINDOW_ptr->position.y);

		int monitorCount = 0;
		int monitorIndex = get_current_monitor();
		GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

		// Use current monitor, so we correctly get the display the window is on
		GLFWmonitor *monitor = (monitorIndex < monitorCount)? monitors[monitorIndex] : NULL;

		if (monitor == NULL)
		{
			printf("GLFW: Failed to get monitor");

			WINDOW_ptr->fullscreen = false;
			WINDOW_ptr->flags &= ~FLAG_FULLSCREEN_MODE;

			glfwSetWindowMonitor(WINDOW_ptr->platformHandle, NULL, 0, 0, (int)WINDOW_ptr->screen.width, (int)WINDOW_ptr->screen.height, GLFW_DONT_CARE);
		}
		else
		{
			WINDOW_ptr->fullscreen = true;
			WINDOW_ptr->flags |= FLAG_FULLSCREEN_MODE;

			glfwSetWindowMonitor(WINDOW_ptr->platformHandle, monitor, 0, 0, (int)WINDOW_ptr->screen.width, (int)WINDOW_ptr->screen.height, GLFW_DONT_CARE);
		}

	}
	else
	{
		WINDOW_ptr->fullscreen = false;
		WINDOW_ptr->flags &= ~FLAG_FULLSCREEN_MODE;

		glfwSetWindowMonitor(WINDOW_ptr->platformHandle, NULL, WINDOW_ptr->position.x, WINDOW_ptr->position.y, (int)WINDOW_ptr->screen.width, (int)WINDOW_ptr->screen.height, GLFW_DONT_CARE);
	}

	// Try to enable GPU V-Sync, so frames are limited to screen refresh rate (60Hz -> 60 FPS)
	// NOTE: V-Sync can be enabled by graphic driver configuration
	if (WINDOW_ptr->flags & FLAG_VSYNC_HINT) glfwSwapInterval(1);
}

// Toggle borderless windowed mode
void toggle_borderless_windowed(void)
{
	// Leave fullscreen before attempting to set borderless windowed mode and get screen position from it
	bool wasOnFullscreen = false;
	if (WINDOW_ptr->fullscreen)
	{
		WINDOW_ptr->previousPosition = WINDOW_ptr->position;
		toggle_fullscreen();
		wasOnFullscreen = true;
	}

	const int monitor = get_current_monitor();
	int monitorCount;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

		if (mode)
		{
			if ((WINDOW_ptr->flags & FLAG_BORDERLESS_WINDOWED_MODE) == 0)
			{
				// Store screen position and size
				// NOTE: If it was on fullscreen, screen position was already stored, so skip setting it here
				if (!wasOnFullscreen) glfwGetWindowPos(WINDOW_ptr->platformHandle, &WINDOW_ptr->previousPosition.x, &WINDOW_ptr->previousPosition.y);
				WINDOW_ptr->previousScreen = WINDOW_ptr->screen;

				// Set undecorated and topmost modes and flags
				glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_DECORATED, GLFW_FALSE);
				WINDOW_ptr->flags |= FLAG_WINDOW_UNDECORATED;
				glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FLOATING, GLFW_TRUE);
				WINDOW_ptr->flags |= FLAG_WINDOW_TOPMOST;

				// Get monitor position and size
				int monitorPosX = 0;
				int monitorPosY = 0;
				glfwGetMonitorPos(monitors[monitor], &monitorPosX, &monitorPosY);
				const int monitorWidth = mode->width;
				const int monitorHeight = mode->height;

				// Set screen position and size
				glfwSetWindowPos(WINDOW_ptr->platformHandle, monitorPosX, monitorPosY);
				glfwSetWindowSize(WINDOW_ptr->platformHandle, monitorWidth, monitorHeight);

				// Refocus window
				glfwFocusWindow(WINDOW_ptr->platformHandle);

				WINDOW_ptr->flags |= FLAG_BORDERLESS_WINDOWED_MODE;
			}
			else
			{
				// Remove topmost and undecorated modes and flags
				glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FLOATING, GLFW_FALSE);
				WINDOW_ptr->flags &= ~FLAG_WINDOW_TOPMOST;
				glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_DECORATED, GLFW_TRUE);
				WINDOW_ptr->flags &= ~FLAG_WINDOW_UNDECORATED;

				// Return previous screen size and position
				// NOTE: The order matters here, it must set size first, then set position, otherwise the screen will be positioned incorrectly
				glfwSetWindowSize(WINDOW_ptr->platformHandle, (int)WINDOW_ptr->previousScreen.width, (int)WINDOW_ptr->previousScreen.height);
				glfwSetWindowPos(WINDOW_ptr->platformHandle, WINDOW_ptr->previousPosition.x, WINDOW_ptr->previousPosition.y);

				// Refocus window
				glfwFocusWindow(WINDOW_ptr->platformHandle);

				WINDOW_ptr->flags &= ~FLAG_BORDERLESS_WINDOWED_MODE;
			}
		}
		else printf("GLFW: Failed to find video mode for selected monitor");
	}
	else printf("GLFW: Failed to find selected monitor");
}

// Set window state: maximized, if resizable
void maximize_window(void)
{
	if (glfwGetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_RESIZABLE) == GLFW_TRUE)
	{
		glfwMaximizeWindow(WINDOW_ptr->platformHandle);
		WINDOW_ptr->flags |= FLAG_WINDOW_MAXIMIZED;
	}
}

// Set window state: minimized
void minimize_window(void)
{
	// NOTE: Following function launches callback that sets appropriate flag!
	glfwIconifyWindow(WINDOW_ptr->platformHandle);
}

// Set window state: not minimized/maximized
void restore_window(void)
{
	if (glfwGetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_RESIZABLE) == GLFW_TRUE)
	{
		// Restores the specified window if it was previously iconified (minimized) or maximized
		glfwRestoreWindow(WINDOW_ptr->platformHandle);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_MINIMIZED;
		WINDOW_ptr->flags &= ~FLAG_WINDOW_MAXIMIZED;
	}
}

// Set window configuration state using flags
void set_window_state(unsigned int flags)
{
	// Check previous state and requested state to apply required changes
	// NOTE: In most cases the functions already change the flags internally

	// State change: FLAG_VSYNC_HINT
	if (((WINDOW_ptr->flags & FLAG_VSYNC_HINT) != (flags & FLAG_VSYNC_HINT)) && ((flags & FLAG_VSYNC_HINT) > 0))
	{
		glfwSwapInterval(1);
		WINDOW_ptr->flags |= FLAG_VSYNC_HINT;
	}

	// State change: FLAG_BORDERLESS_WINDOWED_MODE
	// NOTE: This must be handled before FLAG_FULLSCREEN_MODE because ToggleBorderlessWindowed() needs to get some fullscreen values if fullscreen is running
	if (((WINDOW_ptr->flags & FLAG_BORDERLESS_WINDOWED_MODE) != (flags & FLAG_BORDERLESS_WINDOWED_MODE)) && ((flags & FLAG_BORDERLESS_WINDOWED_MODE) > 0))
	{
		toggle_borderless_windowed();     // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_FULLSCREEN_MODE
	if ((WINDOW_ptr->flags & FLAG_FULLSCREEN_MODE) != (flags & FLAG_FULLSCREEN_MODE))
	{
		toggle_fullscreen();     // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_RESIZABLE
	if (((WINDOW_ptr->flags & FLAG_WINDOW_RESIZABLE) != (flags & FLAG_WINDOW_RESIZABLE)) && ((flags & FLAG_WINDOW_RESIZABLE) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_RESIZABLE, GLFW_TRUE);
		WINDOW_ptr->flags |= FLAG_WINDOW_RESIZABLE;
	}

	// State change: FLAG_WINDOW_UNDECORATED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_UNDECORATED) != (flags & FLAG_WINDOW_UNDECORATED)) && (flags & FLAG_WINDOW_UNDECORATED))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_DECORATED, GLFW_FALSE);
		WINDOW_ptr->flags |= FLAG_WINDOW_UNDECORATED;
	}

	// State change: FLAG_WINDOW_HIDDEN
	if (((WINDOW_ptr->flags & FLAG_WINDOW_HIDDEN) != (flags & FLAG_WINDOW_HIDDEN)) && ((flags & FLAG_WINDOW_HIDDEN) > 0))
	{
		glfwHideWindow(WINDOW_ptr->platformHandle);
		WINDOW_ptr->flags |= FLAG_WINDOW_HIDDEN;
	}

	// State change: FLAG_WINDOW_MINIMIZED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MINIMIZED) != (flags & FLAG_WINDOW_MINIMIZED)) && ((flags & FLAG_WINDOW_MINIMIZED) > 0))
	{
		//GLFW_ICONIFIED
		minimize_window();       // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_MAXIMIZED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MAXIMIZED) != (flags & FLAG_WINDOW_MAXIMIZED)) && ((flags & FLAG_WINDOW_MAXIMIZED) > 0))
	{
		//GLFW_MAXIMIZED
		maximize_window();       // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_UNFOCUSED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_UNFOCUSED) != (flags & FLAG_WINDOW_UNFOCUSED)) && ((flags & FLAG_WINDOW_UNFOCUSED) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
		WINDOW_ptr->flags |= FLAG_WINDOW_UNFOCUSED;
	}

	// State change: FLAG_WINDOW_TOPMOST
	if (((WINDOW_ptr->flags & FLAG_WINDOW_TOPMOST) != (flags & FLAG_WINDOW_TOPMOST)) && ((flags & FLAG_WINDOW_TOPMOST) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FLOATING, GLFW_TRUE);
		WINDOW_ptr->flags |= FLAG_WINDOW_TOPMOST;
	}

	// State change: FLAG_WINDOW_ALWAYS_RUN
	if (((WINDOW_ptr->flags & FLAG_WINDOW_ALWAYS_RUN) != (flags & FLAG_WINDOW_ALWAYS_RUN)) && ((flags & FLAG_WINDOW_ALWAYS_RUN) > 0))
	{
		WINDOW_ptr->flags |= FLAG_WINDOW_ALWAYS_RUN;
	}

	// The following states can not be changed after window creation

	// State change: FLAG_WINDOW_TRANSPARENT
	if (((WINDOW_ptr->flags & FLAG_WINDOW_TRANSPARENT) != (flags & FLAG_WINDOW_TRANSPARENT)) && ((flags & FLAG_WINDOW_TRANSPARENT) > 0))
	{
		printf("WINDOW: Framebuffer transparency can only be configured before window initialization");
	}

	// State change: FLAG_WINDOW_HIGHDPI
	if (((WINDOW_ptr->flags & FLAG_WINDOW_HIGHDPI) != (flags & FLAG_WINDOW_HIGHDPI)) && ((flags & FLAG_WINDOW_HIGHDPI) > 0))
	{
		printf("WINDOW: High DPI can only be configured before window initialization");
	}

	// State change: FLAG_WINDOW_MOUSE_PASSTHROUGH
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) != (flags & FLAG_WINDOW_MOUSE_PASSTHROUGH)) && ((flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
		WINDOW_ptr->flags |= FLAG_WINDOW_MOUSE_PASSTHROUGH;
	}

	// State change: FLAG_MSAA_4X_HINT
	if (((WINDOW_ptr->flags & FLAG_MSAA_4X_HINT) != (flags & FLAG_MSAA_4X_HINT)) && ((flags & FLAG_MSAA_4X_HINT) > 0))
	{
		printf("WINDOW: MSAA can only be configured before window initialization");
	}

	// State change: FLAG_INTERLACED_HINT
	if (((WINDOW_ptr->flags & FLAG_INTERLACED_HINT) != (flags & FLAG_INTERLACED_HINT)) && ((flags & FLAG_INTERLACED_HINT) > 0))
	{
		printf("RPI: Interlaced mode can only be configured before window initialization");
	}
}

// Clear window configuration state flags
void clear_window_state(unsigned int flags)
{
	// Check previous state and requested state to apply required changes
	// NOTE: In most cases the functions already change the flags internally

	// State change: FLAG_VSYNC_HINT
	if (((WINDOW_ptr->flags & FLAG_VSYNC_HINT) > 0) && ((flags & FLAG_VSYNC_HINT) > 0))
	{
		glfwSwapInterval(0);
		WINDOW_ptr->flags &= ~FLAG_VSYNC_HINT;
	}

	// State change: FLAG_BORDERLESS_WINDOWED_MODE
	// NOTE: This must be handled before FLAG_FULLSCREEN_MODE because ToggleBorderlessWindowed() needs to get some fullscreen values if fullscreen is running
	if (((WINDOW_ptr->flags & FLAG_BORDERLESS_WINDOWED_MODE) > 0) && ((flags & FLAG_BORDERLESS_WINDOWED_MODE) > 0))
	{
		toggle_borderless_windowed();     // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_FULLSCREEN_MODE
	if (((WINDOW_ptr->flags & FLAG_FULLSCREEN_MODE) > 0) && ((flags & FLAG_FULLSCREEN_MODE) > 0))
	{
		toggle_fullscreen();     // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_RESIZABLE
	if (((WINDOW_ptr->flags & FLAG_WINDOW_RESIZABLE) > 0) && ((flags & FLAG_WINDOW_RESIZABLE) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_RESIZABLE, GLFW_FALSE);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_RESIZABLE;
	}

	// State change: FLAG_WINDOW_HIDDEN
	if (((WINDOW_ptr->flags & FLAG_WINDOW_HIDDEN) > 0) && ((flags & FLAG_WINDOW_HIDDEN) > 0))
	{
		glfwShowWindow(WINDOW_ptr->platformHandle);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_HIDDEN;
	}

	// State change: FLAG_WINDOW_MINIMIZED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MINIMIZED) > 0) && ((flags & FLAG_WINDOW_MINIMIZED) > 0))
	{
		restore_window();       // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_MAXIMIZED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MAXIMIZED) > 0) && ((flags & FLAG_WINDOW_MAXIMIZED) > 0))
	{
		restore_window();       // NOTE: Window state flag updated inside function
	}

	// State change: FLAG_WINDOW_UNDECORATED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_UNDECORATED) > 0) && ((flags & FLAG_WINDOW_UNDECORATED) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_DECORATED, GLFW_TRUE);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_UNDECORATED;
	}

	// State change: FLAG_WINDOW_UNFOCUSED
	if (((WINDOW_ptr->flags & FLAG_WINDOW_UNFOCUSED) > 0) && ((flags & FLAG_WINDOW_UNFOCUSED) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_UNFOCUSED;
	}

	// State change: FLAG_WINDOW_TOPMOST
	if (((WINDOW_ptr->flags & FLAG_WINDOW_TOPMOST) > 0) && ((flags & FLAG_WINDOW_TOPMOST) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_FLOATING, GLFW_FALSE);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_TOPMOST;
	}

	// State change: FLAG_WINDOW_ALWAYS_RUN
	if (((WINDOW_ptr->flags & FLAG_WINDOW_ALWAYS_RUN) > 0) && ((flags & FLAG_WINDOW_ALWAYS_RUN) > 0))
	{
		WINDOW_ptr->flags &= ~FLAG_WINDOW_ALWAYS_RUN;
	}

	// The following states can not be changed after window creation

	// State change: FLAG_WINDOW_TRANSPARENT
	if (((WINDOW_ptr->flags & FLAG_WINDOW_TRANSPARENT) > 0) && ((flags & FLAG_WINDOW_TRANSPARENT) > 0))
	{
		printf("WINDOW: Framebuffer transparency can only be configured before window initialization");
	}

	// State change: FLAG_WINDOW_HIGHDPI
	if (((WINDOW_ptr->flags & FLAG_WINDOW_HIGHDPI) > 0) && ((flags & FLAG_WINDOW_HIGHDPI) > 0))
	{
		printf("WINDOW: High DPI can only be configured before window initialization");
	}

	// State change: FLAG_WINDOW_MOUSE_PASSTHROUGH
	if (((WINDOW_ptr->flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) > 0) && ((flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) > 0))
	{
		glfwSetWindowAttrib(WINDOW_ptr->platformHandle, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
		WINDOW_ptr->flags &= ~FLAG_WINDOW_MOUSE_PASSTHROUGH;
	}

	// State change: FLAG_MSAA_4X_HINT
	if (((WINDOW_ptr->flags & FLAG_MSAA_4X_HINT) > 0) && ((flags & FLAG_MSAA_4X_HINT) > 0))
	{
		printf("WINDOW: MSAA can only be configured before window initialization");
	}

	// State change: FLAG_INTERLACED_HINT
	if (((WINDOW_ptr->flags & FLAG_INTERLACED_HINT) > 0) && ((flags & FLAG_INTERLACED_HINT) > 0))
	{
		printf("RPI: Interlaced mode can only be configured before window initialization");
	}
}

// Set title for window
void set_window_title(const char *title)
{
	WINDOW_ptr->title = title;
	glfwSetWindowTitle(WINDOW_ptr->platformHandle, title);
}

// Set window position on screen (windowed mode)
void set_window_position(int x, int y)
{
	glfwSetWindowPos(WINDOW_ptr->platformHandle, x, y);
}

// Set monitor for the current window
void set_window_monitor(int monitor)
{
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		if (WINDOW_ptr->fullscreen)
		{
			printf("GLFW: Selected fullscreen monitor: [%i] %s", monitor, glfwGetMonitorName(monitors[monitor]));

			const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);
			glfwSetWindowMonitor(WINDOW_ptr->platformHandle, monitors[monitor], 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			printf("GLFW: Selected monitor: [%i] %s", monitor, glfwGetMonitorName(monitors[monitor]));

			const int screenWidth = (int)WINDOW_ptr->screen.width;
			const int screenHeight = (int)WINDOW_ptr->screen.height;
			int monitorWorkareaX = 0;
			int monitorWorkareaY = 0;
			int monitorWorkareaWidth = 0;
			int monitorWorkareaHeight = 0;
			glfwGetMonitorWorkarea(monitors[monitor], &monitorWorkareaX, &monitorWorkareaY, &monitorWorkareaWidth, &monitorWorkareaHeight);

			// If the screen size is larger than the monitor workarea, anchor it on the top left corner, otherwise, center it
			if ((screenWidth >= monitorWorkareaWidth) || (screenHeight >= monitorWorkareaHeight)) glfwSetWindowPos(WINDOW_ptr->platformHandle, monitorWorkareaX, monitorWorkareaY);
			else
			{
				const int x = monitorWorkareaX + (monitorWorkareaWidth/2) - (screenWidth/2);
				const int y = monitorWorkareaY + (monitorWorkareaHeight/2) - (screenHeight/2);
				glfwSetWindowPos(WINDOW_ptr->platformHandle, x, y);
			}
		}
	}
	else printf("GLFW: Failed to find selected monitor");
}

// Set window minimum dimensions (FLAG_WINDOW_RESIZABLE)
void set_window_min_size(int width, int height)
{
	WINDOW_ptr->screenMin.width = width;
	WINDOW_ptr->screenMin.height = height;

	int minWidth  = (WINDOW_ptr->screenMin.width == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMin.width;
	int minHeight = (WINDOW_ptr->screenMin.height == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMin.height;
	int maxWidth  = (WINDOW_ptr->screenMax.width == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMax.width;
	int maxHeight = (WINDOW_ptr->screenMax.height == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMax.height;

	glfwSetWindowSizeLimits(WINDOW_ptr->platformHandle, minWidth, minHeight, maxWidth, maxHeight);
}

// Set window maximum dimensions (FLAG_WINDOW_RESIZABLE)
void set_window_max_size(int width, int height)
{
	WINDOW_ptr->screenMax.width = width;
	WINDOW_ptr->screenMax.height = height;

	int minWidth  = (WINDOW_ptr->screenMin.width == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMin.width;
	int minHeight = (WINDOW_ptr->screenMin.height == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMin.height;
	int maxWidth  = (WINDOW_ptr->screenMax.width == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMax.width;
	int maxHeight = (WINDOW_ptr->screenMax.height == 0) ? GLFW_DONT_CARE : (int)WINDOW_ptr->screenMax.height;

	glfwSetWindowSizeLimits(WINDOW_ptr->platformHandle, minWidth, minHeight, maxWidth, maxHeight);
}

// Set window dimensions
void set_window_size(int width, int height)
{
	glfwSetWindowSize(WINDOW_ptr->platformHandle, width, height);
}

// Set window opacity, value opacity is between 0.0 and 1.0
void set_window_opacity(float opacity)
{
	if (opacity >= 1.0f) opacity = 1.0f;
	else if (opacity <= 0.0f) opacity = 0.0f;
	glfwSetWindowOpacity(WINDOW_ptr->platformHandle, opacity);
}

// Set window focused
void set_window_focused(void)
{
	glfwFocusWindow(WINDOW_ptr->platformHandle);
}

// Set icon for window
// NOTE 1: Image must be in RGBA format, 8bit per channel
// NOTE 2: Image is scaled by the OS for all required sizes
void set_window_icon(Image image)
{
	if (image.data == NULL)
	{
		// Revert to the default window icon, pass in an empty image array
		glfwSetWindowIcon(WINDOW_ptr->platformHandle, 0, NULL);
	}
	else
	{
		if (image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
		{
			GLFWimage icon[1] = { 0 };

			icon[0].width = image.width;
			icon[0].height = image.height;
			icon[0].pixels = (unsigned char *)image.data;

			// NOTE 1: We only support one image icon
			// NOTE 2: The specified image data is copied before this function returns
			glfwSetWindowIcon(WINDOW_ptr->platformHandle, 1, icon);
		}
		else printf("GLFW: Window icon image must be in R8G8B8A8 pixel format");
	}
}

// Set icon for window, multiple images
// NOTE 1: Images must be in RGBA format, 8bit per channel
// NOTE 2: The multiple images are used depending on provided sizes
// Standard Windows icon sizes: 256, 128, 96, 64, 48, 32, 24, 16
void set_window_icons(Image* images, int count)
{
	if ((images == NULL) || (count <= 0))
	{
		// Revert to the default window icon, pass in an empty image array
		glfwSetWindowIcon(WINDOW_ptr->platformHandle, 0, NULL);
	}
	else
	{
		int valid = 0;
		GLFWimage *icons = CM_CALLOC(count, sizeof(GLFWimage));

		for (int i = 0; i < count; i++)
		{
			if (images[i].format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
			{
				icons[valid].width = images[i].width;
				icons[valid].height = images[i].height;
				icons[valid].pixels = (unsigned char *)images[i].data;

				valid++;
			}
			else printf("GLFW: Window icon image must be in R8G8B8A8 pixel format");
		}
		// NOTE: Images data is copied internally before this function returns
		glfwSetWindowIcon(WINDOW_ptr->platformHandle, valid, icons);

		CM_FREE(icons);
	}
}

// Get native window handle
void *get_window_handle(void)
{
	return WINDOW_ptr->platformHandle;
}

// Get window position XY on monitor
V2 get_window_position(void)
{
	int x = 0;
	int y = 0;

	glfwGetWindowPos(WINDOW_ptr->platformHandle, &x, &y);

	return (V2){ (float)x, (float)y };
}

// Get window scale DPI factor for current monitor
V2 get_window_scale_dpi(void)
{
	float xdpi = 1.0f;
	float ydpi = 1.0f;
	V2 scale = { 1.0f, 1.0f };
	V2 windowPos = get_window_position();

	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	// Check window monitor
	for (int i = 0; i < monitorCount; i++)
	{
		glfwGetMonitorContentScale(monitors[i], &xdpi, &ydpi);

		int xpos, ypos, width, height;
		glfwGetMonitorWorkarea(monitors[i], &xpos, &ypos, &width, &height);

		if ((windowPos.x >= (float)xpos) && (windowPos.x < (float)(xpos + width)) &&
		    (windowPos.y >= (float)ypos) && (windowPos.y < (float)(ypos + height)))
		{
			scale.x = xdpi;
			scale.y = ydpi;
			break;
		}
	}

	return scale;
}

// Set clipboard text content
void set_clipboard_text(const char *text)
{
	glfwSetClipboardString(WINDOW_ptr->platformHandle, text);
}

// Get clipboard text content
// NOTE: returned string is allocated and freed by GLFW
const char *get_clipboard_text(void)
{
	return glfwGetClipboardString(WINDOW_ptr->platformHandle);
}

// Show mouse cursor
void show_cursor(void)
{
	glfwSetInputMode(WINDOW_ptr->platformHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	INPUT_ptr->Mouse.cursorHidden = false;
}

// Hides mouse cursor
void hide_cursor(void)
{
	glfwSetInputMode(WINDOW_ptr->platformHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	INPUT_ptr->Mouse.cursorHidden = true;
}

// Enables cursor (unlock cursor)
void enable_cursor(void)
{
	glfwSetInputMode(WINDOW_ptr->platformHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Set cursor position in the middle
	set_mouse_position((int)WINDOW_ptr->screen.width / 2, (int)WINDOW_ptr->screen.height / 2);

	INPUT_ptr->Mouse.cursorHidden = false;
}

// Disables cursor (lock cursor)
void disable_cursor(void)
{
	glfwSetInputMode(WINDOW_ptr->platformHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set cursor position in the middle
	set_mouse_position((int)WINDOW_ptr->screen.width / 2, (int)WINDOW_ptr->screen.height / 2);

	INPUT_ptr->Mouse.cursorHidden = true;
}

// Swap back buffer with front buffer (screen drawing)
void swap_screen_buffer(void)
{
	glfwSwapBuffers(WINDOW_ptr->platformHandle);
}

// Get elapsed time measure in seconds since InitTimer()
double get_time(void)
{
	double time = glfwGetTime();   // Elapsed time since glfwInit()
	return time;
}

// Set internal gamepad mappings
int set_gamepad_mappings(const char *mappings)
{
	return glfwUpdateGamepadMappings(mappings);
}

// Set mouse position XY
void set_mouse_position(int x, int y)
{
	INPUT_ptr->Mouse.currentPosition = (V2){(float)x, (float)y };
	INPUT_ptr->Mouse.previousPosition = INPUT_ptr->Mouse.currentPosition;

	// NOTE: emscripten not implemented
	glfwSetCursorPos(WINDOW_ptr->platformHandle, INPUT_ptr->Mouse.currentPosition.x, INPUT_ptr->Mouse.currentPosition.y);
}

// Set mouse cursor
void set_mouse_cursor(int cursor)
{
	INPUT_ptr->Mouse.cursor = cursor;
	if (cursor == MOUSE_CURSOR_DEFAULT) glfwSetCursor(WINDOW_ptr->platformHandle, NULL);
	else
	{
		// NOTE: We are relating internal GLFW enum values to our MouseCursor enum values
		glfwSetCursor(WINDOW_ptr->platformHandle, glfwCreateStandardCursor(0x00036000 + cursor));
	}
}

// Register all input events
void poll_input_events(void)
{
	// Reset keys/chars pressed registered
	INPUT_ptr->Keyboard.keyPressedQueueCount = 0;
	INPUT_ptr->Keyboard.charPressedQueueCount = 0;

	// Reset last gamepad button/axis registered state
	INPUT_ptr->Gamepad.lastButtonPressed = 0;       // GAMEPAD_BUTTON_UNKNOWN
	//INPUT->Gamepad.axisCount = 0;

	// Keyboard/Mouse input polling (automatically managed by GLFW3 through callback)

	// Register previous keys states
	for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
	{
		INPUT_ptr->Keyboard.previousKeyState[i] = INPUT_ptr->Keyboard.currentKeyState[i];
		INPUT_ptr->Keyboard.keyRepeatInFrame[i] = 0;
	}

	// Register previous mouse states
	for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) INPUT_ptr->Mouse.previousButtonState[i] = INPUT_ptr->Mouse.currentButtonState[i];

	// Register previous mouse wheel state
	INPUT_ptr->Mouse.previousWheelMove = INPUT_ptr->Mouse.currentWheelMove;
	INPUT_ptr->Mouse.currentWheelMove = (V2){0.0f, 0.0f };

	// Register previous mouse position
	INPUT_ptr->Mouse.previousPosition = INPUT_ptr->Mouse.currentPosition;

	// Register previous touch states
	for (int i = 0; i < MAX_TOUCH_POINTS; i++) INPUT_ptr->Touch.previousTouchState[i] = INPUT_ptr->Touch.currentTouchState[i];

	// Reset touch positions
	//for (int i = 0; i < MAX_TOUCH_POINTS; i++) INPUT->Touch.position[i] = (V2){ 0, 0 };

	// Map touch position to mouse position for convenience
	// WARNING: If the target desktop device supports touch screen, this behavious should be reviewed!
	// TODO: GLFW does not support multi-touch input just yet
	// https://www.codeproject.com/Articles/668404/Programming-for-Multi-Touch
	// https://docs.microsoft.com/en-us/windows/win32/wintouch/getting-started-with-multi-touch-messages
	INPUT_ptr->Touch.position[0] = INPUT_ptr->Mouse.currentPosition;

	// Check if gamepads are ready
	// NOTE: We do it here in case of disconnection
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (glfwJoystickPresent(i)) INPUT_ptr->Gamepad.ready[i] = true;
		else INPUT_ptr->Gamepad.ready[i] = false;
	}

	// Register gamepads buttons events
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (INPUT_ptr->Gamepad.ready[i])     // Check if gamepad is available
		{
			// Register previous gamepad states
			for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++) INPUT_ptr->Gamepad.previousButtonState[i][k] = INPUT_ptr->Gamepad.currentButtonState[i][k];

			// Get current gamepad state
			// NOTE: There is no callback available, so we get it manually
			GLFWgamepadstate state = { 0 };
			glfwGetGamepadState(i, &state); // This remapps all gamepads so they have their buttons mapped like an xbox controller

			const unsigned char *buttons = state.buttons;

			for (int k = 0; (buttons != NULL) && (k < GLFW_GAMEPAD_BUTTON_DPAD_LEFT + 1) && (k < MAX_GAMEPAD_BUTTONS); k++)
			{
				int button = -1;        // GamepadButton enum values assigned

				switch (k)
				{
					case GLFW_GAMEPAD_BUTTON_Y: button = GAMEPAD_BUTTON_RIGHT_FACE_UP; break;
					case GLFW_GAMEPAD_BUTTON_B: button = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT; break;
					case GLFW_GAMEPAD_BUTTON_A: button = GAMEPAD_BUTTON_RIGHT_FACE_DOWN; break;
					case GLFW_GAMEPAD_BUTTON_X: button = GAMEPAD_BUTTON_RIGHT_FACE_LEFT; break;

					case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: button = GAMEPAD_BUTTON_LEFT_TRIGGER_1; break;
					case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: button = GAMEPAD_BUTTON_RIGHT_TRIGGER_1; break;

					case GLFW_GAMEPAD_BUTTON_BACK: button = GAMEPAD_BUTTON_MIDDLE_LEFT; break;
					case GLFW_GAMEPAD_BUTTON_GUIDE: button = GAMEPAD_BUTTON_MIDDLE; break;
					case GLFW_GAMEPAD_BUTTON_START: button = GAMEPAD_BUTTON_MIDDLE_RIGHT; break;

					case GLFW_GAMEPAD_BUTTON_DPAD_UP: button = GAMEPAD_BUTTON_LEFT_FACE_UP; break;
					case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: button = GAMEPAD_BUTTON_LEFT_FACE_RIGHT; break;
					case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: button = GAMEPAD_BUTTON_LEFT_FACE_DOWN; break;
					case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: button = GAMEPAD_BUTTON_LEFT_FACE_LEFT; break;

					case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: button = GAMEPAD_BUTTON_LEFT_THUMB; break;
					case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: button = GAMEPAD_BUTTON_RIGHT_THUMB; break;
					default: break;
				}

				if (button != -1)   // Check for valid button
				{
					if (buttons[k] == GLFW_PRESS)
					{
						INPUT_ptr->Gamepad.currentButtonState[i][button] = 1;
						INPUT_ptr->Gamepad.lastButtonPressed = button;
					}
					else INPUT_ptr->Gamepad.currentButtonState[i][button] = 0;
				}
			}

			// Get current axis state
			const float *axes = state.axes;

			for (int k = 0; (axes != NULL) && (k < GLFW_GAMEPAD_AXIS_LAST + 1) && (k < MAX_GAMEPAD_AXIS); k++)
			{
				INPUT_ptr->Gamepad.axisState[i][k] = axes[k];
			}

			// Register buttons for 2nd triggers (because GLFW doesn't count these as buttons but rather axis)
			INPUT_ptr->Gamepad.currentButtonState[i][GAMEPAD_BUTTON_LEFT_TRIGGER_2] = (char)(INPUT_ptr->Gamepad.axisState[i][GAMEPAD_AXIS_LEFT_TRIGGER] > 0.1f);
			INPUT_ptr->Gamepad.currentButtonState[i][GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = (char)(INPUT_ptr->Gamepad.axisState[i][GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.1f);

			INPUT_ptr->Gamepad.axisCount[i] = GLFW_GAMEPAD_AXIS_LAST + 1;
		}
	}

	WINDOW_ptr->resizedLastFrame = false;

	if (WINDOW_ptr->eventWaiting) glfwWaitEvents();     // Wait for in input events before continue (drawing is paused)
	else glfwPollEvents();      // Poll input events: keyboard/mouse/window events (callbacks) -> Update keys state

	// While window minimized, stop loop execution
	while ((WINDOW_ptr->flags & FLAG_WINDOW_MINIMIZED) > 0 && (WINDOW_ptr->flags & FLAG_WINDOW_ALWAYS_RUN) == 0) glfwWaitEvents();

	WINDOW_ptr->shouldClose = glfwWindowShouldClose(WINDOW_ptr->platformHandle);

	// Reset close status for next frame
	glfwSetWindowShouldClose(WINDOW_ptr->platformHandle, GLFW_FALSE);
}


//endregion

//region Monitor
int get_monitor_count(void)
{
	int monitorCount = 0;

	glfwGetMonitors(&monitorCount);

	return monitorCount;
}

// Get number of monitors
int get_current_monitor(void)
{
	int index = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
	GLFWmonitor *monitor = NULL;

	if (monitorCount >= 1)
	{
		if (WINDOW_ptr->fullscreen)
		{
			// Get the handle of the monitor that the specified window is in full screen on
			monitor = glfwGetWindowMonitor(WINDOW_ptr->platformHandle);

			for (int i = 0; i < monitorCount; i++)
			{
				if (monitors[i] == monitor)
				{
					index = i;
					break;
				}
			}
		}
		else
		{
			// In case the window is between two monitors, we use below logic
			// to try to detect the "current monitor" for that window, note that
			// this is probably an overengineered solution for a very side case
			// trying to match SDL behaviour

			int closestDist = 0x7FFFFFFF;

			// Window center position
			int wcx = 0;
			int wcy = 0;

			glfwGetWindowPos(WINDOW_ptr->platformHandle, &wcx, &wcy);
			wcx += (int)WINDOW_ptr->screen.width / 2;
			wcy += (int)WINDOW_ptr->screen.height / 2;

			for (int i = 0; i < monitorCount; i++)
			{
				// Monitor top-left position
				int mx = 0;
				int my = 0;

				monitor = monitors[i];
				glfwGetMonitorPos(monitor, &mx, &my);
				const GLFWvidmode *mode = glfwGetVideoMode(monitor);

				if (mode)
				{
					const int right = mx + mode->width - 1;
					const int bottom = my + mode->height - 1;

					if ((wcx >= mx) &&
					    (wcx <= right) &&
					    (wcy >= my) &&
					    (wcy <= bottom))
					{
						index = i;
						break;
					}

					int xclosest = wcx;
					if (wcx < mx) xclosest = mx;
					else if (wcx > right) xclosest = right;

					int yclosest = wcy;
					if (wcy < my) yclosest = my;
					else if (wcy > bottom) yclosest = bottom;

					int dx = wcx - xclosest;
					int dy = wcy - yclosest;
					int dist = (dx*dx) + (dy*dy);
					if (dist < closestDist)
					{
						index = i;
						closestDist = dist;
					}
				}
				else printf("GLFW: Failed to find video mode for selected monitor");
			}
		}
	}

	return index;
}

// Get selected monitor position
V2 get_monitor_position(int monitor)
{
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		int x, y;
		glfwGetMonitorPos(monitors[monitor], &x, &y);

		return (V2){ (float)x, (float)y };
	}
	else printf("GLFW: Failed to find selected monitor");
	return (V2){ 0, 0 };
}

// Get selected monitor width (currently used by monitor)
int get_monitor_width(int monitor)
{
	int width = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

		if (mode) width = mode->width;
		else printf("GLFW: Failed to find video mode for selected monitor");
	}
	else printf("GLFW: Failed to find selected monitor");

	return width;
}

// Get selected monitor height (currently used by monitor)
int get_monitor_height(int monitor)
{
	int height = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

		if (mode) height = mode->height;
		else printf("GLFW: Failed to find video mode for selected monitor");
	}
	else printf("GLFW: Failed to find selected monitor");

	return height;
}

// Get selected monitor physical width in millimetres
int get_monitor_physical_width(int monitor)
{
	int width = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount)) glfwGetMonitorPhysicalSize(monitors[monitor], &width, NULL);
	else printf("GLFW: Failed to find selected monitor");

	return width;
}

// Get selected monitor physical height in millimetres
int get_monitor_physical_height(int monitor)
{
	int height = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount)) glfwGetMonitorPhysicalSize(monitors[monitor], NULL, &height);
	else printf("GLFW: Failed to find selected monitor");

	return height;
}

// Get selected monitor refresh rate
int get_monitor_refresh_rate(int monitor)
{
	int refresh = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		const GLFWvidmode *vidmode = glfwGetVideoMode(monitors[monitor]);
		refresh = vidmode->refreshRate;
	}
	else printf("GLFW: Failed to find selected monitor");

	return refresh;
}

// Get the human-readable, UTF-8 encoded name of the selected monitor
const char *get_monitor_name(int monitor)
{
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		return glfwGetMonitorName(monitors[monitor]);
	}
	else printf("GLFW: Failed to find selected monitor");
	return "";
}
//endregion
#pragma clang diagnostic pop