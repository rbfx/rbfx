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

// This file contains VertexBuffer code common to all graphics APIs.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../Math/MathDefs.h"

#include <EASTL/array.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

/// Convert array of values with given convertor. Types must be trivially copyable.
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

/// Helper type for unsigned byte vector.
using Ubyte4 = ea::array<unsigned char, 4>;

/// Convert unsigned byte vector to float vector.
static Vector4 Ubyte4ToVector4(const Ubyte4& value)
{
    return {
        static_cast<float>(value[0]),
        static_cast<float>(value[1]),
        static_cast<float>(value[2]),
        static_cast<float>(value[3])
    };
}

/// Convert float to unsigned byte (with clamping).
static unsigned char FloatToUByte(float value)
{
    return static_cast<unsigned char>(Clamp(RoundToInt(value), 0, 255));
}

/// Convert float vector to unsigned byte vector.
static Ubyte4 Vector4ToUbyte4(const Vector4& value)
{
    return {
        FloatToUByte(value.x_),
        FloatToUByte(value.y_),
        FloatToUByte(value.z_),
        FloatToUByte(value.w_)
    };
}

/// No-op converter from float vector to float vector.
static Vector4 Vector4ToVector4(const Vector4& value) { return { value.x_, value.y_, value.z_, value.w_ }; }

/// Trivial converters from and to Vector4.
/// @{
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
/// @}

}

extern const char* GEOMETRY_CATEGORY;

VertexBuffer::VertexBuffer(Context* context, bool forceHeadless) :
    Object(context),
    GPUObject(forceHeadless ? nullptr : GetSubsystem<Graphics>())
{
    UpdateOffsets();

    // Force shadowing mode if graphics subsystem does not exist
    if (!graphics_)
        shadowed_ = true;
}

VertexBuffer::~VertexBuffer()
{
    Release();
}

void VertexBuffer::RegisterObject(Context* context)
{
    context->RegisterFactory<VertexBuffer>();
}

void VertexBuffer::SetShadowed(bool enable)
{
    // If no graphics subsystem, can not disable shadowing
    if (!graphics_)
        enable = true;

    if (enable != shadowed_)
    {
        if (enable && vertexSize_ && vertexCount_)
            shadowData_ = new unsigned char[vertexCount_ * vertexSize_];
        else
            shadowData_.reset();

        shadowed_ = enable;
    }
}

bool VertexBuffer::SetSize(unsigned vertexCount, unsigned elementMask, bool dynamic)
{
    return SetSize(vertexCount, GetElements(elementMask), dynamic);
}

bool VertexBuffer::SetSize(unsigned vertexCount, const ea::vector<VertexElement>& elements, bool dynamic)
{
    Unlock();

    vertexCount_ = vertexCount;
    elements_ = elements;
    dynamic_ = dynamic;

    UpdateOffsets();

    if (shadowed_ && vertexCount_ && vertexSize_)
        shadowData_ = new unsigned char[vertexCount_ * vertexSize_];
    else
        shadowData_.reset();

    return Create();
}

void VertexBuffer::UpdateOffsets()
{
    unsigned elementOffset = 0;
    elementHash_ = 0;
    elementMask_ = MASK_NONE;

    for (auto i = elements_.begin(); i != elements_.end(); ++i)
    {
        i->offset_ = elementOffset;
        elementOffset += ELEMENT_TYPESIZES[i->type_];
        elementHash_ <<= 6;
        elementHash_ += (((int)i->type_ + 1) * ((int)i->semantic_ + 1) + i->index_);

        for (unsigned j = 0; j < MAX_LEGACY_VERTEX_ELEMENTS; ++j)
        {
            const VertexElement& legacy = LEGACY_VERTEXELEMENTS[j];
            if (i->type_ == legacy.type_ && i->semantic_ == legacy.semantic_ && i->index_ == legacy.index_)
                elementMask_ |= VertexMaskFlags(1u << j);
        }
    }

    vertexSize_ = elementOffset;
}

const VertexElement* VertexBuffer::GetElement(VertexElementSemantic semantic, unsigned char index) const
{
    for (auto i = elements_.begin(); i != elements_.end(); ++i)
    {
        if (i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(VertexElementType type, VertexElementSemantic semantic, unsigned char index) const
{
    for (auto i = elements_.begin(); i != elements_.end(); ++i)
    {
        if (i->type_ == type && i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(const ea::vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    for (auto i = elements.begin(); i != elements.end(); ++i)
    {
        if (i->type_ == type && i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

bool VertexBuffer::HasElement(const ea::vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    return GetElement(elements, type, semantic, index) != nullptr;
}

unsigned VertexBuffer::GetElementOffset(const ea::vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    const VertexElement* element = GetElement(elements, type, semantic, index);
    return element ? element->offset_ : M_MAX_UNSIGNED;
}

ea::vector<VertexElement> VertexBuffer::GetElements(unsigned elementMask)
{
    ea::vector<VertexElement> ret;

    for (unsigned i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (elementMask & (1u << i))
            ret.push_back(LEGACY_VERTEXELEMENTS[i]);
    }

    return ret;
}

unsigned VertexBuffer::GetVertexSize(const ea::vector<VertexElement>& elements)
{
    unsigned size = 0;

    for (unsigned i = 0; i < elements.size(); ++i)
        size += ELEMENT_TYPESIZES[elements[i].type_];

    return size;
}

unsigned VertexBuffer::GetVertexSize(unsigned elementMask)
{
    unsigned size = 0;

    for (unsigned i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (elementMask & (1u << i))
            size += ELEMENT_TYPESIZES[LEGACY_VERTEXELEMENTS[i].type_];
    }

    return size;
}

void VertexBuffer::UpdateOffsets(ea::vector<VertexElement>& elements)
{
    unsigned elementOffset = 0;

    for (auto i = elements.begin(); i != elements.end(); ++i)
    {
        i->offset_ = elementOffset;
        elementOffset += ELEMENT_TYPESIZES[i->type_];
    }
}

void VertexBuffer::UnpackVertexData(const void* source, unsigned stride,
    const VertexElement& element, unsigned start, unsigned count, Vector4* dest)
{
    const unsigned char* sourceBytes = reinterpret_cast<const unsigned char*>(source) + element.offset_ + start * stride;
    unsigned char* destBytes = reinterpret_cast<unsigned char*>(dest);

    switch (element.type_)
    {
    case TYPE_INT:
        ConvertArray<Vector4, int>(destBytes, sourceBytes, sizeof(Vector4), stride, count, IntToVector4);
        break;
    case TYPE_FLOAT:
        ConvertArray<Vector4, float>(destBytes, sourceBytes, sizeof(Vector4), stride, count, FloatToVector4);
        break;
    case TYPE_VECTOR2:
        ConvertArray<Vector4, Vector2>(destBytes, sourceBytes, sizeof(Vector4), stride, count, Vector2ToVector4);
        break;
    case TYPE_VECTOR3:
        ConvertArray<Vector4, Vector3>(destBytes, sourceBytes, sizeof(Vector4), stride, count, Vector3ToVector4);
        break;
    case TYPE_VECTOR4:
        ConvertArray<Vector4, Vector4>(destBytes, sourceBytes, sizeof(Vector4), stride, count, Vector4ToVector4);
        break;
    case TYPE_UBYTE4:
        ConvertArray<Vector4, Ubyte4>(destBytes, sourceBytes, sizeof(Vector4), stride, count, Ubyte4ToVector4);
        break;
    case TYPE_UBYTE4_NORM:
        ConvertArray<Vector4, Ubyte4>(destBytes, sourceBytes, sizeof(Vector4), stride, count, Ubyte4NormToVector4);
        break;
    default:
        assert(0);
        break;
    }
}

void VertexBuffer::PackVertexData(const Vector4* source, void* dest, unsigned stride, const VertexElement& element, unsigned start, unsigned count)
{
    const unsigned char* sourceBytes = reinterpret_cast<const unsigned char*>(source);
    unsigned char* destBytes = reinterpret_cast<unsigned char*>(dest) + element.offset_ + start * stride;

    switch (element.type_)
    {
    case TYPE_INT:
        ConvertArray<int, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToInt);
        break;
    case TYPE_FLOAT:
        ConvertArray<float, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToFloat);
        break;
    case TYPE_VECTOR2:
        ConvertArray<Vector2, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToVector2);
        break;
    case TYPE_VECTOR3:
        ConvertArray<Vector3, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToVector3);
        break;
    case TYPE_VECTOR4:
        ConvertArray<Vector4, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToVector4);
        break;
    case TYPE_UBYTE4:
        ConvertArray<Ubyte4, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToUbyte4);
        break;
    case TYPE_UBYTE4_NORM:
        ConvertArray<Ubyte4, Vector4>(destBytes, sourceBytes, stride, sizeof(Vector4), count, Vector4ToUbyte4Norm);
        break;
    default:
        assert(0);
        break;
    }
}

}
