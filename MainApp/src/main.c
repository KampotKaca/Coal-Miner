#include "coal_miner_internal.h"
#include "game.h"

int main(void)
{
	EngineData data;
	memcpy(&data, &CM_DEFAULT_ENGINE_DATA, sizeof(EngineData));
	data.windowWidth = 1380;
	data.windowHeight = 720;
//	data.configFlags |= FLAG_FULLSCREEN_MODE;
	Path iconLoc = TO_RES_PATH(iconLoc, "2d/app/icons");
	data.iconLocation = iconLoc;

	data.awakeCallback = game_awake;
	data.updateCallback = game_update;
	data.renderCallback = game_render;
	data.appCloseCallback = game_close;

	coal_run(data);
	return 0;
}
