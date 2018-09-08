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
   - 2016: Reserving space for the parameter area in the unwind area, as well
           as adding a NULL return address for sc_make_context so debuggers can
           reasonably know they've reached the top. There unfortunately doesn't
           seem to be a way to tell gdb that it reached the top (it just uses
           symbol files), but at least an address of 0x0000000000000000 is
           quite a bit more obvious than a random address.
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

.file	"make_x86_64_ms_pe_gas.S"
.text
.p2align 4,,15
.globl	sc_make_context
.def	sc_make_context;	.scl	2;	.type	32;	.endef
.seh_proc	sc_make_context
sc_make_context:
.seh_stackalloc 0x20
.seh_endprologue

    /* first arg of sc_make_context() == top of context-stack */
    movq  %rcx, %rax

    /* shift address in RAX to lower 16 byte boundary */
    /* == pointer to sc_context_sp_t and address of context stack */
    andq  $-16, %rax

    /* reserve space for context-data on context-stack */
    /* EXIT will be used as the return address for the context-function and */
    /* must have its end be 16-byte aligned */

    /* 160 bytes xmm storage, 8+8 bytes alignment, 176 bytes stack data */
    subq  $0x160, %rax

    /* third arg of sc_make_context() == address of context-function */
    movq  %r8, 0x110(%rax)

    /* first arg of sc_make_context() == top of context-stack */
    /* save top address of context stack as 'base' */
    movq  %rcx, 0xc0(%rax)
    /* second arg of sc_make_context() == size of context-stack */
    /* negate stack size for LEA instruction (== substraction) */
    negq  %rdx
    /* compute bottom address of context stack (limit) */
    leaq  (%rcx,%rdx), %rcx
    /* save bottom address of context stack as 'limit' */
    movq  %rcx, 0xb8(%rax)
    /* save address of context stack limit as 'dealloction stack' */
    movq  %rcx, 0xb0(%rax)
	/* set fiber-storage to zero */
    xorq  %rcx, %rcx
    movq  %rcx, 0xa8(%rax)

    /* zero out sc_make_context's return address (rcx is still zero) */
    movq  %rcx, 0x140(%rax)

    /* compute address of transport_t */
    leaq  0x148(%rax), %rcx
    /* store address of transport_t in hidden field */
    movq %rcx, 0x108(%rax)

    /* compute abs address of label finish */
    leaq  finish(%rip), %rcx
    /* save address of finish as return-address for context-function */
    /* will be entered after context-function returns */
    movq  %rcx, 0x118(%rax)

    ret /* return pointer to context-data */

finish:
    /* exit code is zero */
    xorq  %rcx, %rcx
    /* exit application */
    call  _exit
    hlt
.seh_endproc

.def	_exit;	.scl	2;	.type	32;	.endef  /* standard C library function */

.section .drectve
.ascii " -export:\"sc_make_context\""

#endif /* SC_META_INCLUDED */
