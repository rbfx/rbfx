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

#include "../IO/Archive.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"

#include <EASTL/hash_set.h>

namespace Urho3D
{

/// Base archive for XML serialization.
template <class T, bool IsInputBool>
class XMLArchiveBase : public ArchiveBaseT<IsInputBool, true>
{
public:
    /// Get context.
    Context* GetContext() final { return context_; }
    /// Return name of the archive.
    ea::string_view GetName() const final { return xmlFile_ ? xmlFile_->GetName() : ""; }

    /// Whether the unordered element access is supported for Unordered blocks.
    bool IsUnorderedSupportedNow() const final { return !stack_.empty() && stack_.back().GetType() == ArchiveBlockType::Unordered; }

    /// Return current string stack.
    ea::string GetCurrentStackString() final
    {
        ea::string result;
        for (const Block& block : stack_)
        {
            if (!result.empty())
                result += "/";
            result += ea::string{ block.GetName() };
        }
        return result;
    }

protected:
    /// Construct from element.
    XMLArchiveBase(Context* context, XMLElement element, XMLFile* xmlFile, bool serializeRootName)
        : context_(context)
        , rootElement_(element)
        , xmlFile_(xmlFile)
        , serializeRootName_(serializeRootName)
    {
        assert(element);
    }

    /// Block type.
    using Block = T;
    /// Default name of the root block.
    static const char* defaultRootName;
    /// Default name of a block.
    static const char* defaultBlockName;
    /// Default name of an element.
    static const char* defaultElementName;

    /// Get current block.
    Block& GetCurrentBlock() { return stack_.back(); }

    /// Context.
    Context* context_{};
    /// Root XML element.
    XMLElement rootElement_{};
    /// Blocks stack.
    ea::vector<Block> stack_;
    /// Whether to serialize root name.
    const bool serializeRootName_{};

private:
    /// XML file.
    XMLFile* xmlFile_{};
};

template <class T, bool B> const char* XMLArchiveBase<T, B>::defaultRootName = "root";
template <class T, bool B> const char* XMLArchiveBase<T, B>::defaultBlockName = "block";
template <class T, bool B> const char* XMLArchiveBase<T, B>::defaultElementName = "element";

/// XML attribute reference for input and output archives.
class XMLAttributeReference
{
public:
    /// Construct invalid.
    XMLAttributeReference() = default;
    /// Construct valid.
    XMLAttributeReference(XMLElement element, const char* attribute) : element_(element), attribute_(attribute) {}

    /// Return the element.
    XMLElement GetElement() const { return element_; }
    /// Return attribute name.
    const char* GetAttributeName() const { return attribute_; }
    /// Return whether the element valid.
    explicit operator bool() const { return !!element_; }

private:
    /// XML element.
    XMLElement element_;
    /// Attribute name.
    const char* attribute_{};
};

/// XML output archive block. Internal.
class XMLOutputArchiveBlock
{
public:
    /// Construct.
    XMLOutputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement, unsigned sizeHint);
    /// Return block type.
    ArchiveBlockType GetType() const { return type_; }
    /// Return block name.
    ea::string_view GetName() const { return name_; }
    /// Set element key.
    bool SetElementKey(ArchiveBase& archive, ea::string key);
    /// Create element in the block.
    XMLElement CreateElement(ArchiveBase& archive, const char* elementName, const char* defaultElementName);
    /// Create attribute (for Unordered blocks) or element (for the rest of the block types).
    XMLAttributeReference CreateElementOrAttribute(ArchiveBase& archive, const char* elementName, const char* defaultElementName);
    /// Close block.
    bool Close(ArchiveBase& archive);

private:
    /// Block name.
    ea::string_view name_;
    /// Block type.
    ArchiveBlockType type_{};
    /// Block element.
    XMLElement blockElement_{};

    /// Expected block size (for arrays and maps).
    unsigned expectedElementCount_{M_MAX_UNSIGNED};
    /// Number of elements in block.
    unsigned numElements_{};

    /// Set of used names for checking.
    ea::hash_set<ea::string> usedNames_{};

    /// Key of the next created element (for Map blocks).
    ea::string elementKey_;
    /// Whether the key is set.
    bool keySet_{};
};

/// XML output archive.
class URHO3D_API XMLOutputArchive : public XMLArchiveBase<XMLOutputArchiveBlock, false>
{
public:
    /// Base type.
    using Base = XMLArchiveBase<XMLOutputArchiveBlock, false>;

    /// Construct from element.
    XMLOutputArchive(Context* context, XMLElement element, XMLFile* xmlFile = nullptr, bool serializeRootName = false)
        : Base(context, element, xmlFile, serializeRootName)
    {
    }
    /// Construct from file.
    explicit XMLOutputArchive(XMLFile* xmlFile)
        : Base(xmlFile->GetContext(), xmlFile->GetOrCreateRoot(defaultRootName), xmlFile, true)
    {}

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

    /// Serialize string key. Used with Map block only.
    bool SerializeKey(ea::string& key) final;
    /// Serialize unsigned integer key. Used with Map block only.
    bool SerializeKey(unsigned& key) final;

    /// Serialize bool.
    bool Serialize(const char* name, bool& value) final;
    /// Serialize signed char.
    bool Serialize(const char* name, signed char& value) final;
    /// Serialize unsigned char.
    bool Serialize(const char* name, unsigned char& value) final;
    /// Serialize signed short.
    bool Serialize(const char* name, short& value) final;
    /// Serialize unsigned short.
    bool Serialize(const char* name, unsigned short& value) final;
    /// Serialize signed int.
    bool Serialize(const char* name, int& value) final;
    /// Serialize unsigned int.
    bool Serialize(const char* name, unsigned int& value) final;
    /// Serialize signed long.
    bool Serialize(const char* name, long long& value) final;
    /// Serialize unsigned long.
    bool Serialize(const char* name, unsigned long long& value) final;
    /// Serialize float.
    bool Serialize(const char* name, float& value) final;
    /// Serialize double.
    bool Serialize(const char* name, double& value) final;
    /// Serialize string.
    bool Serialize(const char* name, ea::string& value) final;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Check EOF.
    bool CheckEOF(const char* elementName, const char* debugName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName, const char* debugName);
    /// Prepare to serialize element.
    XMLAttributeReference CreateElement(const char* name);

    /// Temporary string buffer.
    ea::string tempString_;
};

/// XML input archive block. Internal.
class XMLInputArchiveBlock
{
public:
    /// Construct valid.
    XMLInputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement);
    /// Return name.
    const ea::string_view GetName() const { return name_; }
    /// Return block type.
    ArchiveBlockType GetType() const { return type_; }
    /// Return size hint.
    unsigned CalculateSizeHint() const;
    /// Return current child's key.
    bool ReadCurrentKey(ArchiveBase& archive, ea::string& key);
    /// Read current child and move to the next one.
    XMLElement ReadElement(ArchiveBase& archive, const char* elementName);
    /// Read attribute (for Unordered blocks only) or the element and move to the next one.
    XMLAttributeReference ReadElementOrAttribute(ArchiveBase& archive, const char* elementName);

private:
    /// Block name.
    ea::string_view name_;
    /// Block type.
    ArchiveBlockType type_{};
    /// Block element.
    XMLElement blockElement_{};
    /// Next child to read.
    XMLElement nextChild_{};

    /// Whether the block key is read.
    bool keyRead_{};
};

/// XML input archive.
class URHO3D_API XMLInputArchive : public XMLArchiveBase<XMLInputArchiveBlock, true>
{
public:
    /// Base type.
    using Base = XMLArchiveBase<XMLInputArchiveBlock, true>;

    /// Construct from element.
    XMLInputArchive(Context* context, XMLElement element, XMLFile* xmlFile = nullptr, bool serializeRootName = false)
        : Base(context, element, xmlFile, serializeRootName)
    {
    }
    /// Construct from file.
    explicit XMLInputArchive(XMLFile* xmlFile)
        : Base(xmlFile->GetContext(), xmlFile->GetRoot(), xmlFile, true)
    {}

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

    /// Serialize string key. Used with Map block only.
    bool SerializeKey(ea::string& key) final;
    /// Serialize unsigned integer key. Used with Map block only.
    bool SerializeKey(unsigned& key) final;

    /// Serialize bool.
    bool Serialize(const char* name, bool& value) final;
    /// Serialize signed char.
    bool Serialize(const char* name, signed char& value) final;
    /// Serialize unsigned char.
    bool Serialize(const char* name, unsigned char& value) final;
    /// Serialize signed short.
    bool Serialize(const char* name, short& value) final;
    /// Serialize unsigned short.
    bool Serialize(const char* name, unsigned short& value) final;
    /// Serialize signed int.
    bool Serialize(const char* name, int& value) final;
    /// Serialize unsigned int.
    bool Serialize(const char* name, unsigned int& value) final;
    /// Serialize signed long.
    bool Serialize(const char* name, long long& value) final;
    /// Serialize unsigned long.
    bool Serialize(const char* name, unsigned long long& value) final;
    /// Serialize float.
    bool Serialize(const char* name, float& value) final;
    /// Serialize double.
    bool Serialize(const char* name, double& value) final;
    /// Serialize string.
    bool Serialize(const char* name, ea::string& value) final;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Check EOF.
    bool CheckEOF(const char* elementName, const char* debugName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName, const char* debugName);
    /// Prepare to serialize element.
    XMLAttributeReference ReadElement(const char* name);

    /// Temporary buffer.
    ea::vector<unsigned char> tempBuffer_;
};

}
