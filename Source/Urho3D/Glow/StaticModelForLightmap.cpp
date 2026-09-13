// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Glow/StaticModelForLightmap.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Glow/Helpers.h"
#include "Urho3D/Glow/LightmapUVGenerator.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/StaticModel.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

StaticModelForLightmap::StaticModelForLightmap(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY)
{
}

StaticModelForLightmap::~StaticModelForLightmap() = default;

void StaticModelForLightmap::RegisterObject(Context* context)
{
    context->AddFactoryReflection<StaticModelForLightmap>();
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
