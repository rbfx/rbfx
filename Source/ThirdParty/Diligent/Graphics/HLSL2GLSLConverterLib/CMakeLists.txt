cmake_minimum_required (VERSION 3.6)

project(Diligent-HLSL2GLSLConverterLib CXX)

set(INCLUDE
    include/GLSLDefinitions.h
    include/HLSL2GLSLConverterImpl.hpp
    include/HLSL2GLSLConverterObject.hpp
)

set(INTERFACE
    interface/HLSL2GLSLConverter.h
)

set(SOURCE
    src/HLSL2GLSLConverterImpl.cpp
    src/HLSL2GLSLConverterObject.cpp
)

set(GLSL_DEFINITIONS include/GLSLDefinitions.h)

set(GENERATED_SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaders)
file(MAKE_DIRECTORY ${GENERATED_SHADERS_DIR})

# We must use the full path, otherwise the build system will not be able to properly detect
# changes and shader conversion custom command will run every time
set(GLSL_DEFINITIONS_INC ${GENERATED_SHADERS_DIR}/GLSLDefinitions_inc.h)
set_source_files_properties(${GLSL_DEFINITIONS_INC} PROPERTIES GENERATED TRUE)

add_library(Diligent-HLSL2GLSLConverterLib STATIC
    ${SOURCE} ${INTERFACE} ${INCLUDE}

    # A target created in the same directory (CMakeLists.txt file) that specifies any output of the
    # custom command as a source file is given a rule to generate the file using the command at build time.
    ${GLSL_DEFINITIONS_INC}
)
set_common_target_properties(Diligent-HLSL2GLSLConverterLib)

target_include_directories(Diligent-HLSL2GLSLConverterLib
PUBLIC
    interface
PRIVATE
    include
    ${GENERATED_SHADERS_DIR}
)

target_link_libraries(Diligent-HLSL2GLSLConverterLib
PRIVATE
    Diligent-BuildSettings
    Diligent-Common
    Diligent-PlatformInterface
    Diligent-GraphicsEngine
PUBLIC
    Diligent-GraphicsEngineInterface
    Diligent-ShaderTools
)

find_package(Python3 REQUIRED)
add_custom_command(OUTPUT ${GLSL_DEFINITIONS_INC} # We must use full path here!
                   COMMAND ${Python3_EXECUTABLE} ${FILE2STRING_PATH} ${GLSL_DEFINITIONS} ${GLSL_DEFINITIONS_INC}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   MAIN_DEPENDENCY ${GLSL_DEFINITIONS}
                   COMMENT "Processing GLSLDefinitions.h"
                   VERBATIM
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("generated" FILES ${GLSL_DEFINITIONS_INC})

set_target_properties(Diligent-HLSL2GLSLConverterLib PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-HLSL2GLSLConverterLib)
endif()
