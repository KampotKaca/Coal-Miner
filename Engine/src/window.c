#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "platform.h"

Window WINDOW;

void create_window(unsigned int width, unsigned int height, const char *title)
{
	WINDOW.display.width = width;
	WINDOW.display.height = height;
	WINDOW.eventWaiting = false;
	if ((title != NULL) && (title[0] != 0)) WINDOW.title = title;
	else WINDOW.title = "Coal Miner";
	init_platform(&WINDOW);

	if(!WINDOW.ready)
	{
		printf("Unable to load glfw window!!!");
		return;
	}

	glfwMakeContextCurrent(WINDOW.platformHandle);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Unable to load glad!!!");
		return;
	}
}

void set_window_flags(ConfigFlags hint)
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
	glfwPollEvents();
	glfwSwapBuffers(WINDOW.platformHandle);
}

void terminate_window()
{
	glfwDestroyWindow(WINDOW.platformHandle);
	glfwTerminate();
}
