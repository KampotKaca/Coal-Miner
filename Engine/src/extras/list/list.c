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
	if(list->size < list->endPosition + size)
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

	memcpy(list->data + list->endPosition, value, size);
	list->endPosition += size;
}

bool list_remove(List* list, uint32_t from, uint32_t size)
{
	if(from >= list->endPosition)
	{
		log_error("Unable to remove from the list, from is outside the list");
		return false;
	}
	uint32_t sizeToRemove = cm_min(size, list->endPosition - from);
	uint32_t finish = from + sizeToRemove;
	memcpy(&list->data[from], &list->data[finish], list->endPosition - finish);
	list->endPosition -= sizeToRemove;
	
	return true;
}

void list_clear(List* list)
{
	list->size = 0;
	list->endPosition = 0;
	if(list->data)
	{
		CM_FREE(list->data);
		list->data = NULL;
	}
}

void list_reset(List* list)
{
	list->endPosition = 0;
}

uint32_t list_count(List* list, uint32_t elementSize)
{
	return list->endPosition / elementSize;
}

void list_getElement(List* list, uint32_t elementSize, uint32_t index, void* dest)
{
	memcpy(dest, &list->data[index * elementSize], elementSize);
}

void list_setElement(List* list, uint32_t elementSize, uint32_t index, void* src)
{
	memcpy(&list->data[index * elementSize], src, elementSize);
}

bool list_removeAt(List* list, uint32_t elementSize, uint32_t index)
{
	return list_remove(list, index * elementSize, elementSize);
}

bool list_removeRange(List* list, uint32_t elementSize, uint32_t index, uint32_t count)
{
	return list_remove(list, index * elementSize, elementSize * count);
}