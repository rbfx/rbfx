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
.globl _sc_jump_context
.align 2
_sc_jump_context:
    pushl  %ebp  /* save EBP */
    pushl  %ebx  /* save EBX */
    pushl  %esi  /* save ESI */
    pushl  %edi  /* save EDI */

    /* The IA-32 ABI causes 8 byte structs to be returned split between EAX
       and EDX. For us, this means the parent sc_context_sp_t should be in EAX
       and the data in EDX. */

    /* store current sc_context_sp_t in EAX */
    movl  %esp, %eax

    /* first arg of sc_jump_context() == context jumping to */
    movl  0x14(%esp), %ecx

    /* second arg of sc_jump_context() == data to be transferred */
    movl  0x18(%esp), %edx

    /* restore ESP (pointing to context-data) from ECX */
    movl  %ecx, %esp

    popl  %edi  /* restore EDI */
    popl  %esi  /* restore ESI */
    popl  %ebx  /* restore EBX */
    popl  %ebp  /* restore EBP */

    /* jump to context */
    ret

#endif /* SC_META_INCLUDED */
