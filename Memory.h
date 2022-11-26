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

    return result;
}

inline void __cdecl operator delete(void* ptr, unsigned __int64) 
{
    if (ptr)
    {
        ExFreePool(ptr);
    }
}