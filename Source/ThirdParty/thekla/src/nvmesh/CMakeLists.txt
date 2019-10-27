PROJECT(nvmesh)
set(CMAKE_DEBUG_POSTFIX d)

SET(MESH_SRCS
	nvmesh.h
	BaseMesh.h
	BaseMesh.cpp
	MeshBuilder.h
	MeshBuilder.cpp
	TriMesh.h
	TriMesh.cpp
	QuadTriMesh.h
	QuadTriMesh.cpp
	MeshTopology.h
	MeshTopology.cpp
	halfedge/Edge.h
	halfedge/Edge.cpp
	halfedge/Mesh.h
	halfedge/Mesh.cpp
	halfedge/Face.h
	halfedge/Face.cpp
	halfedge/Vertex.h
	halfedge/Vertex.cpp
	geometry/Bounds.h
	geometry/Bounds.cpp
	geometry/Measurements.h
	geometry/Measurements.cpp
	raster/Raster.h
	raster/Raster.cpp
	raster/ClippedTriangle.h
	param/Atlas.h
	param/Atlas.cpp
	param/AtlasBuilder.h
	param/AtlasBuilder.cpp
	param/AtlasPacker.h
	param/AtlasPacker.cpp
	param/LeastSquaresConformalMap.h
	param/LeastSquaresConformalMap.cpp
	param/OrthogonalProjectionMap.h
	param/OrthogonalProjectionMap.cpp
	param/ParameterizationQuality.h
	param/ParameterizationQuality.cpp
	param/SingleFaceMap.h
	param/SingleFaceMap.cpp
	param/Util.h
	param/Util.cpp
	weld/VertexWeld.h
	weld/VertexWeld.cpp
	weld/Weld.h
	weld/Snap.h
	weld/Snap.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# if Tootle
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/render/tootle/include)
#SET(LIBS ${LIBS} ${CMAKE_CURRENT_SOURCE_DIR}/render/tootle/lib/libTootle.a)

# targets
ADD_DEFINITIONS(-DNVMESH_EXPORTS)

IF(NVMESH_SHARED)	
	ADD_LIBRARY(nvmesh SHARED ${MESH_SRCS})
ELSE(NVMESH_SHARED)
	ADD_LIBRARY(nvmesh STATIC ${MESH_SRCS}) # Urho3D: Fix library type
ENDIF(NVMESH_SHARED)

TARGET_LINK_LIBRARIES(nvmesh ${LIBS} nvcore nvmath nvimage)

INSTALL(TARGETS nvmesh
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

# Urho3D: Get rid of it when CMake is updgaded to 3.13
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS nvmesh EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()
