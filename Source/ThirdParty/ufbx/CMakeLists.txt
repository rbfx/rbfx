# Copyright (c) 2026 the rbfx project.
# This work is licensed under the terms of the MIT license.

add_library(ufbx STATIC ufbx.c)
set_target_properties(ufbx PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
target_compile_definitions(ufbx PRIVATE UFBX_ENABLE_ERROR_STACK=1)
target_include_directories(ufbx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(FILES ufbx.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT ThirdParty)
install_third_party_libs(ufbx)
