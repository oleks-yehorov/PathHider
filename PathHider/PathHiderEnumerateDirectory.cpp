#include "PathHider.h"

#include "FileNameInformation.h"

bool IsControlledProcess(const PEPROCESS Process)
{
    bool currentProcess = PsGetCurrentProcess() == Process;
    HANDLE hProcess;
    if (currentProcess)
        hProcess = NtCurrentProcess();
    else
    {
        auto status = ObOpenObjectByPointer(Process, OBJ_KERNEL_HANDLE, nullptr,
                                            0, nullptr, KernelMode, &hProcess);
        if (!NT_SUCCESS(status))
            return true;
    }

    auto size = 300;
    bool controlledProcess = false;
    auto processName =
        (UNICODE_STRING*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);

    if (processName)
    {
        RtlZeroMemory(processName,
                      size); // ensure string will be NULL-terminated
        auto status = ZwQueryInformationProcess(hProcess, ProcessImageFileName,
                                                processName,
                                                size - sizeof(WCHAR), nullptr);

        if (NT_SUCCESS(status))
        {
            KdPrint(("The operation from %wZ\n", processName));

            if (processName->Length > 0 &&
                wcsstr(processName->Buffer, L"\\Windows\\explorer.exe") !=
                    nullptr)
            {
                controlledProcess = true;
            }
        }
        ExFreePool(processName);
    }
    if (!currentProcess)
        ZwClose(hProcess);

    return controlledProcess;
}

bool IsRelevantFileInfoQuery(PFLT_CALLBACK_DATA Data)
{
    bool relevantQuery = false;

    switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory
                .FileInformationClass)
    {
    case FileIdFullDirectoryInformation:
    case FileIdBothDirectoryInformation:
    case FileBothDirectoryInformation:
    case FileDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileNamesInformation:
        relevantQuery = true;
    }

    return relevantQuery;
}

bool FileObjectToHide(PFLT_CALLBACK_DATA Data,
                      PUNICODE_STRING Name, KUtils::intrusive_ptr<FileList>& FolderDataHead)
{
    KUtils::FilterFileNameInformation info(Data);
    auto status = info.Parse();
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to parse file name info (0x%08X)\n", status));
        return false;
    }
    KUtils::intrusive_ptr<FileList> temp = KUtils::intrusive_ptr<FileList>(FolderDataHead);
    while (temp != nullptr)
    {
        if (RtlEqualUnicodeString(&temp->m_name.GetUnicodeString(), Name, TRUE))
        {
            return true;
        }
        temp = KUtils::intrusive_ptr<FileList>(temp->m_next);
    }
    return false;
}

bool ContextIsPresent(PCFLT_RELATED_OBJECTS FltObjects)
{
    FolderContext* context = nullptr;
    auto status =
        FltGetFileContext(FltObjects->Instance, FltObjects->FileObject,
                          reinterpret_cast<PFLT_CONTEXT*>(&context));
    if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
    {
        return false;
    }
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to get file context (0x%08X)\n", status));
        return false;
    }
    FltReleaseContext(context);
    return true;
}
bool ShouldHandleRequest(PFLT_CALLBACK_DATA Data,
                         PCFLT_RELATED_OBJECTS FltObjects)
{
    
    bool retValue = (Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY) &&
                    IsRelevantFileInfoQuery(Data) && ContextIsPresent(FltObjects);
    return retValue;
}

template <class T> 
bool AddressIsValid(const void* ValidBufferStart, const void* ValidBufferEnd, T* AddressToCheck) 
{    
    return (reinterpret_cast<const void*>(AddressToCheck) >= ValidBufferStart &&
                   reinterpret_cast<const void*>(AddressToCheck) <= reinterpret_cast<const char*>(ValidBufferEnd) - sizeof(T));
}

template <class T>
void HideFiles(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects)
{
    T* fileDirInfo = nullptr;
    T* nextFileDirInfo = nullptr;
    UNICODE_STRING fileName;
    ULONG moveLength;
    T* lastFileDirInfo = nullptr;
    bool moveLastFileDirInfo = true;
    if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length <= 0)
    {
        KdPrint(("Buffer size is <= 0 \n"));
        return;
    }
    if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
    {
        fileDirInfo = static_cast<T*>(MmGetSystemAddressForMdlSafe(Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority));
    }
    else
    {
        fileDirInfo = static_cast<T*>(Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer);
    }
    if (!fileDirInfo)
    {
        KdPrint(("Null buffer \n"));
        return;
    }
    const void* ValidBufferStart = fileDirInfo;
    const void* ValidBufferEnd = reinterpret_cast<const char*>(ValidBufferStart) + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length;
    
    FolderContext* context = nullptr;
    auto status = FltGetFileContext(FltObjects->Instance, FltObjects->FileObject,
                               reinterpret_cast<PFLT_CONTEXT*>(&context));
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to get file context (0x%08X)\n", status));
        return;
    }
    BOOLEAN singleEntry = BooleanFlagOn(Data->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY);
    if (singleEntry)
    {
        __try
        {
            ProbeForWrite(fileDirInfo, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length, 1);
            if (!AddressIsValid<T>(ValidBufferStart, ValidBufferEnd, fileDirInfo))
                __leave;
            fileName.Buffer = fileDirInfo->FileName;
            fileName.Length = (USHORT)fileDirInfo->FileNameLength;
            fileName.MaximumLength = fileName.Length;
            while (FileObjectToHide(Data, &fileName,
                                    context->m_fileListHead)) // Skip this entry
            {
                ULONG retLength;

                status = FltQueryDirectoryFile(FltObjects->Instance, FltObjects->FileObject, fileDirInfo,
                                               Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                               Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass, 
                                               TRUE, 
                                               Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName, 
                                               FALSE, 
                                               &retLength);
                FltIsCallbackDataDirty(Data);
                if (!NT_SUCCESS(status))
                {
                    Data->IoStatus.Status = status;
                    break;
                }

                fileName.Buffer = fileDirInfo->FileName;
                fileName.Length = (USHORT)fileDirInfo->FileNameLength;
                fileName.MaximumLength = fileName.Length;
            }

        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            KdPrint(("An exception occurred at " __FUNCTION__ " status = (0x%08X)\n", status));
        }
    }
    else
    {
        __try
        {
            // Never Trust Buffers
            ProbeForWrite(fileDirInfo, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length, 1);
            for (;;)
            {
                if (!AddressIsValid<T>(ValidBufferStart, ValidBufferEnd, fileDirInfo))
                    __leave;
                fileName.Buffer = fileDirInfo->FileName;
                fileName.Length = (USHORT)fileDirInfo->FileNameLength;
                fileName.MaximumLength = fileName.Length;

                if (FileObjectToHide(Data, &fileName,
                                     context->m_fileListHead)) // Skip this entry
                {
                    if (lastFileDirInfo != NULL) // This is not the first entry
                    {
                        // Just point the last info's offset to the next info
                        if (fileDirInfo->NextEntryOffset != 0)
                        {
                            lastFileDirInfo->NextEntryOffset += fileDirInfo->NextEntryOffset;
                            moveLastFileDirInfo = false;
                        }
                        else // This is the last entry
                        {
                            lastFileDirInfo->NextEntryOffset = 0;
                        }
                    }
                    else
                    {
                        if (fileDirInfo->NextEntryOffset != 0) // This is the first
                                                               // entry
                        {
                            nextFileDirInfo = (T*)((PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset);
                            if (!AddressIsValid<T>(ValidBufferStart, ValidBufferEnd, nextFileDirInfo))
                                __leave;
                            moveLength = 0;
                            while (nextFileDirInfo->NextEntryOffset != 0)
                            {
                                moveLength += FIELD_OFFSET(T, FileName) + nextFileDirInfo->FileNameLength;
                                nextFileDirInfo = (T*)((PUCHAR)nextFileDirInfo + nextFileDirInfo->NextEntryOffset);
                                if (!AddressIsValid<T>(ValidBufferStart, ValidBufferEnd, nextFileDirInfo))
                                    __leave;
                            }

                            moveLength += FIELD_OFFSET(T, FileName) + nextFileDirInfo->FileNameLength;

                            RtlMoveMemory(fileDirInfo, (PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset, moveLength);
                            moveLastFileDirInfo = false;
                        }
                        else // This is the first and last entry, so there's nothing to
                             // return
                        {
                            FltReleaseContext(context);
                            Data->IoStatus.Status = STATUS_NO_MORE_ENTRIES;
                            return;
                        }
                    }
                }

                if (moveLastFileDirInfo)
                {
                    lastFileDirInfo = fileDirInfo;
                }
                else
                {
                    moveLastFileDirInfo = true;
                }
                fileDirInfo = (T*)((PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset);
                if (lastFileDirInfo == fileDirInfo)
                {
                    break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            KdPrint(("An exception occurred at " __FUNCTION__ " status = (0x%08X)\n", status));
        }

    }
    if (context)
        FltReleaseContext(context);
}

void ProcessFileInfoQuery(PFLT_CALLBACK_DATA Data,
                          PCFLT_RELATED_OBJECTS FltObjects)
{
    switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory
                .FileInformationClass)
    {
    case FileDirectoryInformation:
        HideFiles<FILE_DIRECTORY_INFORMATION>(Data, FltObjects);
        break;
    case FileFullDirectoryInformation:
        HideFiles<FILE_FULL_DIR_INFORMATION>(Data, FltObjects);
        break;
    case FileNamesInformation:
        HideFiles<FILE_NAMES_INFORMATION>(Data, FltObjects);
        break;
    case FileBothDirectoryInformation:
        HideFiles<FILE_BOTH_DIR_INFORMATION>(Data, FltObjects);
        break;
    case FileIdBothDirectoryInformation:
        HideFiles<FILE_ID_BOTH_DIR_INFORMATION>(Data, FltObjects);
        break;
    case FileIdFullDirectoryInformation:
        HideFiles<FILE_ID_FULL_DIR_INFORMATION>(Data, FltObjects);
        break;
    default:
        NT_ASSERT(FALSE);
    }
}

/*************************************************************************
        MiniFilter callback routines.
*************************************************************************/

extern "C" FLT_PREOP_CALLBACK_STATUS PathHiderPreDirectoryControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    
    if (ShouldHandleRequest(Data, FltObjects))
    {
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


extern "C" FLT_POSTOP_CALLBACK_STATUS
PathHiderPostDirectoryControl(_Inout_ PFLT_CALLBACK_DATA Data,
                              _In_ PCFLT_RELATED_OBJECTS FltObjects,
                              _In_ PVOID CompletionContext,
                              _In_ FLT_POST_OPERATION_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (NT_SUCCESS(Data->IoStatus.Status))
    {
        ProcessFileInfoQuery(Data, FltObjects);
    }
    return FLT_POSTOP_FINISHED_PROCESSING;
}