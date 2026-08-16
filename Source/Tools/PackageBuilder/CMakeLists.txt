# Copyright (c) 2026 the rbfx-blueprint project.
# SPDX-License-Identifier: MIT

return_if_not_tool(PackageBuilder)

file(GLOB SOURCE_FILES *.cpp *.h)
add_executable(PackageBuilder ${SOURCE_FILES})
target_link_libraries(PackageBuilder Urho3D)
install_tools(PackageBuilder)
install_target_runtime_deps(TARGET PackageBuilder COMPONENT Tools)
