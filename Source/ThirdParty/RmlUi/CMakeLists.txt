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
project(RmlUi LANGUAGES C CXX VERSION 3.3)

set(RMLUI_VERSION_SHORT ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})


file(GLOB_RECURSE SOURCE_FILES Include/*.h Source/*.cpp)

file (GLOB_RECURSE SOURCE_FILES *.h *.cpp)
add_library(RmlUi STATIC ${SOURCE_FILES})
target_compile_definitions(RmlUi
    PRIVATE RMLUI_VERSION="${RMLUI_VERSION_SHORT}"
    PUBLIC  RMLUI_NO_THIRDPARTY_CONTAINERS=1
            RMLUI_CUSTOM_CONFIGURATION_FILE="../RbfxConfig.h"
            $<$<CONFIG:Debug>:RMLUI_DEBUG>
)
if (APPLE)
    # Disables naked builtins from AssertMacros.h which
    # This prevents naming collisions such as those from the check()
    # function macro with LuaType::check
    target_compile_definitions(RmlUi PRIVATE -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
endif ()
target_link_libraries(RmlUi PUBLIC FreeType EASTL)
target_include_directories(RmlUi SYSTEM
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    PUBLIC    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include/RmlUi>
              $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/RmlUi>
    PRIVATE   ../..
)

install (DIRECTORY Include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(RmlUi PRIVATE -DRmlCore_EXPORTS=1 -DRmlDebugger_EXPORTS=1 -DEASTL_EXPORTS -DEASTDC_EXPORTS)
else ()
    target_compile_definitions(RmlUi PUBLIC -DRMLUI_STATIC_LIB=1)
endif ()

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS RmlUi EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
