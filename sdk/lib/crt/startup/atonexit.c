/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _DLL
#define _DLL
#endif

#include <oscalls.h>
#include <internal.h>
#include <stdlib.h>
#include <crtdefs.h>
#include <limits.h>
//#include <windows.h>
#include <debug.h>
#define _EXIT_LOCK1 8

  void __cdecl _lock (int _File);
  void __cdecl _unlock (int _File);

_PVFV *__onexitbegin;
_PVFV *__onexitend;

#define LOCK_EXIT   _lock(_EXIT_LOCK1)
#define UNLOCK_EXIT _unlock(_EXIT_LOCK1)

extern _onexit_t __cdecl __dllonexit (_onexit_t, _PVFV**, _PVFV**);
extern _onexit_t (__cdecl * __MINGW_IMP_SYMBOL(_onexit)) (_onexit_t func);

/* INTERNAL: call atexit functions */
void __call_atexit(void)
{
    /* Note: should only be called with the exit lock held */
    _PVFV *first, *last;

    if (!__onexitbegin)
        return;

    first =  (_PVFV *)_decode_pointer(__onexitbegin);
    last = (_PVFV *)_decode_pointer(__onexitend);

    while (--last >= first)
        if (*last)
            (**last)();

    free(first);

    __onexitbegin = __onexitend = NULL;
}

/* Choose a different name to prevent name conflicts. The CRT one works fine.  */
_onexit_t __cdecl _onexit(_onexit_t func);

_onexit_t __cdecl _onexit(_onexit_t func)
{
    _PVFV *onexitbegin;
    _PVFV *onexitend;
    _onexit_t retval;

#ifndef CRTDLL
    if (__onexitbegin == (_PVFV *) -1)
        return (* __MINGW_IMP_SYMBOL(_onexit)) (func);
#endif

    _lock (_EXIT_LOCK1);

    if (!__onexitbegin)
    {
        /* First time we are called. Initialize our array */
        onexitbegin = calloc(1, sizeof(*onexitbegin));
        if (!onexitbegin)
        {
            _unlock(_EXIT_LOCK1);
            return NULL;
        }
        onexitend = onexitbegin;
    }
    else
    {
        onexitbegin = (_PVFV *) _decode_pointer (__onexitbegin);
        onexitend = (_PVFV *) _decode_pointer (__onexitend);
    }

    retval = __dllonexit (func, &onexitbegin, &onexitend);

    if (retval != NULL)
    {
        /* Update our globals in case of success */
        __onexitbegin = (_PVFV *) _encode_pointer (onexitbegin);
        __onexitend = (_PVFV *) _encode_pointer (onexitend);
    }

    _unlock (_EXIT_LOCK1);
    return retval;
}

int __cdecl
atexit (_PVFV func)
{
  return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}

typedef struct _onexit_table_t {
    _PVFV *_first;
    _PVFV *_last;
    _PVFV *_end;
} _onexit_table_t;


static CRITICAL_SECTION MSVCRT_onexit_cs;
static _onexit_table_t MSVCRT_atexit_table;

int CDECL _register_onexit_function(_onexit_table_t *table, _onexit_t func)
{
 //   DPRINT1("Calling stub _register_onexit_function: This will probably fail\n");
    if (!table)
         return -1;

     EnterCriticalSection(&MSVCRT_onexit_cs);
     if (!table->_first)
     {
         table->_first = calloc(32, sizeof(void *));
         if (!table->_first)
         {
            // WARN("failed to allocate initial table.\n");
             LeaveCriticalSection(&MSVCRT_onexit_cs);
             return -1;
         }
         table->_last = table->_first;
         table->_end = table->_first + 32;
     }

     /* grow if full */
     if (table->_last == table->_end)
     {
         int len = table->_end - table->_first;
         _PVFV *tmp = realloc(table->_first, 2 * len * sizeof(void *));
         if (!tmp)
         {
             //WARN("failed to grow table.\n");
             LeaveCriticalSection(&MSVCRT_onexit_cs);
             return -1;
         }
         table->_first = tmp;
         table->_end = table->_first + 2 * len;
         table->_last = table->_first + len;
     }

     *table->_last = (_PVFV)func;

    return 0;//return register_onexit_function(table, func);
}

 _onexit_t CDECL _Loconexit(_onexit_t func)
 {


   if (!func)
     return NULL;

   LOCK_EXIT;
   _register_onexit_function(&MSVCRT_atexit_table, func);
   UNLOCK_EXIT;

   return func;
 }

int CDECL _crt_atexit(void (__cdecl *func)(void))
{
     //DPRINT1("Calling stub _crt_atexit: This will probably fail\n");
    return atexit(func);
}

/*********************************************************************
 *      _initialize_onexit_table (UCRTBASE.@)
 */
int CDECL _initialize_onexit_table(_onexit_table_t *table)
{
    if (MSVCRT_onexit_cs.SpinCount == 0)
    {
        InitializeCriticalSection(&MSVCRT_onexit_cs);
    }
    //DPRINT1("Calling stub _initialize_onexit_table: This will probably fail\n");
    if (!table)
         return -1;

     if (table->_first == table->_end)
         table->_last = table->_end = table->_first = NULL;
     return 0;
}

int CDECL _execute_onexit_table(_onexit_table_t *table)
{
    //DPRINT1("Calling stub _execute_onexit_table: This will probably fail\n");
    _onexit_table_t copy;
    _PVFV *func;

    if (!table)
        return -1;

    EnterCriticalSection(&MSVCRT_onexit_cs);
    if (!table->_first || table->_first >= table->_last)
    {
        LeaveCriticalSection(&MSVCRT_onexit_cs);
        return 0;
    }
    copy._first = table->_first;
    copy._last  = table->_last;
    copy._end   = table->_end;

    memset(table, 0, sizeof(*table));
    _initialize_onexit_table(table);
    LeaveCriticalSection(&MSVCRT_onexit_cs);

    for (func = copy._last - 1; func >= copy._first; func--)
    {
        if (*func)
           (*func)();
    }

    free(copy._first);
    return 0;
}
