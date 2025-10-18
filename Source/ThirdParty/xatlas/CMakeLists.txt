#cmake_minimum_required(VERSION 2.8.8)
project(xatlas)

# Urho3D: Removed all extra things
add_library(xatlas STATIC xatlas.cpp)

install_third_party_libs(xatlas)

target_include_directories(xatlas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/xatlas.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT ThirdParty
)
