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

set(CMAKE_CXX_STANDARD 17)
if (URHO3D_PROFILING)
    file (GLOB_RECURSE COMMON_SOURCE_FILES common/*.cpp common/*.hpp common/*.h)
    file (GLOB_RECURSE CLIENT_SOURCE_FILES client/*.cpp client/*.hpp client/*.h)
    file (GLOB_RECURSE SERVER_SOURCE_FILES server/*.cpp server/*.hpp server/*.h imguicolortextedit/*.cpp imguicolortextedit/*.h zstd/*.c zstd/*.h)

    list (REMOVE_ITEM COMMON_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/TracySystem.cpp)
    set (TRACY_OPTIONS_PUBLIC -DTRACY_ENABLE=1 -DTRACY_ON_DEMAND=1 -DTRACY_FILESELECTOR=1 -DTRACY_EXTENDED_FONT=1)
    set (TRACY_OPTIONS_PRIVATE -DNOMINMAX=1 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -D_USE_MATH_DEFINES)

    if (NOT URHO3D_PROFILING_SYSTRACE)
        list (APPEND TRACY_OPTIONS_PUBLIC -DTRACY_NO_SYSTEM_TRACING=1)
    endif ()

    if (NOT DEFINED TRACY_NO_PARALLEL_ALGORITHMS AND APPLE)
        set (TRACY_NO_PARALLEL_ALGORITHMS ON)
    endif ()

    if (BUILD_SHARED_LIBS)
        # A workaround library. Usually each dll or executable that wishes to use profiling functionality would have to compile
        # TracyClientDLL.cpp as part of it's sources. Putting it into a static library allows us automating the process and it is
        # now enough to only link to Urho3D target.
        add_library (Tracy STATIC TracyClientDLL.cpp)
        target_compile_definitions (Tracy PUBLIC ${TRACY_OPTIONS_PUBLIC} -DTRACY_IMPORTS=1 PRIVATE ${TRACY_OPTIONS_PRIVATE} -DTRACY_EXPORTS=1)
        target_link_libraries(Tracy INTERFACE Urho3D)
    else ()
        list (REMOVE_ITEM COMMON_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/TracySocket.cpp)
        add_library (Tracy INTERFACE)
        target_compile_definitions (Tracy INTERFACE ${TRACY_OPTIONS_PUBLIC})
    endif ()

    find_package(TBB)
    if (TBB_FOUND)
        if (BUILD_SHARED_LIBS)
            target_link_libraries(Tracy PUBLIC ${TBB_IMPORTED_TARGETS})
        else ()
            target_link_libraries(Tracy INTERFACE ${TBB_IMPORTED_TARGETS})
        endif ()
    endif ()

    if (URHO3D_TOOLS AND URHO3D_SYSTEMUI)
        # Set up common parameters of profiler target
        macro (setup_tracy_profiler_target TARGET)
            target_include_directories(${TARGET} PRIVATE server)
            target_link_libraries(${TARGET} PUBLIC Urho3D IconFontCppHeaders)
            target_compile_definitions(${TARGET} PUBLIC ${TRACY_OPTIONS_PUBLIC} PRIVATE ${TRACY_OPTIONS_PRIVATE})
            if (MSVC)
                target_compile_options(${TARGET} PRIVATE /permissive-)
            endif ()
            if (TRACY_NO_PARALLEL_ALGORITHMS)
                target_compile_definitions(${TARGET} PUBLIC -DTRACY_NO_PARALLEL_ALGORITHMS=1)
            endif ()
            target_link_libraries (${TARGET} PRIVATE capstone-static)
        endmacro ()

        file (GLOB_RECURSE TRACY_PROFILER_SOURCE_FILES profiler/src/*.cpp)

        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        add_executable (TracyProfiler ${TARGET_TYPE} ${SERVER_SOURCE_FILES} ${TRACY_PROFILER_SOURCE_FILES})
        if (BUILD_SHARED_LIBS)
            # Profiler.cpp already includes these files, but types defined there are not exported from dll,
            # therefore inclusion of these files is not necessary in static builds, but required in shared builds.
            target_sources(TracyProfiler PRIVATE ${COMMON_SOURCE_FILES})
        endif ()
        target_include_directories(TracyProfiler PRIVATE .)
        set_target_properties(TracyProfiler PROPERTIES OUTPUT_NAME Profiler)
        setup_tracy_profiler_target(TracyProfiler)
        if (NOT MINI_URHO)
            install (TARGETS TracyProfiler
                LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
                RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
                ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
            )
        endif ()
        # Lib for editor to easily consume profiler
        add_library (TracyEmbedded STATIC ${SERVER_SOURCE_FILES} ${COMMON_SOURCE_FILES})
        setup_tracy_profiler_target(TracyEmbedded)
        target_include_directories(TracyEmbedded SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        target_compile_definitions(TracyEmbedded PRIVATE -DTRACY_NO_ROOT_WINDOW=1 -DTRACY_EMBED_WINDOW=1)
    endif ()

    if (NOT MINI_URHO)
        install (TARGETS Tracy EXPORT Urho3D
            LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
            RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
            ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
        )
    endif ()
endif ()

if (NOT MINI_URHO)
    # Headers are required even in non-profiled builds so we can keep all profiling macros.
    install (FILES Tracy.hpp TracyC.h TracyOpenGL.hpp DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/tracy/)
    foreach (dir common client)
        install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/tracy/${dir}/ FILES_MATCHING PATTERN *.h)
        install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/tracy/${dir}/ FILES_MATCHING PATTERN *.hpp)
    endforeach ()
endif ()
