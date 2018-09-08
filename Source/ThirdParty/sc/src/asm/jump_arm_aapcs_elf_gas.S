/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*******************************************************
 *                                                     *
 *  -------------------------------------------------  *
 *  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  *
 *  -------------------------------------------------  *
 *  | 0x0 | 0x4 | 0x8 | 0xc | 0x10| 0x14| 0x18| 0x1c|  *
 *  -------------------------------------------------  *
 *  |hiddn|  v1 |  v2 |  v3 |  v4 |  v5 |  v6 |  v7 |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
 *  -------------------------------------------------  *
 *  | 0x20| 0x24| 0x28| 0x2c| 0x30| 0x34| 0x38| 0x3c|  *
 *  -------------------------------------------------  *
 *  |  v8 |  lr |  pc | FCTX| DATA|                 |  *
 *  -------------------------------------------------  *
 *                                                     *
 *******************************************************/

#ifdef SC_META_INCLUDED

.text
.globl sc_jump_context
.align 2
.type sc_jump_context,%function
sc_jump_context:
    @ save LR as PC
    push {lr}
    @ save hidden,V1-V8,LR
    push {a1,v1-v8,lr}

    @ store RSP (pointing to context-data) in A1
    mov  a1, sp

    @ restore RSP (pointing to context-data) from A2
    mov  sp, a2

    @ restore hidden,V1-V8,LR
    pop {a4,v1-v8,lr}

    @ return sc_transfer_t from jump
    str  a1, [a4, #0]
    str  a3, [a4, #4]
    @ pass sc_transfer_t as first arg in context function
    @ A1 == FCTX, A2 == DATA
    mov  a2, a3

    @ restore PC
    pop {pc}
.size sc_jump_context,.-sc_jump_context

@ Mark that we don't need executable stack.
.section .note.GNU-stack,"",%progbits

#endif /* SC_META_INCLUDED */
