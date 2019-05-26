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

#include "../Core/StringUtils.h"
#include "../IO/ArchiveSerialization.h"
#include "../Resource/JSONArchive.h"

namespace Urho3D
{

JSONOutputArchiveBlock::JSONOutputArchiveBlock(ArchiveBlockType type, const ea::string& key, unsigned expectedElementCount)
    : type_(type)
    , key_(key)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
        expectedElementCount_ = expectedElementCount;

    if (IsArchiveBlockJSONArray(type_))
        blockValue_.SetType(JSON_ARRAY);
    else if (IsArchiveBlockJSONObject(type_))
        blockValue_.SetType(JSON_OBJECT);
    else
        assert(0);
}

bool JSONOutputArchiveBlock::CanCreateChild(const char* name, const ea::string* stringKey, const unsigned* uintKey) const
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ >= expectedElementCount_)
        return false;

    switch (type_)
    {
    case ArchiveBlockType::Sequential:
    case ArchiveBlockType::Array:
        return true;
    case ArchiveBlockType::Unordered:
        return name && !blockValue_.GetObject().contains(name);
    case ArchiveBlockType::Map:
        if (stringKey)
            return !blockValue_.GetObject().contains(*stringKey);
        else if (uintKey)
            return !blockValue_.GetObject().contains(ea::to_string(*uintKey));
        else
            return false;
    default:
        assert(0);
        return false;
    }
}

ea::string JSONOutputArchiveBlock::GetChildKey(const char* name, const ea::string* stringKey, const unsigned* uintKey)
{
    switch (type_)
    {
    case ArchiveBlockType::Sequential:
    case ArchiveBlockType::Array:
        return "";
    case ArchiveBlockType::Unordered:
        assert(name);
        return name;
    case ArchiveBlockType::Map:
        if (stringKey)
            return *stringKey;
        else if (uintKey)
            return ea::to_string(*uintKey);
        else
        {
            assert(0);
            return "";
        }
    default:
        assert(0);
        return "";
    }
}

void JSONOutputArchiveBlock::CreateChild(const ea::string& key, JSONValue childValue)
{
    switch (type_)
    {
    case ArchiveBlockType::Sequential:
    case ArchiveBlockType::Array:
        ++numElements_;
        blockValue_.Push(std::move(childValue));
        break;
    case ArchiveBlockType::Unordered:
    case ArchiveBlockType::Map:
        ++numElements_;
        blockValue_.Set(key, std::move(childValue));
        break;
    default:
        assert(0);
        break;
    }
}

bool JSONOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    // Check if output is closed
    if (IsEOF())
    {
        SetError();
        return false;
    }

    // Open root block
    if (stack_.empty())
    {
        stack_.push_back(Block{ type, "", sizeHint });
        return true;
    }

    // Validate new block
    Block& currentBlock = GetCurrentBlock();
    if (!currentBlock.CanCreateChild(name, stringKey_, uintKey_))
    {
        SetError();
        return false;
    }

    // Postpone block assignment until the end
    stack_.push_back(Block{ type, currentBlock.GetChildKey(name, stringKey_, uintKey_), sizeHint });
    ResetKeys();
    return true;
}

bool JSONOutputArchive::EndBlock()
{
    if (IsEOF())
    {
        SetError();
        return false;
    }

    if (!GetCurrentBlock().IsComplete())
    {
        SetError();
        return false;
    }

    // Close root block
    if (stack_.size() == 1)
    {
        jsonFile_->GetRoot() = std::move(stack_[0].GetValue());
        CloseArchive();
        stack_.clear();
        return true;
    }

    // Close block
    Block& parentBlock = stack_[stack_.size() - 2];
    Block& childBlock = stack_.back();
    parentBlock.CreateChild(childBlock.GetKey(), std::move(childBlock.GetValue()));
    stack_.pop_back();

    // Close archive
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool JSONOutputArchive::Serialize(const char* name, long long& value)
{
    return SerializeJSONValue(name, JSONValue{ eastl::to_string(value) });
}

bool JSONOutputArchive::Serialize(const char* name, unsigned long long& value)
{
    return SerializeJSONValue(name, JSONValue{ eastl::to_string(value) });
}

bool JSONOutputArchive::SerializeFloatArray(const char* name, float* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string = Detail::FormatFloatArray(values, size);
        return Serialize(name, string);
    }
}

bool JSONOutputArchive::SerializeIntArray(const char* name, int* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string = Detail::FormatIntArray(values, size);
        return Serialize(name, string);
    }
}

bool JSONOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    BufferToHexString(tempString_, bytes, size);
    return SerializeJSONValue(name, JSONValue{ tempString_ });
}

bool JSONOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    return SerializeJSONValue(name, JSONValue{ value });
}

bool JSONOutputArchive::SerializeJSONValue(const char* name, const JSONValue& value)
{
    // Check if output is closed
    if (IsEOF())
    {
        SetError();
        return false;
    }

    // Validate new element
    Block& currentBlock = GetCurrentBlock();
    if (!currentBlock.CanCreateChild(name, stringKey_, uintKey_))
    {
        SetError();
        return false;
    }

    // Create new element.
    currentBlock.CreateChild(currentBlock.GetChildKey(name, stringKey_, uintKey_), value);
    ResetKeys();
    return true;
}

// Generate serialization implementation (JSON output)
#define URHO3D_JSON_OUT_IMPL(type, function) \
    bool JSONOutputArchive::Serialize(const char* name, type& value) \
    { \
        return SerializeJSONValue(name, JSONValue{ value }); \
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

JSONInputArchiveBlock::JSONInputArchiveBlock(ArchiveBlockType type, const JSONValue* value)
    : type_(type)
    , value_(value)
{
    if (value_)
        nextMapElementIterator_ = value_->GetObject().begin();
}

bool JSONInputArchiveBlock::IsValid() const
{
    if (!value_)
        return false;

    return (IsArchiveBlockJSONArray(type_) && value_->IsArray())
        || (IsArchiveBlockJSONObject(type_) && value_->IsObject());
}

const Urho3D::JSONValue* JSONInputArchiveBlock::GetCurrentElement(const char* name, ea::string* stringKey, unsigned* uintKey)
{
    if (IsArchiveBlockJSONArray(type_))
    {
        // Get next array element if any
        if (nextElementIndex_ < value_->Size())
            return &value_->Get(nextElementIndex_);
    }
    else if (IsArchiveBlockJSONObject(type_))
    {
        // Get value from map by name
        if (type_ == ArchiveBlockType::Unordered && name && value_->Contains(name))
            return &value_->Get(name);

        // Get next map element if any
        if (type_ == ArchiveBlockType::Map && nextMapElementIterator_ != value_->GetObject().end())
        {
            if (stringKey)
                *stringKey = nextMapElementIterator_->first;
            else if (uintKey)
                *uintKey = ToUInt(nextMapElementIterator_->first);
            return &nextMapElementIterator_->second;
        }
    }
    return nullptr;
}

void JSONInputArchiveBlock::NextElement()
{
    if (IsArchiveBlockJSONArray(type_) && nextElementIndex_ < value_->Size())
        ++nextElementIndex_;
    else if (type_ == ArchiveBlockType::Map && nextMapElementIterator_ != value_->GetObject().end())
        ++nextMapElementIterator_;
}

bool JSONInputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    // Check if output is closed
    if (IsEOF())
    {
        SetError();
        return false;
    }

    // Open root block
    if (stack_.empty())
    {
        Block frame{ type, &jsonFile_->GetRoot() };
        if (!frame.IsValid())
        {
            SetError();
            return false;
        }

        sizeHint = frame.GetSizeHint();
        stack_.push_back(frame);
        return true;
    }

    // Try open block
    Block& currentBlock = GetCurrentBlock();
    const JSONValue* blockValue = currentBlock.GetCurrentElement(name, stringKey_, uintKey_);

    Block blockFrame{ type, blockValue };
    if (!blockFrame.IsValid())
    {
        SetError();
        return false;
    }

    // Create new block
    currentBlock.NextElement();
    sizeHint = blockFrame.GetSizeHint();
    stack_.push_back(blockFrame);
    ResetKeys();
    return true;
}

bool JSONInputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetError();
        return false;
    }

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool JSONInputArchive::Serialize(const char* name, long long& value)
{
    if (const JSONValue* jsonValue = DeserializeJSONValue(name))
    {
        if (jsonValue->IsString())
        {
            const ea::string stringValue = jsonValue->GetString();
            sscanf(stringValue.c_str(), "%lld", &value);
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::Serialize(const char* name, unsigned long long& value)
{
    if (const JSONValue* jsonValue = DeserializeJSONValue(name))
    {
        if (jsonValue->IsString())
        {
            const ea::string stringValue = jsonValue->GetString();
            sscanf(stringValue.c_str(), "%llu", &value);
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::SerializeFloatArray(const char* name, float* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string;
        if (!Serialize(name, string))
            return false;

        const unsigned realSize = Detail::UnFormatFloatArray(string, values, size);

        if (realSize != size)
            return false;

        return true;
    }
}

bool JSONInputArchive::SerializeIntArray(const char* name, int* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string;
        if (!Serialize(name, string))
            return false;

        const unsigned realSize = Detail::UnFormatIntArray(string, values, size);

        if (realSize != size)
            return false;

        return true;
    }
}

bool JSONInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (const JSONValue* jsonValue = DeserializeJSONValue(name))
    {
        if (jsonValue->IsString())
        {
            if (!HexStringToBuffer(tempBuffer_, jsonValue->GetString()))
                return false;
            if (size != tempBuffer_.size())
                return false;
            ea::copy(tempBuffer_.begin(), tempBuffer_.end(), static_cast<unsigned char*>(bytes));
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (const JSONValue* jsonValue = DeserializeJSONValue(name))
    {
        if (jsonValue->IsNumber())
        {
            value = jsonValue->GetUInt();
            return true;
        }
    }
    return false;
}

const JSONValue* JSONInputArchive::DeserializeJSONValue(const char* name)
{
    Block& currentBlock = GetCurrentBlock();
    const JSONValue* elementValue = currentBlock.GetCurrentElement(name, stringKey_, uintKey_);
    if (!elementValue)
    {
        SetError();
        return nullptr;
    }

    ResetKeys();
    currentBlock.NextElement();
    return elementValue;
}

// Generate serialization implementation (JSON input)
#define URHO3D_JSON_IN_IMPL(type, function, check) \
    bool JSONInputArchive::Serialize(const char* name, type& value) \
    { \
        if (const JSONValue* jsonValue = DeserializeJSONValue(name)) \
        { \
            if (jsonValue->check()) \
            { \
                value = jsonValue->function(); \
                return true; \
            } \
        } \
        return false; \
    }

URHO3D_JSON_IN_IMPL(bool, GetBool, IsBool);
URHO3D_JSON_IN_IMPL(signed char, GetInt, IsNumber);
URHO3D_JSON_IN_IMPL(short, GetInt, IsNumber);
URHO3D_JSON_IN_IMPL(int, GetInt, IsNumber);
URHO3D_JSON_IN_IMPL(unsigned char, GetUInt, IsNumber);
URHO3D_JSON_IN_IMPL(unsigned short, GetUInt, IsNumber);
URHO3D_JSON_IN_IMPL(unsigned int, GetUInt, IsNumber);
URHO3D_JSON_IN_IMPL(float, GetFloat, IsNumber);
URHO3D_JSON_IN_IMPL(double, GetDouble, IsNumber);
URHO3D_JSON_IN_IMPL(ea::string, GetString, IsString);

#undef URHO3D_JSON_IN_IMPL

}
