#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "rendering.h"

Window WINDOW;
Input INPUT;

Image icons[MAX_NUM_APPLICATION_ICONS];
int iconCount;

int FillWithAppIcons(const char* filePath, Image* storeLocation)
{
	struct dirent *entry;
	DIR *dir = opendir(filePath);

	if (dir == NULL)
	{
		printf("%s", filePath);
		perror("Unable to open directory");
		return 0;
	}

	int imageId = 0;
	while ((entry = readdir(dir)) && imageId < MAX_NUM_APPLICATION_ICONS)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

		char newPath[MAX_PATH_SIZE];
		snprintf(newPath, MAX_PATH_SIZE, "%s/%s", filePath, entry->d_name);

		storeLocation[imageId] = cm_load_image(newPath);
		imageId++;
	}

	closedir(dir);
	return imageId;
}

void create_window(unsigned int width, unsigned int height,
				   const char *title, const char* iconLocation)
{
	WINDOW.display.width = width;
	WINDOW.display.height = height;
	WINDOW.eventWaiting = false;
	WINDOW.screenScale = m4x4_identity();
	if ((title != NULL) && (title[0] != 0)) WINDOW.title = title;
	else WINDOW.title = "Coal Miner";

	memset(&INPUT, 0, sizeof(INPUT));     // Reset CORE.Input structure to 0
	INPUT.Keyboard.exitKey = APPLICATION_EXIT_KEY;
	INPUT.Mouse.scale = (V2){ 1.0f, 1.0f };
	INPUT.Mouse.cursor = MOUSE_CURSOR_ARROW;
	INPUT.Gamepad.lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;

	if(!init_platform(&WINDOW, &INPUT))
	{
		printf("Unable to load glfw window!!!");
		return;
	}

	iconCount = 0;

	if(iconLocation != NULL)
	{
		iconCount = FillWithAppIcons(iconLocation, icons);
		cm_set_window_icons(icons, iconCount);
	}

	glDepthFunc(GL_LEQUAL);                                 // Type of depth testing to apply
	glDisable(GL_DEPTH_TEST);                               // Disable depth testing for 2D (only used for 3D)

	// Init state: Blending mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);      // Color blending function (how colors are mixed)
	glEnable(GL_BLEND);                                     // Enable color blending (required to work with transparencies)

	// Init state: Culling
	// NOTE: All shapes/models triangles are drawn CCW
	glCullFace(GL_BACK);                                    // Cull the back face (default)
	glFrontFace(GL_CCW);                                    // Front face are defined counter clockwise (default)
	glEnable(GL_CULL_FACE);                                 // Enable backface culling

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);                   // Set clear color (black)
	glClearDepth(1.0f);                                     // Set clear depth value (default)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup default viewport
	setup_viewport((int)WINDOW.currentFbo.width, (int)WINDOW.currentFbo.height);

	WINDOW.shouldClose = false;

	load_renderer(&WINDOW);
}

void set_window_flags(ConfigFlags hint)
{
	WINDOW.flags |= hint;
}

bool window_should_close()
{
	if (WINDOW.ready) return WINDOW.shouldClose;
	else return true;
}

void begin_draw()
{
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void end_draw()
{
	swap_screen_buffer();
	poll_input_events();
}

void close_window()
{
	unload_renderer();
	cm_unload_images(icons, iconCount);
	WINDOW.ready = false;
	terminate_platform();
}

//region internals

void cm_minimize_window() { minimize_window(); }
void cm_maximize_window() { maximize_window(); }
void cm_toggle_fullscreen() { toggle_fullscreen(); }
void cm_toggle_borderless_windowed() { toggle_borderless_windowed(); }
void cm_restore_window() { restore_window(); }

void cm_set_window_title(const char *title) { set_window_title(title); }
void cm_set_window_position(V2i position) { set_window_position(position.x, position.y); }
void cm_set_window_monitor(int monitor) { set_window_monitor(monitor); }
void cm_set_window_min_size(V2i size) { set_window_min_size(size.x, size.y); }
void cm_set_window_max_size(V2i size) { set_window_max_size(size.x, size.y); }
void cm_set_window_size(V2i size) { set_window_size(size.x, size.y); }
void cm_set_window_opacity(float opacity) { set_window_opacity(opacity); }
void cm_set_window_focused(void) { set_window_focused(); }

void cm_set_window_icon(Image image) { set_window_icon(image); }
void cm_set_window_icons(Image* images, int count) { set_window_icons(images, count); }

V2 cm_get_window_position(void) { return get_window_position(); }
V2 cm_get_window_scale_dpi(void) { return get_window_scale_dpi(); }
void cm_set_clipboard_text(const char *text) { set_clipboard_text(text); }
const char *cm_get_clipboard_text(void) { return get_clipboard_text(); }
void cm_show_cursor(void) { show_cursor(); }
void cm_hide_cursor(void) { hide_cursor(); }
void cm_enable_cursor(void) { enable_cursor(); }
void cm_disable_cursor(void) { disable_cursor(); }
void cm_set_mouse_position(V2i position) { set_mouse_position(position.x, position.y); }
void cm_set_mouse_cursor(int cursor) { set_mouse_cursor(cursor); }

//endregion
