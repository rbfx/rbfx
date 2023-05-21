cmake_minimum_required (VERSION 3.19)

project(DiligentCore)

# Define GNU standard installation directories such as CMAKE_INSTALL_INCLUDEDIR, CMAKE_INSTALL_LIBDIR, etc.
include(GNUInstallDirs)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Generate XCode schemes
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
# Make malloc write 0xAA to newly allocated memory and 0x55 to deallocated memory
set(CMAKE_XCODE_SCHEME_MALLOC_SCRIBBLE YES)
# Place guard pages on each side of large (4096 bytes or more) buffers
set(CMAKE_XCODE_SCHEME_MALLOC_GUARD_EDGES YES)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE is not specified, default to Debug. Note that this is only relevant for single-configuration generators (such as Makefile Generators and Ninja).")
endif()

set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "Debug configurations")
set(RELEASE_CONFIGURATIONS RELEASE RELWITHDEBINFO MINSIZEREL CACHE INTERNAL "Release configurations")

if(BUILD_CONFIGURATION_FILE)
    message("Using build configuration file " ${CUSTOM_BUILD_SCRIPT})
    include(${CMAKE_SOURCE_DIR}/${BUILD_CONFIGURATION_FILE})

    if(COMMAND custom_configure_build)
        custom_configure_build()
    else()
        message("custom_configure_build() function not found in " ${CUSTOM_BUILD_SCRIPT})
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../DiligentCorePro")
    set(DILIGENT_CORE_PRO_EXISTS TRUE)
else()
    set(DILIGENT_CORE_PRO_EXISTS FALSE)
endif()


set(PLATFORM_WIN32             FALSE CACHE INTERNAL "")
set(PLATFORM_UNIVERSAL_WINDOWS FALSE CACHE INTERNAL "")
set(PLATFORM_ANDROID           FALSE CACHE INTERNAL "")
set(PLATFORM_LINUX             FALSE CACHE INTERNAL "")
set(PLATFORM_MACOS             FALSE CACHE INTERNAL "")
set(PLATFORM_IOS               FALSE CACHE INTERNAL "")
set(PLATFORM_TVOS              FALSE CACHE INTERNAL "")
set(PLATFORM_EMSCRIPTEN        FALSE CACHE INTERNAL "")
set(D3D11_SUPPORTED            FALSE CACHE INTERNAL "D3D11 is not supported")
set(D3D12_SUPPORTED            FALSE CACHE INTERNAL "D3D12 is not supported")
set(GL_SUPPORTED               FALSE CACHE INTERNAL "GL is not supported")
set(GLES_SUPPORTED             FALSE CACHE INTERNAL "GLES is not supported")
set(VULKAN_SUPPORTED           FALSE CACHE INTERNAL "Vulkan is not supported")
set(METAL_SUPPORTED            FALSE CACHE INTERNAL "Metal is not supported")
set(ARCHIVER_SUPPORTED         FALSE CACHE INTERNAL "Archiver is not supported")

set(DILIGENT_CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "DiligentCore module source directory")

set(CMAKE_OBJECT_PATH_MAX 4096)

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(ARCH 64 CACHE INTERNAL "64-bit architecture")
else()
    set(ARCH 32 CACHE INTERNAL "32-bit architecture")
endif()

# When not cross-compiling, CMAKE_SYSTEM_PROCESSOR has the same value as the CMAKE_HOST_SYSTEM_PROCESSOR variable.
# In many cases, this will correspond to the target architecture for the build, but this is not guaranteed.
# (E.g. on Windows, the host may be AMD64 even when using a MSVC cl compiler with a 32-bit target.)
# When cross-compiling, a CMAKE_TOOLCHAIN_FILE should set the CMAKE_SYSTEM_PROCESSOR variable to match target
# architecture that it specifies.
if(CMAKE_SYSTEM_PROCESSOR)
    if(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd") OR
       ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "X86") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD"))
        set(TARGET_CPU x86) # Emscripten
    elseif(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd64") OR
           ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "X86_64") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64"))
        # There are no real 32-bit x86 processors these days.
        # Even for 32-bit builds, system processor will be 64-bit.
        set(TARGET_CPU x86_64)
    elseif(("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64") OR
           ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "ARM") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AARCH64"))
        set(TARGET_CPU arm)
    else()
        message("Unknown system processor: " ${CMAKE_SYSTEM_PROCESSOR})
    endif()
else()
    message("Unable to detect target processor: CMAKE_SYSTEM_PROCESSOR is not defined")
endif()

if(TARGET_CPU)
    message("Target processor: " ${TARGET_CPU})
    set(TARGET_CPU ${TARGET_CPU} CACHE INTERNAL "Target CPU architecture")
endif()


message("CMake generator: " ${CMAKE_GENERATOR})

if(WIN32)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        set(PLATFORM_UNIVERSAL_WINDOWS TRUE CACHE INTERNAL "Target platform: Windows Store")
        message("Target platform: Universal Windows " ${ARCH} ". SDK Version: " ${CMAKE_SYSTEM_VERSION})
    else()
        set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32") #WIN32 is a variable, so we cannot use string "WIN32"
        message("Target platform: Win32 " ${ARCH} ". SDK Version: " ${CMAKE_SYSTEM_VERSION})
    endif()
else()
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        set(PLATFORM_ANDROID TRUE CACHE INTERNAL "Target platform: Android")
        message("Target platform: Android " ${ARCH})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(PLATFORM_LINUX TRUE CACHE INTERNAL "Target platform: Linux")
        message("Target platform: Linux " ${ARCH})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(IOS)
            set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
            message("Target platform: iOS " ${ARCH})
        else()
            set(PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform: MacOS")
            message("Target platform: MacOS " ${ARCH})
        endif()
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
        set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
        message("Target platform: iOS " ${ARCH})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "tvOS")
        set(PLATFORM_TVOS TRUE CACHE INTERNAL "Target platform: tvOS")
        message("Target platform: tvOS " ${ARCH})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
        set(PLATFORM_EMSCRIPTEN TRUE CACHE INTERNAL "Target platform: Emscripten")
        message("Target platform: Emscripten " ${ARCH})
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif(WIN32)

if(PLATFORM_MACOS OR PLATFORM_IOS OR PLATFORM_TVOS)
    set(PLATFORM_APPLE TRUE CACHE INTERNAL "Apple platform (macOS, iOS, or tvOS)")
endif()

add_library(Diligent-PublicBuildSettings INTERFACE)

if(PLATFORM_WIN32)
    if(MSVC)
        set(D3D11_SUPPORTED TRUE CACHE INTERNAL "D3D11 is supported on Win32 platform")
        if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
            set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported on Win32 platform")
        endif()
    else()
        message("Building with MinGW")
        set(MINGW_BUILD TRUE CACHE INTERNAL "Building with MinGW")
        set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 requires compiling with MSVC")
        set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 requires compiling with MSVC")
    endif()

    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Win32 platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Win32 platform")
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is supported on Win32 platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_WIN32=1)
elseif(PLATFORM_UNIVERSAL_WINDOWS)
    set(D3D11_SUPPORTED TRUE CACHE INTERNAL "D3D11 is supported on Universal Windows platform")
    if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
        set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported on Universal Windows platform")
    endif()
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is supported on Universal Windows platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_UNIVERSAL_WINDOWS=1)
elseif(PLATFORM_ANDROID)
    set(GLES_SUPPORTED     TRUE CACHE INTERNAL "OpenGLES is supported on Android platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Android platform")
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is supported on Android platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_ANDROID=1)
elseif(PLATFORM_LINUX)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Linux platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Linux platform")
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is supported on Linux platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_LINUX=1)
elseif(PLATFORM_MACOS)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on MacOS platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on MacOS platform")
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is supported on MacOS platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_MACOS=1)
elseif(PLATFORM_IOS)
    set(GLES_SUPPORTED TRUE CACHE INTERNAL "OpenGLES is supported on iOS platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_IOS=1)
elseif(PLATFORM_TVOS)
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_TVOS=1)
elseif(PLATFORM_EMSCRIPTEN)
    set(GLES_SUPPORTED TRUE CACHE INTERNAL "OpenGLES is supported on Emscripten platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_EMSCRIPTEN=1)   
else()
    message(FATAL_ERROR "No PLATFORM_XXX variable defined. Make sure that 'DiligentCore' folder is processed first")
endif()

if(PLATFORM_IOS OR PLATFORM_TVOS)
    if(VULKAN_SDK OR MOLTENVK_LIBRARY)
        if(NOT MOLTENVK_LIBRARY)
            set(MoltenVK_FRAMEWORK "${VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
            if(PLATFORM_IOS)
                if(CMAKE_OSX_SYSROOT STREQUAL "iphonesimulator" OR PLATFORM_IOS_SIMULATOR)
                    set(MOLTENVK_LIBRARY "${MoltenVK_FRAMEWORK}/ios-arm64_x86_64-simulator/libMoltenVK.a")
                else()
                    set(MOLTENVK_LIBRARY "${MoltenVK_FRAMEWORK}/ios-arm64/libMoltenVK.a")
                endif()
            elseif(PLATFORM_TVOS)
                if(CMAKE_OSX_SYSROOT STREQUAL "appletvsimulator" OR PLATFORM_TVOS_SIMULATOR)
                    set(MOLTENVK_LIBRARY "${MoltenVK_FRAMEWORK}/tvos-arm64_x86_64-simulator/libMoltenVK.a")
                else()
                    set(MOLTENVK_LIBRARY "${MoltenVK_FRAMEWORK}/tvos-arm64_arm64e/libMoltenVK.a")
                endif()
            else()
                message(FATAL_ERROR "Unexpected platform")
            endif()
        endif()

        if(EXISTS ${MOLTENVK_LIBRARY})
            message("Using MoltenVK library ${MOLTENVK_LIBRARY}")
            set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on ${CMAKE_SYSTEM_NAME} platform")
            set(MOLTENVK_LIBRARY ${MOLTENVK_LIBRARY} CACHE FILEPATH "MoltenVK library")
        else()
            message(WARNING "${MOLTENVK_LIBRARY} does not exist. Vulkan backend will be disabled.")
            unset(MOLTENVK_LIBRARY CACHE)
        endif()
    else()
        message("Neither VULKAN_SDK nor MOLTENVK_LIBRARY is defined. Vulkan backend will be disabled.")
    endif()
endif()

if(PLATFORM_APPLE)
    if(${DILIGENT_CORE_PRO_EXISTS})
        set(METAL_SUPPORTED TRUE CACHE INTERNAL "Metal is supported on Apple platforms")
    else()
        message("DiligentCorePro module is not found. Metal backend will be disabled")
    endif()
endif()

if(PLATFORM_WIN32 OR PLATFORM_LINUX OR PLATFORM_MACOS OR PLATFORM_EMSCRIPTEN)
    option(DILIGENT_BUILD_TESTS "Build Diligent Engine tests" OFF)
    if(DILIGENT_BUILD_TESTS)
        set(DILIGENT_BUILD_CORE_TESTS    TRUE CACHE INTERNAL "Build Core tests")
        set(DILIGENT_BUILD_TOOLS_TESTS   TRUE CACHE INTERNAL "Build Tools tests")
        set(DILIGENT_BUILD_FX_TESTS      TRUE CACHE INTERNAL "Build FX tests")
        set(DILIGENT_BUILD_SAMPLES_TESTS TRUE CACHE INTERNAL "Build Samples tests")

        set(DILIGENT_BUILD_CORE_INCLUDE_TEST    TRUE CACHE INTERNAL "Build Core Include test")
        set(DILIGENT_BUILD_TOOLS_INCLUDE_TEST   TRUE CACHE INTERNAL "Build Tools Include test")
        set(DILIGENT_BUILD_FX_INCLUDE_TEST      TRUE CACHE INTERNAL "Build FX Include test")
        set(DILIGENT_BUILD_SAMPLES_INCLUDE_TEST TRUE CACHE INTERNAL "Build Samples Include test")
    endif()
    if(DILIGENT_BUILD_CORE_TESTS OR DILIGENT_BUILD_TOOLS_TESTS OR DILIGENT_BUILD_FX_TESTS OR DILIGENT_BUILD_SAMPLES_TESTS)
        set(DILIGENT_BUILD_GOOGLE_TEST TRUE CACHE INTERNAL "Build google test framework" FORCE)
    endif()
else()
    if(DILIGENT_BUILD_TESTS)
        message("Unit tests are not supported on this platform and will be disabled")
    endif()
    set(DILIGENT_BUILD_TESTS FALSE CACHE INTERNAL "Tests are not available on this platform" FORCE)
endif()


option(DILIGENT_NO_HLSL              "Disable HLSL support in non-Direct3D backends" OFF)
option(DILIGENT_NO_FORMAT_VALIDATION "Disable source code format validation" OFF)
option(DILIGENT_NO_DIRECT3D11        "Disable Direct3D11 backend" OFF)
option(DILIGENT_NO_DIRECT3D12        "Disable Direct3D12 backend" OFF)
option(DILIGENT_NO_OPENGL            "Disable OpenGL/GLES backend" OFF)
option(DILIGENT_NO_VULKAN            "Disable Vulkan backend" OFF)
option(DILIGENT_NO_METAL             "Disable Metal backend" OFF)
option(DILIGENT_NO_ARCHIVER          "Do not build archiver" ON)
if(${DILIGENT_NO_DIRECT3D11})
    set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 backend is forcibly disabled")
endif()
if(${DILIGENT_NO_DIRECT3D12})
    set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 backend is forcibly disabled")
endif()
if(${DILIGENT_NO_OPENGL})
    set(GL_SUPPORTED FALSE CACHE INTERNAL "OpenGL backend is forcibly disabled")
    set(GLES_SUPPORTED FALSE CACHE INTERNAL "OpenGLES backend is forcibly disabled")
endif()
if(${DILIGENT_NO_VULKAN})
    set(VULKAN_SUPPORTED FALSE CACHE INTERNAL "Vulkan backend is forcibly disabled")
endif()
if(${DILIGENT_NO_METAL})
    set(METAL_SUPPORTED FALSE CACHE INTERNAL "Metal backend is forcibly disabled")
endif()
if(${DILIGENT_NO_ARCHIVER})
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is forcibly disabled")
endif()

if(NOT (${D3D11_SUPPORTED} OR ${D3D12_SUPPORTED} OR ${GL_SUPPORTED} OR ${GLES_SUPPORTED} OR ${VULKAN_SUPPORTED} OR ${METAL_SUPPORTED}))
    message(FATAL_ERROR "No rendering backends are select to build")
endif()


message("D3D11_SUPPORTED:  " ${D3D11_SUPPORTED})
message("D3D12_SUPPORTED:  " ${D3D12_SUPPORTED})
message("GL_SUPPORTED:     " ${GL_SUPPORTED})
message("GLES_SUPPORTED:   " ${GLES_SUPPORTED})
message("VULKAN_SUPPORTED: " ${VULKAN_SUPPORTED})
message("METAL_SUPPORTED:  " ${METAL_SUPPORTED})

target_compile_definitions(Diligent-PublicBuildSettings
INTERFACE
    D3D11_SUPPORTED=$<BOOL:${D3D11_SUPPORTED}>
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    GLES_SUPPORTED=$<BOOL:${GLES_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
    METAL_SUPPORTED=$<BOOL:${METAL_SUPPORTED}>
)

foreach(DBG_CONFIG ${DEBUG_CONFIGURATIONS})
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE "$<$<CONFIG:${DBG_CONFIG}>:DILIGENT_DEVELOPMENT;DILIGENT_DEBUG>")
endforeach()

if(DILIGENT_DEVELOPMENT)
    foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
        target_compile_definitions(Diligent-PublicBuildSettings INTERFACE "$<$<CONFIG:${REL_CONFIG}>:DILIGENT_DEVELOPMENT>")
    endforeach()
endif()


add_library(Diligent-BuildSettings INTERFACE)
target_link_libraries(Diligent-BuildSettings INTERFACE Diligent-PublicBuildSettings)

foreach(DBG_CONFIG ${DEBUG_CONFIGURATIONS})
    target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${DBG_CONFIG}>:_DEBUG;DEBUG>")
endforeach()

foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
    target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${REL_CONFIG}>:NDEBUG>")
endforeach()

if(MSVC)
    # Treat warnings as errors
    set(DILIGENT_MSVC_COMPILE_OPTIONS "/WX" CACHE STRING "Common MSVC compile options")

    set(DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS "" CACHE STRING "Additional MSVC compile options for debug configuration")

    if("${TARGET_CPU}" STREQUAL "x86_64")
        # Enable AVX2
        set(DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS "/arch:AVX2")
    endif()
    set(DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS ${DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS} CACHE STRING "Additional MSVC compile options for release configurations")

    # Enable level 4 warnings, but disable
    # - w4100  - unreferenced formal parameter
    # - w4201  - nonstandard extension used: nameless struct/union
    # - w4505  - unreferenced local function has been removed
    # - C26812 - Prefer 'enum class' over 'enum' (Code analysis warning)
    target_compile_options(Diligent-BuildSettings INTERFACE /W4 /wd4100 /wd4201 /wd4505 /wd26812 /MP ${DILIGENT_MSVC_COMPILE_OPTIONS})

    # In all release modes also:
    # - disable w4189 - local variable is initialized but not referenced
    # - Disable RTTI (/GR-)
    # - Enable whole program optimization (/GL)
    # - Enable string pooling (/GF)
    set(MSVC_ALL_RELEASE_COMPILE_OPTIONS /wd4189 /GR- /GL /GF)
    #target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:/wd4189 /Ot")
    # In RELEASE mode:
    # - Set favor fast code option (/Ot)
    # - Enable intrinsic functions (/Oi)
    # - Maximize Speed (/O2)
    # - Inline any suitable function (/Ob2)
    set(MSVC_RELEASE_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Ot /Oi /Ob2 /O2 ${DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS})
    set(MSVC_RELWITHDEBINFO_COMPILE_OPTIONS ${MSVC_RELEASE_COMPILE_OPTIONS})
    # In MINSIZEREL mode set favor small code option (/Os)
    set(MSVC_MINSIZEREL_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Os)
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:DEBUG>:${DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS}>")
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:${MSVC_RELEASE_COMPILE_OPTIONS}>")
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:MINSIZEREL>:${MSVC_MINSIZEREL_COMPILE_OPTIONS}>")
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELWITHDEBINFO>:${MSVC_RELWITHDEBINFO_COMPILE_OPTIONS}>")
    # !!!NOTE!!! For some reason above is the only form of generator expression that works
    # For instance, this way
    # target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:/Ot>")
    # does not work as expected
else()
    # Todo: use __attribute__((always_inline)), but it needs to be defined in a header file
    target_compile_definitions(Diligent-BuildSettings INTERFACE __forceinline=inline)
endif(MSVC)


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
    CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(PLATFORM_APPLE)
        # Looks like force_load is preferable over all_load
        set(WHOLE_ARCHIVE_FLAG "-Wl,-force_load" CACHE INTERNAL "all_load flag")
        # Option -noall_load is obsolete and there is no -noforce_load
        set(NO_WHOLE_ARCHIVE_FLAG "" CACHE INTERNAL "noall_load flag")
    else()
        set(WHOLE_ARCHIVE_FLAG "-Wl,--whole-archive" CACHE INTERNAL "whole-archive flag")
        set(NO_WHOLE_ARCHIVE_FLAG "-Wl,--no-whole-archive" CACHE INTERNAL "no-whole-archive flag")
    endif()
else()
    set(WHOLE_ARCHIVE_FLAG "")
    set(NO_WHOLE_ARCHIVE_FLAG "")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Treat warnings as errors
    set(DILIGENT_CLANG_COMPILE_OPTIONS "-Werror" CACHE STRING "Common Clang compile options")

    set(DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS "" CACHE STRING "Additional Clang compile options for debug configuration")

    if("${TARGET_CPU}" STREQUAL "x86_64")
        # Enable AVX2
        set(DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS "-mavx2")
    endif()
    set(DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS ${DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS} CACHE STRING "Additional Clang compile options for release configurations")

    target_compile_options(Diligent-BuildSettings INTERFACE
        ${DILIGENT_CLANG_COMPILE_OPTIONS}
        # Some extra warnings
        -Wall -Wextra -Wuninitialized -Wconditional-uninitialized -Wextra-tokens -Wpointer-arith -Wloop-analysis -Wunused
        # Disable few warnings
        -Wno-overloaded-virtual -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unknown-pragmas
        -Wno-zero-as-null-pointer-constant -Wno-unused-parameter
    )
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:DEBUG>:${DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS}>")

    set(CLANG_RELEASE_OPTIONS -Wno-unused-variable ${DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS})
    target_compile_options(Diligent-BuildSettings INTERFACE $<$<NOT:$<CONFIG:Debug>>:${CLANG_RELEASE_OPTIONS}>)

    if (PLATFORM_APPLE)
        # Allow unavailable API warnings not being errors as they tend to randomly
        # appear in new versions of XCode.
        # Besides, there is a known long-standing issue in simulator SDK:
        # the compiler generates a lot of bogus warnings in Metal
        # headers about unavailable API
        target_compile_options(Diligent-BuildSettings
        INTERFACE
            -Wno-error=unguarded-availability-new
        )
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.0")
        message(WARNING "gcc 9.0 and above seemingly produce invalid binary code with O2 and O3 optimization levels. Optimization in release configurations will be downgraded to O1. It is strongly recommended to use clang.")
        set(GCC_OPTIMIZATION_LEVEL "-O1")
        target_compile_options(Diligent-BuildSettings INTERFACE $<$<CONFIG:Release,RelWithDebInfo>:${GCC_OPTIMIZATION_LEVEL}>)
    endif()
endif()

if(PLATFORM_MACOS)
    find_library(APP_KIT AppKit)
    if (NOT APP_KIT)
            message(FATAL_ERROR "AppKit not found")
    endif()
elseif(PLATFORM_IOS)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "Cannot find CoreFoundation framework")
    endif()

    find_library(FOUNDATION Foundation)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Cannot find Foundation framework")
    endif()

    find_library(OPENGLES OpenGLES)
    if(NOT OPENGLES)
        message(FATAL_ERROR "Cannot find OpenGLES framework")
    endif()
elseif(PLATFORM_TVOS)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "Cannot find CoreFoundation framework")
    endif()

    find_library(FOUNDATION Foundation)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Cannot find Foundation framework")
    endif()
endif()

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX OR PLATFORM_APPLE)
    option(DILIGENT_INSTALL_CORE "Install DiligentCore module headers and libraries" ON)
else()
    set(DILIGENT_INSTALL_CORE OFF)
endif()

if(MSVC)
    option(DILIGENT_INSTALL_PDB "Install PDB files" OFF)
else()
    set(DILIGENT_INSTALL_PDB OFF)
endif()

file(RELATIVE_PATH DILIGENT_CORE_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
SET(DILIGENT_CORE_DIR ${DILIGENT_CORE_DIR} CACHE INTERNAL "Diligent Core installation directory")

SET(DILIGENT_CORE_INSTALL_LIBS_LIST "" CACHE INTERNAL "Core libraries installation list")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation path" FORCE)
elseif (NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})
    # CMAKE_INSTALL_PREFIX must be absolute otherwise rpath won't work
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}" CACHE PATH "Installation path" FORCE)
    message("Transformed CMAKE_INSTALL_PREFIX into absolute path: " ${CMAKE_INSTALL_PREFIX})
endif()

include(BuildUtils.cmake)

add_subdirectory(ThirdParty)
add_subdirectory(BuildTools)
add_subdirectory(Primitives)
add_subdirectory(Platforms)
add_subdirectory(Common)
add_subdirectory(Graphics)
#add_subdirectory(Tests)


# Installation instructions
if(DILIGENT_INSTALL_CORE)

    install_combined_static_lib(
        "${CMAKE_STATIC_LIBRARY_PREFIX}DiligentCore${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${DILIGENT_CORE_INSTALL_LIBS_LIST}"
        DiligentCore-static                     # Custom target name
        DiligentCore                            # Folder
        "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"    # Install destination
    )

    install(FILES License.txt DESTINATION "Licenses" RENAME DiligentEngine-License.txt)
endif(DILIGENT_INSTALL_CORE)

# Create a custom target to run source code formatting validation command
#add_format_validation_target(DiligentCore "${CMAKE_CURRENT_SOURCE_DIR}" DiligentCore/BuildTools)
