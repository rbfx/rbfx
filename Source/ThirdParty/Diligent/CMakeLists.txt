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
set(WEBGPU_SUPPORTED           FALSE CACHE INTERNAL "WebGPU is not supported")
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
    if (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
        set(WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} CACHE INTERNAL "Windows SDK version")
    elseif(DEFINED ENV{WindowsSDKVersion})
        set(WINDOWS_SDK_VERSION $ENV{WindowsSDKVersion})
        # For unbeknown reason, the value ends with a backslash, so we need to remove it
        string(REPLACE "\\" "" WINDOWS_SDK_VERSION ${WINDOWS_SDK_VERSION})
        set(WINDOWS_SDK_VERSION ${WINDOWS_SDK_VERSION} CACHE INTERNAL "Windows SDK version")
    else()
        set(WINDOWS_SDK_VERSION "0.0" CACHE INTERNAL "Windows SDK version")
        message(WARNING "Unable to determine Windows SDK version: neither the CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION CMake variable nor the WindowsSDKVersion environment variable is set")
    endif()

    if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        set(PLATFORM_UNIVERSAL_WINDOWS TRUE CACHE INTERNAL "Target platform: Windows Store")
        message("Target platform: Universal Windows " ${ARCH} ". Windows SDK Version: " ${WINDOWS_SDK_VERSION})
    else()
        set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32") #WIN32 is a variable, so we cannot use string "WIN32"
        message("Target platform: Win32 " ${ARCH} ". Windows SDK Version: " ${WINDOWS_SDK_VERSION})
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

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
    try_compile(HAS_D3D11 "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/BuildTools/CMake/CheckD3D11.cpp")
    try_compile(HAS_D3D12 "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/BuildTools/CMake/CheckD3D12.cpp")
    try_compile(HAS_ATL "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/BuildTools/CMake/CheckATL.cpp")
    message("")
    message("Checking for Direct3D11 and Direct3D12 support:")
    message("  d3d11.h found:   " ${HAS_D3D11})
    message("  d3d12.h found:   " ${HAS_D3D12})
    message("  atlbase.h found: " ${HAS_ATL})

    if (HAS_D3D11 AND HAS_ATL)
        set(D3D11_SUPPORTED TRUE CACHE INTERNAL "D3D11 is supported")
    else()
        set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 is not supported")
    endif()
    if (HAS_D3D12 AND HAS_ATL)
        set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported")
    else()
        set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 is not supported")
    endif()
endif()

if(${CMAKE_GENERATOR} MATCHES "MinGW")
    message("Building with MinGW")
    set(MINGW_BUILD TRUE CACHE INTERNAL "Building with MinGW")
endif()

if(PLATFORM_WIN32)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Win32 platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Win32 platform")
    set(WEBGPU_SUPPORTED   TRUE CACHE INTERNAL "WebGPU is supported on Win32 platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Win32 platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_WIN32=1)
elseif(PLATFORM_UNIVERSAL_WINDOWS)
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Universal Windows platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_UNIVERSAL_WINDOWS=1)
elseif(PLATFORM_ANDROID)
    set(GLES_SUPPORTED     TRUE CACHE INTERNAL "OpenGLES is supported on Android platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Android platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Android platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_ANDROID=1)
elseif(PLATFORM_LINUX)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Linux platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Linux platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Linux platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_LINUX=1)
elseif(PLATFORM_MACOS)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on MacOS platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on MacOS platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on MacOS platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_MACOS=1 PLATFORM_APPLE=1)
elseif(PLATFORM_IOS)
    set(GLES_SUPPORTED TRUE CACHE INTERNAL "OpenGLES is supported on iOS platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_IOS=1 PLATFORM_APPLE=1)
elseif(PLATFORM_TVOS)
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_TVOS=1 PLATFORM_APPLE=1)
elseif(PLATFORM_EMSCRIPTEN)
    set(GLES_SUPPORTED     TRUE CACHE INTERNAL "OpenGLES is supported on Emscripten platform")
    set(WEBGPU_SUPPORTED   TRUE CACHE INTERNAL "WebGPU is supported on Emscripten platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Emscripten platform")
    target_compile_definitions(Diligent-PublicBuildSettings INTERFACE PLATFORM_EMSCRIPTEN=1)   
else()
    message(FATAL_ERROR "No PLATFORM_XXX variable defined. Make sure that 'DiligentCore' folder is processed first")
endif()

if(PLATFORM_IOS OR PLATFORM_TVOS)
    if(VULKAN_SDK OR MOLTENVK_LIBRARY)
        if(NOT MOLTENVK_LIBRARY)
            set(MoltenVK_FRAMEWORK "${VULKAN_SDK}/macOS/lib/MoltenVK.xcframework")
            if(NOT EXISTS "${MoltenVK_FRAMEWORK}")
                set(MoltenVK_FRAMEWORK "${VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
            endif()
            if(EXISTS "${MoltenVK_FRAMEWORK}")
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
            else()
                message(WARNING "Unable to find MoltenVK.xcframework at ${VULKAN_SDK}/macOS/lib or ${VULKAN_SDK}/MoltenVK")
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
option(DILIGENT_NO_FORMAT_VALIDATION "Disable source code format validation" ON)
option(DILIGENT_NO_DIRECT3D11        "Disable Direct3D11 backend" OFF)
option(DILIGENT_NO_DIRECT3D12        "Disable Direct3D12 backend" OFF)
option(DILIGENT_NO_OPENGL            "Disable OpenGL/GLES backend" OFF)
option(DILIGENT_NO_VULKAN            "Disable Vulkan backend" OFF)
option(DILIGENT_NO_METAL             "Disable Metal backend" OFF)
if (PLATFORM_EMSCRIPTEN)
    option(DILIGENT_NO_WEBGPU        "Disable WebGPU backend" OFF)
else()
    option(DILIGENT_NO_WEBGPU        "Disable WebGPU backend" ON)
endif()
option(DILIGENT_NO_ARCHIVER          "Do not build archiver" OFF)

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
if(${DILIGENT_NO_WEBGPU})
    set(WEBGPU_SUPPORTED FALSE CACHE INTERNAL "WebGPU backend is forcibly disabled")
endif()
if(${DILIGENT_NO_ARCHIVER})
    set(ARCHIVER_SUPPORTED FALSE CACHE INTERNAL "Archiver is forcibly disabled")
endif()

if(NOT (${D3D11_SUPPORTED} OR ${D3D12_SUPPORTED} OR ${GL_SUPPORTED} OR ${GLES_SUPPORTED} OR ${VULKAN_SUPPORTED} OR ${METAL_SUPPORTED} OR ${WEBGPU_SUPPORTED}))
    message(FATAL_ERROR "No rendering backends are select to build")
endif()

message("")
message("D3D11_SUPPORTED:  " ${D3D11_SUPPORTED})
message("D3D12_SUPPORTED:  " ${D3D12_SUPPORTED})
message("GL_SUPPORTED:     " ${GL_SUPPORTED})
message("GLES_SUPPORTED:   " ${GLES_SUPPORTED})
message("VULKAN_SUPPORTED: " ${VULKAN_SUPPORTED})
message("METAL_SUPPORTED:  " ${METAL_SUPPORTED})
message("WEBGPU_SUPPORTED: " ${WEBGPU_SUPPORTED})
message("")

target_compile_definitions(Diligent-PublicBuildSettings
INTERFACE
    D3D11_SUPPORTED=$<BOOL:${D3D11_SUPPORTED}>
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    GLES_SUPPORTED=$<BOOL:${GLES_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
    METAL_SUPPORTED=$<BOOL:${METAL_SUPPORTED}>
    WEBGPU_SUPPORTED=$<BOOL:${WEBGPU_SUPPORTED}>
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

if (PLATFORM_EMSCRIPTEN)
    target_compile_options(Diligent-BuildSettings INTERFACE
        "-pthread"
        "-mbulk-memory"
    )
endif()

target_link_libraries(Diligent-BuildSettings INTERFACE Diligent-PublicBuildSettings)

foreach(DBG_CONFIG ${DEBUG_CONFIGURATIONS})
    target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${DBG_CONFIG}>:_DEBUG;DEBUG>")
endforeach()

foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
    target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${REL_CONFIG}>:NDEBUG>")
endforeach()

if(MSVC)
    # Treat warnings as errors
    set(DILIGENT_MSVC_COMPILE_OPTIONS "" CACHE STRING "Common MSVC compile options")
    if (DILIGENT_MSVC_COMPILE_OPTIONS)
        message("Additional MSVC compile options: " ${DILIGENT_MSVC_COMPILE_OPTIONS})
    endif()

    set(DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS "" CACHE STRING "Additional MSVC compile options for debug configuration")
    if (DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS)
        message("Additional MSVC compile options for debug configuration: " ${DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS})
    endif()

    # Enable whole program optimization
    set(DEFAULT_DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS /GL)
    if("${TARGET_CPU}" STREQUAL "x86_64")
        # Enable AVX2
        # rbfx: Don't do any extra options. This should be controlled by Urho3D CMake scripts.
        # set(DEFAULT_DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS ${DEFAULT_DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS} /arch:AVX2)
    endif()
    set(DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS ${DEFAULT_DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS} CACHE STRING "Additional MSVC compile options for release configurations")
    if (DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS)
        message("Additional MSVC compile options for release configurations: " ${DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS})
    endif()

    # Enable level 4 warnings, but disable
    # - w4100  - unreferenced formal parameter
    # - w4201  - nonstandard extension used: nameless struct/union
    # - w4505  - unreferenced local function has been removed
    # - C26812 - Prefer 'enum class' over 'enum' (Code analysis warning)
    target_compile_options(Diligent-BuildSettings INTERFACE /W4 /wd4100 /wd4201 /wd4505 /wd26812 /MP ${DILIGENT_MSVC_COMPILE_OPTIONS})

    if (PLATFORM_WIN32)
        # Enable conformance mode.
        # Note that UWP sources don't compile in conformance mode.
        target_compile_options(Diligent-BuildSettings INTERFACE /permissive-)
    endif()

    # In all release modes also:
    # - disable w4189 - local variable is initialized but not referenced
    # - Disable RTTI (/GR-)
    # - Enable string pooling (/GF)
    set(MSVC_ALL_RELEASE_COMPILE_OPTIONS /wd4189 /GR- /GF)
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
    set(DILIGENT_CLANG_COMPILE_OPTIONS "" CACHE STRING "Common Clang compile options")
    if (DILIGENT_CLANG_COMPILE_OPTIONS)
        message("Additional Clang compile options: " ${DILIGENT_CLANG_COMPILE_OPTIONS})
    endif()

    set(DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS "" CACHE STRING "Additional Clang compile options for debug configuration")
    if (DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS)
        message("Additional Clang compile options for debug configuration: " ${DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS})
    endif()

    if("${TARGET_CPU}" STREQUAL "x86_64")
        # Enable AVX2
        set(DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS "-mavx2")
    endif()
    set(DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS ${DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS} CACHE STRING "Additional Clang compile options for release configurations")
    if (DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS)
        message("Additional Clang compile options for release configurations: " ${DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS})
    endif()

    target_compile_options(Diligent-BuildSettings INTERFACE
        ${DILIGENT_CLANG_COMPILE_OPTIONS}
        # Some extra warnings
        -Wall -Wextra -Wuninitialized -Wconditional-uninitialized -Wextra-tokens -Wpointer-arith -Wloop-analysis -Wunused
        # Disable few warnings
        -Wno-overloaded-virtual -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unknown-pragmas
        -Wno-zero-as-null-pointer-constant -Wno-unused-parameter -Wno-invalid-offsetof
    )
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:DEBUG>:${DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS}>")

    set(CLANG_RELEASE_OPTIONS -Wno-unused-variable ${DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS})
    target_compile_options(Diligent-BuildSettings INTERFACE $<$<NOT:$<CONFIG:Debug>>:${CLANG_RELEASE_OPTIONS}>)

    if (PLATFORM_WIN32)
        target_compile_options(Diligent-BuildSettings
        INTERFACE
            -Wno-microsoft-exception-spec
            -Wno-tautological-constant-out-of-range-compare
            -Wno-enum-constexpr-conversion
        )
    elseif (PLATFORM_APPLE)
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
            message(FATAL_ERROR "Failed to find AppKit framework")
    endif()
elseif(PLATFORM_IOS)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "Failed to find CoreFoundation framework")
    endif()

    find_library(FOUNDATION Foundation)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Failed to find Foundation framework")
    endif()

    find_library(OPENGLES OpenGLES)
    if(NOT OPENGLES)
        message(FATAL_ERROR "Failed to find OpenGLES framework")
    endif()
elseif(PLATFORM_TVOS)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "Failed to find CoreFoundation framework")
    endif()

    find_library(FOUNDATION Foundation)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Failed to find Foundation framework")
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

include(BuildTools/CMake/BuildUtils.cmake)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # Find available .NET SDKs
    execute_process(COMMAND dotnet --list-sdks
                    RESULTS_VARIABLE RES
                    OUTPUT_VARIABLE DOT_NET_SDK_LIST)
    if (${RES} EQUAL 0)
        # Extract versions only from the list
        string(REGEX MATCHALL "[0-9]+\\.[0-9]+\\.[0-9]+" DOT_NET_SDK_VERSIONS "${DOT_NET_SDK_LIST}")
        set(DOT_NET_SDK_VERSIONS ${DOT_NET_SDK_VERSIONS} CACHE STRING "Available .NET SDK versions" FORCE)
        message("Available .NET SDK versions: ${DOT_NET_SDK_VERSIONS}")

        set(MAX_DOT_NET_SDK_VERSION "")
        foreach(VER IN LISTS DOT_NET_SDK_VERSIONS)
            if(VER VERSION_GREATER MAX_DOT_NET_SDK_VERSION)
                set(MAX_DOT_NET_SDK_VERSION "${VER}")
            endif()
        endforeach()
        set(MAX_DOT_NET_SDK_VERSION ${MAX_DOT_NET_SDK_VERSION} CACHE STRING "Maximum .NET SDK version" FORCE)
        message("  Maximum .NET SDK version: ${MAX_DOT_NET_SDK_VERSION}")
    else()
        message("dotnet command is not found. This indicates that .NET SDK is not installed on this machine.")
    endif()
endif()

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
add_format_validation_target(DiligentCore "${CMAKE_CURRENT_SOURCE_DIR}" DiligentCore/BuildTools)
