;
; PROJECT:     ReactOS CRT library
; LICENSE:     MIT (https://spdx.org/licenses/MIT)
; PURPOSE:     Implementation of memmove for x86
; COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
;

OPTION DOTNAME

EXTERN ___memmove_fnptr:DWORD

.MODEL FLAT
.CODE

;
; void*
; memmove(
;     void* Dst@[esp+4],
;     const void* Src@[esp+8],
;     size_t Size@[esp+12]);
;
; This is the main entry point for memmove.
; It dispatches to the appropriate implementation based on CPU features and size.
;
PUBLIC _memmove
_memmove PROC

    ; Quick selection for tiny moves less than 8 bytes
    cmp dword ptr [esp + 12], 8
    jb __memmove_small

    ; Dispatch to the appropriate implementation based on CPU features (ERMS, FSRM)
    jmp DWORD PTR [___memmove_fnptr]

_memmove ENDP


;
; This is the small size handler for memmove, which uses a jump table
; to handle sizes from 0 to 7 bytes efficiently.
; Each case is carefully crafted to handle potential overlaps correctly.
;
ALIGN 16
__memmove_small PROC
.const
    ; jump table for small sizes (0-7)
__memmove_small_jmptbl DD __memmove_small_0, __memmove_small_1, __memmove_small_2, __memmove_small_3,
                          __memmove_small_4, __memmove_small_5, __memmove_small_6, __memmove_small_7
.code

    ; Here we use a jump table for moving small sizes (0-7 bytes) to avoid branching and improve performance.
    ; Each chunk can handle overlapping memory regions correctly.

    ; Jump to the appropriate small size handler based on the value in [esp+12]
    mov eax, [esp + 12] ; Size
    mov ecx, [esp + 4]  ; Dst
    mov edx, [esp + 8]  ; Src
    jmp dword ptr __memmove_small_jmptbl[eax * 4]

__memmove_small_7:
    mov eax, dword ptr [edx]
    mov edx, dword ptr [edx + 3]
    mov dword ptr [ecx], eax
    mov dword ptr [ecx + 3], edx
    jmp __memmove_small_0

__memmove_small_6:
    mov eax, dword ptr [edx]
    mov dx, word ptr [edx + 4]
    mov dword ptr [ecx], eax
    mov word ptr [ecx + 4], dx
    jmp __memmove_small_0

__memmove_small_5:
    mov eax, dword ptr [edx]
    mov dl, byte ptr [edx + 4]
    mov dword ptr [ecx], eax
    mov byte ptr [ecx + 4], dl
    jmp __memmove_small_0

__memmove_small_4:
    mov eax, dword ptr [edx]
    mov dword ptr [ecx], eax
    jmp __memmove_small_0

__memmove_small_3:
    mov ax, word ptr [edx]
    mov dl, byte ptr [edx + 2]
    mov word ptr [ecx], ax
    mov byte ptr [ecx + 2], dl
    jmp __memmove_small_0

__memmove_small_2:
    mov ax, word ptr [edx]
    mov word ptr [ecx], ax
    jmp __memmove_small_0

__memmove_small_1:
    mov al, byte ptr [edx]
    mov byte ptr [ecx], al

__memmove_small_0:
    ; Load Dst into eax
    mov eax, [esp + 4]
    ret

__memmove_small ENDP


;
; This is the ERMS (Enhanced REP MOVSB) implementation of memmove, which is
; used when the CPU supports ERMS. It checks the size and decides whether to
; use the rep movsb instruction or the SSE2 implementation for optimal performance.
;
PUBLIC ___memmove_erms
___memmove_erms PROC

    ; With ERMS, the 'rep stosb' implementation is faster for sizes above ??? We guess here
    cmp dword ptr [esp + 12], 512
    ja short __memmove_movsb

    jmp ___memmove_x86
___memmove_erms ENDP


;
; This is the FSRM (Fast Short REP MOVSB) implementation of memmove, which is
; used when the CPU supports FSRM. It checks the size and decides whether to
; use the rep movsb instruction or the SSE2 implementation for optimal performance.
; Even though FSRM has a faster setup for small sizes, it is slower than SSE2 for
; small sizes, so we use SSE2 for sizes below 1024 bytes (based on local benchmarking)
; and rep movsb for larger sizes.
;
ALIGN 16
PUBLIC ___memmove_fsrm
___memmove_fsrm PROC

    ; With FSRM, the 'rep stosb' implementation is faster for sizes above ~256
    cmp dword ptr [esp + 12], 256
    ja short __memmove_movsb

    jmp ___memmove_x86
___memmove_fsrm ENDP


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

    ; Save original value of ESI,EDI in EAX,EDX
    mov eax, esi
    mov edx, edi

    ; Load destination address into EDI, source address into ESI, and size into ECX
    mov edi, [esp + 4]   ; Dst
    mov esi, [esp + 8]   ; Src
    mov ecx, [esp + 12]  ; Size

    ; Check overlap direction
    ; If Dst < Src: always safe to copy forward
    ; If Dst > Src: need backward copy if regions overlap
    cmp edi, esi
    jb __memmove_movsb_forward

    ; Dst > Src  →  check for overlap
    ; Overlap exists when Src + Size > Dst
    ; i.e. esi + ecx > edi
    add ecx, esi     ; ecx = Src + Size
    cmp ecx, edi
    jbe __memmove_movsb_forward            ; no overlap, forward is fine
   
    ; -----------------------------------------------------------------------
    ; BACKWARD COPY  (Dst > Src, overlapping)
    ; -----------------------------------------------------------------------

    ; Point esi and edi to the last byte of their respective regions
    mov ecx, [esp + 12]  ; Size
    lea edi, [edi + ecx - 1]    ; Dst end
    lea esi, [esi + ecx - 1]    ; Src end

    ; Copy bytes using simple rep movsb, going backward
    std                     ; set direction flag for backward copy
    rep movsb
    cld                     ; clear direction flag

    jmp __memmove_movsb_forward_done

    ; -----------------------------------------------------------------------
    ; FORWARD COPY  (Dst < Src,  or no overlap)
    ; -----------------------------------------------------------------------

__memmove_movsb_forward:

    mov ecx, [esp + 12]  ; Size

    ; Copy bytes using simple rep movsb
    rep movsb

__memmove_movsb_forward_done:
    ; Restore original value of ESI,EDI
    mov esi, eax
    mov edi, edx

    ; Load Dst into eax
    mov eax, [esp + 4]
    ret

__memmove_movsb ENDP


PUBLIC ___memmove_x86
___memmove_x86 PROC

    push ebx
    push esi
    push edi

    mov edi, [esp+16]    ; dst
    mov esi, [esp+20]    ; src
    mov ecx, [esp+24]    ; count

    test ecx, ecx
    jz done

    ;-------------------------------------------------------------------------
    ; Check overlap
    ;-------------------------------------------------------------------------
    lea edx, [esi + ecx]   ; src + count

    cmp edi, esi
    jb forward_copy

    cmp edi, edx
    jae forward_copy

    ;=========================================================================
    ; BACKWARD COPY
    ;=========================================================================

    ; Point esi and edi to the last byte of their respective regions
    lea esi, [esi + ecx - 1]
    lea edi, [edi + ecx - 1]

    ; Copy bytes until destination becomes dword aligned.
back_align:

    test edi, 3
    jz back_dwords

    mov al, [esi]
    mov [edi], al

    dec esi
    dec edi
    dec ecx
    jz done

    jmp back_align

back_dwords:

    mov ebx, ecx
    shr ecx, 2
    jz back_tail

    sub esi, 3
    sub edi, 3

back_loop:

    mov eax, [esi]
    mov [edi], eax

    sub esi, 4
    sub edi, 4

    dec ecx
    jnz back_loop

back_tail:

    and ebx, 3
    jz done

    lea esi, [esi+3]
    lea edi, [edi+3]

    mov ecx, ebx

back_tail_loop:

    mov al, [esi]
    mov [edi], al

    dec esi
    dec edi

    dec ecx
    jnz back_tail_loop

    jmp done

    ;=========================================================================
    ; FORWARD COPY
    ;=========================================================================

forward_copy:

    ; Align destination

align_loop:

    test edi, 3
    jz aligned

    test ecx, ecx
    jz done

    mov al, [esi]
    mov [edi], al

    inc esi
    inc edi
    dec ecx
    jnz align_loop

aligned:

    ;-------------------------------------------------------------------------
    ; 32-byte unrolled loop
    ;-------------------------------------------------------------------------

    cmp ecx, 32
    jb dword_copy

    push ebp

forward_32:

    mov eax, [esi+0]
    mov ebx, [esi+4]
    mov edx, [esi+8]
    mov ebp, [esi+12]
    mov [edi+0], eax
    mov [edi+4], ebx
    mov [edi+8], edx
    mov [edi+12], ebp

    mov eax, [esi+16]
    mov ebx, [esi+20]
    mov edx, [esi+24]
    mov ebp, [esi+28]
    mov [edi+16], eax
    mov [edi+20], ebx
    mov [edi+24], edx
    mov [edi+28], ebp

    add esi, 32
    add edi, 32

    sub ecx, 32
    cmp ecx, 32
    jge forward_32

    pop ebp

dword_copy:

    cmp ecx, 4
    jb byte_tail

forward_dword:

    mov eax, [esi]
    mov [edi], eax

    add esi, 4
    add edi, 4

    sub ecx, 4
    cmp ecx, 4
    jge forward_dword

byte_tail:

    cmp ecx, 0
    je done

forward_tail:

    mov al, [esi]
    mov [edi], al

    inc esi
    inc edi

    dec ecx
    jnz forward_tail

done:

    mov eax, [esp+16] ; return Dst
    pop edi
    pop esi
    pop ebx
    ret

___memmove_x86 ENDP

END
