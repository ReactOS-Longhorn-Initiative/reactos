;
; PROJECT:     NT C Runtime tests
; LICENSE:     MIT (https://spdx.org/licenses/MIT)
; PURPOSE:     Implementation of memset for x86 using rep stosb and unrolled x86 instructions
; COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
;

EXTERN ___memset_fnptr:DWORD

.model flat
.code

;
; memset dispatcher for ERMS CPUs.
; Uses rep stosb directly for sizes above 1024 bytes, and falls back to the
; generic x86 implementation for smaller sizes.
;
; Parameters (cdecl):
;   [esp+4]  = Dst
;   [esp+8]  = Value
;   [esp+12] = Size
;
ALIGN 16
PUBLIC  ___memset_erms
___memset_erms PROC

    cmp     dword ptr [esp+12], 1024
    ja      ___memset_stosb
    jmp     ___memset_x86

___memset_erms ENDP


;
; memset using rep stosb instructions for all sizes.
;
; Parameters (cdecl):
;   [esp+4]  = Dst
;   [esp+8]  = Value
;   [esp+12] = Size
;
ALIGN 16
PUBLIC  ___memset_stosb
___memset_stosb PROC

    push    edi

    ; [esp+0]  = saved EDI
    ; [esp+4]  = return address
    ; [esp+8]  = Dst
    ; [esp+12] = Value
    ; [esp+16] = Size

    mov     edi, [esp+8]            ; EDI = Dst
    movzx   eax, byte ptr [esp+12]  ; AL  = Value (low byte)
    mov     ecx, [esp+16]           ; ECX = Size

    rep     stosb

    mov     eax, [esp+8]            ; Return original Dst
    pop     edi
    ret

___memset_stosb ENDP


;
; Generic x86 memset using plain x86 instructions with an unrolled DWORD store loop.
; No SSE or other extended instructions, suitable for kernel mode.
;
; Parameters (cdecl):
;   [esp+4]  = Dst
;   [esp+8]  = Value
;   [esp+12] = Size
;
ALIGN 16
PUBLIC  ___memset_x86
___memset_x86 PROC

    push    edi
    push    ebx

    ; [esp+0]  = saved EBX
    ; [esp+4]  = saved EDI
    ; [esp+8]  = return address
    ; [esp+12] = Dst
    ; [esp+16] = Value
    ; [esp+20] = Size

    mov     edi, [esp+12]               ; EDI = Dst
    movzx   eax, byte ptr [esp+16]      ; AL  = Value (low byte)
    mov     ah, al                      ; AH  = AL, so AX = 0xVVVV (for word stores)
    mov     ecx, [esp+20]               ; ECX = Size

    ; Trivial: nothing to do
    test    ecx, ecx
    jz      short x86_done

    ; Tiny: 1-3 bytes (AX already holds the byte replicated in both halves)
    cmp     ecx, 4
    jb      short x86_tiny

    ; Extend fill byte to a full DWORD pattern (AX=0xVVVV -> EAX=0xVVVVVVVV)
    movzx   edx, ax
    shl     edx, 16
    or      edx, eax
    mov     eax, edx

    ; Small: 4-7 bytes, use two overlapping DWORD stores to cover all bytes
    cmp     ecx, 8
    jb      short x86_small

    ; Align destination to a 4-byte boundary
    mov     ebx, edi
    and     ebx, 3
    jz      short x86_aligned

    neg     ebx
    add     ebx, 4                      ; EBX = unaligned bytes before boundary (1-3)
    sub     ecx, ebx                    ; Update remaining count

x86_align_loop:
    mov     byte ptr [edi], al
    inc     edi
    dec     ebx
    jnz     short x86_align_loop

x86_aligned:
    ; Write 32-byte chunks using an 8-DWORD unrolled loop
    mov     ebx, ecx                    ; Save remaining count for remainder calculation
    shr     ecx, 5                      ; ECX = number of 32-byte chunks
    jz      short x86_dwords

ALIGN 16
x86_unrolled:
    mov     dword ptr [edi],    eax
    mov     dword ptr [edi+4],  eax
    mov     dword ptr [edi+8],  eax
    mov     dword ptr [edi+12], eax
    mov     dword ptr [edi+16], eax
    mov     dword ptr [edi+20], eax
    mov     dword ptr [edi+24], eax
    mov     dword ptr [edi+28], eax
    add     edi, 32
    dec     ecx
    jnz     short x86_unrolled

x86_dwords:
    ; Write remaining whole DWORDs (0-7 DWORDs, covering 0-28 bytes)
    mov     ecx, ebx
    and     ecx, 28                     ; Isolate bits 2-4: gives 0,4,8,...,28
    shr     ecx, 2                      ; Convert byte count to DWORD count
    jz      short x86_tail

x86_dword_loop:
    mov     dword ptr [edi], eax
    add     edi, 4
    dec     ecx
    jnz     short x86_dword_loop

x86_tail:
    ; Write remaining bytes (0-3)
    mov     ecx, ebx
    and     ecx, 3
    jz      short x86_done

x86_tail_loop:
    mov     byte ptr [edi], al
    inc     edi
    dec     ecx
    jnz     short x86_tail_loop

x86_done:
    mov     eax, [esp+12]               ; Return original Dst
    pop     ebx
    pop     edi
    ret

x86_tiny:
    ; 1-3 bytes: AX = 0xVVVV (byte replicated), AL = VV
    cmp     ecx, 2
    jb      short x86_one_byte
    mov     word ptr [edi], ax
    je      short x86_done
    mov     byte ptr [edi+2], al
    jmp     short x86_done

x86_small:
    ; 4-7 bytes: first DWORD covers bytes 0-3, second (overlapping) covers up to the end
    mov     dword ptr [edi], eax
    mov     dword ptr [edi + ecx - 4], eax
    jmp     short x86_done

x86_one_byte:
    mov     byte ptr [edi], al
    jmp     short x86_done

___memset_x86 ENDP


;
; The dispatcher function. It is called via the public memset symbol and jumps
; to the actual implementation selected at runtime by __memset_init.
;
PUBLIC _memset
_memset PROC
    jmp DWORD PTR [___memset_fnptr]
_memset ENDP

END
