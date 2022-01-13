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

#include "../Core/StringUtils.h"
#include "../IO/ArchiveSerialization.h"
#include "../Resource/JSONArchive.h"

namespace Urho3D
{

namespace
{

inline bool IsArchiveBlockJSONArray(ArchiveBlockType type)
{
    return type == ArchiveBlockType::Array || type == ArchiveBlockType::Sequential;
}

inline bool IsArchiveBlockJSONObject(ArchiveBlockType type)
{
    return type == ArchiveBlockType::Unordered;
}

inline bool IsJSONValueCompatibleWithArray(const JSONValue& value)
{
    return value.IsArray() || value.IsNull() || (value.IsObject() && value.GetObject().empty());
}

inline bool IsJSONValueCompatibleWithObject(const JSONValue& value)
{
    return value.IsObject() || value.IsNull() || (value.IsArray() && value.GetArray().empty());
}

inline bool IsArchiveBlockTypeMatching(const JSONValue& value, ArchiveBlockType type)
{
    return (IsArchiveBlockJSONArray(type) && IsJSONValueCompatibleWithArray(value))
        || (IsArchiveBlockJSONObject(type) && IsJSONValueCompatibleWithObject(value));
}

}

JSONOutputArchiveBlock::JSONOutputArchiveBlock(const char* name, ArchiveBlockType type, JSONValue* blockValue, unsigned sizeHint)
    : ArchiveBlockBase(name, type)
    , blockValue_(blockValue)
{
    if (type_ == ArchiveBlockType::Array)
        expectedElementCount_ = sizeHint;

    if (IsArchiveBlockJSONArray(type_))
        blockValue_->SetType(JSON_ARRAY);
    else if (IsArchiveBlockJSONObject(type_))
        blockValue_->SetType(JSON_OBJECT);
    else
        assert(0);
}

JSONValue& JSONOutputArchiveBlock::CreateElement(ArchiveBase& archive, const char* elementName)
{
    URHO3D_ASSERT(numElements_ < expectedElementCount_);

    switch (type_)
    {
    case ArchiveBlockType::Sequential:
    case ArchiveBlockType::Array:
        ++numElements_;
        blockValue_->Push(JSONValue{});
        return (*blockValue_)[blockValue_->Size() - 1];

    case ArchiveBlockType::Unordered:
        if (blockValue_->GetObject().contains(elementName))
            throw archive.DuplicateElementException(elementName);

        ++numElements_;
        blockValue_->Set(elementName, JSONValue{});
        return (*blockValue_)[elementName];

    default:
        URHO3D_ASSERT(0);
        return *blockValue_;
    }
}

void JSONOutputArchiveBlock::Close(ArchiveBase& archive)
{
    // TODO: Uncomment when PluginManager is fixed
    //URHO3D_ASSERT(expectedElementCount_ == M_MAX_UNSIGNED || numElements_ == expectedElementCount_);
}

void JSONOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);
    CheckBlockOrElementName(name);

    // Open root block
    if (stack_.empty())
    {
        stack_.push_back(Block{ name, type, &rootValue_, sizeHint });
        return;
    }

    // Validate new block
    JSONValue& blockValue = GetCurrentBlock().CreateElement(*this, name);
    stack_.push_back(Block{ name, type, &blockValue, sizeHint });
}

void JSONOutputArchive::Serialize(const char* name, long long& value)
{
    CreateElement(name, JSONValue{ eastl::to_string(value) });
}

void JSONOutputArchive::Serialize(const char* name, unsigned long long& value)
{
    CreateElement(name, JSONValue{ eastl::to_string(value) });
}

void JSONOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    BufferToHexString(tempString_, bytes, size);
    CreateElement(name, JSONValue{ tempString_ });
}

void JSONOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    CreateElement(name, JSONValue{ value });
}

void JSONOutputArchive::CreateElement(const char* name, const JSONValue& value)
{
    CheckBeforeElement(name);
    CheckBlockOrElementName(name);

    JSONValue& jsonValue = GetCurrentBlock().CreateElement(*this, name);
    jsonValue = value;
}

// Generate serialization implementation (JSON output)
#define URHO3D_JSON_OUT_IMPL(type, function) \
    void JSONOutputArchive::Serialize(const char* name, type& value) \
    { \
        CreateElement(name, JSONValue{ value }); \
    }

URHO3D_JSON_OUT_IMPL(bool, SetBool);
URHO3D_JSON_OUT_IMPL(signed char, SetInt);
URHO3D_JSON_OUT_IMPL(short, SetInt);
URHO3D_JSON_OUT_IMPL(int, SetInt);
URHO3D_JSON_OUT_IMPL(unsigned char, SetUInt);
URHO3D_JSON_OUT_IMPL(unsigned short, SetUInt);
URHO3D_JSON_OUT_IMPL(unsigned int, SetUInt);
URHO3D_JSON_OUT_IMPL(float, SetFloat);
URHO3D_JSON_OUT_IMPL(double, SetDouble);
URHO3D_JSON_OUT_IMPL(ea::string, SetString);

#undef URHO3D_JSON_OUT_IMPL

JSONInputArchiveBlock::JSONInputArchiveBlock(const char* name, ArchiveBlockType type, const JSONValue* value)
    : ArchiveBlockBase(name, type)
    , value_(value)
{
}

const JSONValue& JSONInputArchiveBlock::ReadElement(ArchiveBase& archive, const char* elementName, const ArchiveBlockType* elementBlockType)
{
    // Find appropriate value
    const JSONValue* elementValue = nullptr;
    if (IsArchiveBlockJSONArray(type_))
    {
        if (nextElementIndex_ >= value_->Size())
            throw archive.ElementNotFoundException(elementName, nextElementIndex_);

        elementValue = &value_->Get(nextElementIndex_);
        ++nextElementIndex_;
    }
    else if (IsArchiveBlockJSONObject(type_))
    {
        if (!value_->Contains(elementName))
            throw archive.ElementNotFoundException(elementName);

        elementValue = &value_->Get(elementName);
    }
    else
    {
        URHO3D_ASSERT(0);
        return *value_;
    }

    // Check if reading block
    if (elementBlockType && !IsArchiveBlockTypeMatching(*elementValue, *elementBlockType))
        throw archive.UnexpectedElementValueException(name_);

    return *elementValue;
}

void JSONInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);
    CheckBlockOrElementName(name);

    // Open root block
    if (stack_.empty())
    {
        if (!IsArchiveBlockTypeMatching(rootValue_, type))
            throw UnexpectedElementValueException(name);

        Block frame{ name, type, &rootValue_ };
        sizeHint = frame.GetSizeHint();
        stack_.push_back(frame);
        return;
    }

    // Open block
    const JSONValue& blockValue = GetCurrentBlock().ReadElement(*this, name, &type);

    Block blockFrame{ name, type, &blockValue };
    sizeHint = blockFrame.GetSizeHint();
    stack_.push_back(blockFrame);
}

void JSONInputArchive::Serialize(const char* name, long long& value)
{
    const JSONValue& jsonValue = ReadElement(name);
    CheckType(name, jsonValue, JSON_STRING);

    const ea::string stringValue = jsonValue.GetString();
    value = ToInt64(stringValue);
}

void JSONInputArchive::Serialize(const char* name, unsigned long long& value)
{
    const JSONValue& jsonValue = ReadElement(name);
    CheckType(name, jsonValue, JSON_STRING);

    const ea::string stringValue = jsonValue.GetString();
    value = ToUInt64(stringValue);
}

void JSONInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    const JSONValue& jsonValue = ReadElement(name);
    CheckType(name, jsonValue, JSON_STRING);

    ReadBytesFromHexString(name, jsonValue.GetString(), bytes, size);
}

void JSONInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    const JSONValue& jsonValue = ReadElement(name);
    CheckType(name, jsonValue, JSON_NUMBER);

    value = jsonValue.GetUInt();
}

const JSONValue& JSONInputArchive::ReadElement(const char* name)
{
    CheckBeforeElement(name);
    CheckBlockOrElementName(name);
    return GetCurrentBlock().ReadElement(*this, name, nullptr);
}

void JSONInputArchive::CheckType(const char* name, const JSONValue& value, JSONValueType type) const
{
    if (value.GetValueType() != type)
        throw UnexpectedElementValueException(name);
}

// Generate serialization implementation (JSON input)
#define URHO3D_JSON_IN_IMPL(type, function, jsonType) \
    void JSONInputArchive::Serialize(const char* name, type& value) \
    { \
        const JSONValue& jsonValue = ReadElement(name); \
        CheckType(name, jsonValue, jsonType); \
        value = jsonValue.function(); \
    }

URHO3D_JSON_IN_IMPL(bool, GetBool, JSON_BOOL);
URHO3D_JSON_IN_IMPL(signed char, GetInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(short, GetInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(int, GetInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(unsigned char, GetUInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(unsigned short, GetUInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(unsigned int, GetUInt, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(float, GetFloat, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(double, GetDouble, JSON_NUMBER);
URHO3D_JSON_IN_IMPL(ea::string, GetString, JSON_STRING);

#undef URHO3D_JSON_IN_IMPL

}
