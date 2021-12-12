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

#include "../Core/VariantCurve.h"
#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{

bool SerializeVariantValue(Archive& archive, VariantType variantType, const char* name, Variant& value)
{
    static_assert(MAX_VAR_TYPES == 29, "Update me");
    switch (variantType)
    {
    case VAR_NONE:
        return true;
    case VAR_INT:
        return Detail::SerializeVariantValueType<int>(archive, name, value);
    case VAR_BOOL:
        return Detail::SerializeVariantValueType<bool>(archive, name, value);
    case VAR_FLOAT:
        return Detail::SerializeVariantValueType<float>(archive, name, value);
    case VAR_VECTOR2:
        return Detail::SerializeVariantValueType<Vector2>(archive, name, value);
    case VAR_VECTOR3:
        return Detail::SerializeVariantValueType<Vector3>(archive, name, value);
    case VAR_VECTOR4:
        return Detail::SerializeVariantValueType<Vector4>(archive, name, value);
    case VAR_QUATERNION:
        return Detail::SerializeVariantValueType<Quaternion>(archive, name, value);
    case VAR_COLOR:
        return Detail::SerializeVariantValueType<Color>(archive, name, value);
    case VAR_STRING:
        return Detail::SerializeVariantValueType<ea::string>(archive, name, value);
    case VAR_BUFFER:
        if (archive.IsInput() && !value.GetBufferPtr())
            value = VariantBuffer{};
        if (auto ptr = value.GetBufferPtr())
            return SerializeVectorAsBytes(archive, name, *ptr);
        return false;
    case VAR_RESOURCEREF:
        return Detail::SerializeVariantValueType<ResourceRef>(archive, name, value);
    case VAR_RESOURCEREFLIST:
        return Detail::SerializeVariantValueType<ResourceRefList>(archive, name, value);
    case VAR_VARIANTVECTOR:
        if (archive.IsInput() && !value.GetVariantVectorPtr())
            value = VariantVector{};
        if (auto ptr = value.GetVariantVectorPtr())
            return SerializeVectorAsObjects(archive, name, nullptr, *ptr);
        return false;
    case VAR_VARIANTMAP:
        if (archive.IsInput() && !value.GetVariantMapPtr())
            value = VariantMap{};
        if (auto ptr = value.GetVariantMapPtr())
            return SerializeStringHashMap(archive, name, nullptr, *ptr);
        return false;
    case VAR_INTRECT:
        return Detail::SerializeVariantValueType<IntRect>(archive, name, value);
    case VAR_INTVECTOR2:
        return Detail::SerializeVariantValueType<IntVector2>(archive, name, value);
    case VAR_MATRIX3:
        return Detail::SerializeVariantValueType<Matrix3>(archive, name, value);
    case VAR_MATRIX3X4:
        return Detail::SerializeVariantValueType<Matrix3x4>(archive, name, value);
    case VAR_MATRIX4:
        return Detail::SerializeVariantValueType<Matrix4>(archive, name, value);
    case VAR_DOUBLE:
        return Detail::SerializeVariantValueType<double>(archive, name, value);
    case VAR_STRINGVECTOR:
        if (archive.IsInput() && !value.GetStringVectorPtr())
            value = StringVector{};
        if (auto ptr = value.GetStringVectorPtr())
            return SerializeVectorAsObjects(archive, name, nullptr, *ptr);
        return false;
    case VAR_RECT:
        return Detail::SerializeVariantValueType<Rect>(archive, name, value);
    case VAR_INTVECTOR3:
        return Detail::SerializeVariantValueType<IntVector3>(archive, name, value);
    case VAR_INT64:
        return Detail::SerializeVariantValueType<long long>(archive, name, value);
    case VAR_VOIDPTR:
    case VAR_PTR:
        archive.SetError(Format("Unsupported Variant type of element '{0}'", name));
        return false;
    case VAR_CUSTOM:
        // Even if loading, value should be initialized to default value.
        // It's the only way to know type.
        if (CustomVariantValue* ptr = value.GetCustomVariantValuePtr())
            return ptr->Serialize(archive);

        archive.SetError(Format("Custom Variant is not initialized for element '{0}'", name));
        return false;
    case VAR_VARIANTCURVE:
        return Detail::SerializeVariantValueType<VariantCurve>(archive, name, value);
    case MAX_VAR_TYPES:
    default:
        assert(0);
        return false;
    }
}

}
