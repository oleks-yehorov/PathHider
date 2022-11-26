/*++

Module Name:

        PathHider.c

Abstract:

        This is the main module of the PathHider miniFilter driver.

Environment:

        Kernel mode

--*/

#include "PathHider.h"
#include "UnicodeString.h"
#include "UnicodeStringGuard.h"
#include <dontuse.h>
#include <fltKernel.h>

#pragma prefast(disable : __WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES 0x00000001
#define PTDBG_TRACE_OPERATION_STATUS 0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT(_dbgLevel, _string) (FlagOn(gTraceFlags, (_dbgLevel)) ? DbgPrint _string : ((int)0))

LIST_ENTRY gFolderDataHead;
/*************************************************************************
        Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

NTSTATUS
PathHiderUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS
PathHiderPostCreate(_Inout_ PFLT_CALLBACK_DATA Data,
                    _In_ PCFLT_RELATED_OBJECTS FltObjects,
                    _In_ PVOID CompletionContext,
                    _In_ FLT_POST_OPERATION_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS
PathHiderPreDirectoryControl(_Inout_ PFLT_CALLBACK_DATA Data,
                             _In_ PCFLT_RELATED_OBJECTS FltObjects,
                             _Flt_CompletionContext_Outptr_ PVOID* CompletionContext);

FLT_POSTOP_CALLBACK_STATUS
PathHiderPostDirectoryControl(_Inout_ PFLT_CALLBACK_DATA Data,
                              _In_ PCFLT_RELATED_OBJECTS FltObjects,
                              _In_ PVOID CompletionContext,
                              _In_ FLT_POST_OPERATION_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS
PathHiderPostCleanup(_Inout_ PFLT_CALLBACK_DATA Data,
                     _In_ PCFLT_RELATED_OBJECTS FltObjects,
                     _In_ PVOID CompletionContext,
                     _In_ FLT_POST_OPERATION_FLAGS Flags);

// folder list
NTSTATUS Init();
void ShutDown();
NTSTATUS AddPathToHide(const KUtils::UnicodeString& Path, const KUtils::UnicodeString& Name);

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PathHiderUnload)
#pragma alloc_text(PAGE, PathHiderPreDirectoryControl)
#pragma alloc_text(PAGE, PathHiderPostDirectoryControl)
#pragma alloc_text(PAGE, PathHiderPostCreate)
#pragma alloc_text(PAGE, PathHiderPostCleanup)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = { { IRP_MJ_CREATE, 0, nullptr, PathHiderPostCreate },
                                                 { IRP_MJ_DIRECTORY_CONTROL, 0, PathHiderPreDirectoryControl,
                                                   PathHiderPostDirectoryControl },
                                                 { IRP_MJ_CLEANUP, 0, nullptr, PathHiderPostCleanup },
                                                 { IRP_MJ_OPERATION_END } };

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION), //  Size
    FLT_REGISTRATION_VERSION, //  Version
    0,                        //  Flags

    ContextRegistration, //  Context
    Callbacks,           //  Operation callbacks

    PathHiderUnload, //  MiniFilterUnload

    NULL, //  InstanceSetup
    NULL, //  InstanceQueryTeardown
    NULL, //  InstanceTeardownStart
    NULL, //  InstanceTeardownComplete

    NULL, //  GenerateFileName
    NULL, //  GenerateDestinationFileName
    NULL  //  NormalizeNameComponent

};

/*************************************************************************
        MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PathHider!DriverEntry: Entered\n"));

    status = Init();
    if (!NT_SUCCESS(status))
        return status;

    status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status))
    {
        status = FltStartFiltering(gFilterHandle);
        if (!NT_SUCCESS(status))
        {
            FltUnregisterFilter(gFilterHandle);
        }
    }
    return status;
}

NTSTATUS
PathHiderUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PathHider!PathHiderUnload: Entered\n"));

    FltUnregisterFilter(gFilterHandle);

    ShutDown();
    return STATUS_SUCCESS;
}

NTSTATUS AddPathToHide(const KUtils::UnicodeString& Path, const KUtils::UnicodeString& Name)
{
    if (Path.IsEmpty() || Name.IsEmpty())
        return STATUS_INVALID_PARAMETER;

    // enumerate list - looking for the parent folder
    PLIST_ENTRY temp = &gFolderDataHead;
    FolderData* folderData = nullptr;
    while (&gFolderDataHead != temp->Flink)
    {
        temp = temp->Flink;
        auto curFolderData = CONTAINING_RECORD(temp, FolderData, m_listEntry);
        if (curFolderData->m_path == Path)
        {
            folderData = curFolderData;
            break;
        }
    }
    // parent folder does not exist - add new foder entry
    if (!folderData)
    {
        folderData = static_cast<FolderData*>(ExAllocatePoolWithTag(PagedPool, sizeof(FolderData), DRIVER_TAG));
        if (!folderData)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlZeroMemory(folderData, sizeof(FolderData));
        InsertHeadList(&gFolderDataHead, &(folderData->m_listEntry));
        //
        folderData->m_path = Path;
    }
    // insert file name to this folder entry
    intrusive_ptr<FileList> fileEntry = new FileList();
    fileEntry->m_name = Name;
    fileEntry->m_next = intrusive_ptr<FileList>(folderData->m_fileListHead);
    folderData->m_fileListHead = intrusive_ptr<FileList>(fileEntry);

    return STATUS_SUCCESS;
}

NTSTATUS Init()
{
    RtlZeroMemory(&gFolderDataHead, sizeof(gFolderDataHead));
    InitializeListHead(&gFolderDataHead);
    // TODO - remove this hardcoded part
    PWCHAR path1 = L"\\Device\\HarddiskVolume1\\test";
    KUtils::UnicodeString path1str(path1, static_cast<USHORT>(wcslen(path1) * sizeof(WCHAR)), PagedPool);
    PWCHAR name1 = L"1.txt";
    KUtils::UnicodeString name1str(name1, static_cast<USHORT>(wcslen(name1) * sizeof(WCHAR)), PagedPool);
    auto status = AddPathToHide(path1str, name1str);
    if (!NT_SUCCESS(status))
        return status;

    PWCHAR path2 = L"\\Device\\HarddiskVolume1\\test";
    KUtils::UnicodeString path2str(path2, static_cast<USHORT>(wcslen(path2) * sizeof(WCHAR)), PagedPool);
    PWCHAR name2 = L"1.bmp";
    KUtils::UnicodeString name2str(name2, static_cast<USHORT>(wcslen(name2) * sizeof(WCHAR)), PagedPool);
    status = AddPathToHide(path2str, name2str);
    if (!NT_SUCCESS(status))
        return status;

    PWCHAR path3 = L"\\Device\\HarddiskVolume1\\test\\test";
    KUtils::UnicodeString path3str(path3, static_cast<USHORT>(wcslen(path3) * sizeof(WCHAR)), PagedPool);
    PWCHAR name3 = L"2.txt";
    KUtils::UnicodeString name3str(name3, static_cast<USHORT>(wcslen(name3) * sizeof(WCHAR)), PagedPool);
    status = AddPathToHide(path3str, name3str);
    if (!NT_SUCCESS(status))
        return status;

    PWCHAR path4 = L"\\Device\\HarddiskVolume1\\test\\test";
    KUtils::UnicodeString path4str(path4, static_cast<USHORT>(wcslen(path4) * sizeof(WCHAR)), PagedPool);
    PWCHAR name4 = L"0.txt";
    KUtils::UnicodeString name4str(name4, static_cast<USHORT>(wcslen(name4) * sizeof(WCHAR)), PagedPool);
    status = AddPathToHide(path4str, name4str);
    if (!NT_SUCCESS(status))
        return status;
    // END TODO
    return STATUS_SUCCESS;
}

void ShutDown()
{
    PLIST_ENTRY tempFolder = RemoveHeadList(&gFolderDataHead);
    while (&gFolderDataHead != tempFolder)
    {
        FolderData* curFolderData = reinterpret_cast<FolderData*>(tempFolder);
        curFolderData->m_fileListHead = nullptr;
        ExFreePoolWithTag(curFolderData, DRIVER_TAG);
        tempFolder = RemoveHeadList(&gFolderDataHead);
    }
}
