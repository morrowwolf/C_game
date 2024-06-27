
#include "globals.h"

void List_DeallocatePointOnRemove(Point *data)
{
    MemoryManager_DeallocateMemory((void **)&data, sizeof(Point));
}
