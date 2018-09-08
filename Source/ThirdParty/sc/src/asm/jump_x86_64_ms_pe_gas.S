/*
            Copyright Oliver Kowalke 2009.
            Copyright Thomas Sailer 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/*
   Updated by Johan Sköld for sc (https://github.com/rhoot/sc)

   - 2016: XMM6-XMM15 must be preserved by the callee in Windows x64.
   - 2016: Using a `ret` instead of `jmp` to return to the return address. This
           seems to cause debuggers to better understand the stack, and results
           in proper backtraces.
*/

/**************************************************************************************
 * ---------------------------------------------------------------------------------- *
 * |    0x0  |    0x4  |    0x8   |    0xc  |   0x10  |   0x14  |   0x18  |   0x1c  | *
 * ---------------------------------------------------------------------------------- *
 * |                 XMM15                  |                 XMM14                 | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0x20  |   0x24  |   0x28   |   0x2c  |   0x30  |   0x34  |   0x38  |   0x3c  | *
 * ---------------------------------------------------------------------------------- *
 * |                 XMM13                  |                 XMM12                 | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0x40  |   0x44  |   0x48   |   0x4c  |   0x50  |   0x54  |   0x58  |   0x5c  | *
 * ---------------------------------------------------------------------------------- *
 * |                 XMM11                  |                 XMM10                 | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0x60  |   0x64  |   0x68   |   0x6c  |   0x70  |   0x74  |   0x78  |   0x7c  | *
 * ---------------------------------------------------------------------------------- *
 * |                 XMM9                   |                 XMM8                  | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0x80  |   0x84  |   0x88   |   0x8c  |   0x90  |   0x94  |   0x98  |   0x9c  | *
 * ---------------------------------------------------------------------------------- *
 * |                 XMM7                   |                 XMM6                  | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0xa0  |   0xa4  |   0xa8   |   0xac  |   0xb0  |   0xb4  |   0xb8  |   0xbc  | *
 * ---------------------------------------------------------------------------------- *
 * |       align       |      fbr_strg      |     fc_dealloc    |       limit       | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0xc0  |  0xc4   |   0xc8   |   0xcc  |   0xd0  |   0xd4  |   0xd8  |   0xdc  | *
 * ---------------------------------------------------------------------------------- *
 * |        base       |         R12        |        R13        |        R14        | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |   0xe0  |  0xe4   |   0xe8   |   0xec  |   0xf0  |   0xf4  |   0xf8  |   0xfc  | *
 * ---------------------------------------------------------------------------------- *
 * |        R15        |        RDI         |        RSI        |        RBX        | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |  0x100  |  0x104  |  0x108   |  0x10c  |  0x110  |  0x114  |  0x118  |  0x11c  | *
 * ---------------------------------------------------------------------------------- *
 * |        RBP        |       hidden       |        RIP        |        EXIT       | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |  0x120  |  0x124  |  0x128   |  0x12c  |  0x130  |  0x134  |  0x138  |  0x13c  | *
 * ---------------------------------------------------------------------------------- *
 * |                                 parameter area                                 | *
 * ---------------------------------------------------------------------------------- *
 * ---------------------------------------------------------------------------------- *
 * |  0x140  |  0x144  |  0x148   |  0x14c  |  0x150  |  0x154  |  0x158  |  0x15c  | *
 * ---------------------------------------------------------------------------------- *
 * |       NULL        |         FCTX       |        DATA       |       align       | *
 * ---------------------------------------------------------------------------------- *
 *                                                                                    *
 * ***********************************************************************************/

#ifdef SC_META_INCLUDED

.file	"jump_x86_64_ms_pe_gas.S"
.text
.p2align 4,,15
.globl	sc_jump_context
.def	sc_jump_context;	.scl	2;	.type	32;	.endef
.seh_proc	sc_jump_context
sc_jump_context:
.seh_endprologue

    pushq  %rcx  /* save hidden address of transport_t */

    pushq  %rbp  /* save RBP */
    pushq  %rbx  /* save RBX */
    pushq  %rsi  /* save RSI */
    pushq  %rdi  /* save RDI */
    pushq  %r15  /* save R15 */
    pushq  %r14  /* save R14 */
    pushq  %r13  /* save R13 */
    pushq  %r12  /* save R12 */

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* save current stack base */
    movq  0x08(%r10), %rax
    pushq  %rax
    /* save current stack limit */
    movq  0x10(%r10), %rax
    pushq  %rax
    /* save current deallocation stack */
    movq  0x1478(%r10), %rax
    pushq  %rax
    /* save fiber local storage */
    movq  0x20(%r10), %rax
    pushq  %rax

    /* preserve non-volatile xmm registers */
    movq  %rsp, %rax
    andq  $-16, %rax
    subq  $0xa8, %rsp

    movdqa  %xmm6, -0x10(%rax)   /* save xmm6 */
    movdqa  %xmm7, -0x20(%rax)   /* save xmm7 */
    movdqa  %xmm8, -0x30(%rax)   /* save xmm8 */
    movdqa  %xmm9, -0x40(%rax)   /* save xmm9 */
    movdqa  %xmm10, -0x50(%rax)  /* save xmm10 */
    movdqa  %xmm11, -0x60(%rax)  /* save xmm11 */
    movdqa  %xmm12, -0x70(%rax)  /* save xmm12 */
    movdqa  %xmm13, -0x80(%rax)  /* save xmm13 */
    movdqa  %xmm14, -0x90(%rax)  /* save xmm14 */
    movdqa  %xmm15, -0xa0(%rax)  /* save xmm15 */

    /* preserve RSP (pointing to context-data) in R9 */
    movq  %rsp, %r9

    /* restore RSP (pointing to context-data) from RDX */
    movq  %rdx, %rsp

    /* restore non-volatile xmm registers */
    addq  $0xa8, %rsp
    movq  %rsp, %rax
    andq  $-16, %rax

    movdqa  -0x10(%rax), %xmm6   /* restore xmm6 */
    movdqa  -0x20(%rax), %xmm7   /* restore xmm7 */
    movdqa  -0x30(%rax), %xmm8   /* restore xmm8 */
    movdqa  -0x40(%rax), %xmm9   /* restore xmm9 */
    movdqa  -0x50(%rax), %xmm10  /* restore xmm10 */
    movdqa  -0x60(%rax), %xmm11  /* restore xmm11 */
    movdqa  -0x70(%rax), %xmm12  /* restore xmm12 */
    movdqa  -0x80(%rax), %xmm13  /* restore xmm13 */
    movdqa  -0x90(%rax), %xmm14  /* restore xmm14 */
    movdqa  -0xa0(%rax), %xmm15  /* restore xmm15 */

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* restore fiber local storage */
    popq  %rax
    movq  %rax, 0x20(%r10)
    /* restore deallocation stack */
    popq  %rax
    movq  %rax, 0x1478(%r10)
    /* restore stack limit */
    popq  %rax
    movq  %rax, 0x10(%r10)
    /* restore stack base */
    popq  %rax
    movq  %rax, 0x8(%r10)

    popq  %r12  /* restore R12 */
    popq  %r13  /* restore R13 */
    popq  %r14  /* restore R14 */
    popq  %r15  /* restore R15 */
    popq  %rdi  /* restore RDI */
    popq  %rsi  /* restore RSI */
    popq  %rbx  /* restore RBX */
    popq  %rbp  /* restore RBP */

    popq  %rax  /* restore hidden address of transport_t */

    /* transport_t returned in RAX */
    /* return parent sc_context_sp_t */
    movq  %r9, (%rax)
    /* return data */
    movq  %r8, 0x8(%rax)

    /* transport_t as 1.arg of context-function */
    movq  %rax, %rcx

    /* return to return address */
    ret
.seh_endproc

.section .drectve
.ascii " -export:\"sc_jump_context\""

#endif /* SC_META_INCLUDED */
