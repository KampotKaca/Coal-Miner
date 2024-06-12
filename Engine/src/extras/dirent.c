#include "dirent.h"

DIR *opendir(const char *name)
{
	DIR *dir = 0;
	
	if (name && name[0])
	{
		size_t base_length = strlen(name);
		
		// Search pattern must end with suitable wildcard
		const char *all = strchr("/\\", name[base_length - 1]) ? "*" : "/*";
		
		if ((dir = (DIR *)DIRENT_MALLOC(sizeof *dir)) != 0 &&
		    (dir->name = (char *)DIRENT_MALLOC(base_length + strlen(all) + 1)) != 0)
		{
			strcat(strcpy(dir->name, name), all);
			
			if ((dir->handle = (handle_type) _findfirst(dir->name, &dir->info)) != -1)
			{
				dir->result.d_name = 0;
			}
			else  // rollback
			{
				DIRENT_FREE(dir->name);
				DIRENT_FREE(dir);
				dir = 0;
			}
		}
		else  // rollback
		{
			DIRENT_FREE(dir);
			dir   = 0;
			errno = ENOMEM;
		}
	}
	else errno = EINVAL;
	
	return dir;
}

int closedir(DIR *dir)
{
	int result = -1;
	
	if (dir)
	{
		if (dir->handle != -1) result = _findclose(dir->handle);
		
		DIRENT_FREE(dir->name);
		DIRENT_FREE(dir);
	}
	
	// NOTE: All errors ampped to EBADF
	if (result == -1) errno = EBADF;
	
	return result;
}

struct dirent *readdir(DIR *dir)
{
	struct dirent *result = 0;
	
	if (dir && dir->handle != -1)
	{
		if (!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1)
		{
			result = &dir->result;
			result->d_name = dir->info.name;
		}
	}
	else errno = EBADF;
	
	return result;
}

void rewinddir(DIR *dir)
{
	if (dir && dir->handle != -1)
	{
		_findclose(dir->handle);
		dir->handle = (handle_type) _findfirst(dir->name, &dir->info);
		dir->result.d_name = 0;
	}
	else errno = EBADF;
}