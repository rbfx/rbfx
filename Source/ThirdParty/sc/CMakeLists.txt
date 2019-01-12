# ---
#  Copyright (c) 2016 Johan Sköld
#  License: https://opensource.org/licenses/ISC
# ---

cmake_minimum_required(VERSION 3.2)
project(sc)
enable_language(C CXX)


#
# Compiler options
#

if(MSVC)
    enable_language(ASM_MASM)
else()
    enable_language(ASM)
endif()

# For OSX, CMake already handles passing the appropriate `-arch` parameters
# for C/C++, but we need to do it ourselves for assembly files. Due to this
# bug however we must apply it globally through `CMAKE_ASM_FLAGS`:
# https://cmake.org/Bug/view.php?id=15826

if(CMAKE_OSX_ARCHITECTURES)
    string(REPLACE ";" " -arch " SC_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch ${SC_OSX_ARCHITECTURES}")
endif()

if(CMAKE_OSX_SYSROOT)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT}")
endif()

#
# sc
#

add_library(sc STATIC
    include/sc/sc.h
    src/context.h
    src/context.c
    src/sc.c)

if(MSVC)
    target_sources(sc PRIVATE
        src/asm/jump_masm.asm
        src/asm/make_masm.asm)

    if(CMAKE_CL_64 EQUAL 0)
        set_source_files_properties(src/asm/jump_masm.asm src/asm/make_masm.asm src/asm/state_masm.asm PROPERTIES
            COMPILE_FLAGS "/safeseh /DSC_WIN32")
    else()
        set_source_files_properties(src/asm/jump_masm.asm src/asm/make_masm.asm src/asm/state_masm.asm PROPERTIES
            COMPILE_FLAGS "/DSC_WIN64")
    endif()
else()
    target_sources(sc PRIVATE
        src/asm/jump_gas.S
        src/asm/make_gas.S)
endif()

target_compile_options(sc
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
        $<$<C_COMPILER_ID:AppleClang>:-Wall -Werror>
        $<$<C_COMPILER_ID:MSVC>:/W3 /WX>)

target_include_directories(sc
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
