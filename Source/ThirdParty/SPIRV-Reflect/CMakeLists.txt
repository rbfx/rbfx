list(APPEND SOURCE_FILES
    spirv_reflect.c
    spirv_reflect.h
    include/spirv/unified1/spirv.h
)

add_library(SPIRV-Reflect STATIC ${SOURCE_FILES})
target_include_directories(SPIRV-Reflect SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
)

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
