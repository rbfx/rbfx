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
#include "../Resource/XMLArchive.h"

namespace Urho3D
{

/// Name of internal key attribue of Map block.
static const char* keyAttribute = "key";

XMLOutputArchiveBlock::XMLOutputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement, unsigned sizeHint)
    : name_(name)
    , type_(type)
    , blockElement_(blockElement)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
        expectedElementCount_ = sizeHint;
}

bool XMLOutputArchiveBlock::SetElementKey(ArchiveBase& archive, ea::string key)
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

XMLElement XMLOutputArchiveBlock::CreateElement(ArchiveBase& archive, const char* elementName, const char* defaultElementName)
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ >= expectedElementCount_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalBlockOverflow);
        assert(0);
        return {};
    }

    if (type_ == ArchiveBlockType::Map && !keySet_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingKeySerialization);
        assert(0);
        return {};
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
        assert(0);
        return {};
    }

    if (type_ == ArchiveBlockType::Unordered && usedNames_.contains(elementName)
        || type_ == ArchiveBlockType::Map && usedNames_.contains(elementKey_))
    {
        archive.SetErrorFormatted(ArchiveBase::errorDuplicateElement_elementName, elementName);
        return {};
    }

    XMLElement element;
    switch (type_)
    {
    case ArchiveBlockType::Sequential:
        ++numElements_;
        element = blockElement_.CreateChild(elementName ? elementName : defaultElementName);
        break;
    case ArchiveBlockType::Unordered:
        ++numElements_;
        assert(elementName);
        element = blockElement_.CreateChild(elementName);
        usedNames_.emplace(elementName);
        break;
    case ArchiveBlockType::Array:
        ++numElements_;
        element = blockElement_.CreateChild(elementName ? elementName : defaultElementName);
        break;
    case ArchiveBlockType::Map:
        ++numElements_;
        assert(keySet_);
        element = blockElement_.CreateChild(elementName ? elementName : defaultElementName);
        element.SetString(keyAttribute, elementKey_);
        usedNames_.emplace(elementKey_);
        keySet_ = false;
        break;
    default:
        assert(0);
        break;
    }
    return element;
}

XMLAttributeReference XMLOutputArchiveBlock::CreateElementOrAttribute(ArchiveBase& archive, const char* elementName, const char* defaultElementName)
{
    if (type_ != ArchiveBlockType::Unordered)
    {
        XMLElement child = CreateElement(archive, elementName, defaultElementName);
        return { child, "value" };
    }

    // Special case for Unordered    
    if (!elementName)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
        assert(0);
        return {};
    }

    if (usedNames_.contains(elementName))
    {
        archive.SetErrorFormatted(ArchiveBase::errorDuplicateElement_elementName, elementName);
        return {};
    }

    ++numElements_;
    return { blockElement_, elementName };
}

bool XMLOutputArchiveBlock::Close(ArchiveBase& archive)
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

bool XMLOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name, name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        if (serializeRootName_)
            rootElement_.SetName(name ? name : defaultRootName);
        stack_.push_back(Block{ name, type, rootElement_, sizeHint });
        return true;
    }

    if (XMLElement blockElement = GetCurrentBlock().CreateElement(*this, name, defaultBlockName))
    {
        Block block{ name, type, blockElement, sizeHint };
        stack_.push_back(block);
        return true;
    }

    return false;
}

bool XMLOutputArchive::EndBlock()
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

bool XMLOutputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, key);
}

bool XMLOutputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().SetElementKey(*this, ea::to_string(key));
}

bool XMLOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (XMLAttributeReference ref = CreateElement(name))
    {
        BufferToHexString(tempString_, bytes, size);
        ref.GetElement().SetString(ref.GetAttributeName(), tempString_);
        return true;
    }
    return false;
}

bool XMLOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (XMLAttributeReference ref = CreateElement(name))
    {
        ref.GetElement().SetUInt(ref.GetAttributeName(), value);
        return true;
    }
    return false;
}

bool XMLOutputArchive::CheckEOF(const char* elementName, const char* debugName)
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

bool XMLOutputArchive::CheckEOFAndRoot(const char* elementName, const char* debugName)
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

XMLAttributeReference XMLOutputArchive::CreateElement(const char* name)
{
    if (!CheckEOFAndRoot(name, name))
        return {};

    XMLOutputArchiveBlock& block = GetCurrentBlock();
    return GetCurrentBlock().CreateElementOrAttribute(*this, name, defaultElementName);
}

// Generate serialization implementation (XML output)
#define URHO3D_XML_OUT_IMPL(type, function) \
    bool XMLOutputArchive::Serialize(const char* name, type& value) \
    { \
        if (XMLAttributeReference ref = CreateElement(name)) \
        { \
            ref.GetElement().function(ref.GetAttributeName(), value); \
            return true; \
        } \
        return false; \
    }

URHO3D_XML_OUT_IMPL(bool, SetBool);
URHO3D_XML_OUT_IMPL(signed char, SetInt);
URHO3D_XML_OUT_IMPL(short, SetInt);
URHO3D_XML_OUT_IMPL(int, SetInt);
URHO3D_XML_OUT_IMPL(long long, SetInt64);
URHO3D_XML_OUT_IMPL(unsigned char, SetUInt);
URHO3D_XML_OUT_IMPL(unsigned short, SetUInt);
URHO3D_XML_OUT_IMPL(unsigned int, SetUInt);
URHO3D_XML_OUT_IMPL(unsigned long long, SetUInt64);
URHO3D_XML_OUT_IMPL(float, SetFloat);
URHO3D_XML_OUT_IMPL(double, SetDouble);
URHO3D_XML_OUT_IMPL(ea::string, SetString);

#undef URHO3D_XML_OUT_IMPL

XMLInputArchiveBlock::XMLInputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement)
    : name_(name)
    , type_(type)
    , blockElement_(blockElement)
{
    if (blockElement_)
        nextChild_ = blockElement_.GetChild();
}

unsigned XMLInputArchiveBlock::CalculateSizeHint() const
{
    XMLElement child = blockElement_.GetChild();
    unsigned count = 0;
    while (child)
    {
        ++count;
        child = child.GetNext();
    }
    return count;
}

bool XMLInputArchiveBlock::ReadCurrentKey(ArchiveBase& archive, ea::string& key)
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

    if (!nextChild_)
    {
        archive.SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, ArchiveBase::keyElementName_);
        return false;
    }

    if (!nextChild_.HasAttribute(keyAttribute))
    {
        archive.SetErrorFormatted(ArchiveBase::errorMissingMapKey);
        return false;
    }

    key = nextChild_.GetAttribute(keyAttribute);
    keyRead_ = true;
    return true;
}

XMLElement XMLInputArchiveBlock::ReadElement(ArchiveBase& archive, const char* elementName)
{
    if (type_ != ArchiveBlockType::Unordered && !nextChild_)
    {
        archive.SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, elementName);
        return {};
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
        assert(0);
        return {};
    }

    if (type_ == ArchiveBlockType::Map && !keyRead_)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingKeySerialization);
        assert(0);
        return {};
    }

    XMLElement element;
    if (type_ == ArchiveBlockType::Unordered)
        element = blockElement_.GetChild(elementName);
    else
    {
        element = nextChild_;
        nextChild_ = nextChild_.GetNext();
    }

    if (element)
        keyRead_ = false;

    return element;
}

XMLAttributeReference XMLInputArchiveBlock::ReadElementOrAttribute(ArchiveBase& archive, const char* elementName)
{
    if (type_ != ArchiveBlockType::Unordered)
    {
        XMLElement child = ReadElement(archive, elementName);
        return { child, "value" };
    }

    // Special case for Unordered    
    if (!elementName)
    {
        archive.SetErrorFormatted(ArchiveBase::fatalMissingElementName);
        assert(0);
        return {};
    }

    if (!blockElement_.HasAttribute(elementName))
        return {};
    
    return { blockElement_, elementName };
}

bool XMLInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name, name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        if (serializeRootName_ && rootElement_.GetName() != (name ? name : defaultRootName))
        {
            SetErrorFormatted(ArchiveBase::errorElementNotFound_elementName, name);
            return false;
        }

        Block block{ name, type, rootElement_ };
        sizeHint = block.CalculateSizeHint();
        stack_.push_back(block);
        return true;
    }

    // Try open block
    if (XMLElement blockElement = GetCurrentBlock().ReadElement(*this, name))
    {
        Block block{ name, type, blockElement };
        sizeHint = block.CalculateSizeHint();
        stack_.push_back(block);
        return true;
    }

    return false;
}

bool XMLInputArchive::EndBlock()
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

bool XMLInputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot("", ArchiveBase::keyElementName_))
        return false;

    return GetCurrentBlock().ReadCurrentKey(*this, key);
}

bool XMLInputArchive::SerializeKey(unsigned& key)
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

bool XMLInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (XMLAttributeReference ref = ReadElement(name))
    {
        if (!HexStringToBuffer(tempBuffer_, ref.GetElement().GetAttribute(ref.GetAttributeName())))
            return false;
        if (tempBuffer_.size() != size)
            return false;
        ea::copy(tempBuffer_.begin(), tempBuffer_.end(), static_cast<unsigned char*>(bytes));
        return true;
    }
    return false;
}

bool XMLInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (XMLAttributeReference ref = ReadElement(name))
    {
        value = ref.GetElement().GetUInt(ref.GetAttributeName());
        return true;
    }
    return false;
}

bool XMLInputArchive::CheckEOF(const char* elementName, const char* debugName)
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

bool XMLInputArchive::CheckEOFAndRoot(const char* elementName, const char* debugName)
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

XMLAttributeReference XMLInputArchive::ReadElement(const char* name)
{
    if (!CheckEOFAndRoot(name, name))
        return {};

    return GetCurrentBlock().ReadElementOrAttribute(*this, name);
}

// Generate serialization implementation (XML input)
#define URHO3D_XML_IN_IMPL(type, function) \
    bool XMLInputArchive::Serialize(const char* name, type& value) \
    { \
        if (XMLAttributeReference ref = ReadElement(name)) \
        { \
            value = ref.GetElement().function(ref.GetAttributeName()); \
            return true; \
        } \
        return false; \
    }

URHO3D_XML_IN_IMPL(bool, GetBool);
URHO3D_XML_IN_IMPL(signed char, GetInt);
URHO3D_XML_IN_IMPL(short, GetInt);
URHO3D_XML_IN_IMPL(int, GetInt);
URHO3D_XML_IN_IMPL(long long, GetInt64);
URHO3D_XML_IN_IMPL(unsigned char, GetUInt);
URHO3D_XML_IN_IMPL(unsigned short, GetUInt);
URHO3D_XML_IN_IMPL(unsigned int, GetUInt);
URHO3D_XML_IN_IMPL(unsigned long long, GetUInt64);
URHO3D_XML_IN_IMPL(float, GetFloat);
URHO3D_XML_IN_IMPL(double, GetDouble);
URHO3D_XML_IN_IMPL(ea::string, GetAttribute);

#undef URHO3D_XML_IN_IMPL

}
