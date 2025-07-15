// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Vector3.h"

#include <EASTL/vector.h>

class rcContext;

struct dtTileCacheContourSet;
struct dtTileCachePolyMesh;
struct dtTileCacheAlloc;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcHeightfield;
struct rcHeightfieldLayerSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;

namespace Urho3D
{

/// Navigation area stub.
struct URHO3D_API NavAreaStub
{
    /// Area bounding box.
    BoundingBox bounds_;
    /// Area ID.
    unsigned char areaID_;
};

/// Navigation build data.
struct URHO3D_API NavBuildData
{
    /// Constructor.
    NavBuildData();
    /// Destructor.
    virtual ~NavBuildData();

    /// World-space bounding box of the navigation mesh tile.
    BoundingBox worldBoundingBox_;
    /// Vertices from geometries.
    ea::vector<Vector3> vertices_;
    /// Triangle indices from geometries.
    ea::vector<int> indices_;
    /// Triangle area IDs.
    ea::vector<unsigned char> areaIds_;
    /// Offmesh connection vertices.
    ea::vector<Vector3> offMeshVertices_;
    /// Offmesh connection radii.
    ea::vector<float> offMeshRadii_;
    /// Offmesh connection flags.
    ea::vector<unsigned short> offMeshFlags_;
    /// Offmesh connection areas.
    ea::vector<unsigned char> offMeshAreas_;
    /// Offmesh connection direction.
    ea::vector<unsigned char> offMeshDir_;
    /// Recast context.
    rcContext* ctx_;
    /// Recast heightfield.
    rcHeightfield* heightField_;
    /// Recast compact heightfield.
    rcCompactHeightfield* compactHeightField_;
    /// Pretransformed navigation areas, no correlation to the geometry above.
    ea::vector<NavAreaStub> navAreas_;
};

struct URHO3D_API SimpleNavBuildData : public NavBuildData
{
    /// Constructor.
    SimpleNavBuildData();
    /// Descturctor.
    ~SimpleNavBuildData() override;

    /// Recast contour set.
    rcContourSet* contourSet_;
    /// Recast poly mesh.
    rcPolyMesh* polyMesh_;
    /// Recast detail poly mesh.
    rcPolyMeshDetail* polyMeshDetail_;
};

/// @nobind
struct URHO3D_API DynamicNavBuildData : public NavBuildData
{
    /// Constructor.
    explicit DynamicNavBuildData(dtTileCacheAlloc* allocator);
    /// Destructor.
    ~DynamicNavBuildData() override;

    /// TileCache specific recast contour set.
    dtTileCacheContourSet* contourSet_;
    /// TileCache specific recast poly mesh.
    dtTileCachePolyMesh* polyMesh_;
    /// Recast heightfield layer set.
    rcHeightfieldLayerSet* heightFieldLayers_;
    /// Allocator from DynamicNavigationMesh instance.
    dtTileCacheAlloc* alloc_;
};

} // namespace Urho3D
