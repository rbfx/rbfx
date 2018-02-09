
if (URHO3D_64BIT)
    set (NET_PLATFORM /platform:x64)
elseif (DESKTOP)
    set (NET_PLATFORM /platform:x86)
endif()

file (GLOB SOURCE_FILES_MANUAL *.cs)
set (SOURCE_FILES_GENERATED
    ${CMAKE_CURRENT_BINARY_DIR}/../PInvoke.cs
    ${CMAKE_CURRENT_BINARY_DIR}/../Urho3D.cs
)

foreach (filename ${SOURCE_FILES_GENERATED})
    file (WRITE ${filename} "")
endforeach ()

add_custom_target(Urho3DNet
    COMMAND csc /target:library ${NET_PLATFORM} /reference:System.dll /out:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Urho3DNet.dll
    ${SOURCE_FILES_MANUAL} ${SOURCE_FILES_GENERATED}
)
add_dependencies(Urho3DNet Urho3DCSharp)
