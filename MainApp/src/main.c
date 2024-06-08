#include "coal_miner_internal.h"

int main(void)
{
	EngineData data = DEFAULT_ENGINE_DATA;
	char iconLoc[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(iconLoc, MAX_PATH_SIZE, "2d/app/icons");
	data.iconLocation = iconLoc;
	coal_run(data);
	return 0;
}
