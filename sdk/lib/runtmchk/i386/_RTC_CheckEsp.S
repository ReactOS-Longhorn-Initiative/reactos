/*
 * PROJECT:         MSVC runtime check support library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Provides support functions for MSVC runtime checks
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>
.code

EXTERN __RTC_Failure:PROC

/*

    This function is invoked like this:

        mov esi, esp
        // Do the actual function call
        cmp esp, esi
        call __RTC_CheckEsp

    http://stackoverflow.com/questions/3914750/hows-rtc-checkesp-implemented
*/
PUBLIC __RTC_CheckEsp
__RTC_CheckEsp:
 
    ret

END


