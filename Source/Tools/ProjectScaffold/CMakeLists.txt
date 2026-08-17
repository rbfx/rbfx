# SPDX-License-Identifier: MIT

return_if_not_tool(ProjectScaffold)

add_executable(ProjectScaffold main.cpp)
target_compile_features(ProjectScaffold PRIVATE cxx_std_17)
install_tools(ProjectScaffold)
install_target_runtime_deps(TARGET ProjectScaffold COMPONENT Tools)
