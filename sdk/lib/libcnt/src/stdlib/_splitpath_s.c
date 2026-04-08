/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _splitpath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

static void
_clear_splitpath_outputs(
    char* const _Drive,
    size_t const _DriveCount,
    char* const _Dir,
    size_t const _DirCount,
    char* const _Filename,
    size_t const _FilenameCount,
    char* const _Ext,
    size_t const _ExtCount)
{
    if ((_Drive != NULL) && (_DriveCount > 0)) _Drive[0] = '\0';
    if ((_Dir != NULL) && (_DirCount > 0)) _Dir[0] = '\0';
    if ((_Filename != NULL) && (_FilenameCount > 0)) _Filename[0] = '\0';
    if ((_Ext != NULL) && (_ExtCount > 0)) _Ext[0] = '\0';
}

_Check_return_wat_
errno_t
__cdecl
_splitpath_s(
    _In_z_ const char* const _FullPath,
    _Out_writes_opt_z_(_DriveCount) char* const _Drive,
    _In_ size_t const _DriveCount,
    _Out_writes_opt_z_(_DirCount) char* const _Dir,
    _In_ size_t const _DirCount,
    _Out_writes_opt_z_(_FilenameCount) char* const _Filename,
    _In_ size_t const _FilenameCount,
    _Out_writes_opt_z_(_ExtCount) char* const _Ext,
    _In_ size_t const _ExtCount)
{
    const char* path;
    const char* dir_start;
    const char* fname_start;
    const char* ext_start;
    const char* p;
    size_t len;

    /* Validate that buffer/count pairs are consistent */
    if ((_FullPath == NULL) ||
        ((_Drive == NULL) != (_DriveCount == 0)) ||
        ((_Dir == NULL) != (_DirCount == 0)) ||
        ((_Filename == NULL) != (_FilenameCount == 0)) ||
        ((_Ext == NULL) != (_ExtCount == 0)))
    {
        _clear_splitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                 _Filename, _FilenameCount, _Ext, _ExtCount);
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    path = _FullPath;

    /* Extract drive */
    if ((path[0] != '\0') && (path[1] == ':'))
    {
        if (_Drive != NULL)
        {
            if (_DriveCount < 3)
            {
                _clear_splitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                         _Filename, _FilenameCount, _Ext, _ExtCount);
                _invalid_parameter_noinfo();
                return ERANGE;
            }
            _Drive[0] = path[0];
            _Drive[1] = ':';
            _Drive[2] = '\0';
        }
        path += 2;
    }
    else
    {
        if (_Drive != NULL)
            _Drive[0] = '\0';
    }

    dir_start = path;

    /* Find last separator */
    fname_start = path;
    for (p = path; *p != '\0'; p++)
    {
        if ((*p == '\\') || (*p == '/'))
            fname_start = p + 1;
    }

    /* Copy directory */
    if (_Dir != NULL)
    {
        len = (size_t)(fname_start - dir_start);
        if (_DirCount <= len)
        {
            _clear_splitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                     _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Dir, dir_start, len);
        _Dir[len] = '\0';
    }

    /* Find the last dot in the filename portion */
    ext_start = NULL;
    for (p = fname_start; *p != '\0'; p++)
    {
        if (*p == '.')
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
            _clear_splitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                     _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Filename, fname_start, len);
        _Filename[len] = '\0';
    }

    /* Copy extension */
    if (_Ext != NULL)
    {
        len = strlen(ext_start);
        if (_ExtCount <= len)
        {
            _clear_splitpath_outputs(_Drive, _DriveCount, _Dir, _DirCount,
                                     _Filename, _FilenameCount, _Ext, _ExtCount);
            _invalid_parameter_noinfo();
            return ERANGE;
        }
        memcpy(_Ext, ext_start, len + 1);
    }

    return 0;
}
