/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

#define SC_META_INCLUDED

#if defined(_WIN32)

    #if defined(__i386__)
        #include "jump_i386_ms_pe_gas.S"
    #elif defined(__x86_64__)
        #include "jump_x86_64_ms_pe_gas.S"
    #else
        #error "Unsupported architecture"
    #endif

#elif defined(__APPLE__)

    #if defined(__i386__)
        #include "jump_i386_sysv_macho_gas.S"
    #elif defined(__x86_64__)
        #include "jump_x86_64_sysv_macho_gas.S"
    #elif defined(__arm__)
        #include "jump_arm_aapcs_macho_gas.S"
    #elif defined(__arm64__)
        #include "jump_arm64_aapcs_macho_gas.S"
    #else
        #error "Unsupported architecture"
    #endif

#else

    #if defined(__i386__)
        #include "jump_i386_sysv_elf_gas.S"
    #elif defined(__x86_64__)
        #include "jump_x86_64_sysv_elf_gas.S"
    #elif defined(__arm__)
        #include "jump_arm_aapcs_elf_gas.S"
    #elif defined(__arm64__) || defined(__aarch64__)
        #include "jump_arm64_aapcs_elf_gas.S"
    #elif defined(__mips32__)
        #include "jump_mips32_o32_elf_gas.S"
    #else
        #error "Unsupported architecture"
    #endif

#endif

#undef SC_META_INCLUDED
