set(TRACY_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../public/common)

set(TRACY_COMMON_SOURCES
    tracy_lz4.cpp
    tracy_lz4hc.cpp
    TracySocket.cpp
    TracyStackFrames.cpp
    TracySystem.cpp
)

list(TRANSFORM TRACY_COMMON_SOURCES PREPEND "${TRACY_COMMON_DIR}/")


set(TRACY_SERVER_DIR ${CMAKE_CURRENT_LIST_DIR}/../server)

set(TRACY_SERVER_SOURCES
    TracyMemory.cpp
    TracyMmap.cpp
    TracyPrint.cpp
    TracySysUtil.cpp
    TracyTaskDispatch.cpp
    TracyTextureCompression.cpp
    TracyThreadCompress.cpp
    TracyWorker.cpp
)

list(TRANSFORM TRACY_SERVER_SOURCES PREPEND "${TRACY_SERVER_DIR}/")


add_library(TracyServer STATIC EXCLUDE_FROM_ALL ${TRACY_COMMON_SOURCES} ${TRACY_SERVER_SOURCES})
target_include_directories(TracyServer PUBLIC ${TRACY_COMMON_DIR} ${TRACY_SERVER_DIR})
target_link_libraries(TracyServer PUBLIC TracyCapstone libzstd PPQSort::PPQSort)

# Fix GCC 15+ inlining issues with xxHash SSE2 intrinsics in tracy_xxhash.h
# GCC 15 fails to inline XXH3_accumulate_sse2/XXH3_scrambleAcc_sse2 with always_inline attribute
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0")
    target_compile_definitions(TracyServer PUBLIC XXH_NO_INLINE_HINTS=1)
endif()

if(NO_STATISTICS)
    target_compile_definitions(TracyServer PUBLIC TRACY_NO_STATISTICS)
endif()
if(APPLE)
    target_compile_options(TracyServer PUBLIC -fexperimental-library)
endif()
