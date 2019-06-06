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
    Invalid = -1,
    /// Sequential data block. Use for unstructured data blobs.
    /// - Order of serialization must be the same.
    /// - Names are optional and may be chosen arbitrarily.
    /// - Names must be the same for serialization and deserialization.
    /// - Keys are ignored.
    Sequential,
    /// Unordered data block. Use for objects and structures.
    /// - Order of serialization may be irrelevant if target format support it (e.g. XML or JSON).
    /// - Names are required.
    /// - Names must be unique withing block.
    /// - Names mustn't contain double underscore ("__").
    /// - Keys are ignored.
    Unordered,
    /// Array data block. Use for arrays.
    /// - Order of serialization must be the same.
    /// - Names are irrelevant. Target format may use names for readability.
    /// - Keys are ignored.
    /// - Size hint must match actual number of elements in the array (for serialization).
    /// - Size hint indicates actual number of elements (for deserialization).
    Array,
    /// Map data block. Use for maps.
    /// - Order of serialization must be the same.
    /// - Names are irrelevant. Target format may use names for readability.
    /// - Keys are serialized.
    /// - Keys must be unique.
    /// - Size hint must match actual number of elements in the map (for serialization).
    /// - Size hint indicates actual number of elements (for deserialization).
    Map,
};

class Archive;

/// Archive block wrapper.
class URHO3D_API ArchiveBlockGuard
{
public:
    /// Non-copyable.
    ArchiveBlockGuard(const ArchiveBlockGuard& other) = delete;
    /// Non-copy-assignable.
    ArchiveBlockGuard& operator=(const ArchiveBlockGuard& other) = delete;
    /// Construct invalid.
    ArchiveBlockGuard() = default;
    /// Construct valid.
    explicit ArchiveBlockGuard(Archive& archive, unsigned sizeHint = 0) : archive_(&archive), sizeHint_(sizeHint) {}
    /// Destruct.
    ~ArchiveBlockGuard();
    /// Move-construct.
    ArchiveBlockGuard(ArchiveBlockGuard && other) { Swap(other); }
    /// Move-assign.
    ArchiveBlockGuard& operator=(ArchiveBlockGuard && other) { Swap(other); return *this; }
    /// Swap with another.
    void Swap(ArchiveBlockGuard& other)
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
    virtual bool IsInput() const = 0;
    /// Whether the archive is binary.
    virtual bool IsBinary() const = 0;
    /// Whether the any following archive operation will result in failure.
    virtual bool IsEOF() const = 0;
    /// Whether the serialization error occurred.
    virtual bool IsBad() const = 0;
    /// Whether the Unordered blocks are supported.
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
    /// Set string key for the next block of element of the current Map block.
    /// Referenced string must stay alive until the next call of Serializize or BeginBlock!
    virtual bool SetStringKey(ea::string* key) = 0;
    /// Serialize unsigned integer key. Used with Map block only.
    /// Referenced integer must stay alive until the next call of Serializize or BeginBlock!
    virtual bool SetUnsignedKey(unsigned* key) = 0;

    /// @name Serialize
    /// @{

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

    /// Serialize float array. Size is not encoded and should be provided externally!
    virtual bool SerializeFloatArray(const char* name, float* values, unsigned size) = 0;
    /// Serialize integer array. Size is not encoded and should be provided externally!
    virtual bool SerializeIntArray(const char* name, int* values, unsigned size) = 0;
    /// Serialize bytes. Size is not encoded and should be provided externally!
    virtual bool SerializeBytes(const char* name, void* bytes, unsigned size) = 0;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    virtual bool SerializeVLE(const char* name, unsigned& value) = 0;

    /// @{

    /// Begin archive block and return the guard that will end it automatically on destruction.
    ArchiveBlockGuard OpenBlock(const char* name, unsigned sizeHint, ArchiveBlockType type)
    {
        const bool opened = BeginBlock(name, sizeHint, type);
        return opened ? ArchiveBlockGuard{ *this, sizeHint } : ArchiveBlockGuard{};
    }

    /// Open block helpers
    /// @{

    /// Open Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlockGuard OpenSequentialBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Sequential); }
    /// Open Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlockGuard OpenUnorderedBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Unordered); }
    /// Open Array block. Will be automatically closed when returned object is destroyed.
    ArchiveBlockGuard OpenArrayBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Array); }
    /// Open Map block. Will be automatically closed when returned object is destroyed.
    ArchiveBlockGuard OpenMapBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, ArchiveBlockType::Map); }

    /// @}
};

/// Archive implementation helper.
class ArchiveBase : public Archive
{
public:
    /// Artificial element name used for Map keys
    static const char* keyElementName_;

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

    /// Error message: input archive has no more data. Placeholders: {blockName} {elementName}.
    static const ea::string errorReadEOF_blockName_elementName;
    /// Error message: element or block is not found. Placeholders: {blockName} {elementName}.
    static const ea::string errorElementNotFound_blockName_elementName;
    /// Error message: unexpected block type. Placeholders: {blockName}.
    static const ea::string errorUnexpectedBlockType_blockName;
    /// Error message: missing map key. Placeholders: {blockName}.
    static const ea::string errorMissingMapKey_blockName;

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
