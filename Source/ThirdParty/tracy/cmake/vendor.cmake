# Vendor Specific CMake
# The Tracy project keeps most vendor source locally

set (ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../")

# Dependencies are taken from the system first and if not found, they are pulled with CPM and built from source

#include(FindPkgConfig)
include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

option(DOWNLOAD_CAPSTONE "Force download capstone" ON)
option(DOWNLOAD_GLFW "Force download glfw" OFF)
option(DOWNLOAD_FREETYPE "Force download freetype" OFF)

# capstone - Using bundled version from rbfx
# TracyCapstone is already provided by Source/ThirdParty/capstone/CMakeLists.txt

# GLFW

if(NOT USE_WAYLAND AND NOT EMSCRIPTEN AND NOT TARGET TracyGlfw3) # rbfx
    pkg_check_modules(GLFW glfw3)
    if (GLFW_FOUND AND NOT DOWNLOAD_GLFW)
        add_library(TracyGlfw3 INTERFACE)
        target_include_directories(TracyGlfw3 INTERFACE ${GLFW_INCLUDE_DIRS})
        target_link_libraries(TracyGlfw3 INTERFACE ${GLFW_LINK_LIBRARIES})
    else()
        CPMAddPackage(
            NAME glfw
            GITHUB_REPOSITORY glfw/glfw
            GIT_TAG 3.4
            OPTIONS
                "GLFW_BUILD_EXAMPLES OFF"
                "GLFW_BUILD_TESTS OFF"
                "GLFW_BUILD_DOCS OFF"
                "GLFW_INSTALL OFF"
        )
        add_library(TracyGlfw3 INTERFACE)
        target_link_libraries(TracyGlfw3 INTERFACE glfw)
    endif()
endif()

# freetype
if (NOT TARGET TracyFreetype)	# rbfx
pkg_check_modules(FREETYPE freetype2)
if (FREETYPE_FOUND AND NOT DOWNLOAD_FREETYPE)
    add_library(TracyFreetype INTERFACE)
    target_include_directories(TracyFreetype INTERFACE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(TracyFreetype INTERFACE ${FREETYPE_LINK_LIBRARIES})
else()
    CPMAddPackage(
        NAME freetype
        GITHUB_REPOSITORY freetype/freetype
        GIT_TAG VER-2-13-3
        OPTIONS
            "FT_DISABLE_HARFBUZZ ON"
            "FT_WITH_HARFBUZZ OFF"
        EXCLUDE_FROM_ALL TRUE
    )
    add_library(TracyFreetype INTERFACE)
    target_link_libraries(TracyFreetype INTERFACE freetype)
endif()

endif() # rbfx TracyFreetype

# Zstd - Using bundled version from rbfx
# libzstd is already provided by Source/ThirdParty/zstd/CMakeLists.txt

# Diff Template Library

set(DTL_DIR "${ROOT_DIR}/dtl")
file(GLOB_RECURSE DTL_HEADERS CONFIGURE_DEPENDS RELATIVE ${DTL_DIR} "*.hpp")
add_library(TracyDtl INTERFACE)
target_sources(TracyDtl INTERFACE ${DTL_HEADERS})
target_include_directories(TracyDtl INTERFACE ${DTL_DIR})

# Get Opt

set(GETOPT_DIR "${ROOT_DIR}/getopt")
set(GETOPT_SOURCES ${GETOPT_DIR}/getopt.c)
set(GETOPT_HEADERS ${GETOPT_DIR}/getopt.h)
add_library(TracyGetOpt STATIC EXCLUDE_FROM_ALL ${GETOPT_SOURCES} ${GETOPT_HEADERS})
target_include_directories(TracyGetOpt PUBLIC ${GETOPT_DIR})

# ImGui

if (NOT TARGET TracyImGui)	# rbfx
CPMAddPackage(
    NAME ImGui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.91.9b-docking
    DOWNLOAD_ONLY TRUE
    PATCHES
        "${CMAKE_CURRENT_LIST_DIR}/imgui-emscripten.patch"
        "${CMAKE_CURRENT_LIST_DIR}/imgui-loader.patch"
)

set(IMGUI_SOURCES
    imgui_widgets.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui.cpp
    imgui_tables.cpp
    misc/freetype/imgui_freetype.cpp
    backends/imgui_impl_opengl3.cpp
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${ImGui_SOURCE_DIR}/")

add_library(TracyImGui STATIC EXCLUDE_FROM_ALL ${IMGUI_SOURCES})
target_include_directories(TracyImGui PUBLIC ${ImGui_SOURCE_DIR})
target_link_libraries(TracyImGui PUBLIC TracyFreetype)
target_compile_definitions(TracyImGui PRIVATE "IMGUI_ENABLE_FREETYPE")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(TracyImGui PRIVATE "IMGUI_DISABLE_DEBUG_TOOLS")
endif()

endif()		# rbfx

# NFD

if(NOT NO_FILESELECTOR AND NOT EMSCRIPTEN AND NOT TARGET nfd)	# rbfx
    if(GTK_FILESELECTOR)
        set(NFD_PORTAL OFF)
    else()
        set(NFD_PORTAL ON)
    endif()

    CPMAddPackage(
        NAME nfd
        GITHUB_REPOSITORY btzy/nativefiledialog-extended
        GIT_TAG v1.2.1
        EXCLUDE_FROM_ALL TRUE
        OPTIONS
            "NFD_PORTAL ${NFD_PORTAL}"
    )
endif()

# PPQSort - Using bundled version from rbfx
# PPQSort::PPQSort is already provided by Source/ThirdParty/ppqsort/CMakeLists.txt
