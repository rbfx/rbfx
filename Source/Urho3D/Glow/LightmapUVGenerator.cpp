//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Glow/LightmapUVGenerator.h"

#include <thekla/thekla_atlas.h>

namespace Urho3D
{

const ea::string LightmapUVGenerationSettings::LightmapSizeKey{ "LightmapSize" };
const ea::string LightmapUVGenerationSettings::LightmapDensityKey{ "LightmapDensity" };

struct LightmapVertexMapping
{
    unsigned geometryIndex_{};
    unsigned lod_{};
    unsigned index_{};
};

bool GenerateLightmapUV(ModelView& modelView, const LightmapUVGenerationSettings& settings)
{
    const auto& sourceGeometries = modelView.GetGeometries();

    ea::vector<LightmapVertexMapping> atlasMapping;
    ea::vector<Thekla::Atlas_Input_Vertex> atlasVertices;
    ea::vector<Thekla::Atlas_Input_Face> atlasFaces;

    // Fill input mesh
    // TODO: Do something more clever about connectivity
    unsigned geometryIndex = 0;
    for (const GeometryView& geometryView : sourceGeometries)
    {
        unsigned lodIndex = 0;
        for (const GeometryLODView& geometryLodView : geometryView.lods_)
        {
            const unsigned vertexStart = atlasVertices.size();

            unsigned vertexIndex = 0;
            for (const ModelVertex& vertex : geometryLodView.vertices_)
            {
                Thekla::Atlas_Input_Vertex atlasVertex;

                atlasVertex.first_colocal = atlasVertices.size();

                atlasVertex.position[0] = vertex.position_.x_;
                atlasVertex.position[1] = vertex.position_.y_;
                atlasVertex.position[2] = vertex.position_.z_;

                atlasVertex.normal[0] = vertex.normal_.x_;
                atlasVertex.normal[1] = vertex.normal_.y_;
                atlasVertex.normal[2] = vertex.normal_.z_;

                atlasVertex.uv[0] = vertex.uv_[0].x_;
                atlasVertex.uv[1] = vertex.uv_[0].y_;

                atlasVertices.push_back(atlasVertex);
                atlasMapping.push_back({ geometryIndex, lodIndex, vertexIndex });

                ++vertexIndex;
            }

            for (const ModelFace& face : geometryLodView.faces_)
            {
                Thekla::Atlas_Input_Face atlasFace;

                atlasFace.material_index = geometryIndex;
                for (int i = 0; i < 3; ++i)
                    atlasFace.vertex_index[i] = static_cast<int>(vertexStart + face.indices_[i]);

                atlasFaces.push_back(atlasFace);
            }

            ++lodIndex;
        }

        ++geometryIndex;
    }

    // Generate output mesh
    Thekla::Atlas_Input_Mesh atlasMesh;
    atlasMesh.vertex_array = atlasVertices.data();
    atlasMesh.vertex_count = atlasVertices.size();
    atlasMesh.face_array = atlasFaces.data();
    atlasMesh.face_count = atlasFaces.size();

    Thekla::Atlas_Options atlasOptions;
    Thekla::atlas_set_default_options(&atlasOptions);
    atlasOptions.mapper_options.preserve_boundary = true;
    atlasOptions.mapper_options.preserve_uvs = false;

    // disable brute force packing quality, as it has a number of notes about performance
    // and it is turned off in Thekla example in repo as well. I am also seeing some meshes
    // having problems packing with it and hanging on import
    atlasOptions.packer_options.witness.packing_quality = 1;

    atlasOptions.packer_options.witness.texels_per_unit = settings.minDensity_;
    atlasOptions.packer_options.witness.conservative = true;

    Thekla::Atlas_Error error = Thekla::Atlas_Error_Success;
    Thekla::Atlas_Output_Mesh* outputMesh = Thekla::atlas_generate(&atlasMesh, &atlasOptions, &error);

    if (error != Thekla::Atlas_Error_Success)
    {
        return false;
    }

    // Reserve geometries
    ea::vector<GeometryView> newGeometries;
    newGeometries.resize(sourceGeometries.size());
    for (unsigned i = 0; i < sourceGeometries.size(); ++i)
        newGeometries[i].lods_.resize(sourceGeometries[i].lods_.size());

    // Copy vertices
    const IntVector2 atlasSize{ outputMesh->atlas_width, outputMesh->atlas_height };

    const float uScale = 1.f / outputMesh->atlas_width;
    const float vScale = 1.f / outputMesh->atlas_height;

    ea::vector<LightmapVertexMapping> outputMeshVerticesMapping;
    outputMeshVerticesMapping.reserve(outputMesh->vertex_count);

    for (int i = 0; i < outputMesh->vertex_count; ++i)
    {
        const Thekla::Atlas_Output_Vertex& outputVertex = outputMesh->vertex_array[i];
        const LightmapVertexMapping& sourceMapping = atlasMapping[outputVertex.xref];

        const GeometryLODView& sourceGeometry = sourceGeometries[sourceMapping.geometryIndex_].lods_[sourceMapping.lod_];
        GeometryLODView& newGeometry = newGeometries[sourceMapping.geometryIndex_].lods_[sourceMapping.lod_];

        // Create new mapping with new index
        LightmapVertexMapping newMapping = sourceMapping;
        newMapping.index_ = newGeometry.vertices_.size();

        // Create new vertex with UV
        ModelVertex newVertex = sourceGeometry.vertices_[sourceMapping.index_];
        newVertex.uv_[settings.uvChannel_].x_ = uScale * outputVertex.uv[0];
        newVertex.uv_[settings.uvChannel_].y_ = vScale * outputVertex.uv[1];

        // Append vertex and register mapping
        newGeometry.vertices_.push_back(newVertex);
        outputMeshVerticesMapping.push_back(newMapping);
    }

    // Copy indices
    for (unsigned faceIndex = 0; faceIndex < outputMesh->index_count / 3; faceIndex++)
    {
        const unsigned index0 = static_cast<unsigned>(outputMesh->index_array[faceIndex * 3]);
        const unsigned index1 = static_cast<unsigned>(outputMesh->index_array[faceIndex * 3 + 1]);
        const unsigned index2 = static_cast<unsigned>(outputMesh->index_array[faceIndex * 3 + 2]);

        const LightmapVertexMapping& map0 = outputMeshVerticesMapping[index0];
        const LightmapVertexMapping& map1 = outputMeshVerticesMapping[index1];
        const LightmapVertexMapping& map2 = outputMeshVerticesMapping[index2];

        // Find geometry and LOD
        const unsigned geometryIndex = map0.geometryIndex_;
        const unsigned lod = map0.lod_;
        if (map1.geometryIndex_ != geometryIndex || map2.geometryIndex_ != geometryIndex || map1.lod_ != lod || map2.lod_ != lod)
        {
            assert(0);
            return false;
        }

        // Copy faces
        GeometryLODView& newGeometry = newGeometries[geometryIndex].lods_[lod];
        newGeometry.faces_.push_back(ModelFace{ { map0.index_, map1.index_, map2.index_ } });
    }

    // Build vertex format
    ModelVertexFormat vertexFormat = modelView.GetVertexFormat();
    vertexFormat.uv_[1] = TYPE_VECTOR2;

    // Finalize
    Thekla::atlas_free(outputMesh);

    modelView.SetGeometries(newGeometries);
    modelView.SetVertexFormat(vertexFormat);

    modelView.AddMetadata(LightmapUVGenerationSettings::LightmapSizeKey, atlasSize);
    modelView.AddMetadata(LightmapUVGenerationSettings::LightmapDensityKey, settings.minDensity_);

    return true;
}

}
