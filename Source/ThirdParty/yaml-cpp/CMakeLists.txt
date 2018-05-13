project(YAML_CPP)

set(YAML_CPP_VERSION_MAJOR "0")
set(YAML_CPP_VERSION_MINOR "6")
set(YAML_CPP_VERSION_PATCH "2")
set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")

file(GLOB sources "src/[a-zA-Z]*.cpp")
file(GLOB_RECURSE public_headers "include/yaml-cpp/[a-zA-Z]*.h")
file(GLOB private_headers "src/[a-zA-Z]*.h")

if(YAML_CPP_BUILD_CONTRIB)
	file(GLOB contrib_sources "src/contrib/[a-zA-Z]*.cpp")
	file(GLOB contrib_public_headers "include/yaml-cpp/contrib/[a-zA-Z]*.h")
	file(GLOB contrib_private_headers "src/contrib/[a-zA-Z]*.h")
else()
	add_definitions(-DYAML_CPP_NO_CONTRIB)
endif()

# Microsoft VisualC++ specialities
if(MSVC)
	### Project stuff
	# /W3 = set warning level; see http://msdn.microsoft.com/en-us/library/thxezb7y.aspx
	# /wd4127 = disable warning C4127 "conditional expression is constant"; see http://msdn.microsoft.com/en-us/library/6t66728h.aspx
	# /wd4355 = disable warning C4355 "'this' : used in base member initializer list"; http://msdn.microsoft.com/en-us/library/3c594ae3.aspx
	set(yaml_cxx_flags "/W3 /wd4127 /wd4355 ${yaml_cxx_flags}")
endif()

add_library(yaml-cpp OBJECT
    ${sources}
    ${public_headers}
    ${private_headers}
    ${contrib_sources}
    ${contrib_public_headers}
    ${contrib_private_headers}
)
target_include_directories(yaml-cpp
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
