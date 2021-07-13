#
# Copyright (c) 2017-2020 the rbfx project.
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

include(ucm)

find_program(CCACHE ccache)
if (CCACHE_FOUND AND ENV{CCACHE_DIR})
    if (CMAKE_GENERATOR STREQUAL "Xcode")
        if ("${CMAKE_C_COMPILER_LAUNCHER}" STREQUAL "ccache")
            if (NOT CCACHE)
                message(FATAL_ERROR "ccache not found.")
            endif ()
            file(WRITE "${CMAKE_BINARY_DIR}/ccache-cc" "#!/bin/sh\nexec \"${CCACHE}\" \"${CMAKE_C_COMPILER}\" \"$@\"")
            execute_process(COMMAND chmod a+rx "${CMAKE_BINARY_DIR}/ccache-cc")
            set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_BINARY_DIR}/ccache-cc" CACHE INTERNAL "")
            set(CMAKE_XCODE_ATTRIBUTE_LD "${CMAKE_BINARY_DIR}/ccache-cc" CACHE INTERNAL "")
        endif ()

        if ("${CMAKE_CXX_COMPILER_LAUNCHER}" STREQUAL "ccache")
            if (NOT CCACHE)
                message(FATAL_ERROR "ccache not found.")
            endif ()
            file(WRITE "${CMAKE_BINARY_DIR}/ccache-cxx" "#!/bin/sh\nexec \"${CCACHE}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"")
            execute_process(COMMAND chmod a+rx "${CMAKE_BINARY_DIR}/ccache-cxx")
            set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/ccache-cxx" CACHE INTERNAL "")
            set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/ccache-cxx" CACHE INTERNAL "")
        endif ()
    else ()
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
    endif ()
endif ()

if (MSVC AND (DEFINED ENV{CLCACHE_DIR} AND NOT ENV{CLCACHE_DIR} STREQUAL ""))
    ucm_replace_flag("/Z[iI]" "" REGEX CONFIG Debug RelWithDebInfo)
endif ()
