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

file (GLOB SOURCE_FILES *.cpp *.h)
if (BUILD_SHARED_LIBS)
    if (URHO3D_CSHARP OR ANDROID OR IOS)
        # Use C# executable to host .net runtime and load editor.
        set (CMAKE_SHARED_LIBRARY_PREFIX lib)
        add_library (Player SHARED ${SOURCE_FILES})
    else ()
        # Dynamic builds may use player as executable. It will load user code.
        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        add_executable (Player ${TARGET_TYPE} ${SOURCE_FILES})
    endif ()
else ()
    # Static builds should consume player library and provide implementation of `InitializeUserPlugins` function that
    # loads other statically linked plugins.
    add_library(Player STATIC ${SOURCE_FILES})
    target_include_directories(Player PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
target_link_libraries(Player Urho3D PlayerCommon)
if (TARGET cr)
    target_link_libraries(Player cr)
endif ()
install(TARGETS Player
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)
