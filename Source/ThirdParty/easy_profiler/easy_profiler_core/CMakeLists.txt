
file (GLOB_RECURSE SOURCE_FILES *.h *.cpp)
add_library(easy_profiler OBJECT ${SOURCE_FILES} resources.rc)
target_include_directories(easy_profiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS}/easy>
)
target_compile_definitions(easy_profiler PRIVATE

)
target_compile_definitions(easy_profiler
    PUBLIC
        -DBUILD_WITH_EASY_PROFILER=1
        -DEASY_PROFILER_STATIC=1
        -DEASY_PROFILER_VERSION_MAJOR=${EASY_PROGRAM_VERSION_MAJOR}
        -DEASY_PROFILER_VERSION_MINOR=${EASY_PROGRAM_VERSION_MINOR}
        -DEASY_PROFILER_VERSION_PATCH=${EASY_PROGRAM_VERSION_PATCH}
        -DEASY_DEFAULT_PORT=28077
#        -DEASY_CHRONO_STEADY_CLOCK=1
        -DEASY_CHRONO_HIGHRES_CLOCK=1
#        -DEASY_OPTION_START_LISTEN_ON_STARTUP=1
#        -DEASY_OPTION_MEASURE_STORAGE_EXPAND=1
        -DEASY_OPTION_STORAGE_EXPAND_BLOCKS_ON=1
        -DEASY_OPTION_EVENT_TRACING_ENABLED=1
        -DEASY_OPTION_LOW_PRIORITY_EVENT_TRACING=1
#        -DEASY_OPTION_LOG_ENABLED=1
        -DEASY_OPTION_BUILTIN_COLORS=1
#        -DEASY_PROFILER_HASHED_CSTR_DEFINED=1
#        -DEASY_ENABLE_ALIGNMENT=1
#        -DEASY_ENABLE_STRICT_ALIGNMENT=1
        -DEASY_OPTION_IMPLICIT_THREAD_REGISTRATION=1
#        -DEASY_PROFILER_API_DISABLED=1
)

if (UNIX)
    target_compile_options(easy_profiler PRIVATE -Wall -Wno-long-long -Wno-reorder -Wno-braced-scalar-init -pedantic -O3)
    if (NOT ANDROID)
        object_target_link_libraries(easy_profiler pthread)
    endif ()
elseif (WIN32)
    target_compile_definitions(easy_profiler PRIVATE -D_WIN32_WINNT=0x0600 -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
    object_target_link_libraries(easy_profiler ws2_32 psapi)
endif ()

if (MINGW)
    target_compile_definitions(easy_profiler PRIVATE -DSTRSAFE_NO_DEPRECATE)
    target_compile_options(easy_profiler PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>)
endif ()

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS} FILES_MATCHING PATTERN *.h)

# If we're on MSVC, we need a /MD version of the easy profiler library to link with Qt
if (MSVC AND Qt5Widgets_FOUND)
    add_library(easy_profiler_md ${EASY_OPTION_LIB_TYPE} ${SOURCES} resources.rc)
    foreach(prop COMPILE_DEFINITIONS COMPILE_OPTIONS CXX_STANDARD CXX_STANDARD_REQUIRED INCLUDE_DIRECTORIES INTERFACE_COMPILE_DEFINITIONS INTERFACE_COMPILE_OPTIONS INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES)
        get_target_property(val easy_profiler ${prop})
        set_target_properties(easy_profiler_md PROPERTIES ${prop} "${val}")
    endforeach()
    target_compile_options(easy_profiler_md PUBLIC $<$<CONFIG:Debug>:/MDd> $<$<NOT:$<CONFIG:Debug>>:/MD>)
endif ()
