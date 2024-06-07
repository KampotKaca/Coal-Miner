#include "engine.h"
#include "window.h"

void run_engine()
{
	set_window_flags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	create_window(800, 600, "Coal Miner");

	while (!window_should_close())
	{
		begin_draw();
		end_draw();
	}

	terminate_window();
}
