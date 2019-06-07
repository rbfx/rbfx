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

#pragma once

#include "../IO/Archive.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"

#include <EASTL/hash_set.h>

namespace Urho3D
{

/// Base archive for XML serialization.
template <class T>
class XMLArchiveBase : public ArchiveBase
{
public:
    /// Construct.
    explicit XMLArchiveBase(SharedPtr<XMLFile> xmlFile, bool humanReadable = true)
        : xmlFile_(xmlFile)
        , humanReadable_(humanReadable)
    {}

    /// Get context.
    Context* GetContext() final { return xmlFile_->GetContext(); }
    /// Whether the archive is binary.
    bool IsBinary() const final { return false; }
    /// Whether the human-readability is preferred over performance and output size.
    bool IsHumanReadable() const final { return humanReadable_; }
    /// Whether the Unordered blocks are supported.
    bool IsUnorderedSupported() const final { return true; }

protected:
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

    /// XML file.
    SharedPtr<XMLFile> xmlFile_;
    /// Whether to prefer string format.
    bool humanReadable_{};
    /// Blocks stack.
    ea::vector<Block> stack_;
};

template <class T> const char* XMLArchiveBase<T>::defaultRootName = "root";
template <class T> const char* XMLArchiveBase<T>::defaultBlockName = "block";
template <class T> const char* XMLArchiveBase<T>::defaultElementName = "element";

/// XML output archive block. Internal.
class XMLOutputArchiveBlock
{
public:
    /// Construct.
    XMLOutputArchiveBlock(const char* name, ArchiveBlockType type, XMLElement blockElement, unsigned sizeHint);
    /// Get block name.
    ea::string_view GetName() const { return name_; }
    /// Set element key.
    bool SetElementKey(ArchiveBase& archive, ea::string key);
    /// Create element in the block.
    XMLElement CreateElement(ArchiveBase& archive, const char* elementName, const char* defaultElementName);
    /// Close block.
    bool Close(ArchiveBase& archive);

private:
    /// Blcok name.
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
class URHO3D_API XMLOutputArchive : public XMLArchiveBase<XMLOutputArchiveBlock>
{
public:
    using XMLArchiveBase<XMLOutputArchiveBlock>::XMLArchiveBase;

    /// Whether the archive is in input mode.
    bool IsInput() const final { return false; }

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type) final;
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
    bool CheckEOF(const char* elementName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName);
    /// Prepare to serialize element.
    XMLElement CreateElement(const char* name);

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
    /// Return size hint.
    unsigned CalculateSizeHint() const;
    /// Return current child's key.
    bool ReadCurrentKey(ArchiveBase& archive, ea::string& key);
    /// Read current child and move to the next one.
    XMLElement ReadElement(ArchiveBase& archive, const char* elementName);

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
class URHO3D_API XMLInputArchive : public XMLArchiveBase<XMLInputArchiveBlock>
{
public:
    using XMLArchiveBase<XMLInputArchiveBlock>::XMLArchiveBase;

    /// Whether the archive is in input mode.
    bool IsInput() const final { return true; }

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type) final;
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
    bool CheckEOF(const char* elementName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName);
    /// Prepare to serialize element.
    XMLElement ReadElement(const char* name);

    /// Temporary buffer.
    ea::vector<unsigned char> tempBuffer_;
};

}
