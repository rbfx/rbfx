# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

return_if_not_tool(SpritePacker)

file (GLOB SOURCE_FILES *.cpp *.h)
add_executable (SpritePacker ${SOURCE_FILES})
target_link_libraries (SpritePacker Urho3D)
install_tools(SpritePacker)

# Install runtime dependencies
install_target_runtime_deps(TARGET SpritePacker COMPONENT Tools)