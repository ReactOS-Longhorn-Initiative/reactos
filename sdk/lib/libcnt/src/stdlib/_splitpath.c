/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _splitpath
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <string.h>

void
__cdecl
_splitpath(
    _In_z_ const char* const _FullPath,
    _Pre_maybenull_ _Post_z_ char* const _Drive,
    _Pre_maybenull_ _Post_z_ char* const _Dir,
    _Pre_maybenull_ _Post_z_ char* const _Filename,
    _Pre_maybenull_ _Post_z_ char* const _Ext)
{
    const char* path = _FullPath;
    const char* dir_start;
    const char* fname_start;
    const char* ext_start;
    const char* p;

    /* Extract drive */
    if ((path[0] != '\0') && (path[1] == ':'))
    {
        if (_Drive)
        {
            _Drive[0] = path[0];
            _Drive[1] = ':';
            _Drive[2] = '\0';
        }
        path += 2;
    }
    else
    {
        if (_Drive)
            _Drive[0] = '\0';
    }

    dir_start = path;

    /* Find the last separator to determine end of directory */
    fname_start = path;
    for (p = path; *p != '\0'; p++)
    {
        if ((*p == '\\') || (*p == '/'))
            fname_start = p + 1;
    }

    /* Copy directory */
    if (_Dir)
    {
        size_t dir_len = (size_t)(fname_start - dir_start);
        memcpy(_Dir, dir_start, dir_len);
        _Dir[dir_len] = '\0';
    }

    /* Find the last dot in the filename portion for the extension */
    ext_start = NULL;
    for (p = fname_start; *p != '\0'; p++)
    {
        if (*p == '.')
            ext_start = p;
    }

    if (ext_start == NULL)
        ext_start = p; /* point to the null terminator */

    /* Copy filename */
    if (_Filename)
    {
        size_t fname_len = (size_t)(ext_start - fname_start);
        memcpy(_Filename, fname_start, fname_len);
        _Filename[fname_len] = '\0';
    }

    /* Copy extension */
    if (_Ext)
    {
        size_t ext_len = strlen(ext_start);
        memcpy(_Ext, ext_start, ext_len + 1);
    }
}
