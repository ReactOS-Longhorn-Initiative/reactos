/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcstombs
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include "../ntrtl.h"

size_t
__cdecl
wcstombs(
	_Out_writes_opt_(_MaxCount) char* const _Dest,
	_In_z_ wchar_t const* const _Source,
	_In_ size_t const _MaxCount)
{
	NTSTATUS status;
	ULONG size;

	size = (ULONG)wcslen(_Source) * sizeof(WCHAR);
	if (_Dest == NULL)
	{
		RtlUnicodeToMultiByteSize(&size, (wchar_t*)_Source, size);
		return size;
	}

	status = RtlUnicodeToMultiByteN(_Dest,
	                                (ULONG)_MaxCount,
	                                &size,
	                                (wchar_t*)_Source,
	                                size);
	if (!NT_SUCCESS(status))
		return -1;

	return size;
}
