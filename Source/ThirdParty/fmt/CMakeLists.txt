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
# fmt is patched to use eastl::string instead of std::string. When upgrading the library do following:
# 1. Apply core.patch to fmt/core.h
# 2. Run following commands:
#   find . -name "*.h" -o -name "*.cc" | xargs sed -i "s/std::string/std_string/"
#   find . -name "*.h" -o -name "*.cc" | xargs sed -i "s/std::wstring/std_wstring/"
#   find . -name "*.h" -o -name "*.cc" | xargs sed -i "s/std::basic_string/std_basic_string/"
# 3. Some std::runtime_error() require extra `.c_str()` because this function consumes `std::string`.
# 4. to_string_view needs to lose Traits template parameter because EASTL does not support it.
#
include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()
# Define the fmt library, its includes and the needed defines.
# *.cc are added to FMT_HEADERS for the header-only configuration.
file(GLOB FMT_HEADERS fmt/*.h)
set(FMT_SOURCES fmt/format.cc)
if (HAVE_OPEN)
  set(FMT_HEADERS ${FMT_HEADERS} fmt/posix.h)
  set(FMT_SOURCES ${FMT_SOURCES} fmt/posix.cc)
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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

if (NOT MINI_URHO)
    install (DIRECTORY fmt DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS fmt EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
