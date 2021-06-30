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
if (WIN32 AND NOT URHO3D_OPENGL)
    if (NOT UWP)
	    list (APPEND SOURCE_FILES backends/imgui_impl_win32.h backends/imgui_impl_win32.cpp)
    endif ()
    if (URHO3D_D3D11)
        list (APPEND SOURCE_FILES backends/imgui_impl_dx11.h backends/imgui_impl_dx11.cpp)
    else ()
        list (APPEND SOURCE_FILES backends/imgui_impl_dx9.h backends/imgui_impl_dx9.cpp)
    endif ()
else ()
    list (APPEND SOURCE_FILES backends/imgui_impl_opengl3.h backends/imgui_impl_opengl3.cpp)
endif ()

add_library (ImGui STATIC ${SOURCE_FILES})
target_link_libraries(ImGui PUBLIC SDL STB FreeType EASTL)
if (URHO3D_OPENGL)
    if (MOBILE)
        # Mobiles do not need a loader. Lets game Dear ImGui here.
        if (URHO3D_GLES2)
            target_compile_definitions(ImGui PUBLIC -DIMGUI_IMPL_OPENGL_ES2=1)
        elseif (URHO3D_GLES3)
            target_compile_definitions(ImGui PUBLIC -DIMGUI_IMPL_OPENGL_ES3=1)
        else ()
            message (FATAL_ERROR "Graphics API misconfiguration.")
        endif ()
        if (APPLE)
            target_compile_definitions(ImGui PUBLIC -DTARGET_OS_IOS=1)
        endif ()
    else ()
        target_link_libraries(ImGui PUBLIC GLEW)
        target_compile_definitions(ImGui PUBLIC -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM="GL/glew.h")
    endif ()
else ()
    target_link_libraries(ImGui PUBLIC xinput)
endif ()
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
