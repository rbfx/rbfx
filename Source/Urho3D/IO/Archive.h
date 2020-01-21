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

#include "../Core/Context.h"
#include "../Core/NonCopyable.h"
#include "../Core/StringUtils.h"

#include <EASTL/string.h>
#include <EASTL/utility.h>

namespace Urho3D
{

/// Type of archive block.
/// - Default block type is Sequential.
/// - Other block types are used to improve quality of human-readable formats.
/// - Directly nested blocks and elements are called "items".
enum class ArchiveBlockType
{
    /// Sequential data block.
    /// - Items are saved and loaded in the order of serialization.
    /// - Names of items are optional and have no functional purpose.
    Sequential,
    /// Unordered data block.
    /// - Items are saved and loaded in the order of serialization.
    /// - Name must be unique for each item since it is used for input file lookup.
    /// - Input file may contain items of Unordered block in arbitrary order, if it is supported by actual archive format.
    /// - Syntax sugar for structures in human-readable and human-editable formats.
    /// - Best choice when number of items is known and fixed (e.g. structure or object).
    Unordered,
    /// Array data block.
    /// - Items are saved and loaded in the order of serialization.
    /// - Names of items are optional and have no functional purpose.
    /// - When reading, number of items is known when the block is opened.
    /// - When writing, number of items must be provided when the block is opened.
    /// - Syntax sugar for arrays in human-readable and human-editable formats.
    /// - Best choice when items are ordered and number of items is dynamic (e.g. dynamic array).
    Array,
    /// Map data block.
    /// - Order of serialization is not guaranteed.
    /// - Names of items are optional and have no functional purpose.
    /// - Before every item, key should be serialized via SerializeKey exactly once.
    /// - When reading, number of items is known when the block is opened.
    /// - When writing, number of items must be provided when the block is opened.
    /// - Key must be unique for each item.
    /// - Syntax sugar for maps in human-readable and human-editable formats.
    /// - Best choice when there are key-value pairs (e.g. map or hash map).
    Map,
};

class Archive;

/// Archive block scope guard.
class URHO3D_API ArchiveBlock : private NonCopyable
{
public:
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
/// - Archive is a hierarchical structure of blocks and elements.
/// - Archive must have exactly one root block.
/// - Any block may contain other blocks or elements of any type.
/// - Any block or element may have name. Use C++ naming conventions for identifiers, arbitrary strings are not allowed. Name "key" is reserved.
/// - Unsafe block must not be closed until all the items are serialized.
class URHO3D_API Archive
{
public:
    /// Get context.
    virtual Context* GetContext() = 0;
    /// Return name of the archive.
    virtual ea::string_view GetName() const = 0;
    /// Return a checksum if applicable.
    virtual unsigned GetChecksum() = 0;

    /// Whether the archive is in input mode.
    /// It is guaranteed that input archive doesn't read from variable.
    /// It is guaranteed that output archive doesn't write to variable.
    /// It is save to cast away const-ness when serializing into output archive.
    virtual bool IsInput() const = 0;
    /// Whether the human-readability is preferred over performance and output size.
    /// - Binary serialization is disfavored.
    /// - String hashes are serialized as strings, if possible.
    /// - Enumerators serialized as strings, if possible.
    /// - Simple compound types like Vector3 are serialized as formatted strings instead of blocks.
    virtual bool IsHumanReadable() const = 0;

    /// Whether the unordered element access is supported in currently open block.
    /// Always false if current block is not Unordered.
    virtual bool IsUnorderedSupportedNow() const = 0;
    /// Whether the archive can no longer be serialized.
    virtual bool IsEOF() const = 0;
    /// Whether the serialization error occurred.
    virtual bool HasError() const = 0;
    /// Return current string stack.
    virtual ea::string GetCurrentStackString() = 0;
    /// Return first error string.
    virtual ea::string_view GetErrorString() const = 0;
    /// Return first error stack.
    virtual ea::string_view GetErrorStack() const = 0;

    /// Set archive error.
    virtual void SetError(ea::string_view error) = 0;

    /// Begin archive block.
    /// Size is required for Array and Map blocks.
    /// It is guaranteed that errors occurred during serialization of the safe block don't affect data outside of the block.
    virtual bool BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) = 0;
    /// End archive block.
    /// Failure usually means code error, for example one of the following:
    /// - Array or Map size doesn't match the number of serialized elements.
    /// - There is no corresponding BeginBlock call.
    virtual bool EndBlock() = 0;

    /// \name Serialize keys
    /// \{

    /// Serialize string key of the Map block.
    virtual bool SerializeKey(ea::string& key) = 0;
    /// Serialize unsigned integer key of the Map block.
    virtual bool SerializeKey(unsigned& key) = 0;

    /// \}

    /// \name Serialize elements
    /// \{

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
    /// Serialize version number. 0 is invalid version.
    virtual unsigned SerializeVersion(unsigned version) = 0;

    /// \}

    /// Validate element or block name.
    static bool ValidateName(ea::string_view name);

    /// Do BeginBlock and return the guard that will call EndBlock automatically on destruction.
    /// Return null block in case of error.
    ArchiveBlock OpenBlock(const char* name, unsigned sizeHint, bool safe, ArchiveBlockType type)
    {
        const bool opened = BeginBlock(name, sizeHint, safe, type);
        return opened ? ArchiveBlock{ *this, sizeHint } : ArchiveBlock{};
    }

    /// Open block helpers
    /// \{

    /// Open Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSequentialBlock(const char* name) { return OpenBlock(name, 0, false, ArchiveBlockType::Sequential); }
    /// Open Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenUnorderedBlock(const char* name) { return OpenBlock(name, 0, false, ArchiveBlockType::Unordered); }
    /// Open Array block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenArrayBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, false, ArchiveBlockType::Array); }
    /// Open Map block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenMapBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, false, ArchiveBlockType::Map); }

    /// Open safe Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSafeSequentialBlock(const char* name) { return OpenBlock(name, 0, true, ArchiveBlockType::Sequential); }
    /// Open safe Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSafeUnorderedBlock(const char* name) { return OpenBlock(name, 0, true, ArchiveBlockType::Unordered); }

    /// \}
};

/// Archive implementation helper. Provides default Archive implementation for most cases.
class URHO3D_API ArchiveBase : public Archive, private NonCopyable
{
public:
    /// Get context.
    Context* GetContext() override { return nullptr; }
    /// Return name of the archive.
    ea::string_view GetName() const override { return {}; }
    /// Return a checksum if applicable.
    unsigned GetChecksum() override { return 0; }

    /// Whether the any following archive operation will result in failure.
    bool IsEOF() const final { return eof_; }
    /// Whether the serialization error occurred.
    bool HasError() const final { return hasError_; }
    /// Return first error string.
    ea::string_view GetErrorString() const final { return errorString_; }
    /// Return first error stack.
    ea::string_view GetErrorStack() const final { return errorStack_; }

    /// Serialize version number. 0 is invalid version.
    unsigned SerializeVersion(unsigned version) final
    {
        if (!SerializeVLE(versionElementName_, version))
            return 0;
        return version;
    }

    /// Set archive error.
    void SetError(ea::string_view error) override
    {
        SetErrorFormatted(error);
    }

    /// Set archive error (formatted string).
    template <class ... Args>
    void SetErrorFormatted(ea::string_view errorString, const Args& ... args)
    {
        if (!hasError_)
        {
            hasError_ = true;
            errorStack_ = GetCurrentStackString();
            errorString_ = Format(errorString, args...);
        }
    }

public:
    /// Version element name.
    static const char* versionElementName_;
    /// Artificial element name used for error reporting related to Map keys.
    static const char* keyElementName_;
    /// Artificial element name used for error reporting related to block itself.
    static const char* blockElementName_;

    /// Fatal error message: root block was not opened. Placeholders: {elementName}.
    static const ea::string fatalRootBlockNotOpened_elementName;
    /// Fatal error message: unexpected EndBlock call.
    static const ea::string fatalUnexpectedEndBlock;
    /// Fatal error message: missing element name in Unordered block.
    static const ea::string fatalMissingElementName;
    /// Fatal error message: missing key serialization.
    static const ea::string fatalMissingKeySerialization;
    /// Fatal error message: duplicated key serialization.
    static const ea::string fatalDuplicateKeySerialization;
    /// Fatal error message: unexpected key serialization.
    static const ea::string fatalUnexpectedKeySerialization;
    /// Fatal error message: invalid element or block name.
    static const ea::string fatalInvalidName;
    /// Error message: input archive has no more data. Placeholders: {elementName}.
    static const ea::string errorEOF_elementName;
    /// Error message: unspecified I/O failure. Placeholders: {elementName}.
    static const ea::string errorUnspecifiedFailure_elementName;

    /// Input error message: element or block is not found. Placeholders: {elementName}.
    static const ea::string errorElementNotFound_elementName;
    /// Input error message: unexpected block type. Placeholders: {blockName}.
    static const ea::string errorUnexpectedBlockType_blockName;
    /// Input error message: missing map key.
    static const ea::string errorMissingMapKey;

    /// Output error message: duplicate element. Placeholders: {elementName}.
    static const ea::string errorDuplicateElement_elementName;
    /// Output fatal error message: Archive or Map block overflow while writing archive.
    static const ea::string fatalBlockOverflow;
    /// Output fatal error message: Archive or Map block underflow while writing archive.
    static const ea::string fatalBlockUnderflow;

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
    /// Error stack.
    ea::string errorStack_{};
};

/// Archive implementation helper (template). Provides all archive traits
template <bool IsInputBool, bool IsHumanReadableBool>
class ArchiveBaseT : public ArchiveBase
{
public:
    /// Whether the archive is in input mode.
    bool IsInput() const final { return IsInputBool; }
    /// Whether the human-readability is preferred over performance and output size.
    bool IsHumanReadable() const final { return IsHumanReadableBool; }
};

}
