#include "PathHider.h"

#include "FileNameInformation.h"
#include "UnicodeStringGuard.h"


/*************************************************************************
	MiniFilter callback routines.
*************************************************************************/

bool IsControlledProcess(const PEPROCESS Process)
{
	bool currentProcess = PsGetCurrentProcess() == Process;
	HANDLE hProcess;
	if (currentProcess)
		hProcess = NtCurrentProcess();
	else
	{
		auto status = ObOpenObjectByPointer(Process, OBJ_KERNEL_HANDLE,
			nullptr, 0, nullptr, KernelMode, &hProcess);
		if (!NT_SUCCESS(status))
			return true;
	}

	auto size = 300;
	bool controlledProcess = false;
	auto processName = (UNICODE_STRING*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);

	if (processName)
	{
		RtlZeroMemory(processName, size);	// ensure string will be NULL-terminated
		auto status = ZwQueryInformationProcess(hProcess, ProcessImageFileName,
			processName, size - sizeof(WCHAR), nullptr);

		if (NT_SUCCESS(status))
		{
			KdPrint(("The operation from %wZ\n", processName));

			if (processName->Length > 0 && wcsstr(processName->Buffer, L"\\Windows\\explorer.exe") != nullptr)
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

	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
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

bool FolderContainsFilesToHide(PFLT_CALLBACK_DATA Data)
{
	//TODO implement this method - should get the name of the file object from the call
	//and check if it is a parent path for one of the paths we need to hide
	bool retValue = false;

	FilterFileNameInformation info(Data);
	auto status = info.Parse();
	//TODO fix me
	if (!NT_SUCCESS(status))
	{
		return retValue;
	}
	KdPrint(("Folder path: %wZ\n", &info->Name));
	UNICODE_STRING folderToHide;
	RtlInitUnicodeString(&folderToHide, L"\\Device\\HarddiskVolume1\\test");
	if (RtlEqualUnicodeString(&folderToHide, &info->Name, TRUE))
	{
		retValue = true;
	}
	return retValue;
}

bool FileObjectToHide(PUNICODE_STRING Name)
{
	//UNREFERENCED_PARAMETER(Name);
	//TODO implement this method
	//bool retValue = false;
	//return retValue;

    //TODO fix me
	UNICODE_STRING fileToHide;
	RtlInitUnicodeString(&fileToHide, L"1.txt");

	return RtlEqualUnicodeString(&fileToHide, Name, TRUE);
}	

bool ShouldHandleRequest(PFLT_CALLBACK_DATA Data)
{
	bool retValue = (Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY) && IsRelevantFileInfoQuery(Data) && FolderContainsFilesToHide(Data);
	return retValue;
}

extern "C"
FLT_PREOP_CALLBACK_STATUS
PathHiderPreDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	//TODO remove this check after testing and bugfix is done - it is just temporary to reduce the number of calls 
	/*if ((Data->RequestorMode == KernelMode) || !IsControlledProcess(PsGetCurrentProcess()))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}*/

	if (ShouldHandleRequest(Data))
	{
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;

}

template<class T>
void HideFiles(PFLT_CALLBACK_DATA Data)
{
	T* fileDirInfo = nullptr;
	T* nextFileDirInfo = nullptr;
	UNICODE_STRING fileName;
	ULONG moveLength;
	T* lastFileDirInfo = nullptr;
	fileDirInfo = (T*)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
	for (;;)
	{
		//
		// Create a unicode string from file name so we can use FsRtl
		//
		fileName.Buffer = fileDirInfo->FileName;
		fileName.Length = (USHORT)fileDirInfo->FileNameLength;
		fileName.MaximumLength = fileName.Length;

		//
		// Check if this is a match on our hide file name
		//
		if (FileObjectToHide(&fileName))
		{
			//
			// Skip this entry
			//
			if (lastFileDirInfo != NULL)
			{
				//
				// This is not the first entry
				//
				if (fileDirInfo->NextEntryOffset != 0)
				{
					//
					// Just point the last info's offset to the next info
					//
					lastFileDirInfo->NextEntryOffset += fileDirInfo->NextEntryOffset;
				}
				else
				{
					//
					// This is the last entry
					//
					lastFileDirInfo->NextEntryOffset = 0;
				}
			}
			else
			{
				//
				// This is the first entry
				//
				if (fileDirInfo->NextEntryOffset != 0)
				{
					//
					// Calculate the length of the whole list
					//
					nextFileDirInfo = (T*)((PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset);
					moveLength = 0;
					while (nextFileDirInfo->NextEntryOffset != 0)
					{
						//
						// We use the FIELD_OFFSET macro because FileName is declared as FileName[1] which means that
						// we can't just do sizeof(FILE_DIRECTORY_INFORMATION) + nextFileDirInfo->FileNameLength.
						//
						moveLength += FIELD_OFFSET(T, FileName) + nextFileDirInfo->FileNameLength;
						nextFileDirInfo = (T*)((PUCHAR)nextFileDirInfo + nextFileDirInfo->NextEntryOffset);
					}

					//
					// Add the final entry
					//
					moveLength += FIELD_OFFSET(T, FileName) + nextFileDirInfo->FileNameLength;

					//
					// We need to move everything forward.
					// NOTE: RtlMoveMemory (memove) is required for overlapping ranges like this one.
					//
					RtlMoveMemory(
						fileDirInfo,
						(PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset,
						moveLength);
				}
				else
				{
					//
					// This is the first and last entry, so there's nothing to return
					//
					Data->IoStatus.Status = STATUS_NO_MORE_ENTRIES;
					return;
				}
			}
		}

		//
		// Advance to the next directory info
		//
		lastFileDirInfo = fileDirInfo;
		fileDirInfo = (T*)((PUCHAR)fileDirInfo + fileDirInfo->NextEntryOffset);
		if (lastFileDirInfo == fileDirInfo)
		{
			break;
		}
	}
}

void ProcessFileInfoQuery(PFLT_CALLBACK_DATA Data)
{
	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
	{
	case FileDirectoryInformation:
		HideFiles<FILE_DIRECTORY_INFORMATION>(Data);
		break;
	case FileFullDirectoryInformation:
		HideFiles<FILE_FULL_DIR_INFORMATION>(Data);
		break;
	case FileNamesInformation:
		HideFiles<FILE_NAMES_INFORMATION>(Data);
		break;
	case FileBothDirectoryInformation:
		HideFiles<FILE_BOTH_DIR_INFORMATION>(Data);
		break;
	case FileIdBothDirectoryInformation:
		HideFiles<FILE_ID_BOTH_DIR_INFORMATION>(Data);
		break;
	case FileIdFullDirectoryInformation:
		HideFiles<FILE_ID_FULL_DIR_INFORMATION>(Data);
		break;
	default:
		NT_ASSERT(FALSE);
	}
}

extern "C"
FLT_POSTOP_CALLBACK_STATUS
PathHiderPostDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(FltObjects);
	if (NT_SUCCESS(Data->IoStatus.Status))
	{
		ProcessFileInfoQuery(Data);
	}

	return FLT_POSTOP_FINISHED_PROCESSING;

}