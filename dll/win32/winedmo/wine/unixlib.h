/*
 * PROJECT:     ReactOS dwrite DLL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactOS emulation layer for WineDMO unixlib calls
 * COPYRIGHT:   Copyright 2026 Mikhail Tyukin <mishakeys20@gmail.com>
 */

#pragma once


#define __wine_init_unix_call() 0
#define dlclose(x) 0
#define dlsym() 0

// define external functions

typedef NTSTATUS (*unixlib_entry_t)( void *args );

// Defined in unix_demuxer.c
extern const unixlib_entry_t __wine_unix_call_funcs[];
extern const unixlib_entry_t __wine_unix_call_wow64_funcs[];

struct dispatch_callback_params
{
    UINT64 callback;
};

extern NTSTATUS _seek_callback( void *args, ULONG size );
extern NTSTATUS _read_callback( void *args, ULONG size );
extern BOOL LoadFFmpeg();

static inline int __reactos_call_unix_process_attach(PVOID args)
{
    return __wine_unix_call_funcs[0](args);
}

static inline int __reactos_call_unix_demuxer_check(PVOID args)
{
    return __wine_unix_call_funcs[1](args);
}

static inline int __reactos_call_unix_demuxer_create(PVOID args)
{
    return __wine_unix_call_funcs[2](args);
}

static inline int __reactos_call_unix_demuxer_destroy(PVOID args)
{
    return __wine_unix_call_funcs[3](args);
}

static inline int __reactos_call_unix_demuxer_read(PVOID args)
{
    return __wine_unix_call_funcs[4](args);
}

static inline int __reactos_call_unix_demuxer_seek(PVOID args)
{
    return __wine_unix_call_funcs[5](args);
}

static inline int __reactos_call_unix_demuxer_stream_lang(PVOID args)
{
    return __wine_unix_call_funcs[6](args);
}

static inline int __reactos_call_unix_demuxer_stream_name(PVOID args)
{
    return __wine_unix_call_funcs[7](args);
}

static inline int __reactos_call_unix_demuxer_stream_type(PVOID args)
{
    return __wine_unix_call_funcs[8](args);
}

// Forward wine unix call to real freetype library
#undef WINE_UNIX_CALL
#define WINE_UNIX_CALL(code,args) __reactos_call_ ## code(args)
#undef UNIX_CALL
#define UNIX_CALL( func, params ) WINE_UNIX_CALL( unix_##func, params )