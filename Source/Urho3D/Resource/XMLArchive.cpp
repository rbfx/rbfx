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

XMLOutputArchiveBlock::XMLOutputArchiveBlock(
    const char* name, ArchiveBlockType type, XMLElement blockElement, unsigned sizeHint)
    : ArchiveBlockBase(name, type)
    , blockElement_(blockElement)
{
    if (type_ == ArchiveBlockType::Array)
        expectedElementCount_ = sizeHint;
}

XMLElement XMLOutputArchiveBlock::CreateElement(ArchiveBase& archive, const char* elementName)
{
    URHO3D_ASSERT(numElements_ < expectedElementCount_);

    if (type_ == ArchiveBlockType::Unordered && usedNames_.contains(elementName))
        throw archive.DuplicateElementException(elementName);

    XMLElement element = blockElement_.CreateChild(elementName);
    ++numElements_;

    if (type_ == ArchiveBlockType::Unordered)
        usedNames_.emplace(elementName);

    URHO3D_ASSERT(element);
    return element;
}

XMLAttributeReference XMLOutputArchiveBlock::CreateElementOrAttribute(ArchiveBase& archive, const char* elementName)
{
    if (type_ != ArchiveBlockType::Unordered)
    {
        XMLElement child = CreateElement(archive, elementName);
        return { child, "value" };
    }

    // Special case for Unordered
    if (usedNames_.contains(elementName))
        throw archive.DuplicateElementException(elementName);

    ++numElements_;
    return { blockElement_, elementName };
}

void XMLOutputArchiveBlock::Close(ArchiveBase& archive)
{
    URHO3D_ASSERT(expectedElementCount_ == M_MAX_UNSIGNED || numElements_ == expectedElementCount_);
}

void XMLOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);
    CheckBlockOrElementName(name);

    if (stack_.empty())
    {
        if (serializeRootName_)
            rootElement_.SetName(name);
        stack_.push_back(Block{ name, type, rootElement_, sizeHint });
        return;
    }

    XMLElement blockElement = GetCurrentBlock().CreateElement(*this, name);
    Block block{ name, type, blockElement, sizeHint };
    stack_.push_back(block);
}

void XMLOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    XMLAttributeReference ref = CreateElementOrAttribute(name);
    BufferToHexString(tempString_, bytes, size);
    ref.GetElement().SetString(ref.GetAttributeName(), tempString_);
}

void XMLOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    XMLAttributeReference ref = CreateElementOrAttribute(name);
    ref.GetElement().SetUInt(ref.GetAttributeName(), value);
}

XMLAttributeReference XMLOutputArchive::CreateElementOrAttribute(const char* name)
{
    CheckBeforeElement(name);
    CheckBlockOrElementName(name);

    XMLOutputArchiveBlock& block = GetCurrentBlock();
    return GetCurrentBlock().CreateElementOrAttribute(*this, name);
}

// Generate serialization implementation (XML output)
#define URHO3D_XML_OUT_IMPL(type, function) \
    void XMLOutputArchive::Serialize(const char* name, type& value) \
    { \
        XMLAttributeReference ref = CreateElementOrAttribute(name); \
        ref.GetElement().function(ref.GetAttributeName(), value); \
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
    : ArchiveBlockBase(name, type)
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

XMLElement XMLInputArchiveBlock::ReadElement(ArchiveBase& archive, const char* elementName)
{
    if (type_ != ArchiveBlockType::Unordered && !nextChild_)
        throw archive.ElementNotFoundException(elementName);

    XMLElement element;
    if (type_ == ArchiveBlockType::Unordered)
        element = blockElement_.GetChild(elementName);
    else
    {
        element = nextChild_;
        nextChild_ = nextChild_.GetNext();
    }

    if (!element)
        throw archive.ElementNotFoundException(elementName);

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
    if (!blockElement_.HasAttribute(elementName))
        throw archive.ElementNotFoundException(elementName);

    return { blockElement_, elementName };
}

void XMLInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);
    CheckBlockOrElementName(name);

    // Open root block
    if (stack_.empty())
    {
        if (serializeRootName_ && rootElement_.GetName() != name)
            throw ElementNotFoundException(name);

        Block block{ name, type, rootElement_ };
        sizeHint = block.CalculateSizeHint();
        stack_.push_back(block);
        return;
    }

    XMLElement blockElement = GetCurrentBlock().ReadElement(*this, name);
    Block block{ name, type, blockElement };
    sizeHint = block.CalculateSizeHint();
    stack_.push_back(block);
}

void XMLInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    XMLAttributeReference ref = ReadElementOrAttribute(name);
    const ea::string& value = ref.GetElement().GetAttribute(ref.GetAttributeName());
    ReadBytesFromHexString(name, value, bytes, size);
}

void XMLInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    XMLAttributeReference ref = ReadElementOrAttribute(name);
    value = ref.GetElement().GetUInt(ref.GetAttributeName());
}

XMLAttributeReference XMLInputArchive::ReadElementOrAttribute(const char* name)
{
    CheckBeforeElement(name);
    CheckBlockOrElementName(name);

    return GetCurrentBlock().ReadElementOrAttribute(*this, name);
}

// Generate serialization implementation (XML input)
#define URHO3D_XML_IN_IMPL(type, function) \
    void XMLInputArchive::Serialize(const char* name, type& value) \
    { \
        XMLAttributeReference ref = ReadElementOrAttribute(name); \
        value = ref.GetElement().function(ref.GetAttributeName()); \
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

} // namespace Urho3D
