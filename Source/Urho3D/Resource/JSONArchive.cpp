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

JSONOutputArchiveBlock::JSONOutputArchiveBlock(const char* name, ArchiveBlockType type, JSONValue* blockValue, unsigned sizeHint)
    : name_(name)
    , type_(type)
    , blockValue_(blockValue)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
        expectedElementCount_ = sizeHint;

    if (IsArchiveBlockJSONArray(type_))
        blockValue_->SetType(JSON_ARRAY);
    else if (IsArchiveBlockJSONObject(type_))
        blockValue_->SetType(JSON_OBJECT);
    else
        assert(0);
}

bool JSONOutputArchiveBlock::SetElementKey(ArchiveBase& archive, ea::string key)
{
    if (type_ != ArchiveBlockType::Map)
    {
        archive.SetError(ArchiveBase::fatalUnexpectedKeySerialization_blockName, name_);
        assert(0);
        return false;
    }

    if (keySet_)
    {
        archive.SetError(ArchiveBase::fatalDuplicateKeySerialization_blockName, name_);
        assert(0);
        return false;
    }

    elementKey_ = ea::move(key);
    keySet_ = true;
    return true;
}

JSONValue* JSONOutputArchiveBlock::CreateElement(ArchiveBase& archive, const char* elementName)
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ >= expectedElementCount_)
    {
        archive.SetError(ArchiveBase::fatalBlockOverflow_blockName, name_);
        assert(0);
        return false;
    }

    if (type_ == ArchiveBlockType::Map && !keySet_)
    {
        archive.SetError(ArchiveBase::fatalMissingKeySerialization_blockName, name_);
        assert(0);
        return false;
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetError(ArchiveBase::fatalMissingElementName_blockName, name_);
        assert(0);
        return false;
    }

    ea::string jsonObjectKey;
    switch (type_)
    {
    case ArchiveBlockType::Unordered:
        jsonObjectKey = elementName;
        break;
    case ArchiveBlockType::Map:
        jsonObjectKey = elementKey_;
        break;
    default:
        break;
    }

    if (type_ == ArchiveBlockType::Unordered || type_ == ArchiveBlockType::Map)
    {
        if (blockValue_->GetObject().contains(jsonObjectKey))
        {
            archive.SetError(ArchiveBase::errorDuplicateElement_blockName_elementName, name_, elementName);
            return false;
        }
    }

    switch (type_)
    {
    case ArchiveBlockType::Sequential:
    case ArchiveBlockType::Array:
        ++numElements_;
        blockValue_->Push(JSONValue{});
        return &(*blockValue_)[blockValue_->Size() - 1];
    case ArchiveBlockType::Unordered:
    case ArchiveBlockType::Map:
        ++numElements_;
        keySet_ = false;
        blockValue_->Set(jsonObjectKey, JSONValue{});
        return &(*blockValue_)[jsonObjectKey];
    default:
        assert(0);
        return nullptr;
    }
}

bool JSONOutputArchiveBlock::Close(ArchiveBase& archive)
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ != expectedElementCount_)
    {
        if (numElements_ < expectedElementCount_)
            archive.SetError(ArchiveBase::fatalBlockUnderflow_blockName, name_);
        else
            archive.SetError(ArchiveBase::fatalBlockOverflow_blockName, name_);
        assert(0);
        return false;
    }

    return true;
}

bool JSONOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        stack_.push_back(Block{ name, type, &jsonFile_->GetRoot(), sizeHint });
        return true;
    }

    // Validate new block
    if (JSONValue* blockValue = GetCurrentBlock().CreateElement(*this, name))
    {
        stack_.push_back(Block{ name, type, blockValue, sizeHint });
        return true;
    }

    return false;
}

bool JSONOutputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    const bool blockClosed = GetCurrentBlock().Close(*this);

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();

    return blockClosed;
}

bool JSONOutputArchive::SetStringKey(ea::string* key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, *key);
}

bool JSONOutputArchive::SetUnsignedKey(unsigned* key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, ea::to_string(*key));
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

bool JSONOutputArchive::CheckEOF(const char* elementName)
{
    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetError(ArchiveBase::errorReadEOF_blockName_elementName, blockName, elementName);
        return false;
    }
    return true;
}

bool JSONOutputArchive::CheckEOFAndRoot(const char* elementName)
{
    if (!CheckEOF(elementName))
        return false;

    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

bool JSONOutputArchive::SerializeJSONValue(const char* name, const JSONValue& value)
{
    if (!CheckEOFAndRoot(name))
        return false;

    if (JSONValue* jsonValue = GetCurrentBlock().CreateElement(*this, name))
    {
        *jsonValue = value;
        return true;
    }
    return false;
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

JSONInputArchiveBlock::JSONInputArchiveBlock(const char* name, ArchiveBlockType type, const JSONValue* value)
    : name_(name ? name : "")
    , type_(type)
    , value_(value)
{
    if (value_)
        nextMapElementIterator_ = value_->GetObject().begin();
}

bool JSONInputArchiveBlock::ReadCurrentKey(ArchiveBase& archive, ea::string& key)
{
    if (type_ != ArchiveBlockType::Map)
    {
        archive.SetError(ArchiveBase::fatalUnexpectedKeySerialization_blockName, name_);
        assert(0);
        return false;
    }

    if (keyRead_)
    {
        archive.SetError(ArchiveBase::fatalDuplicateKeySerialization_blockName, name_);
        assert(0);
        return false;
    }

    if (nextMapElementIterator_ == value_->GetObject().end())
    {
        archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, ArchiveBase::keyElementName_);
        return false;
    }

    key = nextMapElementIterator_->first;
    keyRead_ = true;
    return true;
}

const JSONValue* JSONInputArchiveBlock::ReadElement(ArchiveBase& archive, const char* elementName, const ArchiveBlockType* elementBlockType)
{
    // Find appropriate value
    const JSONValue* elementValue = nullptr;
    if (IsArchiveBlockJSONArray(type_))
    {
        if (nextElementIndex_ >= value_->Size())
        {
            archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, elementName);
            return nullptr;
        }

        // Read current element from the array
        elementValue = &value_->Get(nextElementIndex_);
    }
    else if (IsArchiveBlockJSONObject(type_))
    {
        if (type_ == ArchiveBlockType::Unordered)
        {
            if (!elementName)
            {
                archive.SetError(ArchiveBase::fatalMissingElementName_blockName, name_);
                assert(0);
                return false;
            }

            if (!value_->Contains(elementName))
            {
                // Not an error in Unordered block
                return nullptr;
            }

            // Find element in map
            elementValue = &value_->Get(elementName);
        }
        else if (type_ == ArchiveBlockType::Map)
        {
            if (!keyRead_)
            {
                archive.SetError(ArchiveBase::fatalMissingKeySerialization_blockName, name_);
                assert(0);
                return nullptr;
            }

            if (nextMapElementIterator_ == value_->GetObject().end())
            {
                archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, elementName);
                return nullptr;
            }

            // Read current element from the map
            elementValue = &nextMapElementIterator_->second;
        }
        else
        {
            assert(0);
            return nullptr;
        }
    }
    else
    {
        assert(0);
        return nullptr;
    }

    // Check if reading block
    assert(elementValue);
    if (elementBlockType)
    {
        if (!IsArchiveBlockTypeMatching(*elementValue, *elementBlockType))
        {
            archive.SetError(ArchiveBase::errorUnexpectedBlockType_blockName, name_);
            return nullptr;
        }
    }

    // Move to next
    keyRead_ = false;
    if (type_ == ArchiveBlockType::Array || type_ == ArchiveBlockType::Sequential)
        ++nextElementIndex_;
    else if (type_ == ArchiveBlockType::Map)
        ++nextMapElementIterator_;

    return elementValue;
}

bool JSONInputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        if (!IsArchiveBlockTypeMatching(jsonFile_->GetRoot(), type))
        {
            SetError(ArchiveBase::errorUnexpectedBlockType_blockName, name);
            return false;
        }

        Block frame{ name, type, &jsonFile_->GetRoot() };
        sizeHint = frame.GetSizeHint();
        stack_.push_back(frame);
        return true;
    }

    // Try open block
    if (const JSONValue* blockValue = GetCurrentBlock().ReadElement(*this, name, &type))
    {
        Block blockFrame{ name, type, blockValue };
        sizeHint = blockFrame.GetSizeHint();
        stack_.push_back(blockFrame);
        return true;
    }

    return false;
}

bool JSONInputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool JSONInputArchive::SetStringKey(ea::string* key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().ReadCurrentKey(*this, *key);
}

bool JSONInputArchive::SetUnsignedKey(unsigned* key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    ea::string stringKey;
    if (GetCurrentBlock().ReadCurrentKey(*this, stringKey))
    {
        *key = ToUInt(stringKey);
        return true;
    }
    return false;
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

bool JSONInputArchive::CheckEOF(const char* elementName)
{
    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetError(ArchiveBase::errorReadEOF_blockName_elementName, blockName, elementName);
        return false;
    }
    return true;
}

bool JSONInputArchive::CheckEOFAndRoot(const char* elementName)
{
    if (!CheckEOF(elementName))
        return false;

    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

const JSONValue* JSONInputArchive::DeserializeJSONValue(const char* name)
{
    if (!CheckEOFAndRoot(name))
        return nullptr;

    return GetCurrentBlock().ReadElement(*this, name, nullptr);
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
