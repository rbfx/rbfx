// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{

/// Collision shape type.
enum ShapeType
{
    SHAPE_BOX = 0,
    SHAPE_SPHERE,
    SHAPE_STATICPLANE,
    SHAPE_CYLINDER,
    SHAPE_CAPSULE,
    SHAPE_CONE,
    SHAPE_TRIANGLEMESH,
    SHAPE_CONVEXHULL,
    SHAPE_TERRAIN,
    SHAPE_GIMPACTMESH
};

} // namespace Urho3D
