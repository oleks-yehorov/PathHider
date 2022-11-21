#include <fltKernel.h>

#include "PathHider.h"
#include "FileNameInformation.h"
#include "UnicodeStringGuard.h"

extern LIST_ENTRY gFolderDataHead;

FolderData* GetFolderDataByFolderPath(PUNICODE_STRING FolderPath)
{
    FolderData* retVal = nullptr;
    PLIST_ENTRY temp = &gFolderDataHead;
    while (&gFolderDataHead != temp->Flink)
    {
        temp = temp->Flink;
        auto curFolderData = CONTAINING_RECORD(temp, FolderData, m_listEntry);
        if (RtlEqualUnicodeString(&curFolderData->m_path, FolderPath, TRUE))
        {
            retVal = curFolderData;
            break;
        }
    }
    return retVal;
}

bool FolderContainsFilesToHide(PFLT_CALLBACK_DATA Data,
                               PCFLT_RELATED_OBJECTS FltObjects)
{
    FilterFileNameInformation info(Data);
    if (!info)
    {
        return false;
    }
    auto status = info.Parse();
    if (!NT_SUCCESS(status))
    {
        // TODO - error message here
        return false;
    }
    KdPrint(("Folder path: %wZ\n", &info->Name));
    auto folderData = GetFolderDataByFolderPath(&info->Name);
    FolderContext* context;
    if (folderData)
    {
        status = FltAllocateContext(FltObjects->Filter, FLT_FILE_CONTEXT,
                                    sizeof(FolderContext), PagedPool,
                                    reinterpret_cast<PFLT_CONTEXT*>(&context));
        if (!NT_SUCCESS(status))
        {
            // TODO - error message here
            return false;
        }
        // RtlZeroMemory(context, sizeof(FolderContext));
        context->m_fileListHead = &folderData->m_fileListHead;
        status =
            FltSetFileContext(FltObjects->Instance, FltObjects->FileObject,
                              FLT_SET_CONTEXT_KEEP_IF_EXISTS, context,
                              nullptr);
        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
        {
            status = STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS(status))
        {
            FltReleaseContext(context);
            return false;
        }
        FltReleaseContext(context);
    }
    return folderData;
}


extern "C" FLT_POSTOP_CALLBACK_STATUS
PathHiderPostCreate(PFLT_CALLBACK_DATA Data,
                    PCFLT_RELATED_OBJECTS FltObjects,
                    PVOID CompletionContext,
                    FLT_POST_OPERATION_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(CompletionContext);

    if (Flags & FLTFL_POST_OPERATION_DRAINING)
        return FLT_POSTOP_FINISHED_PROCESSING;

    // TODO remove this check after testing and bugfix is done - it is just
    // temporary to reduce the number of calls
    /*if ((Data->RequestorMode == KernelMode) ||
    !IsControlledProcess(PsGetCurrentProcess()))
    {
            return FLT_POSTOP_FINISHED_PROCESSING;
    }*/
    if (Data->IoStatus.Information == FILE_DOES_NOT_EXIST)
    {
        //a new file - skip
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    FolderContainsFilesToHide(Data, FltObjects);

    return FLT_POSTOP_FINISHED_PROCESSING;
}

extern "C" FLT_POSTOP_CALLBACK_STATUS
PathHiderPostCleanup(_Inout_ PFLT_CALLBACK_DATA Data,
                     _In_ PCFLT_RELATED_OBJECTS FltObjects,
                     _In_ PVOID CompletionContext,
                     _In_ FLT_POST_OPERATION_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (Flags & FLTFL_POST_OPERATION_DRAINING)
        return FLT_POSTOP_FINISHED_PROCESSING;
    FolderContext* context = nullptr;
    auto status = FltGetFileContext(FltObjects->Instance, FltObjects->FileObject,
                               reinterpret_cast<PFLT_CONTEXT*>(&context));
    if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to get file context (0x%08X)\n", status));
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    FltReleaseContext(context);
    FltDeleteContext(context);
    return FLT_POSTOP_FINISHED_PROCESSING;
}