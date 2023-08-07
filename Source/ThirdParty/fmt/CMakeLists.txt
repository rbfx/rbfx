#
# Copyright (c) 2017-2022 the rbfx project.
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

include(CheckSymbolExists)
if (WIN32)
    check_symbol_exists(open io.h HAVE_OPEN)
else ()
    check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()
# Define the fmt library, its includes and the needed defines.
# *.cc are added to FMT_HEADERS for the header-only configuration.
file(GLOB FMT_HEADERS include/fmt/*.h)
set(FMT_SOURCES src/format.cc)
if (HAVE_OPEN)
    set(FMT_SOURCES ${FMT_SOURCES} src/os.cc)
endif ()

if (BUILD_SHARED_LIBS AND MINGW)
    add_library(fmt INTERFACE)
    set(FMT_PUBLIC INTERFACE)
else ()
    add_library(fmt STATIC ${FMT_HEADERS} ${FMT_SOURCES})
    set(FMT_PUBLIC PUBLIC)
endif ()

target_link_libraries(fmt ${FMT_PUBLIC} EASTL)
target_compile_definitions(fmt ${FMT_PUBLIC} FMT_USE_RVALUE_REFERENCES=1 FMT_USE_VARIADIC_TEMPLATES=1)
target_include_directories(fmt SYSTEM ${FMT_PUBLIC}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
)
if (BUILD_SHARED_LIBS)
    if (MINGW)
        target_compile_definitions(fmt ${FMT_PUBLIC} -DFMT_HEADER_ONLY)
    else ()
        target_compile_definitions(fmt ${FMT_PUBLIC} -DFMT_SHARED
            PRIVATE -DFMT_EXPORT -DEASTL_EXPORTS=1 -DEASTDC_EXPORTS=1
        )
    endif ()
endif ()

install (DIRECTORY include/fmt DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS fmt EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
