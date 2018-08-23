
list (APPEND SOURCE_FILES
    src/include/nativefiledialog/nfd.h
    src/common.h
    src/nfd_common.c
    src/nfd_common.h
)
if (APPLE)
    list (APPEND SOURCE_FILES src/nfd_cocoa.m)
elseif (UNIX)
    list (APPEND SOURCE_FILES src/nfd_linux.cpp)
elseif (WIN32)
    list (APPEND SOURCE_FILES src/nfd_win.cpp)
endif ()
add_library(nativefiledialog STATIC ${SOURCE_FILES})
target_include_directories(nativefiledialog
    PRIVATE src/include/nativefiledialog
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
)
if (BUILD_SHARED_LIBS)
    target_compile_definitions(nativefiledialog PRIVATE -DNFD_EXPORTS=1)
endif ()
if (WIN32)
    target_link_libraries(nativefiledialog shell32 ole32 uuid)
elseif (APPLE)
    target_link_libraries(nativefiledialog "-framework Foundation" "-framework AppKit")
endif ()

install (DIRECTORY src/include/nativefiledialog DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/nativefiledialog FILES_MATCHING PATTERN *.h)
