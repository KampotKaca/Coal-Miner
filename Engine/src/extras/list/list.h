#ifndef LIST_H
#define LIST_H

#include <inttypes.h>

typedef struct List
{
	uint32_t size;
	uint32_t position;
	void* data;
}List;

extern List list_create(uint32_t initialCapacity);
extern void list_add(List* list, uint32_t minAllocSize, void* value, uint32_t size);
//frees allocated memory
extern void list_clear(List* list);
//sets position to 0 but does not free memory
extern void list_reset(List* list);

#endif //LIST_H
