#include "coal_miner_internal.h"
#include "window.h"

EngineData DEFAULT_ENGINE_DATA =
{
	(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE),
	800,
	600,
	"Coal Miner",
	NULL,
};

void coal_run(EngineData data)
{
	set_window_flags(data.configFlags);
	create_window(data.windowWidth, data.windowHeight, data.title, data.iconLocation);

	data.awakeCallback();

	while (!window_should_close())
	{
		data.updateCallback();

		begin_draw();
		data.renderCallback();
		end_draw();

		data.frameEndCallback();
	}

	data.appCloseCallback();
	close_window();
}