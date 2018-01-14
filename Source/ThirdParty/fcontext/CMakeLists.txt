# PROJECT: fcontext
cmake_minimum_required(VERSION 3.0)
project(fcontext)

if (MSVC)
    enable_language(CXX ASM_MASM)
else()
    enable_language(CXX ASM)
endif()

# OS
if (UNIX)
    if (ANDROID)
        # Android
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
            set(CPU_ARCH "arm")
            set(ASM_EXT "aapcs_elf_gas.S")
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
            set(CPU_ARCH "arm64")
            set(ASM_EXT "aapcs_elf_gas.S")
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686")
            set(CPU_ARCH "i386")
            set(ASM_EXT "sysv_elf_gas.S")
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
            set(CPU_ARCH "x86_64")
            set(ASM_EXT "sysv_elf_gas.S")
        endif()
    else()
        # PC (x86/x64)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CPU_ARCH "x86_64")
        else()
            set(CPU_ARCH "i386")
        endif()
        if (APPLE)
            set(ASM_EXT "sysv_macho_gas.S") # OSX
        else()
            set(ASM_EXT "sysv_elf_gas.S")  # Linux/Unix
        endif()
    endif()
elseif(WIN32)
    # Windows PC
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPU_ARCH "x86_64")
    else()
        set(CPU_ARCH "i386")
    endif()
    if (MSVC)
        set(ASM_EXT "ms_pe_masm.asm")
    else ()
        set(ASM_EXT "ms_pe_gas.S")
    endif ()
endif()

set(ASM_SOURCES "asm/make_${CPU_ARCH}_${ASM_EXT}"
                "asm/jump_${CPU_ARCH}_${ASM_EXT}"
                "asm/ontop_${CPU_ARCH}_${ASM_EXT}")

add_library(fcontext OBJECT include/fcontext/fcontext.h source/stack.cpp ${ASM_SOURCES})
target_compile_definitions(fcontext PRIVATE -DBOOST_CONTEXT_EXPORT=)
target_include_directories(fcontext PRIVATE include/fcontext INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS}/fcontext>)


install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS} FILES_MATCHING PATTERN *.h)
