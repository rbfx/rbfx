
SET(POSHLIB_SRCS
	posh.c
	posh.h)

ADD_LIBRARY(posh STATIC ${POSHLIB_SRCS})

# Urho3D: Get rid of it when CMake is updgaded to 3.13
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS posh EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()
