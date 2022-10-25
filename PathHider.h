#ifndef __PATH_HIDER_H__
#define __PATH_HIDER_H__

#include <fltkernel.h>

FLT_PREOP_CALLBACK_STATUS
PathHiderEnumerateDirectory(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

#endif //__PATH_HIDER_H__