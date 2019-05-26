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

#pragma once

#include "../IO//ArchiveSerialization.h"

namespace Urho3D
{

bool SerializeVariantValue(Archive& ar, VariantType variantType, const char* name, Variant& value)
{
    static_assert(MAX_VAR_TYPES == 29, "Update me");
    switch (variantType)
    {
    case VAR_NONE:
        return true;
    case VAR_INT:
        return Detail::SerializeVariantValueType<int>(ar, name, value);
    case VAR_BOOL:
        return Detail::SerializeVariantValueType<bool>(ar, name, value);
    case VAR_FLOAT:
        return Detail::SerializeVariantValueType<float>(ar, name, value);
    case VAR_VECTOR2:
        return Detail::SerializeVariantValueType<Vector2>(ar, name, value);
    case VAR_VECTOR3:
        return Detail::SerializeVariantValueType<Vector3>(ar, name, value);
    case VAR_VECTOR4:
        return Detail::SerializeVariantValueType<Vector4>(ar, name, value);
    case VAR_QUATERNION:
        return Detail::SerializeVariantValueType<Quaternion>(ar, name, value);
    case VAR_COLOR:
        return Detail::SerializeVariantValueType<Color>(ar, name, value);
    case VAR_STRING:
        return Detail::SerializeVariantValueType<ea::string>(ar, name, value);
    case VAR_BUFFER:
        if (auto ptr = value.GetBufferPtr())
        {
            return false;
            //return Detail::SerializeVariantValueType<ea::vector<unsigned char>>(ar, name, value);
        }
        return false;
    case VAR_VOIDPTR:
        return false;
    case VAR_RESOURCEREF:
        return Detail::SerializeVariantValueType<ResourceRef>(ar, name, value);
    case VAR_RESOURCEREFLIST:
        return Detail::SerializeVariantValueType<ResourceRefList>(ar, name, value);
    case VAR_VARIANTVECTOR:
        if (ar.IsInput() && !value.GetVariantVectorPtr())
            value = VariantVector{};
        if (auto ptr = value.GetVariantVectorPtr())
            return SerializeVector(ar, name, nullptr, *ptr);
        return false;
    case VAR_VARIANTMAP:
        if (ar.IsInput() && !value.GetVariantMapPtr())
            value = VariantMap{};
        if (auto ptr = value.GetVariantMapPtr())
            return SerializeStringHashMap(ar, name, nullptr, *ptr);
        return false;
    case VAR_INTRECT:
        return Detail::SerializeVariantValueType<IntRect>(ar, name, value);
    case VAR_INTVECTOR2:
        return Detail::SerializeVariantValueType<IntVector2>(ar, name, value);
    case VAR_PTR:
        return false;
    case VAR_MATRIX3:
        return Detail::SerializeVariantValueType<Matrix3>(ar, name, value);
    case VAR_MATRIX3X4:
        return Detail::SerializeVariantValueType<Matrix3x4>(ar, name, value);
    case VAR_MATRIX4:
        return Detail::SerializeVariantValueType<Matrix4>(ar, name, value);
    case VAR_DOUBLE:
        return Detail::SerializeVariantValueType<double>(ar, name, value);
    case VAR_STRINGVECTOR:
        if (ar.IsInput() && !value.GetStringVectorPtr())
            value = StringVector{};
        if (auto ptr = value.GetStringVectorPtr())
            return SerializeVector(ar, name, nullptr, *ptr);
        return false;
    case VAR_RECT:
        return Detail::SerializeVariantValueType<Rect>(ar, name, value);
    case VAR_INTVECTOR3:
        return Detail::SerializeVariantValueType<IntVector3>(ar, name, value);
    case VAR_INT64:
        return Detail::SerializeVariantValueType<long long>(ar, name, value);
    case VAR_CUSTOM_HEAP:
    case VAR_CUSTOM_STACK:
    case MAX_VAR_TYPES:
    default:
        assert(0);
        return false;
    }
}

}
