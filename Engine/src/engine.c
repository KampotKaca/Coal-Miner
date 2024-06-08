#include "coal_miner_internal.h"
#include "window.h"

void coal_run()
{
	set_window_flags(COAL_WINDOW_FLAGS);
	create_window(COAL_WINDOW_WIDTH, COAL_WINDOW_HEIGHT, COAL_WINDOW_TITLE);

	while (!window_should_close())
	{
		begin_draw();
		end_draw();
	}

	close_window();
}