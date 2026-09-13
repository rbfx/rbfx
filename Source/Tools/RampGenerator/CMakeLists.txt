# Copyright (c) 2008-2022 the Urho3D project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

return_if_not_tool(RampGenerator)

file (GLOB SOURCE_FILES *.cpp *.h)
add_executable (RampGenerator ${SOURCE_FILES})
target_link_libraries (RampGenerator Urho3D)
install_tools(RampGenerator)

# Install runtime dependencies
install_target_runtime_deps(TARGET RampGenerator COMPONENT Tools)
