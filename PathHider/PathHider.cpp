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
#include "UserModeShared.h"
#include "FastMutex.h"
#include "AutoLock.h"
#include <dontuse.h>
#include <fltKernel.h>

#pragma prefast(disable : __WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;
PFLT_PORT FilterPort;
PFLT_PORT SendClientPort;

#define PTDBG_TRACE_ROUTINES 0x00000001
#define PTDBG_TRACE_OPERATION_STATUS 0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT(_dbgLevel, _string) (FlagOn(gTraceFlags, (_dbgLevel)) ? DbgPrint _string : ((int)0))

KUtils::FastMutex gFolderDataLock;
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
NTSTATUS AddPathToHide(const KUtils::UnicodeString& Path, const KUtils::UnicodeString& Name);
void ShutDown();
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

NTSTATUS
PortConnectNotify(PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext, ULONG SizeOfContext, PVOID* ConnectionPortCookie)
{
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionPortCookie);

    SendClientPort = ClientPort;

    return STATUS_SUCCESS;
}

void PortDisconnectNotify(PVOID ConnectionCookie)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);

    FltCloseClientPort(gFilterHandle, &SendClientPort);
    SendClientPort = nullptr;
}

NTSTATUS PortMessageNotify(PVOID PortCookie,
                           PVOID InputBuffer,
                           ULONG InputBufferLength,
                           PVOID OutputBuffer,
                           ULONG OutputBufferLength,
                           PULONG ReturnOutputBufferLength)
{
    UNREFERENCED_PARAMETER(PortCookie);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ReturnOutputBufferLength);
    
    NTSTATUS status = STATUS_SUCCESS;
    __try
    {
        if (InputBuffer != NULL && InputBufferLength == sizeof(PHMessage))
        {
            PHMessage* message = reinterpret_cast<PHMessage*>(InputBuffer);
            switch (message->m_action)
            {
            case PHAction::AddPathToHideAction:
            {
                // TODO - add proper status response to usermode
                KUtils::UnicodeString path(message->m_data->m_path, PagedPool);
                KUtils::UnicodeString name(message->m_data->m_name, PagedPool);
                auto addStatus = AddPathToHide(path, name);
                if (!NT_SUCCESS(addStatus))
                {
                    KdPrint(("Failed to hide path (0x%08X)\n", addStatus));
                }
                break;
            }
            case PHAction::RemoveAllhiddenPaths:
                ShutDown();
                break;
            default:
                KdPrint(("Unexpected action code %i", message->m_action));
                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        KdPrint(("An exception occurred at " __FUNCTION__ "\n"));
    }
    return status;
}


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
        
   	do
    {
        UNICODE_STRING name = RTL_CONSTANT_STRING(COMMUNICATION_PORT);
        PSECURITY_DESCRIPTOR sd;

        status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
        if (!NT_SUCCESS(status))
            break;

        OBJECT_ATTRIBUTES attr;
        InitializeObjectAttributes(&attr, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, sd);

        status = FltCreateCommunicationPort(gFilterHandle, &FilterPort, &attr, nullptr, PortConnectNotify, PortDisconnectNotify,
                                            PortMessageNotify, 1);

        FltFreeSecurityDescriptor(sd);
        if (!NT_SUCCESS(status))
            break;

        status = FltStartFiltering(gFilterHandle);

    } while (false);


    if (!NT_SUCCESS(status))
    {
        FltUnregisterFilter(gFilterHandle);
    }
    return status;
}

NTSTATUS
PathHiderUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PathHider!PathHiderUnload: Entered\n"));
    FltCloseCommunicationPort(FilterPort);
    FltUnregisterFilter(gFilterHandle);

    ShutDown();
    return STATUS_SUCCESS;
}

NTSTATUS AddPathToHide(const KUtils::UnicodeString& Path, const KUtils::UnicodeString& Name)
{
    if (Path.IsEmpty() || Name.IsEmpty())
        return STATUS_INVALID_PARAMETER;

    KUtils::AutoLock<KUtils::FastMutex> guard(gFolderDataLock);
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
    KUtils::intrusive_ptr<FileList> fileEntry = new FileList();
    fileEntry->m_name = Name;
    fileEntry->m_next = KUtils::intrusive_ptr<FileList>(folderData->m_fileListHead);
    folderData->m_fileListHead = KUtils::intrusive_ptr<FileList>(fileEntry);

    return STATUS_SUCCESS;
}

NTSTATUS Init()
{
    gFolderDataLock.Init();
    RtlZeroMemory(&gFolderDataHead, sizeof(gFolderDataHead));
    InitializeListHead(&gFolderDataHead);
    return STATUS_SUCCESS;
}

void ShutDown()
{
    KUtils::AutoLock<KUtils::FastMutex> guard(gFolderDataLock);
    PLIST_ENTRY tempFolder = RemoveHeadList(&gFolderDataHead);
    while (&gFolderDataHead != tempFolder)
    {
        FolderData* curFolderData = reinterpret_cast<FolderData*>(tempFolder);
        delete curFolderData;
        tempFolder = RemoveHeadList(&gFolderDataHead);
    }
}
