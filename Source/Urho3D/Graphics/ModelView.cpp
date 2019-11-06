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

#include "../Graphics/ModelView.h"

#include "../Graphics/VertexBuffer.h"

namespace Urho3D
{

static ModelVertexFormat ParseVertexElements(const ea::vector<VertexElement>& elements)
{
    ModelVertexFormat result;
    for (const VertexElement& element : elements)
    {
        switch (element.semantic_)
        {
        case SEM_POSITION:
            result.position_ = element.type_;
            break;
        case SEM_NORMAL:
            result.normal_ = element.type_;
            break;
        case SEM_BINORMAL:
            result.binormal_ = element.type_;
            break;
        case SEM_TANGENT:
            result.tangent_ = element.type_;
            break;
        case SEM_TEXCOORD:
            if (element.index_ < ModelVertex::MaxUVs)
                result.uv_[element.index_] = element.type_;
            break;
        case SEM_COLOR:
            if (element.index_ < ModelVertex::MaxColors)
                result.color_[element.index_] = element.type_;
            break;

        case SEM_BLENDWEIGHTS:
        case SEM_BLENDINDICES:
        case SEM_OBJECTINDEX:
        default:
            assert(0);
            break;
        }
    }
    return result;
}

static ea::vector<VertexElement> CollectVertexElements(const ModelVertexFormat& vertexFormat)
{
    ea::vector<VertexElement> elements;

    if (vertexFormat.position_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.position_, SEM_POSITION, 0 });

    if (vertexFormat.normal_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.normal_, SEM_NORMAL, 0 });

    if (vertexFormat.binormal_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.binormal_, SEM_BINORMAL, 0 });

    if (vertexFormat.tangent_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.tangent_, SEM_TANGENT, 0 });

    for (unsigned i = 0; i < ModelVertex::MaxUVs; ++i)
    {
        if (vertexFormat.uv_[i] != ModelVertexFormat::Undefined)
            elements.push_back({ vertexFormat.uv_[i], SEM_TEXCOORD, static_cast<unsigned char>(i) });
    }

    for (unsigned i = 0; i < ModelVertex::MaxColors; ++i)
    {
        if (vertexFormat.color_[i] != ModelVertexFormat::Undefined)
            elements.push_back({ vertexFormat.color_[i], SEM_COLOR, static_cast<unsigned char>(i) });
    }

    VertexBuffer::UpdateOffsets(elements);

    return elements;
}

Vector3 GeometryLODView::CalculateCenter() const
{
    Vector3 center;
    for (const ModelVertex& vertex : vertices_)
        center += static_cast<Vector3>(vertex.position_);
    return vertices_.empty() ? Vector3::ZERO : center / static_cast<float>(vertices_.size());
}

bool ModelView::ImportModel(const NativeModelView& nativeView)
{
    vertexFormat_ = ParseVertexElements(nativeView.GetEffectiveVertexElements());

    const auto& nativeVertexBuffers = nativeView.GetVertexBuffers();
    const auto& nativeIndexBuffers = nativeView.GetIndexBuffers();
    const auto& nativeGeometries = nativeView.GetGeometries();

    const unsigned numGeometries = nativeGeometries.size();
    geometries_.resize(numGeometries);
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        const unsigned numLods = nativeGeometries[geometryIndex].lods_.size();
        geometries_[geometryIndex].lods_.resize(numLods);
        for (unsigned lodIndex = 0; lodIndex < numLods; ++lodIndex)
        {
            const NativeModelView::GeometryLODData& nativeGeometry = nativeGeometries[geometryIndex].lods_[lodIndex];

            GeometryLODView geometry;
            geometry.lodDistance_ = nativeGeometry.lodDistance_;

            // Copy indices
            if (nativeGeometry.indexCount_ % 3 != 0 || nativeGeometry.indexBuffer_ >= nativeIndexBuffers.size())
            {
                return false;
            }

            const auto& nativeIndexBuffer = nativeIndexBuffers[nativeGeometry.indexBuffer_];
            const unsigned numFaces = nativeGeometry.indexCount_ / 3;
            geometry.faces_.reserve(numFaces);
            for (unsigned faceIndex = 0; faceIndex < numFaces; ++faceIndex)
            {
                ModelFace face;
                for (unsigned i = 0; i < 3; ++i)
                    face.indices_[i] = nativeIndexBuffer.indices_[faceIndex * 3 + i] - nativeGeometry.vertexStart_;
                geometry.faces_.push_back(face);
            }

            // Copy vertices
            ea::vector<const NativeModelView::VertexBufferData*> vertexBuffers;
            for (const unsigned vertexBufferIndex : nativeGeometry.vertexBuffers_)
            {
                if (vertexBufferIndex >= nativeVertexBuffers.size())
                {
                    return false;
                }

                const NativeModelView::VertexBufferData& vertexBuffer = nativeVertexBuffers[vertexBufferIndex];
                if (nativeGeometry.vertexStart_ + nativeGeometry.vertexCount_ > vertexBuffer.vertices_.size())
                {
                    return false;
                }

                vertexBuffers.push_back(&vertexBuffer);
            }

            const unsigned firstVertex = nativeGeometry.vertexStart_;
            const unsigned lastVertex = nativeGeometry.vertexStart_ + nativeGeometry.vertexCount_;
            geometry.vertices_.reserve(nativeGeometry.vertexCount_);
            for (unsigned vertexIndex = firstVertex; vertexIndex < lastVertex; ++vertexIndex)
            {
                ModelVertex vertex;
                for (const auto vertexBuffer : vertexBuffers)
                {
                    for (const VertexElement& element : vertexBuffer->elements_)
                        vertex.ReplaceElement(vertexBuffer->vertices_[vertexIndex], element);
                }
                vertex.Repair();
                geometry.vertices_.push_back(vertex);
            }

            geometries_[geometryIndex].lods_[lodIndex] = ea::move(geometry);
        }
    }
    return true;
}

void ModelView::ExportModel(NativeModelView& nativeView) const
{
    NativeModelView::VertexBufferData nativeVertexBuffer;
    NativeModelView::IndexBufferData nativeIndexBuffer;
    ea::vector<NativeModelView::GeometryData> nativeGeometries;

    nativeVertexBuffer.elements_ = CollectVertexElements(vertexFormat_);
    for (const GeometryView& sourceGeometry : geometries_)
    {
        NativeModelView::GeometryData nativeGeometry;
        for (const GeometryLODView& sourceGeometryLod : sourceGeometry.lods_)
        {
            NativeModelView::GeometryLODData geometryLod;

            // Initialize geometry metadata
            geometryLod.indexBuffer_ = 0;
            geometryLod.vertexBuffers_ = { 0 };
            geometryLod.indexStart_ = nativeIndexBuffer.indices_.size();
            geometryLod.indexCount_ = sourceGeometryLod.faces_.size() * 3;
            geometryLod.vertexStart_ = nativeVertexBuffer.vertices_.size();
            geometryLod.vertexCount_ = sourceGeometryLod.vertices_.size();

            // Copy vertices
            nativeVertexBuffer.vertices_.insert(nativeVertexBuffer.vertices_.end(),
                sourceGeometryLod.vertices_.begin(), sourceGeometryLod.vertices_.end());

            // Copy indices
            for (const ModelFace& face : sourceGeometryLod.faces_)
            {
                for (unsigned i = 0; i < 3; ++i)
                    nativeIndexBuffer.indices_.push_back(face.indices_[i] + geometryLod.vertexStart_);
            }

            nativeGeometry.lods_.push_back(geometryLod);
        }

        if (!nativeGeometry.lods_.empty())
        {
            // Calculate geometry center
            nativeGeometry.center_ = sourceGeometry.lods_.front().CalculateCenter();;

            // Append geometry
            nativeGeometries.push_back(ea::move(nativeGeometry));
        }
    }

    // Calculate bounding box
    BoundingBox boundingBox;
    for (const ModelVertex& vertex : nativeVertexBuffer.vertices_)
        boundingBox.Merge(static_cast<Vector3>(vertex.position_));

    nativeView.Initialize(boundingBox, { nativeVertexBuffer }, { nativeIndexBuffer }, nativeGeometries);
}

SharedPtr<NativeModelView> ModelView::ExportModel() const
{
    auto nativeModelView = MakeShared<NativeModelView>(context_);
    ExportModel(*nativeModelView);
    return nativeModelView;
}

}
