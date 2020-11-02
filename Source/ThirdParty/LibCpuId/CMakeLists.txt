#
# Copyright (c) 2008-2020 the Urho3D project.
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

# Define generated source files
if (MSVC AND URHO3D_64BIT)
    enable_language (ASM_MASM)
    set (ASM_FILES src/masm-x64.asm)
endif ()

# Define source files
file (GLOB_RECURSE SOURCE_FILES *.c *.h)
list (APPEND SOURCE_FILES ${ASM_FILES})

add_library (LibCpuId STATIC ${SOURCE_FILES})
target_compile_definitions (LibCpuId PRIVATE -DVERSION="0.4.0")
if (HAVE_STDINT_H)
    target_compile_definitions (LibCpuId PRIVATE -DHAVE_STDINT_H)
endif ()

target_include_directories(LibCpuId
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/LibCpuId>
)

if (NOT MINI_URHO)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS LibCpuId EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
