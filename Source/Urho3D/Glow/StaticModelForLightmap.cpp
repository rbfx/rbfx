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

#include "../Precompiled.h"

#include "../Glow/StaticModelForLightmap.h"

#include "../Core/Context.h"
#include "../Glow/Helpers.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/StaticModel.h"

#include "../DebugNew.h"

namespace Urho3D
{

StaticModelForLightmap::StaticModelForLightmap(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY)
{
}

StaticModelForLightmap::~StaticModelForLightmap() = default;

void StaticModelForLightmap::RegisterObject(Context* context)
{
    context->RegisterFactory<StaticModelForLightmap>();
}

GeometryIDToObjectMappingVector StaticModelForLightmap::Initialize(
    unsigned objectIndex, StaticModel* sourceObject, Material* bakingMaterial, unsigned baseGeometryId,
    ea::span<const Vector2> multiTapOffsets, const Vector2& texelSize, const Vector4& scaleOffset,
    const Vector2& scaledAndConstBias)
{
    distance_ = 0.0f;
    lodDistance_ = 0.0f;
    worldBoundingBox_ = BoundingBox{ -Vector3::ONE * M_LARGE_VALUE, Vector3::ONE * M_LARGE_VALUE };

    Model* sourceModel = sourceObject->GetModel();

    const bool sharedLightmapUV = sourceModel->GetMetadata(LightmapUVGenerationSettings::LightmapSharedUV).GetBool();

    GeometryIDToObjectMappingVector mapping;
    for (unsigned geometryIndex = 0; geometryIndex < sourceModel->GetNumGeometries(); ++geometryIndex)
    {
        Material* sourceMaterial = sourceObject->GetMaterial(geometryIndex);
        const unsigned numLods = sourceModel->GetNumGeometryLodLevels(geometryIndex);
        if (numLods == 0)
            continue;

        // Render all lods if necessary, render only one otherwise
        const unsigned numLodsToRender = !sharedLightmapUV ? numLods : 1;
        for (unsigned lodIndex = 0; lodIndex < numLodsToRender; ++lodIndex)
        {
            for (unsigned tap = 0; tap < multiTapOffsets.size(); ++tap)
            {
                const Vector2 tapOffset = multiTapOffsets[tap] * texelSize;
                const Vector4 tapOffset4{ 0.0f, 0.0f, tapOffset.x_, tapOffset.y_ };
                const float tapDepth = 1.0f - static_cast<float>(tap + 1) / (multiTapOffsets.size() + 1);

                SharedPtr<Material> material = CreateBakingMaterial(bakingMaterial, sourceMaterial,
                    scaleOffset, tap, multiTapOffsets.size(), tapOffset, baseGeometryId + mapping.size(),
                    scaledAndConstBias);

                SourceBatch& batch = batches_.push_back();
                batch.distance_ = 0.0f;
                batch.geometry_ = sourceModel->GetGeometry(geometryIndex, lodIndex);
                batch.geometryType_ = GEOM_STATIC_NOINSTANCING;
                batch.material_ = material;
                batch.numWorldTransforms_ = 1;
                batch.worldTransform_ = &node_->GetWorldTransform();
            }

            mapping.push_back(GeometryIDToObjectMapping{ objectIndex, geometryIndex, lodIndex });
        }
    }

    return mapping;
}

}
