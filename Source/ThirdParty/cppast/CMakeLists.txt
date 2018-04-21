# Copyright (C) 2017-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.3)
project(cppast VERSION 0.0)

# options
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(default_assertions ON)
else()
    set(default_assertions OFF)
endif()
option(CPPAST_ENABLE_ASSERTIONS "whether or not to enable internal assertions for the cppast library" ${default_assertions})
option(CPPAST_ENABLE_PRECONDITION_CHECKS "whether or not to enable precondition checks" ON)

include(external/external.cmake)

if(build_test AND CPPAST_TEST_GCOV AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
endif()

add_subdirectory(src)
