#
# Copyright (c) 2025 the rbfx project.
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
cmake_minimum_required(VERSION 3.14...3.22)

project(PPQSort LANGUAGES CXX)

add_library(PPQSort INTERFACE)
add_library(PPQSort::PPQSort ALIAS PPQSort)
set_target_properties(PPQSort PROPERTIES CXX_STANDARD 20 INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(PPQSort INTERFACE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

option(PPQSORT_OPTIMIZE_NATIVE "Build with -march=native" OFF)
if(PPQSORT_OPTIMIZE_NATIVE)
  target_compile_options(PPQSort INTERFACE "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-march=native>")
endif()

# Link against the standard library by default
option(PPQSORT_USE_OPENMP "Build with openmp, otherwise use native threads" OFF)
if(PPQSORT_USE_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(PPQSort INTERFACE OpenMP::OpenMP_CXX)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(PPQSort INTERFACE Threads::Threads)
endif()

target_include_directories(PPQSort INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
