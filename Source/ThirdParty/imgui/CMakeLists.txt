#
# Copyright (c) 2017 the Atomic project.
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

# When syncing repository with upstream be sure to delete stb_rectpack.h. This will make ImGui use main instance of
# stb_rectpack already included in the engine.

file (GLOB SOURCE_FILES *.cpp *.h)
list (APPEND SOURCE_FILES backends/imgui_impl_sdl.h backends/imgui_impl_sdl.cpp)

add_library (ImGui STATIC ${SOURCE_FILES})
target_link_libraries(ImGui PUBLIC SDL2-static STB FreeType EASTL)
target_compile_definitions(ImGui PRIVATE -DSDL_EXPORTS) # A workaround for MinGW to not try to import SDL APIs from a static library that exports them.
target_compile_definitions(ImGui PUBLIC -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    PRIVATE -DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION=1
    PUBLIC  -DIMGUI_DEFINE_MATH_OPERATORS=1 -DUSE_IMGUI_API=1)
target_include_directories(ImGui SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ImGuizmo>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backends>
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(ImGui PRIVATE -DIMGUI_EXPORTS -DEASTL_EXPORTS -DEASTDC_EXPORTS PUBLIC -DIMGUI_IMPORTS -DGLEW_BUILD=1)
endif ()

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS ImGui EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
