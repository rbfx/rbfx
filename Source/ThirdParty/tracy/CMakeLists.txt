#
# Copyright (c) 2017-2019 Rokas Kupstys.
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

set (CMAKE_CXX_STANDARD 14)

file (GLOB_RECURSE COMMON_SOURCE_FILES common/*.cpp common/*.hpp common/*.h)
file (GLOB_RECURSE CLIENT_SOURCE_FILES client/*.cpp client/*.hpp client/*.h)
file (GLOB_RECURSE SERVER_SOURCE_FILES server/*.cpp server/*.hpp server/*.h imguicolortextedit/*.cpp imguicolortextedit/*.h)

list (REMOVE_ITEM COMMON_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/TracySystem.cpp)
set (TRACY_OPTIONS_PUBLIC -DTRACY_ENABLE=1 -DTRACY_ON_DEMAND=1 -DTRACY_FILESELECTOR=1 -DTRACY_EXTENDED_FONT=1)
set (TRACY_OPTIONS_PRIVATE -DNOMINMAX=1 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -D_USE_MATH_DEFINES)

if (BUILD_SHARED_LIBS)
    # A workaround library. Usually each dll or executable that wishes to use profiling functionality would have to compile
    # TracyClientDLL.cpp as part of it's sources. Putting it into a static library allows us automating the process and it is
    # now enough to only link to Urho3D target.
    add_library (tracy-lib STATIC TracyClientDLL.cpp)
    target_compile_definitions (tracy-lib PUBLIC ${TRACY_OPTIONS_PUBLIC} -DTRACY_IMPORTS=1 PRIVATE ${TRACY_OPTIONS_PRIVATE} -DTRACY_EXPORTS=1)
else ()
    list (REMOVE_ITEM COMMON_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/TracySocket.cpp)
    add_library (tracy-lib INTERFACE)
    target_compile_definitions (tracy-lib INTERFACE ${TRACY_OPTIONS_PUBLIC})
endif ()

if (URHO3D_TOOLS AND URHO3D_SYSTEMUI)
    # Set up common parameters of profiler target
    macro (setup_tracy_profiler_target TARGET)
        target_include_directories(${TARGET} PRIVATE server)
        target_link_libraries(${TARGET} Urho3D IconFontCppHeaders)
        target_compile_definitions(${TARGET} PUBLIC ${TRACY_OPTIONS_PUBLIC} PRIVATE ${TRACY_OPTIONS_PRIVATE})
        if (MSVC)
            target_compile_options(${TARGET} PRIVATE /permissive-)
        endif ()
    endmacro ()

    file (GLOB_RECURSE TRACY_PROFILER_SOURCE_FILES profiler/src/*.cpp)
    add_executable (tracy-profiler WIN32 ${SERVER_SOURCE_FILES} ${COMMON_SOURCE_FILES} ${TRACY_PROFILER_SOURCE_FILES})
    target_compile_definitions(tracy-profiler PUBLIC -DTRACY_ROOT_WINDOW=1)
    set_target_properties(tracy-profiler PROPERTIES OUTPUT_NAME Profiler)
    setup_tracy_profiler_target(tracy-profiler)
    install (TARGETS tracy-profiler
        LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
        RUNTIME DESTINATION ${DEST_BIN_DIR}
        ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
    )

    # Lib for editor to easily consume profiler
    add_library (tracy-embedded-profiler STATIC ${SERVER_SOURCE_FILES} ${COMMON_SOURCE_FILES})
    setup_tracy_profiler_target(tracy-embedded-profiler)
    target_compile_definitions(tracy-embedded-profiler PRIVATE -DTRACY_EMBED_WINDOW=1)
    target_include_directories(tracy-embedded-profiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

install (TARGETS tracy-lib EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
