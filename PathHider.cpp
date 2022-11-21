/*++

Module Name:

        PathHider.c

Abstract:

        This is the main module of the PathHider miniFilter driver.

Environment:

        Kernel mode

--*/

#include "PathHider.h"
#include "UnicodeStringGuard.h"
#include "UnicodeString.h"
#include <dontuse.h>
#include <fltKernel.h>

#pragma prefast(disable                                                        \
                : __WARNING_ENCODE_MEMBER_FUNCTION_POINTER,                    \
                  "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES 0x00000001
#define PTDBG_TRACE_OPERATION_STATUS 0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT(_dbgLevel, _string)                                       \
    (FlagOn(gTraceFlags, (_dbgLevel)) ? DbgPrint _string : ((int)0))

LIST_ENTRY gFolderDataHead;
/*************************************************************************
        Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath);

NTSTATUS
PathHiderUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS
PathHiderPostCreate(_Inout_ PFLT_CALLBACK_DATA Data,
                    _In_ PCFLT_RELATED_OBJECTS FltObjects,
                    _In_ PVOID CompletionContext,
                    _In_ FLT_POST_OPERATION_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS
PathHiderPreDirectoryControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
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
NTSTATUS AddPathToHide(_In_ PUNICODE_STRING Path);

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

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE, 0, nullptr, PathHiderPostCreate },
    { IRP_MJ_DIRECTORY_CONTROL, 0, PathHiderPreDirectoryControl,
      PathHiderPostDirectoryControl },
    { IRP_MJ_CLEANUP, 0, nullptr, PathHiderPostCleanup },
    { IRP_MJ_OPERATION_END }
};

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

    status =
        FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);

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

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
                 ("PathHider!PathHiderUnload: Entered\n"));

    FltUnregisterFilter(gFilterHandle);

    ShutDown();
    return STATUS_SUCCESS;
}

NTSTATUS AddPathToHide(_In_ PUNICODE_STRING Path)
{
    if (Path->Length <= 0)
        return STATUS_INVALID_PARAMETER;
    // extract parent folder
    auto lastSeparator = wcsrchr(Path->Buffer, L'\\');
    auto folderPathLength = lastSeparator - Path->Buffer;
    KUtils::UnicodeString folderPath(Path->Buffer, static_cast<USHORT>(folderPathLength*sizeof(WCHAR)));
    KUtils::UnicodeString fileName(lastSeparator + 1);

    // enumerate list - looking for the parent folder
    PLIST_ENTRY temp = &gFolderDataHead;
    FolderData* folderData = nullptr;
    while (&gFolderDataHead != temp->Flink)
    {
        temp = temp->Flink;
        auto curFolderData = CONTAINING_RECORD(temp, FolderData, m_listEntry);
        if (RtlEqualUnicodeString(&curFolderData->m_path,
                                  &folderPath.GetUnicodeString(),
                                  TRUE))
        {
            folderData = curFolderData;
            break;
        }
    }
    // parent folder does not exist - add new foder entry
    if (!folderData)
    {
        folderData = static_cast<FolderData*>(
            ExAllocatePoolWithTag(PagedPool, sizeof(FolderData), DRIVER_TAG));
        if (!folderData)
            return STATUS_INSUFFICIENT_RESOURCES;

        InsertHeadList(&gFolderDataHead, &(folderData->m_listEntry));
        InitializeListHead(&folderData->m_fileListHead);
        //
        folderData->m_path.Buffer = static_cast<PWCH>(ExAllocatePoolWithTag(
            PagedPool, folderPath.MaxByteLength(),
            DRIVER_TAG));
        if (!folderData->m_path.Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;
        // folderData->m_path.Length = 0;
        folderData->m_path.MaximumLength = folderPath.MaxCharLength();
        RtlCopyUnicodeString(&folderData->m_path,
                             &folderPath.GetUnicodeString());
    }
    // insert file name to this folder entry
    FileList* newFileEntity = static_cast<FileList*>(
        ExAllocatePoolWithTag(PagedPool, sizeof(FileList), DRIVER_TAG));
    if (!newFileEntity)
        return STATUS_INSUFFICIENT_RESOURCES;
    InsertHeadList(&folderData->m_fileListHead, &(newFileEntity->m_listEntry));

    // file name
    newFileEntity->m_name.Buffer = static_cast<PWCH>(ExAllocatePoolWithTag(PagedPool, fileName.MaxByteLength(), DRIVER_TAG));
    newFileEntity->m_name.MaximumLength = fileName.MaxCharLength();
    RtlCopyUnicodeString(&newFileEntity->m_name, &fileName.GetUnicodeString());
    return STATUS_SUCCESS;
}

NTSTATUS Init()
{
    RtlZeroMemory(&gFolderDataHead, sizeof(gFolderDataHead));
    InitializeListHead(&gFolderDataHead);
    // TODO - remove this hardcoded part
    UNICODE_STRING path1;
    RtlInitUnicodeString(&path1, L"\\Device\\HarddiskVolume1\\test\\1.txt");
    auto status = AddPathToHide(&path1);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING path2;
    RtlInitUnicodeString(&path2, L"\\Device\\HarddiskVolume1\\test\\1.bmp");
    status = AddPathToHide(&path2);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING path3;
    RtlInitUnicodeString(&path3,
                         L"\\Device\\HarddiskVolume1\\test\\test\\2.txt");
    status = AddPathToHide(&path3);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING path4;
    RtlInitUnicodeString(&path4,
                         L"\\Device\\HarddiskVolume1\\test\\test\\0.txt");
    status = AddPathToHide(&path4);
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
        PLIST_ENTRY tempFileList =
            RemoveHeadList(&curFolderData->m_fileListHead);
        while (&curFolderData->m_fileListHead != tempFileList)
        {
            FileList* curFileData = reinterpret_cast<FileList*>(tempFileList);
            UnicodeStringGuard guardFileName(&curFileData->m_name);
            ExFreePoolWithTag(curFileData, DRIVER_TAG);
            tempFileList = RemoveHeadList(&curFolderData->m_fileListHead);
        }
        UnicodeStringGuard guardFolderPath(&curFolderData->m_path);
        ExFreePoolWithTag(curFolderData, DRIVER_TAG);
        tempFolder = RemoveHeadList(&gFolderDataHead);
    }
}
