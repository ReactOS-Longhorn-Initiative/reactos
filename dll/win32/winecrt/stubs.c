
#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecrt);

#undef UNIMPLEMENTED
#define UNIMPLEMENTED __wine_spec_unimplemented_stub("winecrt.dll", __FUNCTION__)

char **MSVCRT___initenv = NULL;
wchar_t **MSVCRT___winitenv = NULL;
/* LOCALES ************************/


/* STRINGS ****************************/
#define LOCK_EXIT   _mlock(_EXIT_LOCK1)
#define UNLOCK_EXIT _munlock(_EXIT_LOCK1)
/* implemented? */
unsigned short
CDECL
wctype(const char *property)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum", _DIGIT|_ALPHA },
        { "alpha", _ALPHA },
        { "cntrl", _CONTROL },
        { "digit", _DIGIT },
        { "graph", _DIGIT|_PUNCT|_ALPHA },
        { "lower", _LOWER },
        { "print", _DIGIT|_PUNCT|_BLANK|_ALPHA },
        { "punct", _PUNCT },
        { "space", _SPACE },
        { "upper", _UPPER },
        { "xdigit", _HEX }
    };
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(properties); i++)
        if(!strcmp(property, properties[i].name))
            return properties[i].mask;

    return 0;
}

/* file */
int CDECL _get_stream_buffer_pointers(FILE *file, char*** base,
                                             char*** ptr, int** count)
{
    if (base)
        *base = &file->_base;
    if (ptr)
        *ptr = &file->_ptr;
    if (count)
        *count = &file->_cnt;
    return 0;
}


/*********************************************************************
 *      _get_initial_narrow_environment (UCRTBASE.@)
 */
char** CDECL _get_initial_narrow_environment(void)
{
  return MSVCRT___initenv;
}
/*********************************************************************
 *      _configure_narrow_argv (UCRTBASE.@)
 */
int CDECL _configure_narrow_argv(int mode)
{
  ERR("Calling stub _configure_narrow_argv: This will probably fail(%d)\n", mode);

//     __debugbreak();
  return 0;
}
/*********************************************************************
 *      _initialize_narrow_environment (UCRTBASE.@)
 */
int CDECL _initialize_narrow_environment(void)
{
  ERR("Calling stub _initialize_narrow_environment: This will probably fail\n");
     // __debugbreak();
  return 0;
}
/*********************************************************************
 *      _get_initial_wide_environment (UCRTBASE.@)
 */
wchar_t** CDECL _get_initial_wide_environment(void)
{
  return MSVCRT___winitenv;
}
/*********************************************************************
 *      _configure_wide_argv (UCRTBASE.@)
 */
int CDECL _configure_wide_argv(int mode)
{
   //ERR("Calling stub _configure_wide_argv: This will probably fail(%d) stub\n", mode);
   // __debugbreak();
  return 0;
}
/*********************************************************************
 *      _initialize_wide_environment (UCRTBASE.@)
 */
int CDECL _initialize_wide_environment(void)
{
    ERR("Calling stub _initialize_wide_environment: This will probably fail\n");
        ///__debugbreak();
  return 0;
}
/*********************************************************************
 *      _get_narrow_winmain_command_line (UCRTBASE.@)
 */
char* CDECL _get_narrow_winmain_command_line(void)
{
  static char *narrow_command_line;
  char *s;
  if (narrow_command_line)
      return narrow_command_line;
  s = GetCommandLineA();
  while (*s && *s != ' ' && *s != '\t')
  {
      if (*s++ == '"')
      {
          while (*s && *s++ != '"')
              ;
      }
  }
  while (*s == ' ' || *s == '\t')
      s++;
  return narrow_command_line = s;
}
/*********************************************************************
 *      _get_wide_winmain_command_line (UCRTBASE.@)
 */
wchar_t* CDECL _get_wide_winmain_command_line(void)
{
  static wchar_t *wide_command_line;
  wchar_t *s;
  if (wide_command_line)
      return wide_command_line;
  s = GetCommandLineW();
  while (*s && *s != ' ' && *s != '\t')
  {
      if (*s++ == '"')
      {
          while (*s && *s++ != '"')
              ;
      }
  }
  while (*s == ' ' || *s == '\t')
      s++;
  return wide_command_line = s;
}
/* onexit bull shit ******************************/
typedef void (__cdecl *_PVFV)(void);
typedef int (__cdecl *_PIFV)(void);
typedef void (__cdecl *_PVFI)(int);

typedef struct _onexit_table_t {
    _PVFV *_first;
    _PVFV *_last;
    _PVFV *_end;
} _onexit_table_t;

typedef enum _crt_app_type
{
    _crt_unknown_app,
    _crt_console_app,
    _crt_gui_app
} _crt_app_type;
int MSVCRT__fmode = 0;
#ifndef _CRT_ONEXIT_T_DEFINED
#define _CRT_ONEXIT_T_DEFINED
typedef int (__cdecl *_onexit_t)(void);
#endif

void  __cdecl  free(void*);
void* __cdecl  malloc(size_t);
void* __cdecl  calloc(size_t, size_t);
void* __cdecl  realloc(void*, size_t);

int CDECL _set_fmode(int mode)
{
    MSVCRT__fmode = mode;
    return 0;
}
