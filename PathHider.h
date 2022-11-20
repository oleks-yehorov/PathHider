#ifndef __PATH_HIDER_H__
#define __PATH_HIDER_H__

#include <fltkernel.h>

#include "Constants.h"

EXTERN_C_START
NTSTATUS
ZwQueryInformationProcess(_In_ HANDLE ProcessHandle,
                          _In_ PROCESSINFOCLASS ProcessInformationClass,
                          _Out_ PVOID ProcessInformation,
                          _In_ ULONG ProcessInformationLength,
                          _Out_opt_ PULONG ReturnLength);

struct FolderData
{
    LIST_ENTRY m_listEntry;
    UNICODE_STRING m_path;
    LIST_ENTRY m_fileListHead;
};

struct FileList
{
    LIST_ENTRY m_listEntry;
    UNICODE_STRING m_name;
};

struct FolderContext
{
    LIST_ENTRY* m_fileListHead;
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {
    { FLT_FILE_CONTEXT, 0, nullptr, sizeof(FolderContext), DRIVER_TAG, nullptr,
      nullptr, nullptr },
    { FLT_CONTEXT_END }
};

EXTERN_C_END

#endif //__PATH_HIDER_H__