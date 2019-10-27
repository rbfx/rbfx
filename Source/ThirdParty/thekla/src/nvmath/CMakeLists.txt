PROJECT(nvmath)
set(CMAKE_DEBUG_POSTFIX d)

SET(MATH_SRCS
	nvmath.h
    Basis.h Basis.cpp
    Box.h
    Color.h
    ConvexHull.h ConvexHull.cpp
    Fitting.h Fitting.cpp
    KahanSum.h
    Matrix.h
    Plane.h Plane.cpp
    ProximityGrid.h ProximityGrid.cpp
    Quaternion.h
	Random.h Random.cpp
    Solver.h Solver.cpp
	Sparse.h Sparse.cpp
    TypeSerialization.h TypeSerialization.cpp
    Vector.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVMATH_EXPORTS)

IF(NVMATH_SHARED)
    ADD_DEFINITIONS(-DNVMATH_SHARED=1)
    ADD_LIBRARY(nvmath SHARED ${MATH_SRCS})
ELSE(NVMATH_SHARED)
    ADD_LIBRARY(nvmath STATIC ${MATH_SRCS}) # Urho3D: Fix library type
ENDIF(NVMATH_SHARED)

TARGET_LINK_LIBRARIES(nvmath ${LIBS} nvcore)

INSTALL(TARGETS nvmath
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static)

# Urho3D: Get rid of it when CMake is updgaded to 3.13
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS nvmath EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()
