# Copyright (c) 2026 the rbfx project.
# This work is licensed under the terms of the MIT license.

return_if_not_tool(AssetViewer)

if (NOT URHO3D_SYSTEMUI)
    return ()
endif ()

file (GLOB SOURCE_FILES *.cpp *.h)
add_executable (AssetViewer ${SOURCE_FILES})
target_link_libraries (AssetViewer Urho3D)
install_tools(AssetViewer)

install_target_runtime_deps(TARGET AssetViewer COMPONENT Tools)
