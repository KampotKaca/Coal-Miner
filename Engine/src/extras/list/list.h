#ifndef LIST_H
#define LIST_H

#include <inttypes.h>

typedef struct List
{
	uint32_t size;
	uint32_t endPosition;
	void* data;
}List;

extern List list_create(uint32_t initialCapacity);
extern void list_add(List* list, uint32_t minAllocSize, void* value, uint32_t size);
extern bool list_remove(List* list, uint32_t from, uint32_t size);
//frees allocated memory
extern void list_clear(List* list);
//sets position to 0 but does not free memory
extern void list_reset(List* list);

//region ElementControl
extern uint32_t list_count(List* list, uint32_t elementSize);
extern void list_getElement(List* list, uint32_t elementSize, uint32_t index, void* dest);
extern void list_setElement(List* list, uint32_t elementSize, uint32_t index, void* src);
extern bool list_removeAt(List* list, uint32_t elementSize, uint32_t index);
extern bool list_removeRange(List* list, uint32_t elementSize, uint32_t index, uint32_t count);
//endregion

#endif //LIST_H
