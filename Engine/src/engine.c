#include "engine.h"
#include "window.h"

void run_engine()
{
	create_window(800, 600, "Coal Miner");

	while (!window_should_close())
	{
		begin_draw();
		end_draw();
	}

	terminate_window();
}
