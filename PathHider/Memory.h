#pragma once

#include "Constants.h"
#include <ntddk.h>

inline void* __cdecl operator new(unsigned __int64 size) 
{
    PVOID result = ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);

    if (result)
    {
        RtlZeroMemory(result, size);
    }
    else
    {
        ExRaiseStatus(STATUS_NO_MEMORY);
    }

    return result;
}

inline void* __cdecl operator new[](size_t size, POOL_TYPE pool)
{
    PVOID ptr = ExAllocatePoolWithTag(pool, (ULONG)size, DRIVER_TAG);
    if (ptr == NULL)
    {
        ExRaiseStatus(STATUS_NO_MEMORY);
    }
    return ptr;
}

inline void __cdecl operator delete(void* ptr, unsigned __int64) 
{
    if (ptr)
    {
        ExFreePool(ptr);
    }
}