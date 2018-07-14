
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
add_library(nativefiledialog ${THIRDPARTY_LIB_TYPE} ${SOURCE_FILES})
target_include_directories(nativefiledialog
    PRIVATE src/include/nativefiledialog
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
)
if (URHO3D_THIRDPARTY_SHARED_LIBS)
    target_compile_definitions(nativefiledialog PRIVATE -D_EXPORTS=1)
endif ()
if (WIN32)
    target_link_libraries(nativefiledialog shell32 ole32 uuid)
elseif (APPLE)
    target_link_libraries(nativefiledialog "-framework Foundation" "-framework AppKit")
endif ()

install (TARGETS nativefiledialog EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)
install (DIRECTORY src/include/nativefiledialog DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/nativefiledialog FILES_MATCHING PATTERN *.h)
