;
; PROJECT:     ReactOS CRT library
; LICENSE:     MIT (https://spdx.org/licenses/MIT)
; PURPOSE:     Implementation of memmove for x64
; COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
;

OPTION DOTNAME

EXTERN __memmove_fnptr:QWORD

.CODE

;
; void*
; memmove(
;     void* Dst@<rcx>,
;     const void* Src@<rdx>,
;     size_t Size@<r8>);
;
; This is the main entry point for memmove.
; It dispatches to the appropriate implementation based on CPU features and size.
;
PUBLIC memmove
memmove PROC

    ; Copy Dst into rax for return value (preserved in all implementations)
    mov rax, rcx

    ; Quick selection for tiny moves less than 16 bytes
    cmp r8, 16
    jb __memmove_small

    ; Dispatch to the appropriate implementation based on CPU features (ERMS, FSRM)
    jmp QWORD PTR [__memmove_fnptr]

memmove ENDP

;
; This is the small size handler for memmove, which uses a jump table
; to handle sizes from 0 to 15 bytes efficiently.
; Each case is carefully crafted to handle potential overlaps correctly.
;
ALIGN 16
__memmove_small PROC
.const
    ; jump table for small sizes (0-15)
__memmove_small_jmptbl DQ __memmove_small_0, __memmove_small_1, __memmove_small_2, __memmove_small_3,
                          __memmove_small_4, __memmove_small_5, __memmove_small_6, __memmove_small_7,
                          __memmove_small_8, __memmove_small_9, __memmove_small_10, __memmove_small_11,
                          __memmove_small_12, __memmove_small_13, __memmove_small_14, __memmove_small_15
.code

    ; Here we use a jump table for moving small sizes (0-15 bytes) to avoid branching and improve performance.
    ; Each chunk can handle overlapping memory regions correctly.

    ; Jump to the appropriate small size handler based on the value in R8
    lea r9, __memmove_small_jmptbl
    jmp qword ptr [r9 + r8 * 8]

__memmove_small_15:
    mov r10, qword ptr [rdx]
    mov r11, qword ptr [rdx + 7]
    mov qword ptr [rcx], r10
    mov qword ptr [rcx + 7], r11
    ret

__memmove_small_14:
    mov r10, qword ptr [rdx]
    mov r11, qword ptr [rdx + 6]
    mov qword ptr [rcx], r10
    mov qword ptr [rcx + 6], r11
    ret

__memmove_small_13:
    mov r10, qword ptr [rdx]
    mov r11, qword ptr [rdx + 5]
    mov qword ptr [rcx], r10
    mov qword ptr [rcx + 5], r11
    ret

__memmove_small_12:
    mov r10, qword ptr [rdx]
    mov r11d, dword ptr [rdx + 8]
    mov qword ptr [rcx], r10
    mov dword ptr [rcx + 8], r11d
    ret

__memmove_small_11:
    mov r10, qword ptr [rdx]
    mov r11d, dword ptr [rdx + 7]
    mov qword ptr [rcx], r10
    mov dword ptr [rcx + 7], r11d
    ret

__memmove_small_10:
    mov r10, qword ptr [rdx]
    mov r11w, word ptr [rdx + 8]
    mov qword ptr [rcx], r10
    mov word ptr [rcx + 8], r11w
    ret

__memmove_small_9:
    mov r10, qword ptr [rdx]
    mov r11b, byte ptr [rdx + 8]
    mov qword ptr [rcx], r10
    mov byte ptr [rcx + 8], r11b
    ret

__memmove_small_8:
    mov r10, qword ptr [rdx]
    mov qword ptr [rcx], r10
    ret

__memmove_small_7:
    mov r10d, dword ptr [rdx]
    mov r11d, dword ptr [rdx + 3]
    mov dword ptr [rcx], r10d
    mov dword ptr [rcx + 3], r11d
    ret

__memmove_small_6:
    mov r10d, dword ptr [rdx]
    mov r11w, word ptr [rdx + 4]
    mov dword ptr [rcx], r10d
    mov word ptr [rcx + 4], r11w
    ret

__memmove_small_5:
    mov r10d, dword ptr [rdx]
    mov r11b, byte ptr [rdx + 4]
    mov dword ptr [rcx], r10d
    mov byte ptr [rcx + 4], r11b
    ret

__memmove_small_4:
    mov r10d, dword ptr [rdx]
    mov dword ptr [rcx], r10d
    ret

__memmove_small_3:
    mov r10w, word ptr [rdx]
    mov r11b, byte ptr [rdx + 2]
    mov word ptr [rcx], r10w
    mov byte ptr [rcx + 2], r11b
    ret

__memmove_small_2:
    mov r10w, word ptr [rdx]
    mov word ptr [rcx], r10w
    ret

__memmove_small_1:
    mov r10b, byte ptr [rdx]
    mov byte ptr [rcx], r10b
    ret

__memmove_small_0:
    ret

__memmove_small ENDP


;
; This is the ERMS (Enhanced REP MOVSB) implementation of memmove, which is
; used when the CPU supports ERMS. It checks the size and decides whether to
; use the rep movsb instruction or the SSE2 implementation for optimal performance.
;
PUBLIC __memmove_erms
__memmove_erms PROC

    ; With ERMS, the 'rep stosb' implementation is faster for sizes above ??? We guess here
    cmp r8, 2048
    ja short __memmove_movsb

    jmp __memmove_sse2
__memmove_erms ENDP


;
; This is the FSRM (Fast Short REP MOVSB) implementation of memmove, which is
; used when the CPU supports FSRM. It checks the size and decides whether to
; use the rep movsb instruction or the SSE2 implementation for optimal performance.
; Even though FSRM has a faster setup for small sizes, it is slower than SSE2 for
; small sizes, so we use SSE2 for sizes below 1024 bytes (based on local benchmarking)
; and rep movsb for larger sizes.
;
ALIGN 16
PUBLIC __memmove_fsrm
__memmove_fsrm PROC

    ; With FSRM, the 'rep stosb' implementation is faster for sizes above ~1024
    cmp r8, 1024
    ja short __memmove_movsb

    jmp __memmove_sse2
__memmove_fsrm ENDP


; memmove using rep movsb instructions
;
; Parameters:
;   RCX = Dst
;   RDX = Src
;   R8  = Size
;
ALIGN 16
PUBLIC __memmove_movsb
__memmove_movsb PROC

    ; Save original value of RSI,RDI in R10,R11
    mov r10, rsi
    mov r11, rdi

    ; Check overlap direction
    ; If Dst < Src: always safe to copy forward
    ; If Dst > Src: need backward copy if regions overlap
    cmp rcx, rdx
    jb __memmove_movsb_forward

    ; Dst > Src  →  check for overlap
    ; Overlap exists when Src + Size > Dst
    ; i.e. rdx + r8 > rcx
    lea r9, [rdx + r8]       ; r9 = Src + Size
    cmp r9, rcx
    jbe __memmove_movsb_forward            ; no overlap, forward is fine
   
    ; -----------------------------------------------------------------------
    ; BACKWARD COPY  (Dst > Src, overlapping)
    ; -----------------------------------------------------------------------

    ; Point rsi and rdi to the last byte of their respective regions
    lea rdi, [rax + r8 - 1]    ; Dst end
    lea rsi, [rdx + r8 - 1]    ; Src end
    mov rcx, r8

    ; Copy bytes using simple rep movsb, going backward
    std                     ; set direction flag for backward copy
    rep movsb
    cld                     ; clear direction flag

    jmp __memmove_movsb_forward_done

    ; -----------------------------------------------------------------------
    ; FORWARD COPY  (Dst < Src,  or no overlap)
    ; -----------------------------------------------------------------------

__memmove_movsb_forward:

    ; Load destination address into RDI, source address into RSI, and size into RCX
    mov rdi, rax
    mov rsi, rdx
    mov rcx, r8

    ; Copy bytes using simple rep movsb
    rep movsb

__memmove_movsb_forward_done:
    ; Restore original value of RSI,RDI
    mov rsi, r10
    mov rdi, r11
    ret

__memmove_movsb ENDP

;
; This is the main SSE2 implementation of memmove, which handles all sizes
; above 16 bytes and uses SSE2 instructions for optimal performance.
;
ALIGN 16
PUBLIC __memmove_sse2
__memmove_sse2 PROC

    ; Check overlap direction
    ; If Dst < Src: always safe to copy forward
    ; If Dst > Src: need backward copy if regions overlap
    cmp rcx, rdx
    jb __memmove_sse2_fwd

    ; If Src and Dst are equal, nothing to do
    je __memmove_sse2_done

    ; Dst > Src  →  check for overlap
    ; Overlap exists when Src + Size > Dst
    ; i.e. rdx + r8 > rcx
    lea	r9, [rdx + r8]       ; r9 = Src + Size
    cmp r9, rcx
    jbe __memmove_sse2_fwd ; no overlap, forward is fine

    ; -----------------------------------------------------------------------
    ; BACKWARD COPY  (Dst > Src, overlapping)
    ; -----------------------------------------------------------------------
__memmove_sse2_bwd:

    ; Point all pointers to the END of their respective regions
    lea     rcx, [rcx + r8]    ; Dst end
    lea     rdx, [rdx + r8]    ; Src end

    ; Align Dst end to 16-byte boundary by copying bytes one at a time
    mov     r9, rcx
    and     r9, 15              ; bytes above 16-byte boundary
    jz __memmove_sse2_bwd_start
    sub     r8, r9
    sub     rcx, r9
    sub     rdx, r9

    ; scalar byte copy (small, ≤15 iterations)
__memmove_sse2_bwd_align_loop:
    dec     r9
    movzx   r10d, byte ptr [rdx + r9]
    mov     byte ptr [rcx + r9], r10b
    test    r9, r9
    jnz __memmove_sse2_bwd_align_loop

__memmove_sse2_bwd_start:
    ; Main SSE2 loop: 64 bytes per iteration (4 × 16 B), going backward
    ; We need count >= 64
    cmp     r8, 64
    jb __memmove_sse2_bwd_tail_16

__memmove_sse2_bwd_write_aligned_loop:
    prefetchnta [rdx - 256]
    sub     rcx, 64
    sub     rdx, 64
    movdqu  xmm0, xmmword ptr [rdx + 48]
    movdqu  xmm1, xmmword ptr [rdx + 32]
    movdqu  xmm2, xmmword ptr [rdx + 16]
    movdqu  xmm3, xmmword ptr [rdx]
    movdqa  xmmword ptr [rcx + 48], xmm0
    movdqa  xmmword ptr [rcx + 32], xmm1
    movdqa  xmmword ptr [rcx + 16], xmm2
    movdqa  xmmword ptr [rcx],      xmm3
    sub     r8, 64
    cmp     r8, 64
    jae __memmove_sse2_bwd_write_aligned_loop

__memmove_sse2_bwd_tail_16:
    ; Copy remaining 16-byte blocks
    cmp     r8, 16
    jb __memmove_sse2_bwd_tail_small
    sub     rcx, 16
    sub     rdx, 16
    movdqu  xmm0, xmmword ptr [rdx]
    movdqa  xmmword ptr [rcx], xmm0   ; dst is 16-aligned here
    sub     r8, 16
    jmp __memmove_sse2_bwd_tail_16

__memmove_sse2_bwd_tail_small:
    sub rcx, r8
    sub rdx, r8
    jmp __memmove_small

    ; -----------------------------------------------------------------------
    ; FORWARD COPY  (dst < src,  or no overlap)
    ; -----------------------------------------------------------------------
__memmove_sse2_fwd:

    ; Check if Dst is aligned to 16-byte boundary
    test    rcx, 15
    jz __memmove_sse2_fwd_aligned

    ; Calculate bytes to next 16-byte boundary
    mov     r9, rcx
    neg     r9
    and     r9, 15

    ; Adjust pointers and remaining size
    sub     r8, r9
    ; scalar byte copy for alignment head
    xor     r10, r10
__memmove_sse2_fwd_align_loop:
    movzx   r11d, byte ptr [rdx + r10]
    mov     byte ptr [rcx + r10], r11b
    inc     r10
    cmp     r10, r9
    jb __memmove_sse2_fwd_align_loop
    add     rcx, r9
    add     rdx, r9

__memmove_sse2_fwd_aligned:
    ; Main SSE2 loop: 64 bytes per iteration (4 × 16 B)
    cmp     r8, 64
    jb __memmove_sse2_fwd_tail_16

    test   rdx, 15
    jz __memmove_sse2_fwd_fully_aligned_loop

ALIGN 16
__memmove_sse2_fwd_write_aligned_loop:
    ; Dst is 16-aligned, but Src is not. Use unaligned loads and aligned stores.
    prefetchnta [rdx + 256]
    movdqu  xmm0, xmmword ptr [rdx]
    movdqu  xmm1, xmmword ptr [rdx + 16]
    movdqu  xmm2, xmmword ptr [rdx + 32]
    movdqu  xmm3, xmmword ptr [rdx + 48]
    movdqa  xmmword ptr [rcx],      xmm0
    movdqa  xmmword ptr [rcx + 16], xmm1
    movdqa  xmmword ptr [rcx + 32], xmm2
    movdqa  xmmword ptr [rcx + 48], xmm3
    add     rdx, 64
    add     rcx, 64
    sub     r8, 64
    cmp     r8, 64
    jae __memmove_sse2_fwd_write_aligned_loop
    jmp __memmove_sse2_fwd_tail_16

ALIGN 16
__memmove_sse2_fwd_fully_aligned_loop:
    ; Both Src and Dst are 16-byte aligned, we can use aligned loads/stores
    prefetchnta [rdx + 256]
    movdqa  xmm0, xmmword ptr [rdx]
    movdqa  xmm1, xmmword ptr [rdx + 16]
    movdqa  xmm2, xmmword ptr [rdx + 32]
    movdqa  xmm3, xmmword ptr [rdx + 48]
    movdqa  xmmword ptr [rcx],      xmm0
    movdqa  xmmword ptr [rcx + 16], xmm1
    movdqa  xmmword ptr [rcx + 32], xmm2
    movdqa  xmmword ptr [rcx + 48], xmm3
    add     rdx, 64
    add     rcx, 64
    sub     r8, 64
    cmp     r8, 64
    jae __memmove_sse2_fwd_fully_aligned_loop

__memmove_sse2_fwd_tail_16:
    ; Copy remaining 16-byte blocks
    cmp     r8, 16
    jb      __memmove_small
    movdqu  xmm0, xmmword ptr [rdx]
    movdqa  xmmword ptr [rcx], xmm0
    add     rdx, 16
    add     rcx, 16
    sub     r8, 16
    jmp __memmove_sse2_fwd_tail_16

__memmove_sse2_done:
    ret

__memmove_sse2 ENDP


END
