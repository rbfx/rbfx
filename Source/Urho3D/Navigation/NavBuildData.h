// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/NonCopyable.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Vector2.h"
#include "Urho3D/Math/Vector3.h"
#include "Urho3D/Navigation/NavigationDefs.h"

#include <Recast/Recast.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

class rcContext;

struct dtTileCacheCompressor;
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

struct URHO3D_API NavTileData : public NonCopyable
{
    ~NavTileData();
    /// Release owned data without destruction. Should be called if data is successfully added into navigation mesh.
    void Release();

    unsigned char* data{};
    int dataSize{};
};

using NavTileDataPtr = ea::unique_ptr<NavTileData>;

/// Navigation build data.
struct URHO3D_API NavBuildData : public NonCopyable
{
    NavBuildData();
    virtual ~NavBuildData();

    /// Return whether the tile is empty and there is nothing to build.
    bool IsEmpty() const { return vertices_.empty() || indices_.empty(); }

    /// Tile index.
    IntVector2 tileIndex_;
    /// Volume of the tile, ignoring geometry.
    BoundingBox tileColumn_;
    /// Bounding box containing all geometry in the tile.
    BoundingBox tileBoundingBox_;
    /// Extended bounding box containing all geometry important for tile building.
    BoundingBox collectGeometryBoundingBox_;

    /// Partition type.
    NavmeshPartitionType partitionType_{};
    /// Navigation agent height.
    float agentHeight_{};
    /// Navigation agent radius.
    float agentRadius_{};
    /// Navigation agent max vertical climb.
    float agentMaxClimb_{};

    /// Recast configuration for building.
    rcConfig recastConfig_{};

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

    /// Compiled navigation mesh tile.
    NavTileDataPtr tileData_;
};

struct URHO3D_API DynamicNavBuildData : public NavBuildData
{
    /// Constructor.
    explicit DynamicNavBuildData(const ea::shared_ptr<dtTileCacheCompressor>& compressor);
    /// Destructor.
    ~DynamicNavBuildData() override;

    /// Used to compress and decompress tiles.
    ea::shared_ptr<dtTileCacheCompressor> compressor_;

    /// Recast heightfield layer set.
    rcHeightfieldLayerSet* heightFieldLayers_{};

    /// Compiled navigation mesh tiles.
    ea::vector<NavTileDataPtr> tileData_;
};

} // namespace Urho3D
