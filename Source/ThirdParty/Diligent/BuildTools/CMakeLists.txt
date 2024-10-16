cmake_minimum_required (VERSION 3.6)

add_subdirectory(File2Include)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(CLANG_FORMAT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/FormatValidation/clang-format_10.0.0.exe" CACHE INTERNAL "clang-format executable path")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CLANG_FORMAT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/FormatValidation/clang-format_linux_10.0.0" CACHE INTERNAL "clang-format executable path")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CLANG_FORMAT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/FormatValidation/clang-format_mac_10.0.0" CACHE INTERNAL "clang-format executable path")
endif()
