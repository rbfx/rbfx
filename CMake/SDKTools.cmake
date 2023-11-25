#
# Copyright (c) 2017-2023 the rbfx project.
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
# This file is included by rbfx build system when crosscompiling to targets that
# do not natively run on build host. This file helps crosscompiler to comsume
# tools built for the host system.
#
foreach (cfg MinSizeRel Release RelWithDebInfo Debug)
    set(SDK_TOOLS_CONFIG ${CMAKE_CURRENT_LIST_DIR}/SDKTools-${cfg}.cmake)
    if (EXISTS "${SDK_TOOLS_CONFIG}")
        include (${CMAKE_CURRENT_LIST_DIR}/SDKTools-${cfg}.cmake)
    endif ()
    if (TOOL_ANY_EXIST)
        message(STATUS "Using tools from ${cfg} SDK at ${URHO3D_SDK}")
        break()
    endif ()
endforeach ()

if (NOT TOOL_ANY_EXIST)
    message(WARNING "SDK tools were not found at ${URHO3D_SDK}!")
endif ()
