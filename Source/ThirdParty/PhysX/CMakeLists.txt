#
# Copyright (c) 2017-2021 the rbfx project.
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

set (CMAKEMODULES_VERSION 1.0)
set (CMAKEMODULES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/externals/cmakemodules)
set (PHYSX_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/physx)
set (PXSHARED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pxshared)
set (NV_USE_GAMEWORKS_OUTPUT_DIRS OFF)
set (PUBLIC_RELEASE OFF)
set (PX_COPY_EXTERNAL_DLL OFF)
set (PX_GENERATE_STATIC_LIBRARIES ON)
set (NV_USE_DEBUG_WINCRT ON)
add_compile_definitions(PX_PHYSX_CORE_EXPORTS=1 PX_PHYSX_FOUNDATION_EXPORTS=1 PX_PHYSX_COMMON_EXPORTS)
if (WEB OR MINGW)
    set (PX_SCALAR_MATH ON)
endif ()
if (URHO3D_64BIT)
    set (CMAKE_CL_64 1)
else ()
    set (CMAKE_CL_64 0)
endif ()
if (UWP)
    set (TARGET_BUILD_PLATFORM "uwp")
elseif (WIN32)
    set (TARGET_BUILD_PLATFORM "windows")
elseif (IOS)
    set (TARGET_BUILD_PLATFORM "ios")
elseif (APPLE)
    set (TARGET_BUILD_PLATFORM "mac")
elseif (ANDROID)
    set (TARGET_BUILD_PLATFORM "android")
elseif (LINUX)
    set (TARGET_BUILD_PLATFORM "linux")
else ()
    message (FATAL_ERRROR "Unsupported platform.")
endif ()

# Override install() function so physx build scripts do not do any header installation.
function (install)
endfunction ()

add_subdirectory (physx/compiler/public)

if (NOT MINI_URHO)
    _install(DIRECTORY physx/include DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/PhysX FILES_MATCHING PATTERN *.h)

    if (NOT URHO3D_MERGE_STATIC_LIBS)
        _install(TARGETS PhysX PhysXCommon PhysXFoundation PhysXCooking PhysXVehicle PhysXExtensions PhysXCharacterKinematic PhysXPvdSDK EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
