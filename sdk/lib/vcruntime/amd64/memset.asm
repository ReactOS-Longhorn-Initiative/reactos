;
; PROJECT:     NT C Runtime tests
; LICENSE:     MIT (https://spdx.org/licenses/MIT)
; PURPOSE:     Implementation of memset for x64 using ERMS, SSE2 and rep stosb instructions
; COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
;

EXTERN __memset_fnptr:QWORD

.code

; memset using rep stosb instructions
;
; Parameters:
;   RCX = Dest
;   EDX = Value
;   R8  = Size
;
ALIGN 16
PUBLIC  __memset_erms
__memset_erms PROC
    
    ; With ERMS, the 'rep stosb' implementation is faster for sizes above ~1024
    cmp r8, 1024
    ja short __memset_stosb

    jmp __memset_sse2

__memset_erms ENDP


; memset using rep stosb instructions
;
; Parameters:
;   RCX = Dest
;   EDX = Value
;   R8  = Size
;
ALIGN 16
PUBLIC  __memset_stosb
__memset_stosb PROC

    ; Save original value of RDI in R10
    mov r10, rdi

    ; Save original value of Dest in R9
    mov r9, rcx

    ; Load destination address into RDI, Size into RCX and value into AL
    mov rdi, rcx
    mov rax, rdx
    mov rcx, r8

    ; Write bytes using simple rep stosb
    rep stosb

    ; Restore original value of RDI and return original value of Dest in RAX
    mov rax, r9
    mov rdi, r10
    ret

__memset_stosb ENDP


; memset using rep stosq instructions
;
; Pramaters:
;   RCX = Dest
;   EDX = Value
;   R8  = Size
;
PUBLIC  __memset_stosq
__memset_stosq PROC

    ; Save original value of RDI in R10
    mov r10, rdi

    ; Save original value of Dest in R9
    mov r9, rcx

    ; Load destination address into RDI and value into AL
    mov rdi, rcx
    movzx rax, dl
  
    ; First do unaligned bytes until we reach an 8-byte boundary
    neg rcx
    and rcx, 7
    jz  short aligned

    ; Calculate the number of unaligned bytes to write
    cmp rcx, r8
    jb short unaligned

    mov rcx, r8

unaligned:
    sub r8, rcx
    rep stosb
    
aligned:

    ; Check if there are at least 8 bytes left to write
    cmp r8, 8
    jb short remainder

    ; Get the 8-byte value to write by replicating the byte in AL across RAX
    mov r9, 0101010101010101h
    mul r9  ; RAX now contains the 8-byte value to write

    ; Get the number of 8-byte chunks to write
    mov rcx, r8
    shr rcx, 3

    rep stosq

remainder:
    ; Write any remaining bytes
    and r8, 7
    jz  short done
    mov rcx, r8
    rep stosb

done:
    ; Restore original value of RDI and return original value of Dest in RAX
    mov rax, r9
    mov rdi, r10
    ret

__memset_stosq ENDP


; memset using SSE2 instructions
;
; Pramaters:
;   RCX = Dest
;   EDX = Value
;   R8  = Size
;
PUBLIC __memset_sse2
__memset_sse2 PROC

    ; Set return value to original destination
    mov rax, rcx

    ; Check if there are at least 16 bytes left to write
    cmp r8, 16
    jb __memset_less_than_16

    ; Get the 16-byte value to write by replicating the byte in AL across XMM0
    xorpd xmm1, xmm1
    movd xmm0, edx
    pshufb xmm0, xmm1

    ; Check if the destination address is aligned to a 16-byte boundary
    test rcx, 15
    jz short xmm_aligned

    ; Write one unaligned 16-byte chunk using SSE2 instructions
    movups xmmword ptr [rcx], xmm0

    ; Update destination address and size for the remaining aligned portion
    mov r9, rcx
    neg r9
    and r9, 15
    add rcx, r9
    sub r8, r9

xmm_aligned:

    ; Get the number of 64-byte chunks to write
    mov r9, r8
    shr r9, 6
    jz short xmm_remainder

ALIGN 16
xmm_unrolled:
    movaps xmmword ptr [rcx], xmm0
    movaps xmmword ptr [rcx + 16], xmm0
    movaps xmmword ptr [rcx + 32], xmm0
    movaps xmmword ptr [rcx + 48], xmm0
    add rcx, 64
    dec r9
    jnz short xmm_unrolled

xmm_remainder:
    and r8, 63
    jz short __memset_sse2_done

    ; Get the number of 16-byte chunks to write
    mov r9, r8
    shr r9, 4
    jz short remaining_bytes

    ; Write 16-byte chunks using SSE2 instructions
xmm_remainder_loop:
    movaps xmmword ptr [rcx], xmm0
    add rcx, 16
    dec r9
    jnz short xmm_remainder_loop

remaining_bytes:
    ; Write any remaining bytes as an unaligned 16-byte chunk using SSE2 instructions
    and r8, 15
    jz short __memset_sse2_done
    movups xmmword ptr [rcx + r8 - 16], xmm0

__memset_sse2_done:
    ret

.const
    ; jump table for small sizes (0-15)
__memset_small_jmptbl DQ __memset_small_0, __memset_small_1, __memset_small_2, __memset_small_3,
                         __memset_small_4, __memset_small_5, __memset_small_6, __memset_small_7,
                         __memset_small_8, __memset_small_9, __memset_small_10, __memset_small_11,
                         __memset_small_12, __memset_small_13, __memset_small_14, __memset_small_15
.code

__memset_less_than_16::

    ; Get the 8-byte value to write by replicating the byte in DL across RDX
    movzx edx, dl
    mov r10, 0101010101010101h
    imul rdx, r10


    mov rax, rcx

    ; Jump to the appropriate small size handler based on the value in R8
    lea r9, __memset_small_jmptbl
    jmp qword ptr [r9 + r8 * 8]

__memset_small_15:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 7], rdx
    ret
__memset_small_14:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 6], rdx
    ret
__memset_small_13:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 5], rdx
    ret
__memset_small_12:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 4], rdx
    ret
__memset_small_11:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 3], rdx
    ret
__memset_small_10:
    mov qword ptr [rcx], rdx
    mov qword ptr [rcx + 2], rdx
    ret
__memset_small_9:
    mov qword ptr [rcx], rdx
    mov byte ptr [rcx + 8], dl
    ret
__memset_small_8:
    mov qword ptr [rcx], rdx
    ret
__memset_small_7:
    mov dword ptr [rcx], edx
    mov dword ptr [rcx + 3], edx
    ret
__memset_small_6:
    mov dword ptr [rcx], edx
    mov word ptr [rcx + 4], dx
    ret
__memset_small_5:
    mov dword ptr [rcx], edx
    mov byte ptr [rcx + 4], dl
    ret
__memset_small_4:
    mov dword ptr [rcx], edx
    ret
__memset_small_3:
    mov word ptr [rcx], dx
    mov byte ptr [rcx + 2], dl
    ret
__memset_small_2:
    mov word ptr [rcx], dx
    ret
__memset_small_1:
    mov byte ptr [rcx], dl
    ret
__memset_small_0:
    ret

__memset_sse2 ENDP

PUBLIC memset
memset PROC
    ; Set return value to original destination
;    mov rax, rcx

    ; Check if there are at least 16 bytes left to write
    cmp r8, 16
    jb __memset_less_than_16

    jmp QWORD PTR [__memset_fnptr]
memset ENDP

END
