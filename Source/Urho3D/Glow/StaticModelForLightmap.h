//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
