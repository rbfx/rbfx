PROJECT(nvcore)
set(CMAKE_DEBUG_POSTFIX d)

SET(CORE_SRCS
	nvcore.h
	DefsGnucDarwin.h
	DefsGnucLinux.h
	DefsGnucWin32.h
	DefsVcWin32.h
	Ptr.h
	RefCounted.h
	BitArray.h
	Memory.h
	Memory.cpp
	Debug.h
	Debug.cpp
	StrLib.h
	StrLib.cpp
	Stream.h
	StdStream.h
	FileSystem.h
	FileSystem.cpp
	RadixSort.h
	RadixSort.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVCORE_EXPORTS)

IF(UNIX)
    SET(LIBS ${LIBS} ${CMAKE_DL_LIBS})
ENDIF(UNIX)

IF(NVCORE_SHARED)
    ADD_DEFINITIONS(-DNVCORE_SHARED=1)
    ADD_LIBRARY(nvcore SHARED ${CORE_SRCS})
ELSE(NVCORE_SHARED)
	ADD_LIBRARY(nvcore ${CORE_SRCS})
ENDIF(NVCORE_SHARED)

TARGET_LINK_LIBRARIES(nvcore ${LIBS})

INSTALL(TARGETS nvcore
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
