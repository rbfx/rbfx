#
# Copyright (c) 2008-2017 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
project(swig)

add_subdirectory(pcre)

check_include_file(inttypes.h     HAVE_INTTYPES_H)
check_include_file(memory.h     HAVE_MEMORY_H)
check_function_exists(popen HAVE_POPEN)
check_include_file(stdint.h     HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
set (HAVE_PCRE 1)
set (SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
set (SWIG_CXX "${CMAKE_CXX_COMPILER_ID}")
set (SWIG_LIB "${CMAKE_CURRENT_SOURCE_DIR}/Lib/")
set (SWIG_LIB_WIN_UNIX ${SWIG_LIB})
set(SWIG_PLATFORM "${CMAKE_SYSTEM_NAME}")
set (SWIG_LANG "-csharp")
set (PACKAGE_BUGREPORT void)
set (PACKAGE_NAME "Swig@Urho3D")
set (PACKAGE_VERSION "4.0.0")

configure_file(Source/Include/swigconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/swigconfig.h)

find_package(BISON)
if (BISON_FOUND)
  if ("${BISON_VERSION}" VERSION_GREATER_EQUAL "3.5.4")
    set (BISON_NO_COLORS "--color=no")
  endif ()
  bison_target(SWIG ${CMAKE_CURRENT_SOURCE_DIR}/Source/CParse/parser.y ${CMAKE_CURRENT_SOURCE_DIR}/Source/CParse/parser.c COMPILE_FLAGS "-y -d ${BISON_NO_COLORS}")
endif ()

file (GLOB_RECURSE SWIG_SOURCES Source/*.c Source/*.cxx Source/*.h)
add_executable(swig ${SWIG_SOURCES})
target_link_libraries(swig PUBLIC pcre)
target_include_directories(swig PRIVATE Source/CParse Source/DOH Source/Include Source/Modules Source/Preprocessor Source/Swig ${CMAKE_CURRENT_BINARY_DIR})

# A lame way to detect that target is built alone, with a native toolchain when corsscompiling.
if (NOT TARGET Urho3D)
    install (TARGETS swig RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG} BUNDLE DESTINATION ${DEST_BIN_DIR_CONFIG})
endif ()

file(TIMESTAMP Lib/swigwarn.swg SWIGWARN_SWG_TIMESTAMP "%s")
file(TIMESTAMP Source/Include/swigwarn.h SWIGWARN_H_TIMESTAMP "%s")

if ("${SWIGWARN_SWG_TIMESTAMP}" VERSION_LESS "${SWIGWARN_H_TIMESTAMP}" OR NOT EXISTS Lib/swigwarn.swg)
    file(WRITE Lib/swigwarn.swg "")
    file(READ Source/Include/swigwarn.h SWIGWARN_H)
    string(REPLACE "\r" "" SWIGWARN_H "${SWIGWARN_H}")
    string(REPLACE "\n" ";" SWIGWARN_H "${SWIGWARN_H}")
    foreach (ln ${SWIGWARN_H})
        if ("${ln}" MATCHES "#define WARN.*")
            string(REGEX REPLACE "#define (WARN[^\\r\\n]*[0-9]+)([^\\r\\n]*)" "%define SWIG\\1 %enddef\\2" ln "${ln}")
            file(APPEND Lib/swigwarn.swg "${ln}\n")
        endif ()
    endforeach ()
endif ()
