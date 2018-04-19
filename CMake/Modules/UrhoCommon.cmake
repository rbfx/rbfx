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
set (DEST_BASE_INCLUDE_DIR include)
set (DEST_INCLUDE_DIR ${DEST_BASE_INCLUDE_DIR}/Urho3D)
set (DEST_BIN_DIR bin)
set (DEST_TOOLS_DIR ${DEST_BIN_DIR})
set (DEST_SAMPLES_DIR ${DEST_BIN_DIR})
set (DEST_SHARE_DIR share)
set (DEST_RESOURCE_DIR ${DEST_BIN_DIR})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
set (DEST_ARCHIVE_DIR lib${LIB_SUFFIX})
set (DEST_PKGCONFIG_DIR ${DEST_ARCHIVE_DIR}/pkgconfig)
set (DEST_THIRDPARTY_HEADERS_DIR ${DEST_INCLUDE_DIR}/ThirdParty)
if (ANDROID)
    set (DEST_LIBRARY_DIR ${DEST_ARCHIVE_DIR})
else ()
    set (DEST_LIBRARY_DIR bin)
endif ()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_ARCHIVE_DIR})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DURHO3D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DURHO3D_DEBUG")
if (NOT DEFINED URHO3D_64BIT)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(URHO3D_64BIT ON)
    else ()
        set(URHO3D_64BIT OFF)
    endif ()
endif ()

if (MINGW)
    find_file(DLL_FILE_PATH_1 "libstdc++-6.dll")
    find_file(DLL_FILE_PATH_2 "libgcc_s_seh-1.dll")
    find_file(DLL_FILE_PATH_3 "libwinpthread-1.dll")
    foreach (DLL_FILE_PATH ${DLL_FILE_PATH_1} ${DLL_FILE_PATH_2} ${DLL_FILE_PATH_3})
        if (DLL_FILE_PATH)
            # Copies dlls to bin or tools.
            file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_TOOLS_DIR})
            if (NOT URHO3D_STATIC_RUNTIME)
                file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_SAMPLES_DIR})
            endif ()
        endif ()
    endforeach ()
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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CLANG ON)
    set (GNU ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (GCC ON)
    set (GNU ON)
endif()

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
        set (RESULT_CODE 1)
        if(${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 6.0)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE RESULT_CODE)
            endif ()
        endif ()
        if (NOT "${RESULT_CODE}" STREQUAL "0")
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            if (NOT EXISTS ${ABS_DESTINATION})
                execute_process (${COMMAND})
            endif ()
        endif ()
    else ()
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${ABS_SOURCE} ${ABS_DESTINATION})
    endif ()
endmacro ()

macro (add_sample TARGET)
    if ("${ARGN}" STREQUAL CSHARP)
        if (DESKTOP)    # TODO: Support other platforms
            file (GLOB SOURCE_FILES *.cs)
            if (MSVC)
                set (URHO3DNET_LIBRARY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/$<CONFIG>/Urho3DNet.dll)
            else ()
                set (URHO3DNET_LIBRARY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/Urho3DNet.dll)
            endif ()
            # Managed executable must find Urho3DCSharp and Urho3DNet in the same directory, that is why it is put to DEST_BIN_DIR
            # instead of samples directory.
            set (OUTPUT_FILE ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/${TARGET}.exe)
            add_target_csharp(102_CSharpProject /target:exe /reference:System.dll
                /reference:${URHO3DNET_LIBRARY} /out:${OUTPUT_FILE}
                ${SOURCE_FILES}
            )
            add_dependencies(${TARGET} Urho3DNet)
            install(FILES ${OUTPUT_FILE} DESTINATION ${DEST_SAMPLES_DIR})
        endif ()
    else ()
        file (GLOB SOURCE_FILES *.cpp *.h)
        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        if (ANDROID)
            add_library(${TARGET} SHARED ${SOURCE_FILES})
        else ()
            add_executable (${TARGET} ${TARGET_TYPE} ${SOURCE_FILES})
        endif ()
        target_link_libraries (${TARGET} Urho3D)
        target_include_directories(${TARGET} PRIVATE ..)
        set_target_properties(${TARGET} PROPERTIES FOLDER Samples)
        if (NOT ANDROID)
            install(TARGETS ${TARGET} RUNTIME DESTINATION ${DEST_SAMPLES_DIR})
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

# Groups sources into subfolders.
macro(group_sources)
    file (GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/**)
    foreach (child ${children})
        if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
            string(REPLACE "/" "\\" groupname "${child}")
            file (GLOB files LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*)
            source_group(${groupname} FILES ${files})
        endif ()
    endforeach ()
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
            if ("${_MAIN_TARGET_TYPE}" STREQUAL "OBJECT_LIBRARY")
                get_target_property(LINK_LIBRARIES ${TARGET} OBJECT_LINK_LIBRARIES)
                if (NOT LINK_LIBRARIES)
                    unset (LINK_LIBRARIES)
                endif ()
                foreach (link ${ARGN})
                    list (APPEND LINK_LIBRARIES ${link})
                endforeach()
                list(REMOVE_DUPLICATES LINK_LIBRARIES)
                set_target_properties(${TARGET} PROPERTIES OBJECT_LINK_LIBRARIES "${LINK_LIBRARIES}")
            else ()
                target_link_libraries(${TARGET} ${link_to})
            endif ()
        endif ()
    endforeach()
endmacro()

macro (initialize_submodule SUBMODULE_DIR)
    file(GLOB SUBMODULE_FILES ${SUBMODULE_DIR}/*)
    list(LENGTH SUBMODULE_FILES SUBMODULE_FILES_LEN)

    if (SUBMODULE_FILES_LEN LESS 2)
        find_program(GIT git)
        if (NOT GIT)
            message(FATAL_ERROR "git not found.")
        endif ()
        execute_process(
            COMMAND git submodule update --init --remote "${SUBMODULE_DIR}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE SUBMODULE_RESULT
        )
        if (NOT SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to initialize submodule ${SUBMODULE_DIR}")
        endif ()
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
