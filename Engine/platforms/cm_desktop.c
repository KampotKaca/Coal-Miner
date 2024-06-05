#define GLFW_INCLUDE_NONE       // Disable the standard OpenGL header inclusion on GLFW3

#if defined(PLATFORM_LINUX)
#include <sys/time.h>               // Required for: timespec, nanosleep(), select() - POSIX

    //#define GLFW_EXPOSE_NATIVE_X11      // WARNING: Exposing Xlib.h > X.h results in dup symbols for Font type
    //#define GLFW_EXPOSE_NATIVE_WAYLAND
    //#define GLFW_EXPOSE_NATIVE_MIR
    #include "GLFW/glfw3native.h"       // Required for: glfwGetX11Window()
#endif
#if defined(PLATFORM_MAC)
#include <unistd.h>                 // Required for: usleep()

    //#define GLFW_EXPOSE_NATIVE_COCOA    // WARNING: Fails due to type redefinition
    void *glfwGetCocoaWindow(GLFWwindow* handle);
    #include "GLFW/glfw3native.h"       // Required for: glfwGetCocoaWindow()
#endif

#include "platform.h"
#include "GLFW/glfw3.h"

static void glfw_error_callback(int error, const char *description)
{
	printf("GLFW_Error: %i Description: %s", error, description);
}

void init_platform(Window* window)
{
	glfwSetErrorCallback(glfw_error_callback);

	int result = glfwInit();
	if (result == GLFW_FALSE) { printf("Failed to initialize GLFW"); return; }
	glfwDefaultWindowHints();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, COAL_PROFILE_MAJOR);          // Choose OpenGL major version (just hint)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, COAL_PROFILE_MINOR);          // Choose OpenGL minor version (just hint)
	glfwWindowHint(GLFW_OPENGL_PROFILE, COAL_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	glfwSetJoystickCallback(NULL);

	// Find monitor resolution
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	if (!monitor)
	{
		printf("GLFW: Failed to get primary monitor");
		return;
	}

	//region flags
	if ((window->flags & FLAG_WINDOW_HIDDEN) > 0) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Visible window
	else glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);     // Window initially hidden

	if ((window->flags & FLAG_WINDOW_UNDECORATED) > 0) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Border and buttons on Window
	else glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);   // Decorated window

	if ((window->flags & FLAG_WINDOW_RESIZABLE) > 0) glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Resizable window
	else glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // Avoid window being resizable

	// Disable FLAG_WINDOW_MINIMIZED, not supported on initialization
	if ((window->flags & FLAG_WINDOW_MINIMIZED) > 0) window->flags &= ~FLAG_WINDOW_MINIMIZED;

	// Disable FLAG_WINDOW_MAXIMIZED, not supported on initialization
	if ((window->flags & FLAG_WINDOW_MAXIMIZED) > 0) window->flags &= ~FLAG_WINDOW_MAXIMIZED;

	if ((window->flags & FLAG_WINDOW_UNFOCUSED) > 0) glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
	else glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

	if ((window->flags & FLAG_WINDOW_TOPMOST) > 0) glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	else glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

	// NOTE: Some GLFW flags are not supported on HTML5
	if ((window->flags & FLAG_WINDOW_TRANSPARENT) > 0) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);     // Transparent framebuffer
	else glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);  // Opaque framebuffer

	if ((window->flags & FLAG_WINDOW_HIGHDPI) > 0)
	{
		// Resize window content area based on the monitor content scale.
		// NOTE: This hint only has an effect on platforms where screen coordinates and pixels always map 1:1 such as Windows and X11.
		// On platforms like macOS the resolution of the framebuffer is changed independently of the window size.
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);   // Scale content area based on the monitor content scale where window is placed on
#if defined(PLATFORM_MAC)
		glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif
	}
	else glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

	// Mouse passthrough
	if ((window->flags & FLAG_WINDOW_MOUSE_PASSTHROUGH) > 0) glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
	else glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);

	if (window->flags & FLAG_MSAA_4X_HINT)
		glfwWindowHint(GLFW_SAMPLES, 4);   // Tries to enable multisampling x4 (MSAA), default is 0
	//endregion

	GLFWwindow* mainWindow = glfwCreateWindow((int)window->display.width, (int)window->display.height, window->title, NULL, NULL);
	if(mainWindow)
	{
		window->ready = true;
		window->platformHandle = mainWindow;
	}
}