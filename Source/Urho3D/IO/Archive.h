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

#include "../Core/Context.h"
#include "../Core/StringUtils.h"

#include <EASTL/string.h>
#include <EASTL/utility.h>

namespace Urho3D
{

/// Type of archive block.
enum class ArchiveBlockType
{
    /// Default data block. Stores data in the order of serialization.
    /// - Names are optional and arbitrary.
    /// - Names are not checked by any means.
    /// - Keys are not allowed.
    Sequential,
    /// Unordered data block. Stored data may be accessed in any order.
    /// - May be not supported by some archive types.
    /// - Identical to Sequential block if not supported.
    /// - If supported, order of serialization is irrelevant.
    /// - Names are required.
    /// - Names must be unique withing block.
    /// - Names mustn't contain double underscore ("__").
    /// - Keys are not allowed.
    Unordered,
    /// Data block storing dynamic number of elements.
    /// - Similar to Sequential block type.
    /// - When reading, exact size of the array is provided.
    /// - When writing, exact size of the array must be specified.
    Array,
    /// Data block storing dynamic number of key-element pairs.
    /// - Similar to Sequential block type.
    /// - When reading, exact size of the array is provided.
    /// - When writing, exact size of the array must be specified.
    /// - Keys must be provided for each element exactly once.
    /// - Keys must be unique.
    Map,
};

class Archive;

/// Archive block scope guard.
class URHO3D_API ArchiveBlock
{
public:
    /// Non-copyable.
    ArchiveBlock(const ArchiveBlock& other) = delete;
    /// Non-copy-assignable.
    ArchiveBlock& operator=(const ArchiveBlock& other) = delete;
    /// Construct invalid.
    ArchiveBlock() = default;
    /// Construct valid.
    explicit ArchiveBlock(Archive& archive, unsigned sizeHint = 0) : archive_(&archive), sizeHint_(sizeHint) {}
    /// Destruct.
    ~ArchiveBlock();
    /// Move-construct.
    ArchiveBlock(ArchiveBlock && other) { Swap(other); }
    /// Move-assign.
    ArchiveBlock& operator=(ArchiveBlock && other) { Swap(other); return *this; }
    /// Swap with another.
    void Swap(ArchiveBlock& other)
    {
        ea::swap(archive_, other.archive_);
        ea::swap(sizeHint_, other.sizeHint_);
    }
    /// Convert to bool.
    explicit operator bool() const { return !!archive_; }
    /// Get size hint.
    unsigned GetSizeHint() const { return sizeHint_; }

private:
    /// Archive.
    Archive* archive_{};
    /// Block size.
    unsigned sizeHint_{};
};

/// Archive interface.
class URHO3D_API Archive
{
public:
    /// Get context.
    virtual Context* GetContext() = 0;
    /// Whether the archive is in input mode.
    /// It is guaranteed that input archive doesn't read from variable.
    /// It is guaranteed that output archive doesn't write to variable.
    virtual bool IsInput() const = 0;
    /// Whether the archive is binary.
    virtual bool IsBinary() const = 0;
    /// Whether the human-readability is preferred over performance and output size.
    /// - String hashes are serialized as strings, if possible.
    /// - Enumerators serialized as strings, if possible.
    /// - Simple compound types like Vector3 are serialized as formatted strings instead of blocks.
    virtual bool IsHumanReadable() const = 0;
    /// Whether the archive can no longer be serialized.
    virtual bool IsEOF() const = 0;
    /// Whether the serialization error occurred.
    virtual bool IsBad() const = 0;
    /// Whether the unordered element access is supported for Unordered blocks.
    /// If false, serializing code should treat Unordered blocks as Sequential.
    virtual bool IsUnorderedSupported() const = 0;

    /// Return latest error string.
    virtual ea::string_view GetErrorString() const { return {}; }
    /// Return name of the archive.
    virtual ea::string_view GetName() const { return {}; }
    /// Return a checksum if applicable.
    virtual unsigned GetChecksum() { return 0; }

    /// Begin archive block.
    virtual bool BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type) = 0;
    /// End archive block.
    /// Failure usually means implementation error, for example one of the following:
    /// - Array or Map wasn't completely serialized for output archive.
    /// - There were no corresponding BeginBlock call.
    virtual bool EndBlock() = 0;

    /// @name Serialize
    /// @{

    /// Serialize string key of the Map block.
    virtual bool SerializeKey(ea::string& key) = 0;
    /// Serialize unsigned integer key of the Map block.
    virtual bool SerializeKey(unsigned& key) = 0;

    /// Serialize bool.
    virtual bool Serialize(const char* name, bool& value) = 0;
    /// Serialize signed char.
    virtual bool Serialize(const char* name, signed char& value) = 0;
    /// Serialize unsigned char.
    virtual bool Serialize(const char* name, unsigned char& value) = 0;
    /// Serialize signed short.
    virtual bool Serialize(const char* name, short& value) = 0;
    /// Serialize unsigned short.
    virtual bool Serialize(const char* name, unsigned short& value) = 0;
    /// Serialize signed int.
    virtual bool Serialize(const char* name, int& value) = 0;
    /// Serialize unsigned int.
    virtual bool Serialize(const char* name, unsigned int& value) = 0;
    /// Serialize signed long.
    virtual bool Serialize(const char* name, long long& value) = 0;
    /// Serialize unsigned long.
    virtual bool Serialize(const char* name, unsigned long long& value) = 0;
    /// Serialize float.
    virtual bool Serialize(const char* name, float& value) = 0;
    /// Serialize double.
    virtual bool Serialize(const char* name, double& value) = 0;
    /// Serialize string.
    virtual bool Serialize(const char* name, ea::string& value) = 0;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    virtual bool SerializeBytes(const char* name, void* bytes, unsigned size) = 0;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    virtual bool SerializeVLE(const char* name, unsigned& value) = 0;

    /// @}

    /// Begin archive block and return the guard that will end it automatically on destruction.
    ArchiveBlock OpenBlock(const char* name, unsigned sizeHint, ArchiveBlockType type)
    {
        const bool opened = BeginBlock(name, sizeHint, type);
        return opened ? ArchiveBlock{ *this, sizeHint } : ArchiveBlock{};
    }

    /// Open block helpers
    /// @{

    /// Open Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSequentialBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Sequential); }
    /// Open Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenUnorderedBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Unordered); }
    /// Open Array block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenArrayBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Array); }
    /// Open Map block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenMapBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Map); }

    /// @}
};

/// Archive implementation helper.
class ArchiveBase : public Archive
{
public:
    /// Artificial element name used for Map keys
    static const char* keyElementName_;
    /// Artificial element name used for block
    static const char* blockElementName_;
    /// Artificial element name used for checked block guard
    static const char* checkedBlockGuardElementName_;

    /// Fatal error message: root block was not opened. Placeholders: {elementName}.
    static const ea::string fatalRootBlockNotOpened_elementName;
    /// Fatal error message: unexpected EndBlock call.
    static const ea::string fatalUnexpectedEndBlock;
    /// Fatal error message: missing element name in Unordered block. Placeholders: {blockName}.
    static const ea::string fatalMissingElementName_blockName;
    /// Fatal error message: missing key serialization. Placeholders: {blockName}.
    static const ea::string fatalMissingKeySerialization_blockName;
    /// Fatal error message: duplicated key serialization. Placeholders: {blockName}.
    static const ea::string fatalDuplicateKeySerialization_blockName;
    /// Fatal error message: unexpected key serialization. Placeholders: {blockName}.
    static const ea::string fatalUnexpectedKeySerialization_blockName;

    /// Error message: generic input error. Placeholders: {blockName} {elementName}.
    static const ea::string errorCannotReadData_blockName_elementName;
    /// Error message: input archive has no more data. Placeholders: {blockName} {elementName}.
    static const ea::string errorReadEOF_blockName_elementName;
    /// Error message: element or block is not found. Placeholders: {blockName} {elementName}.
    static const ea::string errorElementNotFound_blockName_elementName;
    /// Error message: unexpected block type. Placeholders: {blockName}.
    static const ea::string errorUnexpectedBlockType_blockName;
    /// Error message: missing map key. Placeholders: {blockName}.
    static const ea::string errorMissingMapKey_blockName;

    /// Error message: generic output error. Placeholders: {blockName} {elementName}.
    static const ea::string errorCannotWriteData_blockName_elementName;
    /// Error message: output archive is finished. Placeholders: {blockName} {elementName}.
    static const ea::string errorWriteEOF_blockName_elementName;
    /// Error message: duplicate element. Placeholders: {blockName} {elementName}.
    static const ea::string errorDuplicateElement_blockName_elementName;
    /// Fatal error message: Archive or Map block overflow while writing archive. Placeholders: {blockName}
    static const ea::string fatalBlockOverflow_blockName;
    /// Fatal error message: Archive or Map block underflow while writing archive. Placeholders: {blockName}
    static const ea::string fatalBlockUnderflow_blockName;

    /// Whether the any following archive operation will result in failure.
    bool IsEOF() const final { return eof_; }
    /// Whether the serialization error occurred.
    bool IsBad() const final { return hasError_; }
    /// Return latest error string.
    ea::string_view GetErrorString() const final { return errorString_; }

    /// Set error flag.
    void SetError() { hasError_ = true; }
    /// Set error flag and error string.
    template <class ... Args>
    void SetError(ea::string_view errorString, const Args& ... args)
    {
        if (!hasError_)
        {
            hasError_ = true;
            errorString_ = Format(errorString, args...);
        }
    }

protected:
    /// Set EOF flag.
    void CloseArchive() { eof_ = true; }

private:
    /// End-of-file flag.
    bool eof_{};
    /// Error flag.
    bool hasError_{};
    /// Error string.
    ea::string errorString_{};
};

}
