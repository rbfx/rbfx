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

file (GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    src/*.h
    include/*.h
    3rdparty/fcpp/cpp*.c
    3rdparty/fcpp/*.h
    3rdparty/forsyth-too/*.cpp
    3rdparty/forsyth-too/*.h
    3rdparty/ib-compress/*.cpp
    3rdparty/ib-compress/*.h
)
list (REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.mm)
add_library(bgfx STATIC ${SOURCE_FILES})
target_link_libraries(bgfx bimg)

if (WIN32)
    target_link_libraries(bgfx dbghelp psapi)
endif ()

target_include_directories(bgfx PUBLIC include
    PRIVATE
    3rdparty
    3rdparty/dxsdk/include
    3rdparty/freetype
    3rdparty/khronos
)

install (DIRECTORY include/bgfx/ DESTINATION ${DEST_THIRDPARTY_HEADERS}/bgfx FILES_MATCHING PATTERN *.h)
install_to_build_tree(bgfx)

# Tools

# geometryc
add_executable(geometryc tools/geometryc/geometryc.cpp examples/common/bounds.cpp)
target_link_libraries(geometryc bgfx ${CMAKE_DL_LIBS})
target_include_directories(geometryc PRIVATE 3rdparty examples/common)
set_target_properties(geometryc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}")
install(TARGETS geometryc RUNTIME DESTINATION bin)

# shaderc
file(GLOB_RECURSE GLSL_LANG_SOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    3rdparty/glslang/glslang/*.cpp
    3rdparty/glslang/glslang/*.h
    3rdparty/glslang/hlsl/*.cpp
    3rdparty/glslang/hlsl/*.h
    3rdparty/glslang/SPIRV/*.cpp
    3rdparty/glslang/SPIRV/*.h
    3rdparty/glslang/OGLCompilersDLL/*.cpp
    3rdparty/glslang/OGLCompilersDLL/*.h
)
list(REMOVE_ITEM GLSL_LANG_SOURCE_FILES 3rdparty/glslang/glslang/OSDependent/Unix/main.cpp)
list(REMOVE_ITEM GLSL_LANG_SOURCE_FILES 3rdparty/glslang/glslang/OSDependent/Windows/main.cpp)
if (WIN32)
    list(REMOVE_ITEM GLSL_LANG_SOURCE_FILES 3rdparty/glslang/glslang/OSDependent/Unix/ossource.cpp)
else ()
    list(REMOVE_ITEM GLSL_LANG_SOURCE_FILES 3rdparty/glslang/glslang/OSDependent/Windows/ossource.cpp)
    list(REMOVE_ITEM GLSL_LANG_SOURCE_FILES 3rdparty/glslang/glslang/OSDependent/Windows/main.cpp)
endif ()
add_library(glslang STATIC ${GLSL_LANG_SOURCE_FILES})
target_compile_options(glslang PRIVATE -DENABLE_HLSL=1)
target_include_directories(glslang PUBLIC 3rdparty/glslang)
if (MSVC)
    target_compile_options(glslang PRIVATE
        /wd4005    # warning C4005: '_CRT_SECURE_NO_WARNINGS': macro redefinition
        /wd4100    # error C4100: 'inclusionDepth' : unreferenced formal parameter
        /wd4127    # warning C4127: conditional expression is constant
        /wd4244    # warning C4244: '=': conversion from 'int' to 'char', possible loss of data
        /wd4456    # warning C4456: declaration of 'feature' hides previous local declaration
        /wd4457    # warning C4457: declaration of 'token' hides function parameter
        /wd4458    # warning C4458: declaration of 'language' hides class member
        /wd4702    # warning C4702: unreachable code
    )
else ()
    target_compile_options(glslang PRIVATE
        -Wno-deprecated-register
        -Wno-ignored-qualifiers
        -Wno-inconsistent-missing-override
        -Wno-missing-field-initializers
        -Wno-reorder
        -Wno-return-type
        -Wno-shadow
        -Wno-sign-compare
        -Wno-undef
        -Wno-unknown-pragmas
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif ()

file(GLOB_RECURSE GLSL_OPTIMIZER_SOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    3rdparty/glsl-optimizer/src/mesa/*.c
    3rdparty/glsl-optimizer/src/glsl/*.cpp
    3rdparty/glsl-optimizer/src/mesa/*.h
    3rdparty/glsl-optimizer/src/glsl/*.c
    3rdparty/glsl-optimizer/src/glsl/*.cpp
    3rdparty/glsl-optimizer/src/glsl/*.h
    3rdparty/glsl-optimizer/src/util/*.c
    3rdparty/glsl-optimizer/src/util/*.h
)
list(REMOVE_ITEM GLSL_OPTIMIZER_SOURCE_FILES 3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp.c)
list(REMOVE_ITEM GLSL_OPTIMIZER_SOURCE_FILES 3rdparty/glsl-optimizer/src/glsl/ir_set_program_inouts.cpp)
list(REMOVE_ITEM GLSL_OPTIMIZER_SOURCE_FILES 3rdparty/glsl-optimizer/src/glsl/main.cpp)
list(REMOVE_ITEM GLSL_OPTIMIZER_SOURCE_FILES 3rdparty/glsl-optimizer/src/glsl/builtin_stubs.cpp)
add_library(glsl-optimizer STATIC ${GLSL_OPTIMIZER_SOURCE_FILES})
target_include_directories(glsl-optimizer PRIVATE
    3rdparty/glsl-optimizer/src
    3rdparty/glsl-optimizer/include
    3rdparty/glsl-optimizer/src/mesa
    3rdparty/glsl-optimizer/src/mapi
    3rdparty/glsl-optimizer/src/glsl
)
if (MSVC)
    target_include_directories(glsl-optimizer PRIVATE 3rdparty/glsl-optimizer/src/glsl/msvc)
    target_compile_definitions(glsl-optimizer PRIVATE
        -D__STDC__
        -D__STDC_VERSION__=199901L
        -Dstrdup=_strdup
        -Dalloca=_alloca
        -Disascii=__isascii
    )
    target_compile_options(glsl-optimizer PRIVATE
        /wd4100    # error C4100: '' : unreferenced formal parameter
        /wd4127    # warning C4127: conditional expression is constant
        /wd4132    # warning C4132: 'deleted_key_value': const object should be initialized
        /wd4189    # warning C4189: 'interface_type': local variable is initialized but not referenced
        /wd4204    # warning C4204: nonstandard extension used: non-constant aggregate initializer
        /wd4244    # warning C4244: '=': conversion from 'const flex_int32_t' to 'YY_CHAR', possible loss of data
        /wd4389    # warning C4389: '!=': signed/unsigned mismatch
        /wd4245    # warning C4245: 'return': conversion from 'int' to 'unsigned int', signed/unsigned mismatch
        /wd4701    # warning C4701: potentially uninitialized local variable 'lower' used
        /wd4702    # warning C4702: unreachable code
        /wd4706    # warning C4706: assignment within conditional expression
        /wd4996    # warning C4996: 'strdup': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strdup.
    )
elseif(UNIX OR MINGW)
    target_compile_options(glsl-optimizer PRIVATE
        -fno-strict-aliasing   # glsl-optimizer has bugs if strict aliasing is used.
        -Wno-unused-parameter
    )
endif ()

file (GLOB SHADERC_SOURCE_FILES tools/shaderc/*.cpp)
add_executable(shaderc ${SHADERC_SOURCE_FILES})
target_link_libraries(shaderc glsl-optimizer glslang bgfx ${CMAKE_DL_LIBS})
if (UNIX)
    target_link_libraries(shaderc pthread)
endif ()
target_include_directories(shaderc PRIVATE
    3rdparty
    3rdparty/dxsdk/include
    3rdparty/fcpp
    3rdparty/glslang/glslang/Public
    3rdparty/glslang/glslang/Include
    3rdparty/glslang
    3rdparty/glsl-optimizer/include
    3rdparty/glsl-optimizer/src/glsl
)
set_target_properties(shaderc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}")
install(TARGETS shaderc RUNTIME DESTINATION bin)

