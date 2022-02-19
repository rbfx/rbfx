//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Core/Macros.h"
#include "../IO/Serializer.h"
#include "../Scene/Serializable.h"

#include "../DebugNew.h"

namespace Urho3D
{

static const float q = 32767.0f;

Serializer::~Serializer() = default;

bool Serializer::WriteInt64(long long value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteInt(int value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteShort(short value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteByte(signed char value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteUInt64(unsigned long long value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteUInt(unsigned value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteUShort(unsigned short value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteUByte(unsigned char value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteBool(bool value)
{
    return WriteUByte((unsigned char)(value ? 1 : 0)) == 1;
}

bool Serializer::WriteFloat(float value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteDouble(double value)
{
    return Write(&value, sizeof value) == sizeof value;
}

bool Serializer::WriteIntRect(const IntRect& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteIntVector2(const IntVector2& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteIntVector3(const IntVector3& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteRect(const Rect& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteVector2(const Vector2& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteVector3(const Vector3& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WritePackedVector3(const Vector3& value, float maxAbsCoord)
{
    short coords[3];
    float v = 32767.0f / maxAbsCoord;

    coords[0] = (short)Round(Clamp(value.x_, -maxAbsCoord, maxAbsCoord) * v);
    coords[1] = (short)Round(Clamp(value.y_, -maxAbsCoord, maxAbsCoord) * v);
    coords[2] = (short)Round(Clamp(value.z_, -maxAbsCoord, maxAbsCoord) * v);
    return Write(&coords[0], sizeof coords) == sizeof coords;
}

bool Serializer::WriteVector4(const Vector4& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteQuaternion(const Quaternion& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WritePackedQuaternion(const Quaternion& value)
{
    short coords[4];
    Quaternion norm = value.Normalized();

    coords[0] = (short)Round(Clamp(norm.w_, -1.0f, 1.0f) * q);
    coords[1] = (short)Round(Clamp(norm.x_, -1.0f, 1.0f) * q);
    coords[2] = (short)Round(Clamp(norm.y_, -1.0f, 1.0f) * q);
    coords[3] = (short)Round(Clamp(norm.z_, -1.0f, 1.0f) * q);
    return Write(&coords[0], sizeof coords) == sizeof coords;
}

bool Serializer::WriteMatrix3(const Matrix3& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteMatrix3x4(const Matrix3x4& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteMatrix4(const Matrix4& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteColor(const Color& value)
{
    return Write(value.Data(), sizeof value) == sizeof value;
}

bool Serializer::WriteBoundingBox(const BoundingBox& value)
{
    bool success = true;
    success &= WriteVector3(value.min_);
    success &= WriteVector3(value.max_);
    return success;
}

bool Serializer::WriteString(const ea::string& value)
{
    const char* chars = value.c_str();
    // Count length to the first zero, because ReadString() does the same
    unsigned length = CStringLength(chars);
    return Write(chars, length + 1) == length + 1;
}

bool Serializer::WriteFileID(const ea::string& value)
{
    bool success = true;
    unsigned length = Min(value.length(), 4U);

    success &= Write(value.c_str(), length) == length;
    for (unsigned i = value.length(); i < 4; ++i)
        success &= WriteByte(' ');
    return success;
}

bool Serializer::WriteStringHash(const StringHash& value)
{
    return WriteUInt(value.Value());
}

bool Serializer::WriteBuffer(const ea::vector<unsigned char>& value)
{
    bool success = true;
    unsigned size = value.size();

    success &= WriteVLE(size);
    if (size)
        success &= Write(&value[0], size) == size;
    return success;
}

bool Serializer::WriteResourceRef(const ResourceRef& value)
{
    bool success = true;
    success &= WriteStringHash(value.type_);
    success &= WriteString(value.name_);
    return success;
}

bool Serializer::WriteResourceRefList(const ResourceRefList& value)
{
    bool success = true;

    success &= WriteStringHash(value.type_);
    success &= WriteVLE(value.names_.size());
    for (unsigned i = 0; i < value.names_.size(); ++i)
        success &= WriteString(value.names_[i]);

    return success;
}

bool Serializer::WriteVariant(const Variant& value)
{
    bool success = true;
    VariantType type = value.GetType();

    success &= WriteUByte((unsigned char)type);
    success &= WriteVariantData(value);
    return success;
}

bool Serializer::WriteVariantData(const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_NONE:
        return true;

    case VAR_INT:
        return WriteInt(value.GetInt());

    case VAR_INT64:
        return WriteInt64(value.GetInt64());

    case VAR_BOOL:
        return WriteBool(value.GetBool());

    case VAR_FLOAT:
        return WriteFloat(value.GetFloat());

    case VAR_VECTOR2:
        return WriteVector2(value.GetVector2());

    case VAR_VECTOR3:
        return WriteVector3(value.GetVector3());

    case VAR_VECTOR4:
        return WriteVector4(value.GetVector4());

    case VAR_QUATERNION:
        return WriteQuaternion(value.GetQuaternion());

    case VAR_COLOR:
        return WriteColor(value.GetColor());

    case VAR_STRING:
        return WriteString(value.GetString());

    case VAR_BUFFER:
        return WriteBuffer(value.GetBuffer());

        // Serializing pointers and custom values is not supported. Write null
    case VAR_CUSTOM:
    {
        if (const Serializable* object = value.GetCustom<SharedPtr<Serializable>>())
        {
            WriteUInt(object->GetType().Value());
            return object->Save(*this);
        }
        // When this variant contains null SharedPtr<Serializable> or something else entirely - write a null. This indicates empty value.
        URHO3D_FALLTHROUGH;
    }

    case VAR_VOIDPTR:
    case VAR_PTR:
        return WriteUInt(0);

    case VAR_RESOURCEREF:
        return WriteResourceRef(value.GetResourceRef());

    case VAR_RESOURCEREFLIST:
        return WriteResourceRefList(value.GetResourceRefList());

    case VAR_VARIANTVECTOR:
        return WriteVariantVector(value.GetVariantVector());

    case VAR_STRINGVECTOR:
        return WriteStringVector(value.GetStringVector());

    case VAR_RECT:
        return WriteRect(value.GetRect());

    case VAR_VARIANTMAP:
        return WriteVariantMap(value.GetVariantMap());

    case VAR_INTRECT:
        return WriteIntRect(value.GetIntRect());

    case VAR_INTVECTOR2:
        return WriteIntVector2(value.GetIntVector2());

    case VAR_INTVECTOR3:
        return WriteIntVector3(value.GetIntVector3());

    case VAR_MATRIX3:
        return WriteMatrix3(value.GetMatrix3());

    case VAR_MATRIX3X4:
        return WriteMatrix3x4(value.GetMatrix3x4());

    case VAR_MATRIX4:
        return WriteMatrix4(value.GetMatrix4());

    case VAR_DOUBLE:
        return WriteDouble(value.GetDouble());

    default:
        return false;
    }
}

bool Serializer::WriteVariantVector(const VariantVector& value)
{
    bool success = true;
    success &= WriteVLE(value.size());
    for (auto i = value.begin(); i != value.end(); ++i)
        success &= WriteVariant(*i);
    return success;
}

bool Serializer::WriteStringVector(const StringVector& value)
{
    bool success = true;
    success &= WriteVLE(value.size());
    for (auto i = value.begin(); i != value.end(); ++i)
        success &= WriteString(*i);
    return success;
}

bool Serializer::WriteVariantMap(const VariantMap& value)
{
    bool success = true;
    success &= WriteVLE(value.size());
    for (auto i = value.begin(); i != value.end(); ++i)
    {
        WriteStringHash(i->first);
        WriteVariant(i->second);
    }
    return success;
}

bool Serializer::WriteVLE(unsigned value)
{
    unsigned char data[4];

    if (value < 0x80)
        return WriteUByte((unsigned char)value);
    else if (value < 0x4000)
    {
        data[0] = (unsigned char)(value | 0x80u);
        data[1] = (unsigned char)(value >> 7u);
        return Write(data, 2) == 2;
    }
    else if (value < 0x200000)
    {
        data[0] = (unsigned char)(value | 0x80u);
        data[1] = (unsigned char)(value >> 7u | 0x80u);
        data[2] = (unsigned char)(value >> 14u);
        return Write(data, 3) == 3;
    }
    else
    {
        data[0] = (unsigned char)(value | 0x80u);
        data[1] = (unsigned char)(value >> 7u | 0x80u);
        data[2] = (unsigned char)(value >> 14u | 0x80u);
        data[3] = (unsigned char)(value >> 21u);
        return Write(data, 4) == 4;
    }
}

bool Serializer::WriteNetID(unsigned value)
{
    return Write(&value, 3) == 3;
}

bool Serializer::WriteLine(const ea::string& value)
{
    bool success = true;
    success &= Write(value.c_str(), value.length()) == value.length();
    success &= WriteUByte(13);
    success &= WriteUByte(10);
    return success;
}

}
