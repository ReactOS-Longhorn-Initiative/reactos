/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wsplitpath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

static void
_clear_wsplitpath_outputs(
    wchar_t* const _Drive,
    size_t const _DriveCount,
    wchar_t* const _Dir,
    size_t const _DirCount,
    wchar_t* const _Filename,
    size_t const _FilenameCount,
    wchar_t* const _Ext,
    size_t const _ExtCount)
{
    if ((_Drive != NULL) && (_DriveCount > 0)) _Drive[0] = L'\0';
    if ((_Dir != NULL) && (_DirCount > 0)) _Dir[0] = L'\0';
    if ((_Filename != NULL) && (_FilenameCount > 0)) _Filename[0] = L'\0';
    if ((_Ext != NULL) && (_ExtCount > 0)) _Ext[0] = L'\0';
}

_Check_return_wat_
errno_t
__cdecl
_wsplitpath_s(
    _In_z_ const wchar_t* const _FullPath,
    _Out_writes_opt_z_(_DriveCount) wchar_t* const _Drive,
    _In_ size_t const _DriveCount,
    _Out_writes_opt_z_(_DirCount) wchar_t* const _Dir,
    _In_ size_t const _DirCount,
    _Out_writes_opt_z_(_FilenameCount) wchar_t* const _Filename,
    _In_ size_t const _FilenameCount,
    _Out_writes_opt_z_(_ExtCount) wchar_t* const _Ext,
    _In_ size_t const _ExtCount)
{
    const wchar_t* path;
    const wchar_t* dir_start;
    const wchar_t* fname_start;
    const wchar_t* ext_start;
    const wchar_t* p;
    size_t len;

    /* Validate that buffer/count pairs are consistent */
    if ((_FullPath == NULL) ||
        ((_Drive == NULL) != (_DriveCount == 0)) ||
        ((_Dir == NULL) != (_DirCount == 0)) ||
        ((_Filename == NULL) != (_FilenameCount == 0)) ||
        ((_Ext == NULL) != (_ExtCount == 0)))
    {
        _clear_wsplitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                  _Filename, _FilenameCount, _Ext, _ExtCount);
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    path = _FullPath;

    /* Extract drive */
    if ((path[0] != L'\0') && (path[1] == L':'))
    {
        if (_Drive != NULL)
        {
            if (_DriveCount < 3)
            {
                _clear_wsplitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                          _Filename, _FilenameCount, _Ext, _ExtCount);
                _invalid_parameter_noinfo();
                return ERANGE;
            }
            _Drive[0] = path[0];
            _Drive[1] = L':';
            _Drive[2] = L'\0';
        }
        path += 2;
    }
    else
    {
        if (_Drive != NULL)
            _Drive[0] = L'\0';
    }

    dir_start = path;

    /* Find last separator */
    fname_start = path;
    for (p = path; *p != L'\0'; p++)
    {
        if ((*p == L'\\') || (*p == L'/'))
            fname_start = p + 1;
    }

    /* Copy directory */
    if (_Dir != NULL)
    {
        len = (size_t)(fname_start - dir_start);
        if (_DirCount <= len)
        {
            _clear_wsplitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                      _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Dir, dir_start, len * sizeof(wchar_t));
        _Dir[len] = L'\0';
    }

    /* Find the last dot in the filename portion */
    ext_start = NULL;
    for (p = fname_start; *p != L'\0'; p++)
    {
        if (*p == L'.')
            ext_start = p;
    }

    if (ext_start == NULL)
        ext_start = p;

    /* Copy filename */
    if (_Filename != NULL)
    {
        len = (size_t)(ext_start - fname_start);
        if (_FilenameCount <= len)
        {
            _clear_wsplitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                      _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Filename, fname_start, len * sizeof(wchar_t));
        _Filename[len] = L'\0';
    }

    /* Copy extension */
    if (_Ext != NULL)
    {
        len = wcslen(ext_start);
        if (_ExtCount <= len)
        {
            _clear_wsplitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                      _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Ext, ext_start, (len + 1) * sizeof(wchar_t));
    }

    return 0;
}
