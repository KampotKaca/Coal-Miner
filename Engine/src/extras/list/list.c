#include "list.h"
#include "coal_miner.h"
#include "coal_helper.h"

List list_create(uint32_t initialCapacity)
{
	List list = { .size = initialCapacity, .data = NULL };
	if(initialCapacity > 0) list.data = CM_CALLOC(1, initialCapacity);
	return list;
}

void list_add(List* list, uint32_t minAllocSize, void* value, uint32_t size)
{
	if(list->size < list->position + size)
	{
		uint32_t oldSize = list->size;
		uint32_t newSize = cm_max(oldSize * 2, minAllocSize * size);
		void* newMemory = CM_REALLOC(list->data, newSize);
		if(newMemory == NULL)
		{
			perror("Unable to reallocate memory!!! exiting the program.\n");
			exit(-1);
		}
		else
		{
			memset(newMemory + oldSize, 0, newSize - oldSize);
			list->size = newSize;
			list->data = newMemory;
		}
	}

	memcpy(list->data + list->position, value, size);
	list->position += size;
}

void list_clear(List* list)
{
	list->size = 0;
	list->position = 0;
	if(list->data)
	{
		CM_FREE(list->data);
		list->data = NULL;
	}
}

void list_reset(List* list)
{
	list->position = 0;
}