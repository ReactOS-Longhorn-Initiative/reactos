#include <dwrite_private.h>

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

/* print a message */
void
FT_Message(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    WARN((PCHAR)format, va);
    va_end(va);
}

/* print a message and exit */
void
FT_Panic(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    ERR((PCHAR)format, va);
    va_end(va);
}
