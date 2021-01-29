#
# Copyright (c) 2017-2020 the rbfx project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# spdlog is patched to use EASTL. Do following when upgrading the library:
# Run:
#   find . -name "*.h" | xargs sed -i "s/std::string/eastl::string/"
#   find . -name "*.h" | xargs sed -i "s/std::to_string/eastl::to_string/"
#   find . -name "*.h" | xargs sed -i "s/std::unordered_map/eastl::unordered_map/"
#   find . -name "*.h" | xargs sed -i "s|<unordered_map>|<EASTL/unordered_map.h>|"
#
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(spdlog INTERFACE fmt)
target_compile_definitions(spdlog INTERFACE -DSPDLOG_FMT_EXTERNAL=1 -DSPDLOG_DISABLE_DEFAULT_LOGGER=1)
install (TARGETS spdlog EXPORT Urho3D)
