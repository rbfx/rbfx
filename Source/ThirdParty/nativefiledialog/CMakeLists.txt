
list (APPEND SOURCE_FILES
    src/include/nfd.h
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
#target_include_directories(nativefiledialog PUBLIC src/include) 
target_include_directories(nativefiledialog
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/src/include>
)
if (WIN32)
    target_link_libraries(nativefiledialog shell32 ole32 uuid)
endif ()

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

install(TARGETS nativefiledialog EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})