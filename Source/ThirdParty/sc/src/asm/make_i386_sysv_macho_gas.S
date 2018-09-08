/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*
   Updated by Johan Sköld for sc (https://github.com/rhoot/sc)

   - 2016: Updated to conform with Apple's IA-32 ABI; 8-byte return values
           should be returned in eax/edx, not on the stack.
*/

/*****************************************************************************************
 *                                                                                       *
 *  -----------------------------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |    6     |    7    |  *
 *  -----------------------------------------------------------------------------------  *
 *  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |   0x18   |   0x1c  |  *
 *  -----------------------------------------------------------------------------------  *
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   EIP    |  hidden |    to    |   data  |  *
 *  -----------------------------------------------------------------------------------  *
 *                                                                                       *
 *****************************************************************************************/

#ifdef SC_META_INCLUDED

.text
.globl _sc_make_context
.align 2
_sc_make_context:
    /* first arg of sc_make_context() == top of context-stack */
    movl  0x4(%esp), %eax

    /* shift address in EAX to lower 16 byte boundary */
    andl  $-16, %eax

    /* reserve space for context-data on context-stack */
    leal  -0x28(%eax), %eax

    /* thrid arg of sc_make_context() == address of context-function */
    /* stored in EBX */
    movl  0xc(%esp), %edx
    movl  %edx, 0x8(%eax)

    /* compute abs address of label trampoline */
    call  1f
    /* address of trampoline 1 */
1:  popl  %ecx
    /* compute abs address of label trampoline */
    addl  $trampoline-1b, %ecx
    /* save address of trampoline as return address */
    /* will be entered after calling sc_jump_context() first time */
    movl  %ecx, 0x10(%eax)

    /* compute abs address of label finish */
    call  2f
    /* address of label 2 */
2:  popl  %ecx
    /* compute abs address of label finish */
    addl  $finish-2b, %ecx
    /* save address of finish as return-address for context-function */
    /* will be entered after context-function returns */
    movl  %ecx, 0xc(%eax)

    ret /* return pointer to context-data */

trampoline:
    /* move transport_t from the return value from sc_jump_context, to be passed
       as parameter when entering the context-function */
    movl  %eax, (%esp)
    movl  %edx, 0x4(%esp)
    pushl %ebp
    /* jump to context-function */
    jmp *%ebx

finish:
    /* 16-bit alignment required for call */
    andl  $-16, %esp
    /* exit code is zero */
    xorl  %eax, %eax
    movl  %eax, (%esp)
    /* exit application */
    call  __exit
    hlt

#endif /* SC_META_INCLUDED */
