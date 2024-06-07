#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "platform.h"

Window WINDOW;
Input INPUT;

void create_window(unsigned int width, unsigned int height, const char *title)
{
	WINDOW.display.width = width;
	WINDOW.display.height = height;
	WINDOW.eventWaiting = false;
	if ((title != NULL) && (title[0] != 0)) WINDOW.title = title;
	else WINDOW.title = "Coal Miner";

	if(!init_platform(&WINDOW, &INPUT))
	{
		printf("Unable to load glfw window!!!");
		return;
	}

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Unable to load glad!!!");
		return;
	}

	// Setup default viewport
	setup_viewport((int)WINDOW.currentFbo.width, (int)WINDOW.currentFbo.height);

#if defined(SUPPORT_MODULE_RTEXT) && defined(SUPPORT_DEFAULT_FONT)
	// Load default font
    // WARNING: External function: Module required: rtext
    LoadFontDefault();
    #if defined(SUPPORT_MODULE_RSHAPES)
    // Set font white rectangle for shapes drawing, so shapes and text can be batched together
    // WARNING: rshapes module is required, if not available, default internal white rectangle is used
    Rectangle rec = GetFontDefault().recs[95];
    if (CORE.Window.flags & FLAG_MSAA_4X_HINT)
    {
        // NOTE: We try to maxime rec padding to avoid pixel bleeding on MSAA filtering
        SetShapesTexture(GetFontDefault().texture, (Rectangle){ rec.x + 2, rec.y + 2, 1, 1 });
    }
    else
    {
        // NOTE: We set up a 1px padding on char rectangle to avoid pixel bleeding
        SetShapesTexture(GetFontDefault().texture, (Rectangle){ rec.x + 1, rec.y + 1, rec.width - 2, rec.height - 2 });
    }
    #endif
#else
#if defined(SUPPORT_MODULE_RSHAPES)
	// Set default texture and rectangle to be used for shapes drawing
    // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });    // WARNING: Module required: rshapes
#endif
#endif
#if defined(SUPPORT_MODULE_RTEXT) && defined(SUPPORT_DEFAULT_FONT)
	if ((CORE.Window.flags & FLAG_WINDOW_HIGHDPI) > 0)
    {
        // Set default font texture filter for HighDPI (blurry)
        // RL_TEXTURE_FILTER_LINEAR - tex filter: BILINEAR, no mipmaps
        rlTextureParameters(GetFontDefault().texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
        rlTextureParameters(GetFontDefault().texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
    }
#endif

	WINDOW.shouldClose = false;
}

void set_window_flag(ConfigFlags hint)
{
	WINDOW.flags |= hint;
}

bool window_should_close()
{
	return glfwWindowShouldClose(WINDOW.platformHandle);
}

void begin_draw()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void end_draw()
{
	poll_input_events();
	swap_screen_buffer();
}

void terminate_window()
{
	glfwDestroyWindow(WINDOW.platformHandle);
	glfwTerminate();
}
