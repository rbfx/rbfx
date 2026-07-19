// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/Serializer.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Navigation/NavBuildData.h"
#include "Urho3D/Navigation/NavigationDefs.h"

#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class NavArea;
class Node;
class OffMeshConnection;

DetourAllocation ReadDetourBuffer(Deserializer& source);
void WriteDetourBuffer(Serializer& dest, const DetourAllocation& buffer);
void WriteDetourBuffer(Serializer& dest, const unsigned char* data, int dataSize);
void WriteDetourBuffer(Serializer& dest, const ConstByteSpan& buffer);

/// Calculate tile offset.
ea::optional<ea::pair<IntVector2, int>> CalculateTileOffset(const IntVector3& delta, int tileSize, float cellSize);

/// Calculate bounding box of given geometry.
URHO3D_API BoundingBox CalculateBoundingBox(const NavigationGeometryInfoVector& geometryList, const Vector3& padding);

/// Calculate bounding box of given geometry within the given tile volume.
URHO3D_API BoundingBox CalculateTileBoundingBox(
    const NavigationGeometryInfoVector& geometryList, const BoundingBox& tileColumn);

/// Calculate maximum number of tiles required to contain the given bounding box.
URHO3D_API unsigned CalculateMaxTiles(const BoundingBox& boundingBox, int tileSize, float cellSize);

/// Deduce area ID when applicable for walkable triangles.
/// @see rcMarkWalkableTriangles
URHO3D_API void DeduceAreaIds(
    float walkableSlopeAngle, const float* vertices, const int* triangles, int numTriangles, unsigned char* areas);

/// Append Node geometry to geometry list.
URHO3D_API void AppendNavigationGeometry(NavigationGeometryInfoVector& geometryList, Node* node,
    const Matrix3x4& inverseRootTransform, unsigned char areaId);

/// Append off-mesh connection to geometry list.
URHO3D_API void AppendOffMessConnection(NavigationGeometryInfoVector& geometryList,
    OffMeshConnection* offMeshConnection, const Matrix3x4& inverseRootTransform);

/// Append navigation area to geometry list.
URHO3D_API void AppendNavArea(
    NavigationGeometryInfoVector& geometryList, NavArea* navArea, const Matrix3x4& inverseRootTransform);

} // namespace Urho3D
