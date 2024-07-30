#
# Copyright (c) 2024-2024 the rbfx project.
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

if (NOT DEFINED STEAMAUDIO_ROOT)
    set (STEAMAUDIO_ROOT "${CMAKE_SOURCE_DIR}/../steamaudio" CACHE PATH "Steam Audio SDK")
endif ()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH x86)
endif()

set(STEAMAUDIO_FOUND TRUE)
set(STEAMAUDIO_INCLUDE_DIRS "${STEAMAUDIO_ROOT}/include")
if (WIN32)
    set(STEAMAUDIO_LIBRARIES "${STEAMAUDIO_ROOT}/lib/windows-${ARCH}/phonon.lib")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(STEAMAUDIO_LIBRARIES "${STEAMAUDIO_ROOT}/lib/linux-${ARCH}/phonon.so")
elseif (ANDROID)
    set(ANDROID_ABI ${ANDROID_NDK_ABI_NAME})
    string(REPLACE "v(7|8)a" "v\\1" ANDROID_ABI "${ANDROID_ABI}")
    set(STEAMAUDIO_LIBRARIES "${STEAMAUDIO_ROOT}/lib/android-${ANDROID_ABI}/libphonon.so")
elseif (IOS)
    set(STEAMAUDIO_LIBRARIES "${STEAMAUDIO_ROOT}/lib/ios/libphonon.a")
elseif (APPLE)
    set(STEAMAUDIO_LIBRARIES "${STEAMAUDIO_ROOT}/lib/osx/libphonon.dylib")
else ()
    set(STEAMAUDIO_FOUND FALSE)
endif ()
