#include "coal_miner_internal.h"
#include "game.h"

int main(void)
{
	EngineData data;
	memcpy(&data, &CM_DEFAULT_ENGINE_DATA, sizeof(EngineData));
	char iconLoc[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(iconLoc, MAX_PATH_SIZE, "2d/app/icons");
	data.iconLocation = iconLoc;

	data.awakeCallback = game_awake;
	data.updateCallback = game_update;
	data.renderCallback = game_render;
	data.appCloseCallback = game_close;

	coal_run(data);
	return 0;
}
