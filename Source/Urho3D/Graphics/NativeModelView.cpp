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

#include "../Graphics/NativeModelView.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/VertexBuffer.h"

#include <EASTL/span.h>

namespace Urho3D
{

static_assert(ModelVertex::MaxColors == 4 && ModelVertex::MaxUVs == 4, "Update ModelVertex::VertexElements!");

const ea::vector<VertexElement> ModelVertex::VertexElements =
{
    VertexElement{ TYPE_VECTOR4, SEM_POSITION },
    VertexElement{ TYPE_VECTOR4, SEM_NORMAL },
    VertexElement{ TYPE_VECTOR4, SEM_TANGENT },
    VertexElement{ TYPE_VECTOR4, SEM_BINORMAL },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 0 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 1 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 2 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 3 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 0 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 1 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 2 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 3 },
};

static int GetModelVertexElementOffset(VertexElementSemantic sem, unsigned index)
{
    switch (sem)
    {
    case SEM_POSITION:  return offsetof(ModelVertex, position_);
    case SEM_NORMAL:    return offsetof(ModelVertex, normal_);
    case SEM_BINORMAL:  return offsetof(ModelVertex, binormal_);
    case SEM_TANGENT:   return offsetof(ModelVertex, tangent_);
    case SEM_TEXCOORD:  return offsetof(ModelVertex, uv_) + index * sizeof(Vector4);
    case SEM_COLOR:     return offsetof(ModelVertex, color_) + index * sizeof(Vector4);

    case SEM_BLENDWEIGHTS:
    case SEM_BLENDINDICES:
    case SEM_OBJECTINDEX:
    default:
        return -1;
    }
}

static bool CheckVertexElements(const ea::vector<VertexElement>& elements)
{
    for (const VertexElement& element : elements)
    {
        if (GetModelVertexElementOffset(element.semantic_, element.index_) < 0)
        {
            return false;
        }

        if (element.semantic_ == SEM_COLOR)
        {
            if (element.index_ >= ModelVertex::MaxColors)
            {
                return false;
            }
        }
        else if (element.semantic_ == SEM_TEXCOORD)
        {
            if (element.index_ >= ModelVertex::MaxUVs)
            {
                return false;
            }
        }
        else
        {
            if (element.index_ > 0)
            {
                return false;
            }
        }
    }
    return true;
}

static ea::vector<ModelVertex> GetVertexBufferData(VertexBuffer* vertexBuffer)
{
    const unsigned vertexCount = vertexBuffer->GetVertexCount();
    const ea::vector<Vector4> unpackedData = vertexBuffer->GetUnpackedData();

    ea::vector<ModelVertex> result(vertexCount);
    VertexBuffer::ShuffleUnpackedVertexData(vertexCount, unpackedData.data(), vertexBuffer->GetElements(),
        reinterpret_cast<Vector4*>(result.data()), ModelVertex::VertexElements);

    return result;
}

static void SetVertexBufferData(VertexBuffer* vertexBuffer, const ea::vector<ModelVertex>& data)
{
    const unsigned vertexCount = vertexBuffer->GetVertexCount();
    const ea::vector<VertexElement>& vertexElements = vertexBuffer->GetElements();

    ea::vector<Vector4> buffer(vertexElements.size() * vertexCount);
    VertexBuffer::ShuffleUnpackedVertexData(vertexCount,
        reinterpret_cast<const Vector4*>(data.data()), ModelVertex::VertexElements, buffer.data(), vertexElements);

    vertexBuffer->SetUnpackedData(buffer.data());
}

static bool CompareVertexElementSemantics(const VertexElement& lhs, const VertexElement& rhs)
{
    return lhs.semantic_ == rhs.semantic_ && lhs.index_ == rhs.index_;
}

bool ModelVertex::ReplaceElement(const ModelVertex& source, const VertexElement& element)
{
    switch (element.semantic_)
    {
    case SEM_POSITION:
        position_ = source.position_;
        return true;

    case SEM_NORMAL:
        normal_ = source.normal_;
        return true;

    case SEM_BINORMAL:
        binormal_ = source.binormal_;
        return true;

    case SEM_TANGENT:
        tangent_ = source.tangent_;
        return true;

    case SEM_TEXCOORD:
        if (element.index_ >= MaxUVs)
            return false;
        uv_[element.index_] = source.uv_[element.index_];
        return true;

    case SEM_COLOR:
        if (element.index_ >= MaxColors)
            return false;
        color_[element.index_] = source.color_[element.index_];
        return true;

    case SEM_BLENDWEIGHTS:
    case SEM_BLENDINDICES:
    case SEM_OBJECTINDEX:
    default:
        assert(0);
        return false;
    }
}

void ModelVertex::Repair()
{
    normal_.w_ = 0;
    binormal_.w_ = 0;

    if (HasNormal())
    {
        if (HasTangent())
        {
            const bool hasBinormal = HasBinormal();
            const bool hasTangentBinormalCombined = HasTangentBinormalCombined();

            if (hasTangentBinormalCombined && !hasBinormal)
            {
                // Repair binormal from tangent and normal
                const Vector3 normal3 = static_cast<Vector3>(tangent_);
                const Vector3 tangent3 = static_cast<Vector3>(normal_);
                const Vector3 binormal3 = tangent_.w_ * normal3.CrossProduct(tangent3);
                binormal_ = { binormal3.Normalized(), 0};
            }
            else if (hasBinormal && !hasTangentBinormalCombined)
            {
                // Repair tangent W component from binormal, tangent and normal
                const Vector3 normal3 = static_cast<Vector3>(tangent_);
                const Vector3 tangent3 = static_cast<Vector3>(normal_);
                const Vector3 binormal3 = static_cast<Vector3>(binormal_);
                const Vector3 crossBinormal = normal3.CrossProduct(tangent3);
                tangent_.w_ = crossBinormal.DotProduct(binormal3) >= 0 ? 1.0f : -1.0f;
            }
        }
        else
        {
            // Reset binormal if tangent is missing
            binormal_ = Vector4::ZERO;
        }
    }
    else
    {
        // Reset tangent and binormal if normal is missing
        tangent_ = Vector4::ZERO;
        binormal_ = Vector4::ZERO;
    }
}

bool NativeModelView::ImportModel(Model* model)
{
    if (!model)
    {
        return false;
    }

    boundingBox_ = model->GetBoundingBox();

    // Read vertex buffers
    ea::vector<Vector4> conversionBuffer;
    vertexBuffers_.clear();
    ea::vector<SharedPtr<VertexBuffer>> sourceVertexBuffers = model->GetVertexBuffers();
    for (VertexBuffer* sourceBuffer : sourceVertexBuffers)
    {
        VertexBufferData dest;
        dest.elements_ = sourceBuffer->GetElements();

        if (!CheckVertexElements(dest.elements_))
        {
            return false;
        }

        dest.vertices_ = GetVertexBufferData(sourceBuffer);
        vertexBuffers_.push_back(ea::move(dest));
    }

    // Read index buffers
    indexBuffers_.clear();
    ea::vector<SharedPtr<IndexBuffer>> sourceIndexBuffers = model->GetIndexBuffers();
    for (IndexBuffer* sourceBuffer : sourceIndexBuffers)
    {
        IndexBufferData dest;
        dest.indices_ = sourceBuffer->GetUnpackedData();

        indexBuffers_.push_back(ea::move(dest));
    }

    // Read geometries
    const unsigned numGeometries = model->GetNumGeometries();
    geometries_.resize(numGeometries);
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        const unsigned numLods = model->GetNumGeometryLodLevels(geometryIndex);
        geometries_[geometryIndex].center_ = model->GetGeometryCenter(geometryIndex);
        geometries_[geometryIndex].lods_.resize(numLods);
        for (unsigned lodIndex = 0; lodIndex < numLods; ++lodIndex)
        {
            const Geometry* sourceGeometry = model->GetGeometry(geometryIndex, lodIndex);
            ea::vector<SharedPtr<VertexBuffer>> geometryVertexBuffers = sourceGeometry->GetVertexBuffers();
            SharedPtr<IndexBuffer> indexBuffer{ sourceGeometry->GetIndexBuffer() };

            GeometryLODData dest;
            dest.indexStart_ = sourceGeometry->GetIndexStart();
            dest.indexCount_ = sourceGeometry->GetIndexCount();
            dest.vertexStart_ = sourceGeometry->GetVertexStart();
            dest.vertexCount_ = sourceGeometry->GetVertexCount();
            dest.lodDistance_ = sourceGeometry->GetLodDistance();

            if (indexBuffer)
            {
                dest.indexBuffer_ = sourceIndexBuffers.index_of(indexBuffer);
                if (dest.indexBuffer_ >= sourceIndexBuffers.size())
                {
                    return false;
                }
            }

            for (const SharedPtr<VertexBuffer>& vertexBuffer : geometryVertexBuffers)
            {
                const unsigned index = sourceVertexBuffers.index_of(vertexBuffer);
                if (index >= sourceVertexBuffers.size())
                {
                    return false;
                }

                dest.vertexBuffers_.push_back(index);
            }

            geometries_[geometryIndex].lods_[lodIndex] = ea::move(dest);
        }
    }

    // Read metadata
    for (const ea::string& key : model->GetMetadataKeys())
        metadata_.emplace(key, model->GetMetadata(key));

    return true;
}

void NativeModelView::ExportModel(Model* model)
{
    // Set bounding box
    model->SetBoundingBox(boundingBox_);

    // Write vertex buffers
    ea::vector<Vector4> conversionBuffer;
    ea::vector<SharedPtr<VertexBuffer>> modelVertexBuffers;
    for (VertexBufferData& sourceBuffer : vertexBuffers_)
    {
        VertexBuffer::UpdateOffsets(sourceBuffer.elements_);

        auto vertexBuffer = MakeShared<VertexBuffer>(context_);
        vertexBuffer->SetShadowed(true);
        vertexBuffer->SetSize(sourceBuffer.vertices_.size(), sourceBuffer.elements_);
        SetVertexBufferData(vertexBuffer, sourceBuffer.vertices_);

        modelVertexBuffers.push_back(vertexBuffer);
    }
    model->SetVertexBuffers(modelVertexBuffers, {}, {});

    // Write index buffers
    ea::vector<SharedPtr<IndexBuffer>> modelIndexBuffers;
    for (const IndexBufferData& sourceBuffer : indexBuffers_)
    {
        const bool largeIndices = sourceBuffer.HasLargeIndexes();
        const unsigned stride = largeIndices ? 4 : 2;
        const unsigned count = sourceBuffer.indices_.size();

        auto indexBuffer = MakeShared<IndexBuffer>(context_);
        indexBuffer->SetShadowed(true);
        indexBuffer->SetSize(count, largeIndices);
        indexBuffer->SetUnpackedData(sourceBuffer.indices_.data());

        modelIndexBuffers.push_back(indexBuffer);
    }
    model->SetIndexBuffers(modelIndexBuffers);

    // Write geometries
    const unsigned numGeometries = geometries_.size();
    model->SetNumGeometries(numGeometries);
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        const unsigned numLods = geometries_[geometryIndex].lods_.size();
        model->SetNumGeometryLodLevels(geometryIndex, numLods);
        model->SetGeometryCenter(geometryIndex, geometries_[geometryIndex].center_);

        for (unsigned lodIndex = 0; lodIndex < numLods; ++lodIndex)
        {
            const GeometryLODData& sourceGeometry = geometries_[geometryIndex].lods_[lodIndex];
            const unsigned numVertexBuffers = sourceGeometry.vertexBuffers_.size();

            SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);

            geometry->SetNumVertexBuffers(numVertexBuffers);
            for (unsigned i = 0; i < numVertexBuffers; ++i)
                geometry->SetVertexBuffer(i, modelVertexBuffers[sourceGeometry.vertexBuffers_[i]]);
            if (sourceGeometry.indexBuffer_ != M_MAX_UNSIGNED)
                geometry->SetIndexBuffer(modelIndexBuffers[sourceGeometry.indexBuffer_]);
            geometry->SetLodDistance(sourceGeometry.lodDistance_);

            geometry->SetDrawRange(TRIANGLE_LIST,
                sourceGeometry.indexStart_, sourceGeometry.indexCount_,
                sourceGeometry.vertexStart_, sourceGeometry.vertexCount_);

            model->SetGeometry(geometryIndex, lodIndex, geometry);
        }
    }

    // Write metadata
    for (const auto& var : metadata_)
        model->AddMetadata(var.first, var.second);
}

SharedPtr<Model> NativeModelView::ExportModel(ea::string name)
{
    auto model = MakeShared<Model>(context_);
    model->SetName(name);
    ExportModel(model);
    return model;
}

const ea::vector<VertexElement> NativeModelView::GetEffectiveVertexElements() const
{
    ea::vector<VertexElement> elements;
    for (const VertexBufferData& vertexBuffer : vertexBuffers_)
    {
        for (const VertexElement& element : vertexBuffer.elements_)
        {
            if (ea::find(elements.begin(), elements.end(), element, CompareVertexElementSemantics) == elements.end())
                elements.push_back(element);
        }
    }
    return elements;
}

}
