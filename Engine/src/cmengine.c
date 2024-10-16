#include "coal_miner_internal.h"
#include "cmwindow.h"

EngineData CM_DEFAULT_ENGINE_DATA =
{
	(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE),
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
	bool isLoading = true;

	while (!window_should_close())
	{
		if(isLoading) isLoading = data.loadingCallback();
		if(!isLoading) data.updateCallback();

		begin_draw();
		if(!isLoading) data.renderCallback();
		end_draw();
	}

	data.appCloseCallback();
	close_window();
}