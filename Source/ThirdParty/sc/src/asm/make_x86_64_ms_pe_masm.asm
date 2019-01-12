
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)

;  Updated by Johan Sköld for sc (https://github.com/rhoot/sc)
;
;  - 2016: XMM6-XMM15 must be preserved by the callee in Windows x64.
;  - 2016: Reserving space for the parameter area in the unwind area, as well
;          as adding a NULL return address for sc_make_context so debuggers
;          will know they've reached the top.
;  - 2016: Removed the END directive; this file is included from a meta file.

;  ----------------------------------------------------------------------------------
;  |    0x0  |    0x4  |    0x8   |    0xc  |   0x10  |   0x14  |   0x18  |   0x1c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM15                  |                 XMM14                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0x20  |   0x24  |   0x28   |   0x2c  |   0x30  |   0x34  |   0x38  |   0x3c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM13                  |                 XMM12                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0x40  |   0x44  |   0x48   |   0x4c  |   0x50  |   0x54  |   0x58  |   0x5c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM11                  |                 XMM10                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0x60  |   0x64  |   0x68   |   0x6c  |   0x70  |   0x74  |   0x78  |   0x7c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM9                   |                 XMM8                  |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0x80  |   0x84  |   0x88   |   0x8c  |   0x90  |   0x94  |   0x98  |   0x9c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM7                   |                 XMM6                  |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0xa0  |   0xa4  |   0xa8   |   0xac  |   0xb0  |   0xb4  |   0xb8  |   0xbc  |
;  ----------------------------------------------------------------------------------
;  |       align       |      fbr_strg      |     fc_dealloc    |       limit       |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0xc0  |  0xc4   |   0xc8   |   0xcc  |   0xd0  |   0xd4  |   0xd8  |   0xdc  |
;  ----------------------------------------------------------------------------------
;  |        base       |         R12        |        R13        |        R14        |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |   0xe0  |  0xe4   |   0xe8   |   0xec  |   0xf0  |   0xf4  |   0xf8  |   0xfc  |
;  ----------------------------------------------------------------------------------
;  |        R15        |        RDI         |        RSI        |        RBX        |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |  0x100  |  0x104  |  0x108   |  0x10c  |  0x110  |  0x114  |  0x118  |  0x11c  |
;  ----------------------------------------------------------------------------------
;  |        RBP        |       hidden       |        RIP        |        EXIT       |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |  0x120  |  0x124  |  0x128   |  0x12c  |  0x130  |  0x134  |  0x138  |  0x13c  |
;  ----------------------------------------------------------------------------------
;  |                                 parameter area                                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |  0x140  |  0x144  |  0x148   |  0x14c  |  0x150  |  0x154  |  0x158  |  0x15c  |
;  ----------------------------------------------------------------------------------
;  |       NULL        |         FCTX       |        DATA       |       align       |
;  ----------------------------------------------------------------------------------

; standard C library function
EXTERN  _exit:PROC
.code

; generate function table entry in .pdata and unwind information in
sc_make_context PROC FRAME
    ; Tell the assembler we're allocating 32 bytes on the stack, to fit the
    ; parameter area. This allows the debugger to see the NULL field just
    ; above it as the return address from sc_make_context, causing it to realize
    ; it's reached the top of the callstack.
    .allocstack 020h

    ; .xdata for a function's structured exception handling unwind behavior
    .endprolog

    ; first arg of sc_make_context() == top of context-stack
    mov  rax, rcx

    ; shift address in RAX to lower 16 byte boundary
    ; == pointer to sc_context_sp_t and address of context stack
    and  rax, -16

    ; reserve space for context-data on context-stack
    ; EXIT will be used as the return address for the context-function and
    ; must have its end be 16-byte aligned

    ; 160 bytes xmm storage, 8+8 bytes alignment, 176 bytes stack data
    sub  rax, 0160h

    ; third arg of sc_make_context() == address of context-function
    mov  [rax+0110h], r8

    ; first arg of sc_make_context() == top of context-stack
    ; save top address of context stack as 'base'
    mov  [rax+0c0h], rcx
    ; second arg of sc_make_context() == size of context-stack
    ; negate stack size for LEA instruction (== substraction)
    neg  rdx
    ; compute bottom address of context stack (limit)
    lea  rcx, [rcx+rdx]
    ; save bottom address of context stack as 'limit'
    mov  [rax+0b8h], rcx
    ; save address of context stack limit as 'dealloction stack'
    mov  [rax+0b0h], rcx
    ; set fiber-storage to zero
    xor  rcx, rcx
    mov  [rax+0a8h], rcx

    ; zero out sc_make_context's return address (rcx is still zero)
    mov  [rax+0140h], rcx

    ; compute address of transport_t
    lea rcx, [rax+0148h]
    ; store address of transport_t in hidden field
    mov [rax+0108h], rcx

    ; compute abs address of label finish
    lea  rcx, finish
    ; save address of finish as return-address for context-function
    ; will be entered after context-function returns
    mov  [rax+0118h], rcx

    ret ; return pointer to context-data

finish:
    ; exit code is zero
    xor  rcx, rcx
    ; exit application
    call  _exit
    hlt
sc_make_context ENDP
