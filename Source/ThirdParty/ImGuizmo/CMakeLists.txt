#
# Copyright (c) 2024 the rbfx project.
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

file (GLOB SOURCE_FILES *.cpp *.h)

add_library (ImGuizmo STATIC ${SOURCE_FILES})
target_link_libraries(ImGuizmo PUBLIC ImGui)
target_include_directories(ImGuizmo SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/ImGuizmo>
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(ImGuizmo PRIVATE -DIMGUI_EXPORTS)# -DEASTL_EXPORTS -DEASTDC_EXPORTS PUBLIC -DIMGUI_IMPORTS -DGLEW_BUILD=1)
endif ()

install (DIRECTORY ./ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ImGuizmo/ FILES_MATCHING PATTERN *.h)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS ImGuizmo EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
