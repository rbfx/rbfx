#
# Android NDK toolchain wrapper.
#
# Set ANDROID_NDK_HOME either as a CMake cache variable or an environment
# variable. All Android-specific behavior is delegated to the NDK toolchain.

if (NOT ANDROID_NDK_HOME AND DEFINED ENV{ANDROID_NDK_HOME})
    set (ANDROID_NDK_HOME "$ENV{ANDROID_NDK_HOME}")
endif ()

if (NOT ANDROID_NDK_HOME)
    message (FATAL_ERROR
        "ANDROID_NDK_HOME is not set. Pass -DANDROID_NDK_HOME=/path/to/ndk "
        "or define the ANDROID_NDK_HOME environment variable."
    )
endif ()

file (TO_CMAKE_PATH "${ANDROID_NDK_HOME}" ANDROID_NDK_HOME)
set (ANDROID_NDK_HOME "${ANDROID_NDK_HOME}" CACHE PATH "Android NDK directory" FORCE)
list (APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ANDROID_NDK_HOME)

set (ANDROID_NDK_TOOLCHAIN_FILE "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
if (NOT EXISTS "${ANDROID_NDK_TOOLCHAIN_FILE}")
    message (FATAL_ERROR "Android NDK toolchain file does not exist: ${ANDROID_NDK_TOOLCHAIN_FILE}")
endif ()

include ("${ANDROID_NDK_TOOLCHAIN_FILE}")
