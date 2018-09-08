
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)


;  Updated by Johan Sköld for sc (https://github.com/rhoot/sc)
;
;  - 2016: XMM6-XMM15 must be preserved by the callee in Windows x64.
;  - 2016: Using a `ret` instead of `jmp` to return to the return address. This
;          seems to cause debuggers to better understand the stack, and results
;          in proper backtraces.
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

.code

sc_jump_context PROC FRAME
    .endprolog

    push  rcx  ; save hidden address of transport_t

    push  rbp  ; save RBP
    push  rbx  ; save RBX
    push  rsi  ; save RSI
    push  rdi  ; save RDI
    push  r15  ; save R15
    push  r14  ; save R14
    push  r13  ; save R13
    push  r12  ; save R12

    ; load NT_TIB
    mov  r10,  gs:[030h]
    ; save current stack base
    mov  rax,  [r10+08h]
    push  rax
    ; save current stack limit
    mov  rax, [r10+010h]
    push  rax
    ; save current deallocation stack
    mov  rax, [r10+01478h]
    push  rax
    ; save fiber local storage
    mov  rax, [r10+020h]
    push  rax

    ; preserve non-volatile xmm registers
    mov  rax,  rsp
    and  rax,  -16
    sub  rsp,  0a8h

    movdqa  [rax-010h],  xmm6  ; save xmm6
    movdqa  [rax-020h],  xmm7  ; save xmm7
    movdqa  [rax-030h],  xmm8  ; save xmm8
    movdqa  [rax-040h],  xmm9  ; save xmm9
    movdqa  [rax-050h],  xmm10  ; save xmm10
    movdqa  [rax-060h],  xmm11  ; save xmm11
    movdqa  [rax-070h],  xmm12  ; save xmm12
    movdqa  [rax-080h],  xmm13  ; save xmm13
    movdqa  [rax-090h],  xmm14  ; save xmm14
    movdqa  [rax-0a0h],  xmm15  ; save xmm15

    ; preserve RSP (pointing to context-data) in R9
    mov  r9, rsp

    ; restore RSP (pointing to context-data) from RDX
    mov  rsp, rdx

    ; restore non-volatile xmm registers
    add  rsp,  0a8h
    mov  rax,  rsp
    and  rax,  -16

    movdqa  xmm6,  [rax-010h]  ; restore xmm6
    movdqa  xmm7,  [rax-020h]  ; restore xmm7
    movdqa  xmm8,  [rax-030h]  ; restore xmm8
    movdqa  xmm9,  [rax-040h]  ; restore xmm9
    movdqa  xmm10,  [rax-050h]  ; restore xmm10
    movdqa  xmm11,  [rax-060h]  ; restore xmm11
    movdqa  xmm12,  [rax-070h]  ; restore xmm12
    movdqa  xmm13,  [rax-080h]  ; restore xmm13
    movdqa  xmm14,  [rax-090h]  ; restore xmm14
    movdqa  xmm15,  [rax-0a0h]  ; restore xmm15

    ; load NT_TIB
    mov  r10, gs:[030h]
    ; restore fiber local storage
    pop  rax
    mov  [r10+020h], rax
    ; restore deallocation stack
    pop  rax
    mov  [r10+01478h], rax
    ; restore stack limit
    pop  rax
    mov  [r10+010h], rax
    ; restore stack base
    pop  rax
    mov  [r10+08h], rax

    pop  r12  ; restore R12
    pop  r13  ; restore R13
    pop  r14  ; restore R14
    pop  r15  ; restore R15
    pop  rdi  ; restore RDI
    pop  rsi  ; restore RSI
    pop  rbx  ; restore RBX
    pop  rbp  ; restore RBP

    pop  rax  ; restore hidden address of transport_t

    ; transport_t returned in RAX
    ; return parent sc_context_sp_t
    mov  [rax], r9
    ; return data
    mov  [rax+08h], r8

    ; transport_t as 1.arg of context-function
    mov  rcx,  rax

    ; hop back to the return address
    ret
sc_jump_context ENDP
