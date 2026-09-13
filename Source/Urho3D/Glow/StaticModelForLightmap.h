// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Glow/LightmapGeometryBuffer.h"
#include "../Graphics/Drawable.h"

#include <EASTL/span.h>

namespace Urho3D
{

class Model;
class StaticModel;

/// Static model for rendering into lightmap. Lods, culling and features unrelated to rendering are disabled.
class URHO3D_API StaticModelForLightmap : public Drawable
{
    URHO3D_OBJECT(StaticModelForLightmap, Drawable);

public:
    /// Construct.
    explicit StaticModelForLightmap(Context* context);
    /// Destruct.
    ~StaticModelForLightmap() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Initialize. Return mapping for each batch.
    GeometryIDToObjectMappingVector Initialize(
        unsigned objectIndex, StaticModel* sourceObject, Material* bakingMaterial, unsigned baseGeometryId,
        ea::span<const Vector2> multiTapOffsets, const Vector2& texelSize, const Vector4& scaleOffset,
        const Vector2& scaledAndConstBias);

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override {}
};

}
