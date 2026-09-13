// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Glow/LightmapUVGenerator.h"

#include <xatlas.h>

namespace Urho3D
{

const ea::string LightmapUVGenerationSettings::LightmapSizeKey{ "LightmapSize" };
const ea::string LightmapUVGenerationSettings::LightmapDensityKey{ "LightmapDensity" };
const ea::string LightmapUVGenerationSettings::LightmapSharedUV{ "LightmapSharedUV" };

bool GenerateLightmapUV(ModelView& modelView, const LightmapUVGenerationSettings& settings)
{
    // Create atlas
    const auto releaseAtlas = [](xatlas::Atlas* atlas)
    {
        xatlas::Destroy(atlas);
    };
    ea::unique_ptr<xatlas::Atlas, decltype(releaseAtlas)> atlas(xatlas::Create(), releaseAtlas);

    // Fill input mesh
    // TODO: Do something more clever about connectivity
    auto& sourceGeometries = modelView.GetGeometries();
    ea::vector<ea::pair<unsigned, unsigned>> meshToGeometryLodMapping;
    unsigned geometryIndex = 0;
    for (const GeometryView& geometryView : sourceGeometries)
    {
        unsigned lodIndex = 0;
        for (const GeometryLODView& geometryLodView : geometryView.lods_)
        {
            if (!geometryLodView.vertices_.empty() && geometryLodView.primitiveType_ == TRIANGLE_LIST)
            {
                xatlas::MeshDecl meshDecl;
                meshDecl.vertexCount = geometryLodView.vertices_.size();
                meshDecl.vertexPositionData = &geometryLodView.vertices_[0].position_;
                meshDecl.vertexPositionStride = sizeof(ModelVertex);
                meshDecl.vertexNormalData = &geometryLodView.vertices_[0].normal_;
                meshDecl.vertexNormalStride = sizeof(ModelVertex);
                meshDecl.indexData = geometryLodView.indices_.data();
                meshDecl.indexCount = geometryLodView.indices_.size();
                meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

                const xatlas::AddMeshError::Enum error = xatlas::AddMesh(atlas.get(), meshDecl);
                if (error != xatlas::AddMeshError::Success)
                    return false;

                meshToGeometryLodMapping.emplace_back(geometryIndex, lodIndex);
            }

            ++lodIndex;
        }

        ++geometryIndex;
    }

    // Generate things
    xatlas::PackOptions packOptions;
    packOptions.padding = 1;
    packOptions.texelsPerUnit = settings.texelPerUnit_;

    xatlas::AddMeshJoin(atlas.get());
    xatlas::Generate(atlas.get(), {}, nullptr, packOptions);

    // Copy output
    const IntVector2 atlasSize{ static_cast<int>(atlas->width), static_cast<int>(atlas->height) };

    const float uScale = 1.f / atlas->width;
    const float vScale = 1.f / atlas->height;

    for (unsigned meshIndex = 0; meshIndex < atlas->meshCount; ++meshIndex)
    {
        const unsigned geometryIndex = meshToGeometryLodMapping[meshIndex].first;
        const unsigned lodIndex = meshToGeometryLodMapping[meshIndex].second;
        GeometryLODView& geometryLodView = sourceGeometries[geometryIndex].lods_[lodIndex];
        const xatlas::Mesh& mesh = atlas->meshes[meshIndex];

        ea::vector<ModelVertex> newVertices;
        for (unsigned vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex)
        {
            const xatlas::Vertex& vertex = mesh.vertexArray[vertexIndex];
            ModelVertex newVertex = geometryLodView.vertices_[vertex.xref];
            newVertex.uv_[settings.uvChannel_].x_ = uScale * vertex.uv[0];
            newVertex.uv_[settings.uvChannel_].y_ = vScale * vertex.uv[1];
            newVertices.push_back(newVertex);
        }

        ea::vector<unsigned> newIndices;
        newIndices.assign(mesh.indexArray, mesh.indexArray + mesh.indexCount);

        geometryLodView.vertices_ = ea::move(newVertices);
        geometryLodView.indices_ = ea::move(newIndices);
        geometryLodView.vertexFormat_.uv_[settings.uvChannel_] = TYPE_VECTOR2;
    }

    // Finalize
    modelView.AddMetadata(LightmapUVGenerationSettings::LightmapSizeKey, atlasSize);
    modelView.AddMetadata(LightmapUVGenerationSettings::LightmapDensityKey, settings.texelPerUnit_);
    modelView.AddMetadata(LightmapUVGenerationSettings::LightmapSharedUV, false);

    return true;
}

}
