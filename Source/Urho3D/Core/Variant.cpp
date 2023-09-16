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

#include "../Core/StringUtils.h"
#include "../IO/VectorBuffer.h"
#include "../Core/VariantCurve.h"

#include <cstring>

namespace Urho3D
{

const Variant Variant::EMPTY { };
const VariantBuffer Variant::emptyBuffer { };
const ResourceRef Variant::emptyResourceRef { };
const ResourceRefList Variant::emptyResourceRefList { };
const VariantMap Variant::emptyVariantMap;
const VariantVector Variant::emptyVariantVector { };
const StringVector Variant::emptyStringVector { };
const VariantCurve Variant::emptyCurve;
const StringVariantMap Variant::emptyStringVariantMap;

static const char* typeNames[] =
{
    "None",
    "Int",
    "Bool",
    "Float",
    "Vector2",
    "Vector3",
    "Vector4",
    "Quaternion",
    "Color",
    "String",
    "Buffer",
    "VoidPtr",
    "ResourceRef",
    "ResourceRefList",
    "VariantVector",
    "VariantMap",
    "IntRect",
    "IntVector2",
    "Ptr",
    "Matrix3",
    "Matrix3x4",
    "Matrix4",
    "Double",
    "StringVector",
    "Rect",
    "IntVector3",
    "Int64",
    "Custom",
    "VariantCurve",
    "StringVariantMap",
    nullptr
};

static_assert(sizeof(typeNames) / sizeof(const char*) == (size_t)MAX_VAR_TYPES + 1, "Variant type name array is out-of-date");

Variant& Variant::operator =(const Variant& rhs)
{
    // Handle custom types separately
    if (rhs.GetType() == VAR_CUSTOM)
    {
        SetCustomVariantValue(*rhs.GetCustomVariantValuePtr());
        return *this;
    }

    // Assign other types here
    SetType(rhs.GetType());

    switch (type_)
    {
    case VAR_STRING:
        value_.string_ = rhs.value_.string_;
        break;

    case VAR_BUFFER:
        value_.buffer_ = rhs.value_.buffer_;
        break;

    case VAR_RESOURCEREF:
        value_.resourceRef_ = rhs.value_.resourceRef_;
        break;

    case VAR_RESOURCEREFLIST:
        value_.resourceRefList_ = rhs.value_.resourceRefList_;
        break;

    case VAR_VARIANTVECTOR:
        value_.variantVector_ = rhs.value_.variantVector_;
        break;

    case VAR_STRINGVECTOR:
        value_.stringVector_ = rhs.value_.stringVector_;
        break;

    case VAR_VARIANTMAP:
        *value_.variantMap_ = *rhs.value_.variantMap_;
        break;

    case VAR_PTR:
        value_.weakPtr_ = rhs.value_.weakPtr_;
        break;

    case VAR_MATRIX3:
        *value_.matrix3_ = *rhs.value_.matrix3_;
        break;

    case VAR_MATRIX3X4:
        *value_.matrix3x4_ = *rhs.value_.matrix3x4_;
        break;

    case VAR_MATRIX4:
        *value_.matrix4_ = *rhs.value_.matrix4_;
        break;

    case VAR_VARIANTCURVE:
        *value_.variantCurve_ = *rhs.value_.variantCurve_;
        break;

    case VAR_STRINGVARIANTMAP:
        *value_.stringVariantMap_ = *rhs.value_.stringVariantMap_;
        break;

    default:
        memcpy(&value_, &rhs.value_, sizeof(VariantValue));     // NOLINT(bugprone-undefined-memory-manipulation)
        break;
    }

    return *this;
}

Variant& Variant::operator =(Variant&& rhs)
{
    // Clear current value
    SetType(VAR_NONE);

    // Handle custom types separately
    if (rhs.GetType() == VAR_CUSTOM)
    {
        SetCustomVariantValue(ea::move(*rhs.GetCustomVariantValuePtr()));
        return *this;
    }

    // Similar to SetType()
    // Call move-ctor for non-POD objects stored inplace.
    // Bitwise copy POD objects and objects stored by pointer.
    type_ = rhs.type_;

    const auto moveConstruct = [&](auto& lhs, auto& rhs)
    {
        using Type = ea::decay_t<decltype(lhs)>;
        new (&lhs) Type(ea::move(rhs));
    };

    switch (type_)
    {
    case VAR_STRING: moveConstruct(value_.string_, rhs.value_.string_); break;

    case VAR_BUFFER: moveConstruct(value_.buffer_, rhs.value_.buffer_); break;

    case VAR_RESOURCEREF: moveConstruct(value_.resourceRef_, rhs.value_.resourceRef_); break;

    case VAR_RESOURCEREFLIST: moveConstruct(value_.resourceRefList_, rhs.value_.resourceRefList_); break;

    case VAR_VARIANTVECTOR: moveConstruct(value_.variantVector_, rhs.value_.variantVector_); break;

    case VAR_STRINGVECTOR: moveConstruct(value_.stringVector_, rhs.value_.stringVector_); break;

    case VAR_PTR: moveConstruct(value_.weakPtr_, rhs.value_.weakPtr_); break;

    default:
        // Clear the moved object so it doesn't call destructor for its value.
        memcpy(&value_, &rhs.value_, sizeof(VariantValue)); // NOLINT(bugprone-undefined-memory-manipulation)
        rhs.type_ = VAR_NONE;
        break;
    }

    return *this;
}

Variant& Variant::operator =(const VectorBuffer& rhs)
{
    SetType(VAR_BUFFER);
    value_.buffer_ = rhs.GetBuffer();
    return *this;
}

Variant& Variant::operator =(const VariantCurve& rhs)
{
    SetType(VAR_VARIANTCURVE);
    *value_.variantCurve_ = rhs;
    return *this;
}

bool Variant::operator ==(const Variant& rhs) const
{
    if (type_ == VAR_VOIDPTR || type_ == VAR_PTR)
        return GetVoidPtr() == rhs.GetVoidPtr();
    else if (type_ == VAR_CUSTOM && rhs.type_ == VAR_CUSTOM)
        return GetCustomVariantValuePtr()->Compare(*rhs.GetCustomVariantValuePtr());
    else if (type_ != rhs.type_)
        return false;

    switch (type_)
    {
    case VAR_INT:
        return value_.int_ == rhs.value_.int_;

    case VAR_INT64:
        return value_.int64_ == rhs.value_.int64_;

    case VAR_BOOL:
        return value_.bool_ == rhs.value_.bool_;

    case VAR_FLOAT:
        return value_.float_ == rhs.value_.float_;

    case VAR_VECTOR2:
        return value_.vector2_ == rhs.value_.vector2_;

    case VAR_VECTOR3:
        return value_.vector3_ == rhs.value_.vector3_;

    case VAR_VECTOR4:
        return value_.vector4_ == rhs.value_.vector4_;

    case VAR_QUATERNION:
        return value_.quaternion_ == rhs.value_.quaternion_;

    case VAR_COLOR:
        return value_.color_ == rhs.value_.color_;

    case VAR_STRING:
        return value_.string_ == rhs.value_.string_;

    case VAR_BUFFER:
        return value_.buffer_ == rhs.value_.buffer_;

    case VAR_RESOURCEREF:
        return value_.resourceRef_ == rhs.value_.resourceRef_;

    case VAR_RESOURCEREFLIST:
        return value_.resourceRefList_ == rhs.value_.resourceRefList_;

    case VAR_VARIANTVECTOR:
        return value_.variantVector_ == rhs.value_.variantVector_;

    case VAR_STRINGVECTOR:
        return value_.stringVector_ == rhs.value_.stringVector_;

    case VAR_VARIANTMAP:
        return *value_.variantMap_ == *rhs.value_.variantMap_;

    case VAR_INTRECT:
        return value_.intRect_ == rhs.value_.intRect_;

    case VAR_INTVECTOR2:
        return value_.intVector2_ == rhs.value_.intVector2_;

    case VAR_INTVECTOR3:
        return value_.intVector3_ == rhs.value_.intVector3_;

    case VAR_MATRIX3:
        return *value_.matrix3_ == *rhs.value_.matrix3_;

    case VAR_MATRIX3X4:
        return *value_.matrix3x4_ == *rhs.value_.matrix3x4_;

    case VAR_MATRIX4:
        return *value_.matrix4_ == *rhs.value_.matrix4_;

    case VAR_DOUBLE:
        return value_.double_ == rhs.value_.double_;

    case VAR_RECT:
        return value_.rect_ == rhs.value_.rect_;

    case VAR_VARIANTCURVE:
        return *value_.variantCurve_ == *rhs.value_.variantCurve_;

    case VAR_STRINGVARIANTMAP:
        return *value_.stringVariantMap_ == *rhs.value_.stringVariantMap_;

    default:
        return true;
    }
}

bool Variant::operator ==(const VariantBuffer& rhs) const
{
    // Use strncmp() instead of VariantBuffer::operator ==()
    const VariantBuffer& buffer = value_.buffer_;
    return type_ == VAR_BUFFER && buffer.size() == rhs.size() ?
        strncmp(reinterpret_cast<const char*>(&buffer[0]), reinterpret_cast<const char*>(&rhs[0]), buffer.size()) == 0 :
        false;
}

bool Variant::operator ==(const VectorBuffer& rhs) const
{
    const VariantBuffer& buffer = value_.buffer_;
    return type_ == VAR_BUFFER && buffer.size() == rhs.GetSize() ?
        strncmp(reinterpret_cast<const char*>(&buffer[0]), reinterpret_cast<const char*>(rhs.GetData()), buffer.size()) == 0 :
        false;
}

bool Variant::operator ==(const VariantCurve& rhs) const
{
    return type_ == VAR_VARIANTCURVE ? *value_.variantCurve_ == rhs : false;
}

Variant::Variant(VariantType type)
{
    SetType(type);
    switch (type)
    {
    case VAR_INT:
        *this = 0;
        break;

    case VAR_INT64:
        *this = 0LL;
        break;

    case VAR_BOOL:
        *this = false;
        break;

    case VAR_FLOAT:
        *this = 0.0f;
        break;

    case VAR_VECTOR2:
        *this = Vector2::ZERO;
        break;

    case VAR_VECTOR3:
        *this = Vector3::ZERO;
        break;

    case VAR_VECTOR4:
        *this = Vector4::ZERO;
        break;

    case VAR_QUATERNION:
        *this = Quaternion::IDENTITY;
        break;

    case VAR_COLOR:
        *this = Color::BLACK;
        break;

    case VAR_STRING:
        *this = EMPTY_STRING;
        break;

    case VAR_VOIDPTR:
        *this = (void*)nullptr;
        break;

    case VAR_INTRECT:
        *this = IntRect::ZERO;
        break;

    case VAR_INTVECTOR2:
        *this = IntVector2::ZERO;
        break;

    case VAR_INTVECTOR3:
        *this = IntVector3::ZERO;
        break;

    case VAR_PTR:
        *this = (RefCounted*)nullptr;
        break;

    case VAR_MATRIX3:
        *this = Matrix3::ZERO;
        break;

    case VAR_MATRIX3X4:
        *this = Matrix3x4::ZERO;
        break;

    case VAR_MATRIX4:
        *this = Matrix4::ZERO;
        break;

    case VAR_DOUBLE:
        *this = 0.0;
        break;

    case VAR_RECT:
        *this = Rect::ZERO;
        break;

    case VAR_VARIANTCURVE:
        *this = emptyCurve;
        break;

    case VAR_BUFFER:
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST:
    case VAR_VARIANTVECTOR:
    case VAR_VARIANTMAP:
    case VAR_STRINGVECTOR:
    case VAR_STRINGVARIANTMAP:
        SetType(type);
        break;

    default:
        SetType(VAR_NONE);
    }
}

void Variant::FromString(const ea::string& type, const ea::string& value)
{
    return FromString(GetTypeFromName(type), value.c_str());
}

void Variant::FromString(const char* type, const char* value)
{
    return FromString(GetTypeFromName(type), value);
}

void Variant::FromString(VariantType type, const ea::string& value)
{
    return FromString(type, value.c_str());
}

void Variant::FromString(VariantType type, const char* value)
{
    switch (type)
    {
    case VAR_INT:
        *this = ToInt(value);
        break;

    case VAR_INT64:
        *this = ToInt64(value);
        break;

    case VAR_BOOL:
        *this = ToBool(value);
        break;

    case VAR_FLOAT:
        *this = ToFloat(value);
        break;

    case VAR_VECTOR2:
        *this = ToVector2(value);
        break;

    case VAR_VECTOR3:
        *this = ToVector3(value);
        break;

    case VAR_VECTOR4:
        *this = ToVector4(value);
        break;

    case VAR_QUATERNION:
        *this = ToQuaternion(value);
        break;

    case VAR_COLOR:
        *this = ToColor(value);
        break;

    case VAR_STRING:
        *this = value;
        break;

    case VAR_BUFFER:
        SetType(VAR_BUFFER);
        StringToBuffer(value_.buffer_, value);
        break;

    case VAR_VOIDPTR:
        // From string to void pointer not supported, set to null
        *this = (void*)nullptr;
        break;

    case VAR_RESOURCEREF:
    {
        StringVector values = ea::string::split(value, ';');
        if (values.size() == 2)
        {
            SetType(VAR_RESOURCEREF);
            value_.resourceRef_.type_ = values[0];
            value_.resourceRef_.name_ = values[1];
        }
        break;
    }

    case VAR_RESOURCEREFLIST:
    {
        StringVector values = ea::string::split(value, ';', true);
        if (values.size() >= 1)
        {
            SetType(VAR_RESOURCEREFLIST);
            value_.resourceRefList_.type_ = values[0];
            value_.resourceRefList_.names_.resize(values.size() - 1);
            for (unsigned i = 1; i < values.size(); ++i)
                value_.resourceRefList_.names_[i - 1] = values[i];
        }
        break;
    }

    case VAR_INTRECT:
        *this = ToIntRect(value);
        break;

    case VAR_INTVECTOR2:
        *this = ToIntVector2(value);
        break;

    case VAR_INTVECTOR3:
        *this = ToIntVector3(value);
        break;

    case VAR_PTR:
        // From string to RefCounted pointer not supported, set to null
        *this = (RefCounted*)nullptr;
        break;

    case VAR_MATRIX3:
        *this = ToMatrix3(value);
        break;

    case VAR_MATRIX3X4:
        *this = ToMatrix3x4(value);
        break;

    case VAR_MATRIX4:
        *this = ToMatrix4(value);
        break;

    case VAR_DOUBLE:
        *this = ToDouble(value);
        break;

    case VAR_RECT:
        *this = ToRect(value);
        break;

    default:
        SetType(VAR_NONE);
    }
}

void Variant::SetBuffer(const void* data, unsigned size)
{
    if (size && !data)
        size = 0;

    SetType(VAR_BUFFER);
    VariantBuffer& buffer = value_.buffer_;
    buffer.resize(size);
    if (size)
        memcpy(&buffer[0], data, size);
}

void Variant::SetCustomVariantValue(const CustomVariantValue& value)
{
    URHO3D_ASSERT(value.GetSize() <= VARIANT_VALUE_SIZE);

    // Assign value if destination is already initialized
    if (CustomVariantValue* thisValueWrapped = GetCustomVariantValuePtr())
    {
        if (value.CopyTo(*thisValueWrapped))
            return;
    }

    SetType(VAR_CUSTOM);
    value_.AsCustomValue().~CustomVariantValue();
    value.CloneTo(value_.storage_);
}

void Variant::SetCustomVariantValue(CustomVariantValue&& value)
{
    URHO3D_ASSERT(value.GetSize() <= VARIANT_VALUE_SIZE);

    // Assign value if destination is already initialized
    if (CustomVariantValue* thisValueWrapped = GetCustomVariantValuePtr())
    {
        if (value.MoveTo(*thisValueWrapped))
            return;
    }

    SetType(VAR_CUSTOM);
    value_.AsCustomValue().~CustomVariantValue();
    value.RelocateTo(value_.storage_);
}

VectorBuffer Variant::GetVectorBuffer() const
{
    return VectorBuffer(type_ == VAR_BUFFER ? value_.buffer_ : emptyBuffer);
}

const VariantCurve& Variant::GetVariantCurve() const
{
    return type_ == VAR_VARIANTCURVE ? *value_.variantCurve_ : emptyCurve;
}

const char* const* Variant::GetTypeNameList()
{
    return typeNames;
}

ea::string Variant::GetTypeName() const
{
    return typeNames[type_];
}

ea::string Variant::ToString() const
{
    switch (type_)
    {
    case VAR_INT:
        return ea::to_string(value_.int_);

    case VAR_INT64:
        return ea::to_string(value_.int64_);

    case VAR_BOOL:
        return ToStringBool(value_.bool_);

    case VAR_FLOAT:
        return ea::to_string(value_.float_);

    case VAR_VECTOR2:
        return value_.vector2_.ToString();

    case VAR_VECTOR3:
        return value_.vector3_.ToString();

    case VAR_VECTOR4:
        return value_.vector4_.ToString();

    case VAR_QUATERNION:
        return value_.quaternion_.ToString();

    case VAR_COLOR:
        return value_.color_.ToString();

    case VAR_STRING:
        return value_.string_;

    case VAR_BUFFER:
        {
            const VariantBuffer& buffer = value_.buffer_;
            ea::string ret;
            BufferToString(ret, buffer.data(), buffer.size());
            return ret;
        }

    case VAR_VOIDPTR:
    case VAR_PTR:
        // Pointer serialization not supported (convert to null)
        return ea::string();

    case VAR_INTRECT:
        return value_.intRect_.ToString();

    case VAR_INTVECTOR2:
        return value_.intVector2_.ToString();

    case VAR_INTVECTOR3:
        return value_.intVector3_.ToString();

    case VAR_MATRIX3:
        return value_.matrix3_->ToString();

    case VAR_MATRIX3X4:
        return value_.matrix3x4_->ToString();

    case VAR_MATRIX4:
        return value_.matrix4_->ToString();

    case VAR_DOUBLE:
        return ea::to_string(value_.double_);

    case VAR_RECT:
        return value_.rect_.ToString();

    case VAR_CUSTOM:
        return GetCustomVariantValuePtr()->ToString();

    default:
        // VAR_RESOURCEREF, VAR_RESOURCEREFLIST, VAR_VARIANTVECTOR, VAR_STRINGVECTOR, VAR_VARIANTMAP, VAR_VARIANTCURVE, VAR_STRINGVARIANTMAP
        // Reference string serialization requires typehash-to-name mapping from the context. Can not support here
        // Also variant map or vector string serialization is not supported. XML or binary save should be used instead
        return EMPTY_STRING;
    }
}

bool Variant::IsZero() const
{
    switch (type_)
    {
    case VAR_INT:
        return value_.int_ == 0;

    case VAR_INT64:
        return value_.int64_ == 0;

    case VAR_BOOL:
        return value_.bool_ == false;

    case VAR_FLOAT:
        return value_.float_ == 0.0f;

    case VAR_VECTOR2:
        return value_.vector2_ == Vector2::ZERO;

    case VAR_VECTOR3:
        return value_.vector3_ == Vector3::ZERO;

    case VAR_VECTOR4:
        return value_.vector4_ == Vector4::ZERO;

    case VAR_QUATERNION:
        return value_.quaternion_ == Quaternion::IDENTITY;

    case VAR_COLOR:
        // WHITE is considered empty (i.e. default) color in the Color class definition
        return value_.color_ == Color::WHITE;

    case VAR_STRING:
        return value_.string_.empty();

    case VAR_BUFFER:
        return value_.buffer_.empty();

    case VAR_VOIDPTR:
        return value_.voidPtr_ == nullptr;

    case VAR_RESOURCEREF:
        return value_.resourceRef_.name_.empty();

    case VAR_RESOURCEREFLIST:
    {
        const StringVector& names = value_.resourceRefList_.names_;
        for (auto i = names.begin(); i != names.end(); ++i)
        {
            if (!i->empty())
                return false;
        }
        return true;
    }

    case VAR_VARIANTVECTOR:
        return value_.variantVector_.empty();

    case VAR_STRINGVECTOR:
        return value_.stringVector_.empty();

    case VAR_VARIANTMAP:
        return value_.variantMap_->empty();

    case VAR_INTRECT:
        return value_.intRect_ == IntRect::ZERO;

    case VAR_INTVECTOR2:
        return value_.intVector2_ == IntVector2::ZERO;

    case VAR_INTVECTOR3:
        return value_.intVector3_ == IntVector3::ZERO;

    case VAR_PTR:
        return value_.weakPtr_ == nullptr;

    case VAR_MATRIX3:
        return *value_.matrix3_ == Matrix3::IDENTITY;

    case VAR_MATRIX3X4:
        return *value_.matrix3x4_ == Matrix3x4::IDENTITY;

    case VAR_MATRIX4:
        return *value_.matrix4_ == Matrix4::IDENTITY;

    case VAR_DOUBLE:
        return value_.double_ == 0.0;

    case VAR_RECT:
        return value_.rect_ == Rect::ZERO;

    case VAR_CUSTOM:
        return GetCustomVariantValuePtr()->IsZero();

    case VAR_VARIANTCURVE:
        return *value_.variantCurve_ == emptyCurve;

    case VAR_STRINGVARIANTMAP:
        return value_.stringVariantMap_->empty();

    default:
        return true;
    }
}

void Variant::SetType(VariantType newType)
{
    if (type_ == newType)
        return;

    switch (type_)
    {
    case VAR_STRING:
        value_.string_.~basic_string<char>();
        break;

    case VAR_BUFFER:
        value_.buffer_.~vector<unsigned char>();
        break;

    case VAR_RESOURCEREF:
        value_.resourceRef_.~ResourceRef();
        break;

    case VAR_RESOURCEREFLIST:
        value_.resourceRefList_.~ResourceRefList();
        break;

    case VAR_VARIANTVECTOR:
        value_.variantVector_.~VariantVector();
        break;

    case VAR_STRINGVECTOR:
        value_.stringVector_.~StringVector();
        break;

    case VAR_VARIANTMAP:
        delete value_.variantMap_;
        break;

    case VAR_PTR:
        value_.weakPtr_.~WeakPtr<RefCounted>();
        break;

    case VAR_MATRIX3:
        delete value_.matrix3_;
        break;

    case VAR_MATRIX3X4:
        delete value_.matrix3x4_;
        break;

    case VAR_MATRIX4:
        delete value_.matrix4_;
        break;

    case VAR_CUSTOM:
        value_.AsCustomValue().~CustomVariantValue();
        break;

    case VAR_VARIANTCURVE:
        delete value_.variantCurve_;
        break;

    case VAR_STRINGVARIANTMAP:
        delete value_.stringVariantMap_;
        break;

    default:
        break;
    }

    type_ = newType;

    switch (type_)
    {
    case VAR_STRING:
        new(&value_.string_) ea::string();
        break;

    case VAR_BUFFER:
        new(&value_.buffer_) VariantBuffer();
        break;

    case VAR_RESOURCEREF:
        new(&value_.resourceRef_) ResourceRef();
        break;

    case VAR_RESOURCEREFLIST:
        new(&value_.resourceRefList_) ResourceRefList();
        break;

    case VAR_VARIANTVECTOR:
        new(&value_.variantVector_) VariantVector();
        break;

    case VAR_STRINGVECTOR:
        new(&value_.stringVector_) StringVector();
        break;

    case VAR_VARIANTMAP:
        value_.variantMap_ = new VariantMap();
        break;

    case VAR_PTR:
        new(&value_.weakPtr_) WeakPtr<RefCounted>();
        break;

    case VAR_MATRIX3:
        value_.matrix3_ = new Matrix3();
        break;

    case VAR_MATRIX3X4:
        value_.matrix3x4_ = new Matrix3x4();
        break;

    case VAR_MATRIX4:
        value_.matrix4_ = new Matrix4();
        break;

    case VAR_CUSTOM:
        // Initialize virtual table with void dummy custom object
        new (&value_.storage_) CustomVariantValue();
        break;

    case VAR_VARIANTCURVE:
        value_.variantCurve_ = new VariantCurve();
        break;

    case VAR_STRINGVARIANTMAP:
        value_.stringVariantMap_ = new StringVariantMap();
        break;

    default:
        break;
    }
}

template <> int Variant::Get<int>(int) const
{
    return GetInt();
}

template <> unsigned Variant::Get<unsigned>(int) const
{
    return GetUInt();
}

template <> long long Variant::Get<long long>(int) const
{
    return GetInt64();
}

template <> unsigned long long Variant::Get<unsigned long long>(int) const
{
    return GetUInt64();
}

template <> StringHash Variant::Get<StringHash>(int) const
{
    return GetStringHash();
}

template <> bool Variant::Get<bool>(int) const
{
    return GetBool();
}

template <> float Variant::Get<float>(int) const
{
    return GetFloat();
}

template <> double Variant::Get<double>(int) const
{
    return GetDouble();
}

template <> const Vector2& Variant::Get<const Vector2&>(int) const
{
    return GetVector2();
}

template <> const Vector3& Variant::Get<const Vector3&>(int) const
{
    return GetVector3();
}

template <> const Vector4& Variant::Get<const Vector4&>(int) const
{
    return GetVector4();
}

template <> const Quaternion& Variant::Get<const Quaternion&>(int) const
{
    return GetQuaternion();
}

template <> const Color& Variant::Get<const Color&>(int) const
{
    return GetColor();
}

template <> const ea::string& Variant::Get<const ea::string&>(int) const
{
    return GetString();
}

template <> const Rect& Variant::Get<const Rect&>(int) const
{
    return GetRect();
}

template <> const IntRect& Variant::Get<const IntRect&>(int) const
{
    return GetIntRect();
}

template <> const IntVector2& Variant::Get<const IntVector2&>(int) const
{
    return GetIntVector2();
}

template <> const IntVector3& Variant::Get<const IntVector3&>(int) const
{
    return GetIntVector3();
}

template <> const VariantBuffer& Variant::Get<const VariantBuffer&>(int) const
{
    return GetBuffer();
}

template <> void* Variant::Get<void*>(int) const
{
    return GetVoidPtr();
}

template <> RefCounted* Variant::Get<RefCounted*>(int) const
{
    return GetPtr();
}

template <> const Matrix3& Variant::Get<const Matrix3&>(int) const
{
    return GetMatrix3();
}

template <> const Matrix3x4& Variant::Get<const Matrix3x4&>(int) const
{
    return GetMatrix3x4();
}

template <> const Matrix4& Variant::Get<const Matrix4&>(int) const
{
    return GetMatrix4();
}

template <> const VariantCurve& Variant::Get<const VariantCurve&>(int) const
{
    return GetVariantCurve();
}

template <> ResourceRef Variant::Get<ResourceRef>(int) const
{
    return GetResourceRef();
}

template <> ResourceRefList Variant::Get<ResourceRefList>(int) const
{
    return GetResourceRefList();
}

template <> VariantVector Variant::Get<VariantVector>(int) const
{
    return GetVariantVector();
}

template <> StringVector Variant::Get<StringVector >(int) const
{
    return GetStringVector();
}

template <> VariantMap Variant::Get<VariantMap>(int) const
{
    return GetVariantMap();
}

template <> Vector2 Variant::Get<Vector2>(int) const
{
    return GetVector2();
}

template <> Vector3 Variant::Get<Vector3>(int) const
{
    return GetVector3();
}

template <> Vector4 Variant::Get<Vector4>(int) const
{
    return GetVector4();
}

template <> Quaternion Variant::Get<Quaternion>(int) const
{
    return GetQuaternion();
}

template <> Color Variant::Get<Color>(int) const
{
    return GetColor();
}

template <> ea::string Variant::Get<ea::string>(int) const
{
    return GetString();
}

template <> Rect Variant::Get<Rect>(int) const
{
    return GetRect();
}

template <> IntRect Variant::Get<IntRect>(int) const
{
    return GetIntRect();
}

template <> IntVector2 Variant::Get<IntVector2>(int) const
{
    return GetIntVector2();
}

template <> IntVector3 Variant::Get<IntVector3>(int) const
{
    return GetIntVector3();
}

template <> VariantBuffer Variant::Get<VariantBuffer >(int) const
{
    return GetBuffer();
}

template <> Matrix3 Variant::Get<Matrix3>(int) const
{
    return GetMatrix3();
}

template <> Matrix3x4 Variant::Get<Matrix3x4>(int) const
{
    return GetMatrix3x4();
}

template <> Matrix4 Variant::Get<Matrix4>(int) const
{
    return GetMatrix4();
}

template <> VariantCurve Variant::Get<VariantCurve>(int) const
{
    return GetVariantCurve();
}

template <> StringVariantMap Variant::Get<StringVariantMap>(int) const
{
    return GetStringVariantMap();
}

ea::string Variant::GetTypeName(VariantType type)
{
    return typeNames[type];
}

VariantType Variant::GetTypeFromName(const ea::string& typeName)
{
    return GetTypeFromName(typeName.c_str());
}

VariantType Variant::GetTypeFromName(const char* typeName)
{
    return (VariantType)GetStringListIndex(typeName, typeNames, VAR_NONE);
}

Variant Variant::Lerp(const Variant& rhs, float t) const
{
    switch (type_)
    {
    case VAR_FLOAT:
        return Urho3D::Lerp(GetFloat(), rhs.GetFloat(), t);

    case VAR_DOUBLE:
        return Urho3D::Lerp(GetDouble(), rhs.GetDouble(), t);

    case VAR_INT:
        return Urho3D::Lerp(GetInt(), rhs.GetInt(), t);

    case VAR_INT64:
        return Urho3D::Lerp(GetInt64(), rhs.GetInt64(), t);

    case VAR_VECTOR2:
        return GetVector2().Lerp(rhs.GetVector2(), t);

    case VAR_VECTOR3:
        return GetVector3().Lerp(rhs.GetVector3(), t);

    case VAR_VECTOR4:
        return GetVector4().Lerp(rhs.GetVector4(), t);

    case VAR_QUATERNION:
        return GetQuaternion().Slerp(rhs.GetQuaternion(), t);

    case VAR_COLOR:
        return GetColor().Lerp(rhs.GetColor(), t);

    case VAR_INTRECT:
        {
            const float s = 1.0f - t;
            const IntRect& r1 = GetIntRect();
            const IntRect& r2 = rhs.GetIntRect();
            return IntRect(
                static_cast<int>(r1.left_ * s + r2.left_ * t),
                static_cast<int>(r1.top_ * s + r2.top_ * t),
                static_cast<int>(r1.right_ * s + r2.right_ * t),
                static_cast<int>(r1.bottom_ * s + r2.bottom_ * t));
        }

    case VAR_INTVECTOR2:
        {
            const float s = 1.0f - t;
            const IntVector2& v1 = GetIntVector2();
            const IntVector2& v2 = rhs.GetIntVector2();
            return IntVector2(
                static_cast<int>(v1.x_ * s + v2.x_ * t),
                static_cast<int>(v1.y_ * s + v2.y_ * t));
        }

    case VAR_INTVECTOR3:
        {
            const float s = 1.0f - t;
            const IntVector3& v1 = GetIntVector3();
            const IntVector3& v2 = rhs.GetIntVector3();
            return IntVector3(
                static_cast<int>(v1.x_ * s + v2.x_ * t),
                static_cast<int>(v1.y_ * s + v2.y_ * t),
                static_cast<int>(v1.z_ * s + v2.z_ * t));
        }

    default:
        return *this;
    }
}

unsigned Variant::ToHash() const
{
    switch (GetType())
    {
    case Urho3D::VAR_NONE:
        return 0;
    case Urho3D::VAR_INT:
        return MakeHash(Get<int>());
    case Urho3D::VAR_BOOL:
        return MakeHash(Get<bool>());
    case Urho3D::VAR_FLOAT:
        return MakeHash(Get<float>());
    case Urho3D::VAR_VECTOR2:
        return MakeHash(Get<Urho3D::Vector2>());
    case Urho3D::VAR_VECTOR3:
        return MakeHash(Get<Urho3D::Vector3>());
    case Urho3D::VAR_VECTOR4:
        return MakeHash(Get<Urho3D::Vector4>());
    case Urho3D::VAR_QUATERNION:
        return MakeHash(Get<Urho3D::Quaternion>());
    case Urho3D::VAR_COLOR:
        return MakeHash(Get<Urho3D::Color>());
    case Urho3D::VAR_STRING:
        return MakeHash(Get<ea::string>());
    case Urho3D::VAR_BUFFER:
        return MakeHash(Get<ea::vector<unsigned char>>());
    case Urho3D::VAR_VOIDPTR:
        return MakeHash(Get<void*>());
    case Urho3D::VAR_RESOURCEREF:
        return MakeHash(Get<Urho3D::ResourceRef>());
    case Urho3D::VAR_RESOURCEREFLIST:
        return MakeHash(Get<Urho3D::ResourceRefList>());
    case Urho3D::VAR_VARIANTVECTOR:
        return MakeHash(Get<Urho3D::VariantVector>());
    case Urho3D::VAR_VARIANTMAP:
        return MakeHash(Get<Urho3D::VariantMap>());
    case Urho3D::VAR_INTRECT:
        return MakeHash(Get<Urho3D::IntRect>());
    case Urho3D::VAR_INTVECTOR2:
        return MakeHash(Get<Urho3D::IntVector2>());
    case Urho3D::VAR_PTR:
        return MakeHash(Get<Urho3D::RefCounted*>());
    case Urho3D::VAR_MATRIX3:
        return MakeHash(Get<Urho3D::Matrix3>());
    case Urho3D::VAR_MATRIX3X4:
        return MakeHash(Get<Urho3D::Matrix3x4>());
    case Urho3D::VAR_MATRIX4:
        return MakeHash(Get<Urho3D::Matrix4>());
    case Urho3D::VAR_DOUBLE:
        return MakeHash(Get<double>());
    case Urho3D::VAR_STRINGVECTOR:
        return MakeHash(Get<Urho3D::StringVector>());
    case Urho3D::VAR_RECT:
        return MakeHash(Get<Urho3D::Rect>());
    case Urho3D::VAR_INTVECTOR3:
        return MakeHash(Get<Urho3D::IntVector3>());
    case Urho3D::VAR_INT64:
        return MakeHash(Get<long long>());
    case Urho3D::VAR_VARIANTCURVE:
        return MakeHash(Get<VariantCurve>());
    case Urho3D::VAR_STRINGVARIANTMAP:
        return MakeHash(Get<StringVariantMap>());
    case Urho3D::VAR_CUSTOM:
    default:
        assert(false);
        return 0;
    }
}

unsigned GetVariantTypeSize(VariantType variant)
{
    switch (variant)
    {
    case VAR_NONE:
        return 0;
    case VAR_INT:
        return sizeof(int);
    case VAR_INT64:
        return sizeof(long long);
    case VAR_BOOL:
        return sizeof(bool);
    case VAR_FLOAT:
        return sizeof(float);
    case VAR_DOUBLE:
        return sizeof(double);
    case VAR_VECTOR2:
        return sizeof(Vector2);
    case VAR_VECTOR3:
        return sizeof(Vector3);
    case VAR_VECTOR4:
        return sizeof(Vector4);
    case VAR_QUATERNION:
        return sizeof(Quaternion);
    case VAR_COLOR:
        return sizeof(Color);
    case VAR_STRING:
        return sizeof(ea::string);
    case VAR_BUFFER:
        return sizeof(VariantBuffer);
    case VAR_RESOURCEREF:
        return sizeof(ResourceRef);
    case VAR_RESOURCEREFLIST:
        return sizeof(ResourceRefList);
    case VAR_VARIANTVECTOR:
        return sizeof(VariantVector);
    case VAR_STRINGVECTOR:
        return sizeof(StringVector);
    case VAR_VARIANTMAP:
        return sizeof(VariantMap);
    case VAR_RECT:
        return sizeof(Rect);
    case VAR_INTRECT:
        return sizeof(IntRect);
    case VAR_INTVECTOR2:
        return sizeof(IntVector2);
    case VAR_INTVECTOR3:
        return sizeof(IntVector3);
    case VAR_MATRIX3:
        return sizeof(Matrix3);
    case VAR_MATRIX3X4:
        return sizeof(Matrix3x4);
    case VAR_MATRIX4:
        return sizeof(Matrix4);
    case VAR_STRINGVARIANTMAP:
        return sizeof(StringVariantMap);
    default:
        assert(!"Unsupported value type");
        return 0;
    }
}

}
