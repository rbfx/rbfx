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

#pragma once

#include "../IO/ArchiveBase.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"

#include <EASTL/hash_set.h>

namespace Urho3D
{

/// Base archive for XML serialization.
template <class BlockType, bool IsInputBool>
class XMLArchiveBase : public ArchiveBaseT<BlockType, IsInputBool, true>
{
public:
    /// @name Archive implementation
    /// @{
    ea::string_view GetName() const { return xmlFile_ ? xmlFile_->GetName() : ""; }
    /// @}

protected:
    XMLArchiveBase(Context* context, XMLElement element, const XMLFile* xmlFile, bool serializeRootName)
        : ArchiveBaseT<BlockType, IsInputBool, true>(context)
        , rootElement_(element)
        , xmlFile_(xmlFile)
        , serializeRootName_(serializeRootName)
    {
        URHO3D_ASSERT(element);
    }

    XMLElement rootElement_{};
    const bool serializeRootName_{};

private:
    /// XML file.
    const XMLFile* xmlFile_{};
};

/// XML attribute reference for input and output archives.
class XMLAttributeReference
{
public:
    XMLAttributeReference(XMLElement element, const char* attribute) : element_(element), attribute_(attribute) {}

    XMLElement GetElement() const { return element_; }
    const char* GetAttributeName() const { return attribute_; }

private:
    XMLElement element_;
    const char* attribute_{};
};

/// XML output archive block.
class URHO3D_API XMLOutputArchiveBlock : public ArchiveBlockBase
{
public:
    XMLOutputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement, unsigned sizeHint);

    XMLElement CreateElement(ArchiveBase& archive, const char* elementName);
    XMLAttributeReference CreateElementOrAttribute(ArchiveBase& archive, const char* elementName);

    bool IsUnorderedAccessSupported() const { return type_ == ArchiveBlockType::Unordered; }
    bool HasElementOrBlock(const char* name) const { return false; }
    void Close(ArchiveBase& archive);

private:
    XMLElement blockElement_{};

    /// Expected block size (for arrays).
    unsigned expectedElementCount_{M_MAX_UNSIGNED};
    /// Number of elements in block.
    unsigned numElements_{};

    /// Set of used names for checking.
    ea::hash_set<ea::string> usedNames_{};
};

/// XML output archive.
class URHO3D_API XMLOutputArchive : public XMLArchiveBase<XMLOutputArchiveBlock, false>
{
public:
    /// Construct from element.
    XMLOutputArchive(Context* context, XMLElement element, XMLFile* xmlFile = nullptr, bool serializeRootName = false)
        : XMLArchiveBase(context, element, xmlFile, serializeRootName)
    {
    }
    /// Construct from file.
    explicit XMLOutputArchive(XMLFile* xmlFile)
        : XMLArchiveBase(xmlFile->GetContext(), xmlFile->GetOrCreateRoot(rootBlockName), xmlFile, true)
    {}

    /// @name Archive implementation
    /// @{
    void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;

    void Serialize(const char* name, bool& value) final;
    void Serialize(const char* name, signed char& value) final;
    void Serialize(const char* name, unsigned char& value) final;
    void Serialize(const char* name, short& value) final;
    void Serialize(const char* name, unsigned short& value) final;
    void Serialize(const char* name, int& value) final;
    void Serialize(const char* name, unsigned int& value) final;
    void Serialize(const char* name, long long& value) final;
    void Serialize(const char* name, unsigned long long& value) final;
    void Serialize(const char* name, float& value) final;
    void Serialize(const char* name, double& value) final;
    void Serialize(const char* name, ea::string& value) final;

    void SerializeBytes(const char* name, void* bytes, unsigned size) final;
    void SerializeVLE(const char* name, unsigned& value) final;
    /// @}

private:
    XMLAttributeReference CreateElementOrAttribute(const char* name);

    ea::string tempString_;
};

/// XML input archive block.
class URHO3D_API XMLInputArchiveBlock : public ArchiveBlockBase
{
public:
    XMLInputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement);

    /// Return size hint.
    unsigned CalculateSizeHint() const;
    /// Read current child and move to the next one.
    XMLElement ReadElement(ArchiveBase& archive, const char* elementName);
    /// Read attribute (for Unordered blocks only) or the element and move to the next one.
    XMLAttributeReference ReadElementOrAttribute(ArchiveBase& archive, const char* elementName);

    bool IsUnorderedAccessSupported() const { return type_ == ArchiveBlockType::Unordered; }
    bool HasElementOrBlock(const char* name) const { return blockElement_.HasChild(name) || blockElement_.HasAttribute(name); }
    void Close(ArchiveBase& archive) {}

private:
    /// Block element.
    XMLElement blockElement_{};
    /// Next child to read.
    XMLElement nextChild_{};
};

/// XML input archive.
class URHO3D_API XMLInputArchive : public XMLArchiveBase<XMLInputArchiveBlock, true>
{
public:
    /// Construct from element.
    XMLInputArchive(Context* context, XMLElement element, const XMLFile* xmlFile = nullptr, bool serializeRootName = false)
        : XMLArchiveBase(context, element, xmlFile, serializeRootName)
    {
    }
    /// Construct from file.
    explicit XMLInputArchive(const XMLFile* xmlFile)
        : XMLArchiveBase(xmlFile->GetContext(), xmlFile->GetRoot(), xmlFile, false)
    {}

    /// @name Archive implementation
    /// @{
    void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;

    void Serialize(const char* name, bool& value) final;
    void Serialize(const char* name, signed char& value) final;
    void Serialize(const char* name, unsigned char& value) final;
    void Serialize(const char* name, short& value) final;
    void Serialize(const char* name, unsigned short& value) final;
    void Serialize(const char* name, int& value) final;
    void Serialize(const char* name, unsigned int& value) final;
    void Serialize(const char* name, long long& value) final;
    void Serialize(const char* name, unsigned long long& value) final;
    void Serialize(const char* name, float& value) final;
    void Serialize(const char* name, double& value) final;
    void Serialize(const char* name, ea::string& value) final;

    void SerializeBytes(const char* name, void* bytes, unsigned size) final;
    void SerializeVLE(const char* name, unsigned& value) final;
    /// @}

private:
    XMLAttributeReference ReadElementOrAttribute(const char* name);
};

}
