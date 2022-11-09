/*++

Module Name:

	PathHider.c

Abstract:

	This is the main module of the PathHider miniFilter driver.

Environment:

	Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include "PathHider.h"
#include "UnicodeStringGuard.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")



PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

/*************************************************************************
	Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);


NTSTATUS
PathHiderUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
PathHiderPreDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PathHiderPostDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PathHiderUnload)
#pragma alloc_text(PAGE, PathHiderPreDirectoryControl)
#pragma alloc_text(PAGE, PathHiderPostDirectoryControl)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] =
{
	{ IRP_MJ_DIRECTORY_CONTROL, 0, PathHiderPreDirectoryControl, PathHiderPostDirectoryControl },
	{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration =
{

	sizeof(FLT_REGISTRATION),           //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                          //  Operation callbacks

	PathHiderUnload,                    //  MiniFilterUnload

	NULL,                               //  InstanceSetup
	NULL,                               //  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};

/*************************************************************************
	MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("PathHider!DriverEntry: Entered\n"));
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

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("PathHider!PathHiderUnload: Entered\n"));

	FltUnregisterFilter(gFilterHandle);

	return STATUS_SUCCESS;
}



