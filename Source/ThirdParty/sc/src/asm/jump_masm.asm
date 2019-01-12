; Copyright (c) 2016 Johan Sköld
; License: https://opensource.org/licenses/ISC
;

IFDEF SC_WIN64
    INCLUDE jump_x86_64_ms_pe_masm.asm
ENDIF

IFDEF SC_WIN32
    INCLUDE jump_i386_ms_pe_masm.asm
ENDIF

END