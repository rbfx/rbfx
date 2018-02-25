

file (GLOB SOURCE_FILES_MANUAL *.cpp *.h *.hpp)
set (SOURCE_FILES_GENERATED
    ${CMAKE_CURRENT_BINARY_DIR}/../CApi.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../ClassWrappers.hpp
)

foreach (filename ${SOURCE_FILES_GENERATED})
    if (NOT EXISTS ${filename})
        file (WRITE ${filename} "")
    endif ()
endforeach ()

add_library(Urho3DCSharp SHARED ${SOURCE_FILES_MANUAL} ${SOURCE_FILES_GENERATED})
target_link_libraries(Urho3DCSharp Urho3D)
target_include_directories(Urho3DCSharp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(Urho3DCSharp CSharpGenerateBindings)
