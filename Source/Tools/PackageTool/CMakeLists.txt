# Copyright (c) 2008-2022 the Urho3D project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

return_if_not_tool(PackageTool)

file (GLOB SOURCE_FILES *.cpp *.h)
add_executable (PackageTool ${SOURCE_FILES})
# Minimal engine builds which this target uses do not merge third party dependencies to monolithic lib. Link to
# dependencies explicitly.
target_link_libraries (PackageTool Urho3D LZ4)
install_tools(PackageTool)

# Install runtime dependencies
install_target_runtime_deps(TARGET PackageTool COMPONENT Tools)
