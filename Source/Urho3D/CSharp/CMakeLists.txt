#
# Copyright (c) 2018 Rokas Kupstys.
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

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

set(CMAKE_SWIG_FLAGS
    -v
    -namespace Urho3DNet
    -debug-tmused
    -debug-tmsearch
)

set (SWIG_SYSTEM_INCLUDE_DIRS "${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_SYSTEM_INCLUDE_PATH};${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS}")
string (REPLACE ";" ";-I" SWIG_SYSTEM_INCLUDE_DIRS "${SWIG_SYSTEM_INCLUDE_DIRS}")


set_source_files_properties(Urho3D.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(Urho3D.i PROPERTIES SWIG_FLAGS "-I${SWIG_SYSTEM_INCLUDE_DIRS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(../..)

swig_add_module(Urho3DCSharp csharp Urho3D.i)
swig_link_libraries(Urho3DCSharp Urho3D)
set_target_properties(Urho3DCSharp PROPERTIES PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
