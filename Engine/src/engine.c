#include "coal_miner_internal.h"
#include "window.h"

int fill_with_app_icons(const char* filePath, Image* storeLocation);

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
	create_window(data.windowWidth, data.windowHeight, data.title);

	Image icons[MAX_NUM_APPLICATION_ICONS];
	int iconCount = 0;
	
	if(data.iconLocation != NULL)
	{
		iconCount = fill_with_app_icons(data.iconLocation, icons);
		cm_set_window_icons(icons, iconCount);
	}
	
	while (!window_should_close())
	{
		begin_draw();
		end_draw();
	}
	
	cm_unload_images(icons, iconCount);
	
	close_window();
}

int fill_with_app_icons(const char* filePath, Image* storeLocation)
{
	struct dirent *entry;
	DIR *dir = opendir(filePath);
	
	if (dir == NULL)
	{
		printf("%s", filePath);
		perror("Unable to open directory");
		return 0;
	}
	
	int imageId = 0;
	while ((entry = readdir(dir)) && imageId < MAX_NUM_APPLICATION_ICONS)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		
		char newPath[MAX_PATH_SIZE];
		int len = snprintf(newPath, MAX_PATH_SIZE, "%s/%s", filePath, entry->d_name);
		
//		newPath[len] = 0;
		storeLocation[imageId] = cm_load_image(newPath);
		printf("%s\n", newPath);
		imageId++;
	}
	
	closedir(dir);
	return imageId;
}