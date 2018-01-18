#
# Copyright (c) 2008-2018 the Urho3D project.
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

# Set compiler variable
set ("${CMAKE_CXX_COMPILER_ID}" ON)

# Configure variables
set (URHO3D_URL "https://github.com/urho3d/Urho3D")
set (URHO3D_DESCRIPTION "Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D (http://www.horde3d.org).")
execute_process (COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE URHO3D_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED "${URHO3D_VERSION}")

# Setup SDK install destinations
if (WIN32)
    set (SCRIPT_EXT .bat)
else ()
    set (SCRIPT_EXT .sh)
endif ()
if (ANDROID)
    # For Android platform, install to a path based on the chosen Android ABI, e.g. libs/armeabi-v7a
    set (LIB_SUFFIX s/${ANDROID_NDK_ABI_NAME})
endif ()
set (DEST_INCLUDE_DIR include/Urho3D)
set (DEST_BIN_DIR bin)
set (DEST_SHARE_DIR share)
set (DEST_RESOURCE_DIR ${DEST_BIN_DIR})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
set (DEST_LIBRARY_DIR lib${LIB_SUFFIX})
set (DEST_PKGCONFIG_DIR lib${LIB_SUFFIX}/pkgconfig)
set (DEST_THIRDPARTY_HEADERS ${DEST_INCLUDE_DIR}/ThirdParty)
if (WIN32)
    set (SHARED_LIB_INSTALL_DIR bin)
else ()
    set (SHARED_LIB_INSTALL_DIR lib${LIB_SUFFIX})
endif ()
# Configuration step will try to create symlinks in this dir. It must exist.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_THIRDPARTY_HEADERS})
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR})
endif ()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${SHARED_LIB_INSTALL_DIR})
endif ()
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${SHARED_LIB_INSTALL_DIR})
endif ()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DURHO3D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DURHO3D_DEBUG")
if (NOT DEFINED URHO3D_64BIT)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(URHO3D_64BIT ON)
    else ()
        set(URHO3D_64BIT OFF)
    endif ()
endif ()

# Configure for web
if (EMSCRIPTEN)
    # Emscripten-specific setup
    if (EMSCRIPTEN_EMCC_VERSION VERSION_LESS 1.31.3)
        message(FATAL_ERROR "Unsupported compiler version")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    if (URHO3D_THREADING)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
    endif ()
    set (CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
    set (CMAKE_CXX_FLAGS_RELEASE "-Oz -DNDEBUG")
    # Remove variables to make the -O3 regalloc easier, embed data in asm.js to reduce number of moving part
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    set (CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    # Preserve LLVM debug information, show line number debug comments, and generate source maps; always disable exception handling codegen
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
    set (CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
endif ()

# Macro for setting symbolic link on platform that supports it
macro (create_symlink SOURCE DESTINATION)
    # Make absolute paths so they work more reliably on cmake-gui
    if (IS_ABSOLUTE ${SOURCE})
        set (ABS_SOURCE ${SOURCE})
    else ()
        set (ABS_SOURCE ${CMAKE_SOURCE_DIR}/${SOURCE})
    endif ()
    if (IS_ABSOLUTE ${DESTINATION})
        set (ABS_DESTINATION ${DESTINATION})
    else ()
        set (ABS_DESTINATION ${CMAKE_BINARY_DIR}/${DESTINATION})
    endif ()
    if (CMAKE_HOST_WIN32)
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_D /D)
        else ()
            unset (SLASH_D)
        endif ()
        if(${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 6.0)
            set (HAS_MKLINK ON)
        endif ()
        if (HAS_MKLINK)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET)
            endif ()
        elseif ("${ARGN}" STREQUAL FALLBACK_TO_COPY)
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            execute_process (${COMMAND})
            if (TARGET ${TARGET_NAME})
                # Fallback to copy every time the target is built
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD ${COMMAND})
            endif ()
        else ()
            message (WARNING "Unable to create symbolic link on this host system, you may need to manually copy file/dir from \"${SOURCE}\" to \"${DESTINATION}\"")
        endif ()
    else ()
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${ABS_SOURCE} ${ABS_DESTINATION})
    endif ()
endmacro ()

macro (add_sample TARGET)
    file (GLOB SOURCE_FILES *.cpp *.h)
    if (ANDROID)
        add_library(${TARGET} SHARED ${SOURCE_FILES})
    else ()
        add_executable (${TARGET} WIN32 ${SOURCE_FILES})
    endif ()
    target_link_libraries (${TARGET} Urho3D)
    target_include_directories(${TARGET} PRIVATE ..)
    set_target_properties(${TARGET} PROPERTIES FOLDER Samples)
    if (NOT ANDROID)
        install(TARGETS ${TARGET} RUNTIME DESTINATION ${DEST_BIN_DIR})
    endif ()

    if (EMSCRIPTEN)
        add_dependencies(${TARGET} pkg_resources_web)
        set_target_properties (${TARGET} PROPERTIES SUFFIX .html)
        if (EMSCRIPTEN_MEMORY_LIMIT)
            math(EXPR EMSCRIPTEN_TOTAL_MEMORY "${EMSCRIPTEN_MEMORY_LIMIT} * 1024 * 1024")
            target_link_libraries(${TARGET} "-s TOTAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY}")
        endif ()
        if (EMSCRIPTEN_MEMORY_GROWTH)
            target_link_libraries(${TARGET} "-s ALLOW_MEMORY_GROWTH=1")
        endif ()
        target_link_libraries(${TARGET} "-s NO_EXIT_RUNTIME=1" "-s WASM=1" "--pre-js ${CMAKE_BINARY_DIR}/Source/pak-loader.js")
    endif ()
endmacro ()

# Macro deploys Qt dlls to folder where target executable is located.
# Macro arguments:
#  target - name of target which links to Qt.
macro(deploy_qt_dlls target)
    if (WIN32)
        if (TARGET ${target})
            # Find Qt dir
            foreach (dir ${CMAKE_PREFIX_PATH})
                if (EXISTS ${dir}/bin/windeployqt.exe)
                    set(windeployqt "${dir}/bin/windeployqt.exe")
                    break()
                endif ()
            endforeach ()
            if (windeployqt)
                add_custom_command(
                    TARGET ${target}
                    POST_BUILD
                    COMMAND "${windeployqt}" --no-quick-import --no-translations --no-system-d3d-compiler --no-compiler-runtime --no-webkit2 --no-angle --no-opengl-sw $<TARGET_FILE_DIR:${target}>
                    VERBATIM
                )
            endif ()
        endif ()
    endif ()
endmacro()

macro(group_sources curdir)
    if (WIN32)
        file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${curdir} ${CMAKE_CURRENT_SOURCE_DIR}/${curdir}/*)
        foreach (child ${children})
            if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${curdir}/${child})
                if ("${curdir}" STREQUAL "")
                    group_sources(${child})
                else ()
                    group_sources(${curdir}/${child})
                endif ()
            else ()
                string(REPLACE "/" "\\" groupname ${curdir})
                source_group(${groupname} FILES ${CMAKE_CURRENT_SOURCE_DIR}/${curdir}/${child})
            endif ()
        endforeach ()
    endif ()
endmacro()

# Macro for object libraries to "link" to other object libraries. Macro does not actually link them, only pulls in
# interface include directories and defines. Macro operates under assumption that all the object library dependencies
# will eventually end up in one compiled target.
macro (target_link_objects TARGET)
    get_target_property(_MAIN_TARGET_TYPE ${TARGET} TYPE)
    foreach (link_to ${ARGN})
        if (TARGET ${link_to})
            add_dependencies(${TARGET} ${link_to})
            get_target_property(_TARGET_TYPE ${link_to} TYPE)
            if(_TARGET_TYPE STREQUAL "OBJECT_LIBRARY")
                get_target_property(LINK_LIBRARIES ${link_to} OBJECT_LINK_LIBRARIES)
                if (LINK_LIBRARIES)
                    target_link_objects(${TARGET} ${LINK_LIBRARIES})
                endif ()

                get_target_property(INCLUDE_DIRECTORIES ${link_to} INTERFACE_INCLUDE_DIRECTORIES)
                if (INCLUDE_DIRECTORIES)
                    get_target_property(TARGET_INCLUDE_DIRECTORIES ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
                    foreach(dir ${INCLUDE_DIRECTORIES})
                        list(FIND TARGET_INCLUDE_DIRECTORIES "${dir}" CONTAINS)
                        if (${CONTAINS} EQUAL -1)
                            target_include_directories(${TARGET} PUBLIC ${INCLUDE_DIRECTORIES})
                        endif ()
                    endforeach()
                endif ()
                get_target_property(COMPILE_DEFINITIONS ${link_to} INTERFACE_COMPILE_DEFINITIONS)
                if (COMPILE_DEFINITIONS)
                    get_target_property(TARGET_INTERFACE_COMPILE_DEFINITIONS ${TARGET} INTERFACE_COMPILE_DEFINITIONS)
                    foreach(def ${COMPILE_DEFINITIONS})
                        list(FIND TARGET_INTERFACE_COMPILE_DEFINITIONS "${def}" CONTAINS)
                        if (${CONTAINS} EQUAL -1)
                            target_compile_definitions(${TARGET} PUBLIC ${COMPILE_DEFINITIONS})
                        endif ()
                    endforeach()
                endif ()
                get_target_property(LINK_LIBRARIES ${TARGET} OBJECT_LINK_LIBRARIES)
                if (NOT LINK_LIBRARIES)
                    unset (LINK_LIBRARIES)
                endif ()
                list (APPEND LINK_LIBRARIES "${link_to}")
                list (REMOVE_DUPLICATES LINK_LIBRARIES)
                set_target_properties(${TARGET} PROPERTIES OBJECT_LINK_LIBRARIES "${LINK_LIBRARIES}")
            elseif (NOT _MAIN_TARGET_TYPE STREQUAL "OBJECT_LIBRARY")
                target_link_libraries(${TARGET} ${link_to})
            endif ()
        else ()
            target_link_libraries(${TARGET} ${link_to})
        endif ()
    endforeach()
endmacro()

macro (object_target_link_libraries TARGET)
    get_target_property(LINK_LIBRARIES ${TARGET} OBJECT_LINK_LIBRARIES)
    if (NOT LINK_LIBRARIES)
        unset (LINK_LIBRARIES)
    endif ()
    foreach (link ${ARGN})
        list (APPEND LINK_LIBRARIES ${link})
    endforeach()
    list(REMOVE_DUPLICATES LINK_LIBRARIES)
    set_target_properties(${TARGET} PROPERTIES OBJECT_LINK_LIBRARIES "${LINK_LIBRARIES}")
endmacro ()



# Macro for defining source files with optional arguments as follows:
#  GLOB_CPP_PATTERNS <list> - Use the provided globbing patterns for CPP_FILES instead of the default *.cpp
#  GLOB_H_PATTERNS <list> - Use the provided globbing patterns for H_FILES instead of the default *.h
#  EXCLUDE_PATTERNS <list> - Use the provided regex patterns for excluding the unwanted matched source files
#  EXTRA_CPP_FILES <list> - Include the provided list of files into CPP_FILES result
#  EXTRA_H_FILES <list> - Include the provided list of files into H_FILES result
#  PCH <list> - Enable precompiled header support on the defined source files using the specified header file, the list is "<path/to/header> [C++|C]"
#  RECURSE - Option to glob recursively
#  GROUP - Option to group source files based on its relative path to the corresponding parent directory
macro (define_source_files)
    # Source files are defined by globbing source files in current source directory and also by including the extra source files if provided
    cmake_parse_arguments (ARG "RECURSE;GROUP" "" "PCH;EXTRA_CPP_FILES;EXTRA_H_FILES;GLOB_CPP_PATTERNS;GLOB_H_PATTERNS;EXCLUDE_PATTERNS" ${ARGN})
    if (NOT ARG_GLOB_CPP_PATTERNS)
        set (ARG_GLOB_CPP_PATTERNS *.cpp)    # Default glob pattern
    endif ()
    if (NOT ARG_GLOB_H_PATTERNS)
        set (ARG_GLOB_H_PATTERNS *.h)
    endif ()
    if (ARG_RECURSE)
        set (ARG_RECURSE _RECURSE)
    else ()
        unset (ARG_RECURSE)
    endif ()
    file (GLOB${ARG_RECURSE} CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_CPP_PATTERNS})
    file (GLOB${ARG_RECURSE} H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_H_PATTERNS})
    if (ARG_EXCLUDE_PATTERNS)
        set (CPP_FILES_WITH_SENTINEL ";${CPP_FILES};")  # Stringify the lists
        set (H_FILES_WITH_SENTINEL ";${H_FILES};")
        foreach (PATTERN ${ARG_EXCLUDE_PATTERNS})
            foreach (LOOP RANGE 1)
                string (REGEX REPLACE ";${PATTERN};" ";;" CPP_FILES_WITH_SENTINEL "${CPP_FILES_WITH_SENTINEL}")
                string (REGEX REPLACE ";${PATTERN};" ";;" H_FILES_WITH_SENTINEL "${H_FILES_WITH_SENTINEL}")
            endforeach ()
        endforeach ()
        set (CPP_FILES ${CPP_FILES_WITH_SENTINEL})      # Convert strings back to lists, extra sentinels are harmless
        set (H_FILES ${H_FILES_WITH_SENTINEL})
    endif ()
    list (APPEND CPP_FILES ${ARG_EXTRA_CPP_FILES})
    list (APPEND H_FILES ${ARG_EXTRA_H_FILES})
    set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
    # Optionally enable PCH
    if (ARG_PCH)
        enable_pch (${ARG_PCH})
    endif ()
    # Optionally group the sources based on their physical subdirectories
    if (ARG_GROUP)
        foreach (CPP_FILE ${CPP_FILES})
            get_filename_component (PATH ${CPP_FILE} PATH)
            if (PATH)
                string (REPLACE / \\ PATH ${PATH})
                source_group ("Source Files\\${PATH}" FILES ${CPP_FILE})
            endif ()
        endforeach ()
        foreach (H_FILE ${H_FILES})
            get_filename_component (PATH ${H_FILE} PATH)
            if (PATH)
                string (REPLACE / \\ PATH ${PATH})
                source_group ("Header Files\\${PATH}" FILES ${H_FILE})
            endif ()
        endforeach ()
    endif ()
endmacro ()







# Configure for MingW
if (CMAKE_CROSSCOMPILING AND MINGW)
    # Symlink windows libraries and headers to appease some libraries that do not use all-lowercase names and break on
    # case-sensitive file systems.
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/workaround)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/windows.h workaround/Windows.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/shobjidl.h workaround/ShObjIdl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/strsafe.h workaround/Strsafe.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/psapi.h workaround/Psapi.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/sddl.h workaround/Sddl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/accctrl.h workaround/AccCtrl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/aclapi.h workaround/Aclapi.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/oleidl.h workaround/OleIdl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/shlobj.h workaround/Shlobj.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libws2_32.a workaround/libWs2_32.a)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libiphlpapi.a workaround/libIphlpapi.a)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libwldap32.a workaround/libWldap32.a)
    include_directories(${CMAKE_BINARY_DIR}/workaround)
    link_libraries(-L${CMAKE_BINARY_DIR}/workaround)
endif ()
