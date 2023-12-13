#ifndef _CRT_PRECOMP_H
#define _CRT_PRECOMP_H

#define WIN32_NO_STATUS

#include <internal/wine/msvcrt.h>

//#include <windows.h>

#include <wine/debug.h>

#define MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define MAGIC_HEAP      MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)  (((a)/(b))*(b))

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define __TRY _SEH2_TRY
#define __EXCEPT_PAGE_FAULT _SEH2_EXCEPT(_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION)
#define __ENDTRY _SEH2_END


#endif /* _CRT_PRECOMP_H */
