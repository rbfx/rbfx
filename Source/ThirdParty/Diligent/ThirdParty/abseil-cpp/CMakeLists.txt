cmake_minimum_required (VERSION 3.11)

FetchContent_DeclareShallowGit(
    abseil-cpp
    GIT_REPOSITORY         https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp
    GIT_TAG                f81f6c011baf9b0132a5594c034fe0060820711d
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)

if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang"))
    add_compile_options(
        -Wno-array-parameter
        -Wno-deprecated-builtins
        -Wno-unknown-warning-option
    )
endif()

FetchContent_MakeAvailable(abseil-cpp)
install(FILES "${abseil-cpp_SOURCE_DIR}/LICENSE" DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME abseil-cpp-License.txt)
