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
        archive.SetErrorFormatted(ArchiveBase::fatalUnexpectedKeySerialization);
        assert(0);
        return false;
    }

    if (keySet_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalDuplicateKeySerialization);
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
        archive.SetErrorFormatted(ArchiveBase::fatalBlockOverflow);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Map && !keySet_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingKeySerialization);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
        assert(0);
        return nullptr;
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
            archive.SetErrorFormatted(ArchiveBase::errorDuplicateElement_elementName, elementName);
            return nullptr;
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
            archive.SetErrorFormatted(ArchiveBase::fatalBlockUnderflow);
        else
            archive.SetErrorFormatted(ArchiveBase::fatalBlockOverflow);
        assert(0);
        return false;
    }

    return true;
}

bool JSONOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name, name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        stack_.push_back(Block{ name, type, &rootValue_, sizeHint });
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
        SetErrorFormatted(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    const bool blockClosed = GetCurrentBlock().Close(*this);

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();

    return blockClosed;
}

bool JSONOutputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, key);
}

bool JSONOutputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, ea::to_string(key));
}

bool JSONOutputArchive::Serialize(const char* name, long long& value)
{
    return CreateElement(name, JSONValue{ eastl::to_string(value) });
}

bool JSONOutputArchive::Serialize(const char* name, unsigned long long& value)
{
    return CreateElement(name, JSONValue{ eastl::to_string(value) });
}

bool JSONOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    BufferToHexString(tempString_, bytes, size);
    return CreateElement(name, JSONValue{ tempString_ });
}

bool JSONOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    return CreateElement(name, JSONValue{ value });
}

bool JSONOutputArchive::CheckEOF(const char* elementName, const char* debugName)
{
    if (HasError())
        return false;

    if (!ValidateName(elementName))
    {
        SetErrorFormatted(ArchiveBase::fatalInvalidName, debugName);
        return false;
    }
    
    if (IsEOF())
    {
        SetErrorFormatted(ArchiveBase::errorEOF_elementName, debugName);
        return false;
    }

    return true;
}

bool JSONOutputArchive::CheckEOFAndRoot(const char* elementName, const char* debugName)
{
    if (!CheckEOF(elementName, debugName))
        return false;

    if (stack_.empty())
    {
        SetErrorFormatted(ArchiveBase::fatalRootBlockNotOpened_elementName, debugName);
        assert(0);
        return false;
    }

    return true;
}

bool JSONOutputArchive::CreateElement(const char* name, const JSONValue& value)
{
    if (!CheckEOFAndRoot(name, name))
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
        return CreateElement(name, JSONValue{ value }); \
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
        archive.SetErrorFormatted(ArchiveBase::fatalUnexpectedKeySerialization);
        assert(0);
        return false;
    }

    if (keyRead_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalDuplicateKeySerialization);
        assert(0);
        return false;
    }

    if (nextMapElementIterator_ == value_->GetObject().end())
    {
        archive.SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, ArchiveBase::keyElementName_);
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
            archive.SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, elementName);
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
                archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
                assert(0);
                return nullptr;
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
                archive.SetErrorFormatted(ArchiveBase::fatalMissingKeySerialization);
                assert(0);
                return nullptr;
            }

            if (nextMapElementIterator_ == value_->GetObject().end())
            {
                archive.SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, elementName);
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
            archive.SetErrorFormatted(ArchiveBase::errorUnexpectedBlockType_blockName, name_);
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

bool JSONInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name, name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        if (!IsArchiveBlockTypeMatching(rootValue_, type))
        {
            SetErrorFormatted(ArchiveBase::errorUnexpectedBlockType_blockName, name);
            return false;
        }

        Block frame{ name, type, &rootValue_ };
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
        SetErrorFormatted(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool JSONInputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().ReadCurrentKey(*this, key);
}

bool JSONInputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    ea::string stringKey;
    if (GetCurrentBlock().ReadCurrentKey(*this, stringKey))
    {
        key = ToUInt(stringKey);
        return true;
    }
    return false;
}

bool JSONInputArchive::Serialize(const char* name, long long& value)
{
    if (const JSONValue* jsonValue = ReadElement(name))
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
    if (const JSONValue* jsonValue = ReadElement(name))
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

bool JSONInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (const JSONValue* jsonValue = ReadElement(name))
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
    if (const JSONValue* jsonValue = ReadElement(name))
    {
        if (jsonValue->IsNumber())
        {
            value = jsonValue->GetUInt();
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::CheckEOF(const char* elementName, const char* debugName)
{
    if (HasError())
        return false;

    if (!ValidateName(elementName))
    {
        SetErrorFormatted(ArchiveBase::fatalInvalidName, debugName);
        return false;
    }

    if (IsEOF())
    {
        SetErrorFormatted(ArchiveBase::errorEOF_elementName, debugName);
        return false;
    }

    return true;
}

bool JSONInputArchive::CheckEOFAndRoot(const char* elementName, const char* debugName)
{
    if (!CheckEOF(elementName, debugName))
        return false;

    if (stack_.empty())
    {
        SetErrorFormatted(ArchiveBase::fatalRootBlockNotOpened_elementName, debugName);
        assert(0);
        return false;
    }

    return true;
}

const JSONValue* JSONInputArchive::ReadElement(const char* name)
{
    if (!CheckEOFAndRoot(name, name))
        return nullptr;

    return GetCurrentBlock().ReadElement(*this, name, nullptr);
}

// Generate serialization implementation (JSON input)
#define URHO3D_JSON_IN_IMPL(type, function, check) \
    bool JSONInputArchive::Serialize(const char* name, type& value) \
    { \
        if (const JSONValue* jsonValue = ReadElement(name)) \
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
