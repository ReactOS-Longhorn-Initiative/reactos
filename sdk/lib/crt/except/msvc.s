
#include <asm.inc>

.code
.align 4

MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ___std_terminate, ?terminate@@YAXXZ, PROC

END
