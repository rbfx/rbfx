# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

include(${CMAKE_CURRENT_LIST_DIR}/ucm.cmake)

find_program(CCACHE ccache)
if (NOT MSVC AND CCACHE_FOUND AND ENV{CCACHE_DIR})  # MSVC side is handled in ci_build.sh action-build-msvc()
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

if (MSVC AND (DEFINED ENV{CCACHE_DIR} AND NOT ENV{CCACHE_DIR} STREQUAL ""))
    ucm_replace_flag("/Z[iI]" "" REGEX CONFIG Debug RelWithDebInfo)
    ucm_replace_flag("/FI"    "" REGEX)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif ()
