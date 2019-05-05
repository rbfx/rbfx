# Replace all instances of `std::string` with `eastl::string`.
# Replace all instances of `std::unordered_map` with `eastl::unordered_map`.
# Replace all instances of `#include <unordered_map>` with `#include <EASTL/unordered_map.h>`.

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(spdlog INTERFACE fmt)
target_compile_definitions(spdlog INTERFACE -DSPDLOG_FMT_EXTERNAL=1)
install (TARGETS spdlog EXPORT Urho3D)
