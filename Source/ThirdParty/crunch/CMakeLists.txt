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

add_library(crn_decomp INTERFACE)
target_include_directories(crn_decomp INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if (NOT MINI_URHO)
    install (TARGETS crn_decomp EXPORT Urho3D)
    install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
endif ()

if (NOT URHO3D_TOOLS)
    return ()
endif ()

file (GLOB CRNLIB_SOURCE_FILES crnlib/*.cpp crnlib/*.h include/crunch/*.h include/crunch/*.hpp)
foreach (path ${CRNLIB_SOURCE_FILES})
    if (WIN32)
        if ("${path}" MATCHES ".*_pthreads\\..*")
            list (REMOVE_ITEM CRNLIB_SOURCE_FILES ${path})
        endif ()
    else ()
        if ("${path}" MATCHES ".*_win(32|hdr)\\..*")
            list (REMOVE_ITEM CRNLIB_SOURCE_FILES ${path})
        endif ()
    endif ()
endforeach()

if (NOT WIN32)
    list (REMOVE_ITEM CRNLIB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/crnlib/lzma_Threads.cpp)
    list (REMOVE_ITEM CRNLIB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/crnlib/lzma_LzFindMt.cpp)
endif ()

add_library (crnlib STATIC ${CRNLIB_SOURCE_FILES})
target_include_directories(crnlib SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/crunch>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/crnlib>
)
if (WIN32)
    target_compile_options(crnlib PRIVATE -DCOMPRESS_MF_MT=1)
endif ()

if (NOT WIN32 OR MINGW)
    target_compile_options(crnlib PRIVATE -O3 -fomit-frame-pointer -ffast-math -fno-math-errno -fno-strict-aliasing -Wall -Wno-unused-value -Wno-unused -march=core2)
    target_link_libraries(crnlib pthread)
endif ()

if (MINGW AND NOT URHO3D_64BIT)
    target_compile_options(crnlib PRIVATE -fpermissive)
endif ()

file (GLOB CRUNCH_SOURCE_FILES crunch/*.cpp crunch/*.h)
add_executable (crunch ${CRUNCH_SOURCE_FILES})
target_link_libraries(crunch crnlib)
set_target_properties(crunch PROPERTIES FOLDER Tools)

if (NOT MINI_URHO)
    install(TARGETS crunch RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG})
endif ()
