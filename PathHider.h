#ifndef __PATH_HIDER_H__
#define __PATH_HIDER_H__

#include <fltkernel.h>

#include "Constants.h"

extern "C" NTSTATUS ZwQueryInformationProcess(
	_In_      HANDLE           ProcessHandle,
	_In_      PROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
);

#endif //__PATH_HIDER_H__