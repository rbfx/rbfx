#
# Mingw-w64 cross compiler toolchain
#
# - The usual CMAKE variables will point to the cross compiler
# - HOST_EXE_LINKER, HOST_C_COMPILER, HOST_EXE_LINKER_FLAGS,
#   HOST_C_FLAGS point to a host compiler
#
# Bsed on toolchain from neovim project

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)
if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
    set (CMAKE_SYSTEM_PROCESSOR $ENV{CMAKE_SYSTEM_PROCESSOR})
    if (NOT CMAKE_SYSTEM_PROCESSOR)
        set (CMAKE_SYSTEM_PROCESSOR x86_64)
    endif ()
endif ()

set(MINGW_TRIPLET ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)
set(MINGW_THREAD_MODEL posix)

# which compilers to use for C and C++
find_program(CMAKE_C_COMPILER   NAMES ${MINGW_TRIPLET}-gcc-${MINGW_THREAD_MODEL} ${MINGW_TRIPLET}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${MINGW_TRIPLET}-g++-${MINGW_THREAD_MODEL} ${MINGW_TRIPLET}-g++)
set(CMAKE_RC_COMPILER ${MINGW_TRIPLET}-windres)

# Where is the target environment located
set(CMAKE_FIND_ROOT_PATH "/usr/${MINGW_TRIPLET}")

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CROSS_TARGET ${MINGW_TRIPLET})

# We need a host compiler too - assuming mildly sane Unix
# defaults here
set(HOST_C_COMPILER cc)
set(HOST_EXE_LINKER ld)

if (MINGW_TRIPLET MATCHES "^x86_64")
    set(HOST_C_FLAGS -m64)
    set(HOST_EXE_LINKER_FLAGS -m64)
else()
    # In 32 bits systems have the HOST compiler generate 32 bits binaries
    set(HOST_C_FLAGS -m32)
    set(HOST_EXE_LINKER_FLAGS -m32)
endif()
