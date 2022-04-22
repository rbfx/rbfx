cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(DE.Android)

add_subdirectory("../../../../../.." Core)
add_subdirectory("../../../../../../Tests/IncludeTest" CoreTests)

add_library(DETestLauncher SHARED main.cpp)

target_link_libraries(DETestLauncher
PUBLIC
	Diligent-Common
	DiligentCore-IncludeTest
	Diligent-GraphicsEngineOpenGL-static
	Diligent-GraphicsEngineVk-static
)
	
set_target_properties(DETestLauncher PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")
