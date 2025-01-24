# Based on original work by David Manura
# Copyright (C) 2007-2012 LuaDist.
# Copyright (C) 2013 Brian Sidebotham

# Redistribution and use of this file is allowed according to the terms of the
# MIT license.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# In order to update compiled source file lists in "ssl" and "crypto" subdirs, configure OpenSSL and look at object
# files to be compiled at the end of generated Makefile in root OpenSSL source dir.
# Example OpenSSL configuration:
#   ./Configure no-asm linux-x86_64
#   ./config

project(openssl)
cmake_minimum_required(VERSION 3.0)

include(CheckTypeSize)
check_type_size("long" LONG_INT)
check_type_size("long long" LONG_LONG_INT)
check_type_size("int" INT)
if("${LONG_INT}" EQUAL 8)
    set(SIXTY_FOUR_BIT_LONG ON )
elseif("${LONG_LONG_INT}" EQUAL 8)
    set(SIXTY_FOUR_BIT ON)
else()
    set(THIRTY_TWO_BIT ON)
endif()
if (MSVC OR (WIN32 AND MINGW AND NOT CYGWIN))
    set(OPENSSL_EXPORT_VAR_AS_FUNCTION 1)
endif()
configure_file(include/openssl/opensslconf.h.in ${PROJECT_BINARY_DIR}/include/openssl/opensslconf.h)
configure_file(include/crypto/bn_conf.h.in ${PROJECT_BINARY_DIR}/include/openssl/crypto/bn_conf.h)
configure_file(include/crypto/dso_conf.h.in ${PROJECT_BINARY_DIR}/include/openssl/crypto/dso_conf.h)
file(COPY buildinf.h DESTINATION ${PROJECT_BINARY_DIR}/include/openssl/)

if (MINGW)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all" )
endif()

add_subdirectory(crypto)
add_subdirectory(ssl)
file(COPY e_os.h DESTINATION ${PROJECT_BINARY_DIR}/include/)

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
install (DIRECTORY ${PROJECT_BINARY_DIR}/include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS ssl crypto EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
