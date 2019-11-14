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

template <class ToType, class FromType, class Converter>
void ConvertArray(unsigned char* dest, const unsigned char* src, unsigned destStride, unsigned srcStride, unsigned count, Converter convert)
{
    for (unsigned i = 0; i < count; ++i)
    {
        FromType sourceValue;
        memcpy(&sourceValue, src, sizeof(sourceValue));
        const ToType destValue = convert(sourceValue);
        memcpy(dest, &destValue, sizeof(destValue));

        dest += destStride;
        src += srcStride;
    }
}

using Ubyte4 = ea::array<unsigned char, 4>;

static Vector4 Ubyte4ToVector4(const Ubyte4& value)
{
    return {
        static_cast<float>(value[0]),
        static_cast<float>(value[1]),
        static_cast<float>(value[2]),
        static_cast<float>(value[3])
    };
}

static unsigned char FloatToUByte(float value)
{
    return static_cast<unsigned char>(Clamp(RoundToInt(value), 0, 255));
}

static Ubyte4 Vector4ToUbyte4(const Vector4& value)
{
    return {
        FloatToUByte(value.x_),
        FloatToUByte(value.y_),
        FloatToUByte(value.z_),
        FloatToUByte(value.w_)
    };
}

static Vector4 Vector4ToVector4(const Vector4& value) { return { value.x_, value.y_, value.z_, value.w_ }; }

static Vector4 IntToVector4(int value) { return { static_cast<float>(value), 0.0f, 0.0f, 0.0f }; }
static Vector4 FloatToVector4(float value) { return { value, 0.0f, 0.0f, 0.0f }; }
static Vector4 Vector2ToVector4(const Vector2& value) { return { value.x_, value.y_, 0.0f, 0.0f }; }
static Vector4 Vector3ToVector4(const Vector3& value) { return { value.x_, value.y_, value.z_, 0.0f }; }
static Vector4 Ubyte4NormToVector4(const Ubyte4& value) { return Ubyte4ToVector4(value) / 255.0f; }

static int Vector4ToInt(const Vector4& value) { return static_cast<int>(value.x_); }
static float Vector4ToFloat(const Vector4& value) { return value.x_; }
static Vector2 Vector4ToVector2(const Vector4& value) { return { value.x_, value.y_ }; }
static Vector3 Vector4ToVector3(const Vector4& value) { return { value.x_, value.y_, value.z_ }; }
static Ubyte4 Vector4ToUbyte4Norm(const Vector4& value) { return Vector4ToUbyte4(value * 255.0f); }

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

static bool UnpackVertexBuffer(ea::vector<ModelVertex>& dest,
    const unsigned char* src, unsigned count, unsigned stride, const ea::vector<VertexElement>& elements)
{
    if (count == 0)
        return true;

    if (!CheckVertexElements(elements))
    {
        return false;
    }

    const unsigned start = dest.size();
    dest.resize(start + count);

    for (const VertexElement& element : elements)
    {
        const int destOffset = GetModelVertexElementOffset(element.semantic_, element.index_);
        unsigned char* destElementArray = reinterpret_cast<unsigned char*>(&dest[start]) + destOffset;
        const unsigned char* srcElementArray = src + element.offset_;

        switch (element.type_)
        {
        case TYPE_INT:
            ConvertArray<Vector4, int>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, IntToVector4);
            break;
        case TYPE_FLOAT:
            ConvertArray<Vector4, float>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, FloatToVector4);
            break;
        case TYPE_VECTOR2:
            ConvertArray<Vector4, Vector2>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, Vector2ToVector4);
            break;
        case TYPE_VECTOR3:
            ConvertArray<Vector4, Vector3>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, Vector3ToVector4);
            break;
        case TYPE_VECTOR4:
            ConvertArray<Vector4, Vector4>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, Vector4ToVector4);
            break;
        case TYPE_UBYTE4:
            ConvertArray<Vector4, Ubyte4>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, Ubyte4ToVector4);
            break;
        case TYPE_UBYTE4_NORM:
            ConvertArray<Vector4, Ubyte4>(destElementArray, srcElementArray, sizeof(ModelVertex), stride, count, Ubyte4NormToVector4);
            break;
        default:
            break;
        }
    }

    return true;
}

static void PackVertexBuffer(VariantBuffer& dest, ea::span<const ModelVertex> src, const ea::vector<VertexElement>& elements)
{
    if (src.empty())
        return;

    const unsigned count = src.size();
    const unsigned stride = VertexBuffer::GetVertexSize(elements);
    const unsigned start = dest.size();
    dest.resize(start + count * stride);

    for (const VertexElement& element : elements)
    {
        const int srcOffset = GetModelVertexElementOffset(element.semantic_, element.index_);
        const unsigned char* srcElementArray = reinterpret_cast<const unsigned char*>(src.data()) + srcOffset;
        unsigned char* destElementArray = reinterpret_cast<unsigned char*>(&dest[start] + element.offset_);

        switch (element.type_)
        {
        case TYPE_INT:
            ConvertArray<int, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToInt);
            break;
        case TYPE_FLOAT:
            ConvertArray<float, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToFloat);
            break;
        case TYPE_VECTOR2:
            ConvertArray<Vector2, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToVector2);
            break;
        case TYPE_VECTOR3:
            ConvertArray<Vector3, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToVector3);
            break;
        case TYPE_VECTOR4:
            ConvertArray<Vector4, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToVector4);
            break;
        case TYPE_UBYTE4:
            ConvertArray<Ubyte4, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToUbyte4);
            break;
        case TYPE_UBYTE4_NORM:
            ConvertArray<Ubyte4, Vector4>(destElementArray, srcElementArray, stride, sizeof(ModelVertex), count, Vector4ToUbyte4Norm);
            break;
        default:
            assert(0);
            break;
        }
    }
}

static bool UnpackIndexBuffer(ea::vector<unsigned>& dest, const unsigned char* src, unsigned count, unsigned stride)
{
    if (count == 0)
        return true;

    if (stride != 2 && stride != 4)
    {
        return false;
    }

    const unsigned start = dest.size();
    dest.resize(start + count);

    if (stride == 4)
    {
        memcpy(&dest[start], src, count * stride);
    }
    else
    {
        const unsigned short* srcData = reinterpret_cast<const unsigned short*>(src);
        for (unsigned i = 0; i < count; ++i)
        {
            // May by unaligned
            unsigned short index{};
            memcpy(&index, &src[i * stride], sizeof(index));
            dest[start + i] = index;
        }
    }

    return true;
}

static void PackIndexBuffer(VariantBuffer& dest, ea::span<const unsigned> src, bool largeIndices)
{
    if (src.empty())
        return;

    const unsigned count = src.size();
    const unsigned stride = largeIndices ? 4 : 2;
    const unsigned start = dest.size();
    dest.resize(dest.size() + count * stride);

    if (largeIndices)
    {
        memcpy(&dest[start], src.data(), count * stride);
    }
    else
    {
        for (unsigned i = 0; i < count; ++i)
        {
            const unsigned short index = static_cast<unsigned short>(src[i]);
            memcpy(&dest[start + i * stride], &index, sizeof(index));
        }
    }
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
    vertexBuffers_.clear();
    ea::vector<SharedPtr<VertexBuffer>> sourceVertexBuffers = model->GetVertexBuffers();
    for (VertexBuffer* sourceBuffer : sourceVertexBuffers)
    {
        VertexBufferData dest;
        dest.elements_ = sourceBuffer->GetElements();

        const unsigned char* data = sourceBuffer->GetShadowData();
        const unsigned count = sourceBuffer->GetVertexCount();
        const unsigned stride = sourceBuffer->GetVertexSize();

        if (!UnpackVertexBuffer(dest.vertices_, data, count, stride, dest.elements_))
        {
            return false;
        }

        vertexBuffers_.push_back(ea::move(dest));
    }

    // Read index buffers
    indexBuffers_.clear();
    ea::vector<SharedPtr<IndexBuffer>> sourceIndexBuffers = model->GetIndexBuffers();
    for (IndexBuffer* sourceBuffer : sourceIndexBuffers)
    {
        const unsigned char* data = sourceBuffer->GetShadowData();
        const unsigned count = sourceBuffer->GetIndexCount();
        const unsigned stride = sourceBuffer->GetIndexSize();

        IndexBufferData dest;
        if (!UnpackIndexBuffer(dest.indices_, data, count, stride))
        {
            return false;
        }

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
    ea::vector<SharedPtr<VertexBuffer>> modelVertexBuffers;
    for (VertexBufferData& sourceBuffer : vertexBuffers_)
    {
        VertexBuffer::UpdateOffsets(sourceBuffer.elements_);

        ea::vector<unsigned char> verticesData;
        PackVertexBuffer(verticesData, sourceBuffer.vertices_, sourceBuffer.elements_);

        const unsigned count = sourceBuffer.vertices_.size();

        auto vertexBuffer = MakeShared<VertexBuffer>(context_);
        vertexBuffer->SetShadowed(true);
        vertexBuffer->SetSize(count, sourceBuffer.elements_);
        vertexBuffer->SetData(verticesData.data());

        modelVertexBuffers.push_back(vertexBuffer);
    }
    model->SetVertexBuffers(modelVertexBuffers, {}, {});

    // Write index buffers
    ea::vector<SharedPtr<IndexBuffer>> modelIndexBuffers;
    for (const IndexBufferData& sourceBuffer : indexBuffers_)
    {
        const bool largeIndices = sourceBuffer.HasLargeIndexes();

        ea::vector<unsigned char> indicesData;
        PackIndexBuffer(indicesData, sourceBuffer.indices_, largeIndices);

        const unsigned count = sourceBuffer.indices_.size();

        auto indexBuffer = MakeShared<IndexBuffer>(context_);
        indexBuffer->SetShadowed(true);
        indexBuffer->SetSize(count, largeIndices);
        indexBuffer->SetData(indicesData.data());

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
