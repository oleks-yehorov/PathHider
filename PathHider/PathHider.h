#ifndef __PATH_HIDER_H__
#define __PATH_HIDER_H__

#include <fltkernel.h>

#include "IntrusivePtr.hpp"
#include "UnicodeString.h"
#include "Constants.h"
#include "Memory.h"

EXTERN_C_START
NTSTATUS
ZwQueryInformationProcess(_In_ HANDLE ProcessHandle,
                          _In_ PROCESSINFOCLASS ProcessInformationClass,
                          _Out_ PVOID ProcessInformation,
                          _In_ ULONG ProcessInformationLength,
                          _Out_opt_ PULONG ReturnLength);

struct FileList : public RefCountedBase
{
    // LIST_ENTRY m_listEntry;
    intrusive_ptr<FileList> m_next;
    KUtils::UnicodeString m_name;
};

struct FolderData
{
    LIST_ENTRY m_listEntry;
    KUtils::UnicodeString m_path;
    //LIST_ENTRY m_fileListHead;
    intrusive_ptr<FileList> m_fileListHead;
};

struct FolderContext
{
    intrusive_ptr<FileList> m_fileListHead;
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {
    { FLT_FILE_CONTEXT, 0, nullptr, sizeof(FolderContext), DRIVER_TAG, nullptr,
      nullptr, nullptr },
    { FLT_CONTEXT_END }
};

EXTERN_C_END

#endif //__PATH_HIDER_H__