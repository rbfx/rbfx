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
#include "../Resource/XMLArchive.h"

namespace Urho3D
{

XMLOutputArchiveBlock::XMLOutputArchiveBlock(ArchiveBlockType type, XMLElement blockElement, unsigned expectedElementCount)
    : type_(type)
    , blockElement_(blockElement)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
        expectedElementCount_ = expectedElementCount;
}

bool XMLOutputArchiveBlock::CanCreateChild(const char* name, const ea::string* stringKey, const unsigned* uintKey) const
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ >= expectedElementCount_)
        return false;

    switch (type_)
    {
    case ArchiveBlockType::Sequential:
        return true;
    case ArchiveBlockType::Unordered:
        return name && !usedNames_.contains(name);
    case ArchiveBlockType::Array:
        return true;
    case ArchiveBlockType::Map:
        if (stringKey)
            return !usedNames_.contains(*stringKey);
        else if (uintKey)
            return !usedNames_.contains(ea::to_string(*uintKey));
        else
            return false;
    default:
        assert(0);
        return false;
    }
}

XMLElement XMLOutputArchiveBlock::CreateChild(const char* name, const ea::string* stringKey, const unsigned* uintKey, const char* defaultName)
{
    XMLElement child;
    switch (type_)
    {
    case ArchiveBlockType::Sequential:
        ++numElements_;
        child = blockElement_.CreateChild(name ? name : defaultName);
        break;
    case ArchiveBlockType::Unordered:
        assert(name);
        ++numElements_;
        child = blockElement_.CreateChild(name);
        usedNames_.emplace(name);
        break;
    case ArchiveBlockType::Array:
        ++numElements_;
        child = blockElement_.CreateChild(name ? name : defaultName);
        break;
    case ArchiveBlockType::Map:
        assert(stringKey || uintKey);
        ++numElements_;
        child = blockElement_.CreateChild(name ? name : defaultName);
        if (stringKey)
            child.SetString("key", *stringKey);
        else if (uintKey)
            child.SetUInt("key", *uintKey);
        else
            assert(0);
        usedNames_.emplace(name);
        break;
    default:
        assert(0);
        break;
    }
    return child;
}

bool XMLOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
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
        stack_.push_back(Block{ type, xmlFile_->CreateRoot(name ? name : defaultRootName), sizeHint });
        return true;
    }

    // Validate new block
    Block& currentBlock = GetCurrentBlock();
    if (!currentBlock.CanCreateChild(name, stringKey_, uintKey_))
    {
        SetError();
        return false;
    }

    // Create new block
    stack_.push_back(Block{ type, currentBlock.CreateChild(name, stringKey_, uintKey_, defaultBlockName), sizeHint });
    ResetKeys();
    return true;
}

bool XMLOutputArchive::EndBlock()
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

    // Pop the frame, close output if root is closed
    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool XMLOutputArchive::SerializeFloatArray(const char* name, float* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string = Detail::FormatFloatArray(values, size);
        return Serialize(name, string);
    }
}

bool XMLOutputArchive::SerializeIntArray(const char* name, int* values, unsigned size)
{
    if (!preferStrings_)
        return Detail::SerializePrimitiveStaticArray(*this, name, values, size);
    else
    {
        ea::string string = Detail::FormatIntArray(values, size);
        return Serialize(name, string);
    }
}

bool XMLOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (XMLElement child = PrepareToSerialize(name))
    {
        BufferToHexString(tempString_, bytes, size);
        child.SetString("value", tempString_);
        return true;
    }
    return false;
}

bool XMLOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (XMLElement child = PrepareToSerialize(name))
    {
        child.SetUInt("value", value);
        return true;
    }
    return false;
}

XMLElement XMLOutputArchive::PrepareToSerialize(const char* name)
{
    // Check if output is closed
    if (IsEOF())
    {
        SetError();
        return {};
    }

    // Validate new element
    Block& currentBlock = GetCurrentBlock();
    if (!currentBlock.CanCreateChild(name, stringKey_, uintKey_))
    {
        SetError();
        return {};
    }

    // Create new element
    XMLElement child = currentBlock.CreateChild(name, stringKey_, uintKey_, defaultElementName);
    if (!child)
    {
        SetError();
        return {};
    }

    ResetKeys();
    return child;
}

// Generate serialization implementation (XML output)
#define URHO3D_XML_OUT_IMPL(type, function) \
    bool XMLOutputArchive::Serialize(const char* name, type& value) \
    { \
        if (XMLElement child = PrepareToSerialize(name)) \
        { \
            child.function("value", value); \
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

XMLInputArchiveBlock::XMLInputArchiveBlock(ArchiveBlockType type, XMLElement blockElement)
    : type_(type)
    , blockElement_(blockElement)
{
    if (blockElement_)
        nextChild_ = blockElement_.GetChild();
}

unsigned XMLInputArchiveBlock::CountChildren() const
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

XMLElement XMLInputArchiveBlock::GetCurrentElement(const char* name, const char* defaultName)
{
    switch (type_)
    {
    case ArchiveBlockType::Sequential:
        if (nextChild_.GetName() == (name ? name : defaultName))
            return nextChild_;
        return {};
    case ArchiveBlockType::Unordered:
        if (name)
            return blockElement_.GetChild(name);
        return {};
    case ArchiveBlockType::Array:
    case ArchiveBlockType::Map:
        return nextChild_;
    default:
        assert(0);
        return {};
    }
}

void XMLInputArchiveBlock::ReadCurrentElementKey(ea::string* stringKey, unsigned* uintKey)
{
    if (type_ == ArchiveBlockType::Map)
    {
        if (stringKey)
            *stringKey = nextChild_.GetAttribute("key");
        if (uintKey)
            *uintKey = nextChild_.GetUInt("key");
    }
}

bool XMLInputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
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
        Block block{ type, xmlFile_->GetRoot(name ? name : defaultRootName) };
        if (!block.IsValid())
        {
            SetError();
            return false;
        }

        sizeHint = block.CountChildren();
        stack_.push_back(block);
        return true;
    }

    // Try open block
    Block& currentBlock = GetCurrentBlock();
    Block block{ type, currentBlock.GetCurrentElement(name, defaultBlockName) };

    if (!block.IsValid())
    {
        SetError();
        return false;
    }

    // Read key if present
    currentBlock.ReadCurrentElementKey(stringKey_, uintKey_);
    ResetKeys();

    // Create new block
    currentBlock.NextElement();
    sizeHint = block.CountChildren();
    stack_.push_back(block);
    return true;
}

bool XMLInputArchive::EndBlock()
{
    if (IsEOF())
    {
        SetError();
        return false;
    }

    stack_.pop_back();

    if (stack_.empty())
        CloseArchive();
    return true;
}

bool XMLInputArchive::SerializeFloatArray(const char* name, float* values, unsigned size)
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

bool XMLInputArchive::SerializeIntArray(const char* name, int* values, unsigned size)
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

bool XMLInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (XMLElement child = PrepareToSerialize(name))
    {
        if (!HexStringToBuffer(tempBuffer_, child.GetAttribute("value")))
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
    if (XMLElement child = PrepareToSerialize(name))
    {
        value = child.GetUInt("value");
        return true;
    }
    return false;
}

XMLElement XMLInputArchive::PrepareToSerialize(const char* name)
{
    // Check if output is closed
    if (IsEOF())
    {
        SetError();
        return {};
    }

    // Find element
    Block& currentBlock = GetCurrentBlock();
    XMLElement child = currentBlock.GetCurrentElement(name, defaultElementName);
    if (!child)
    {
        SetError();
        return {};
    }

    // Read key if present
    currentBlock.ReadCurrentElementKey(stringKey_, uintKey_);
    ResetKeys();

    // Read value.
    currentBlock.NextElement();
    return child;
}

// Generate serialization implementation (XML input)
#define URHO3D_XML_IN_IMPL(type, function) \
    bool XMLInputArchive::Serialize(const char* name, type& value) \
    { \
        if (XMLElement child = PrepareToSerialize(name)) \
        { \
            value = child.function("value"); \
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
