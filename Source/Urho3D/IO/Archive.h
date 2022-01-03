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

#include "../Core/Exception.h"
#include "../Core/NonCopyable.h"

#include <EASTL/string.h>
#include <EASTL/utility.h>

namespace Urho3D
{

class Archive;
class Context;

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
    Array
};

/// Archive block scope guard.
class URHO3D_API ArchiveBlock : private MovableNonCopyable
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
    virtual ~Archive() {}
    /// Get context.
    virtual Context* GetContext() = 0;
    /// Return name of the archive if applicable.
    virtual ea::string_view GetName() const = 0;
    /// Return a checksum if applicable.
    virtual unsigned GetChecksum() = 0;

    /// Whether the archive is in input mode.
    /// It is guaranteed that input archive doesn't read from any variable.
    /// It is guaranteed that output archive doesn't write to any variable.
    /// It is save to cast away const-ness when serializing into output archive.
    virtual bool IsInput() const = 0;
    /// Whether the human-readability is preferred over performance and output size.
    /// - Binary serialization is disfavored.
    /// - String hashes are serialized as strings, if possible.
    /// - Enumerators serialized as strings, if possible.
    /// - Simple compound types like Vector3 are serialized as formatted strings instead of blocks.
    virtual bool IsHumanReadable() const = 0;

    /// Return whether the unordered element access is supported in currently open block.
    /// Always false if current block is not Unordered. Always false for some archive types.
    virtual bool IsUnorderedAccessSupportedInCurrentBlock() const = 0;
    /// Return whether the element or block with given name is present.
    /// Should be called only if both IsInput and IsUnorderedAccessSupportedInCurrentBlock are true.
    virtual bool HasElementOrBlock(const char* name) const = 0;
    /// Whether the archive can no longer be serialized.
    virtual bool IsEOF() const = 0;
    /// Return current string stack.
    virtual ea::string GetCurrentBlockPath() const = 0;

    /// Begin archive block.
    /// Size is required for Array blocks.
    /// It is guaranteed that errors occurred during serialization of the safe block don't affect data outside of the block.
    virtual void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) = 0;
    /// End archive block. May postpone ArchiveException until later.
    virtual void EndBlock() noexcept = 0;
    /// Flush all pending events. Should be called at least once before destructor.
    virtual void Flush() = 0;

    /// @name Serialize primitive element
    /// @{
    virtual void Serialize(const char* name, bool& value) = 0;
    virtual void Serialize(const char* name, signed char& value) = 0;
    virtual void Serialize(const char* name, unsigned char& value) = 0;
    virtual void Serialize(const char* name, short& value) = 0;
    virtual void Serialize(const char* name, unsigned short& value) = 0;
    virtual void Serialize(const char* name, int& value) = 0;
    virtual void Serialize(const char* name, unsigned int& value) = 0;
    virtual void Serialize(const char* name, long long& value) = 0;
    virtual void Serialize(const char* name, unsigned long long& value) = 0;
    virtual void Serialize(const char* name, float& value) = 0;
    virtual void Serialize(const char* name, double& value) = 0;
    virtual void Serialize(const char* name, ea::string& value) = 0;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    virtual void SerializeBytes(const char* name, void* bytes, unsigned size) = 0;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    virtual void SerializeVLE(const char* name, unsigned& value) = 0;
    /// Serialize version number. 0 is invalid version.
    virtual unsigned SerializeVersion(unsigned version) = 0;
    /// @}

    /// Validate element or block name.
    static bool ValidateName(ea::string_view name);

    /// Do BeginBlock and return the guard that will call EndBlock automatically on destruction.
    /// Return null block in case of error.
    ArchiveBlock OpenBlock(const char* name, unsigned sizeHint, bool safe, ArchiveBlockType type)
    {
        BeginBlock(name, sizeHint, safe, type);
        return ArchiveBlock{*this, sizeHint};
    }

    /// Open block helpers
    /// @{

    /// Open Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSequentialBlock(const char* name) { return OpenBlock(name, 0, false, ArchiveBlockType::Sequential); }
    /// Open Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenUnorderedBlock(const char* name) { return OpenBlock(name, 0, false, ArchiveBlockType::Unordered); }
    /// Open Array block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenArrayBlock(const char* name, unsigned sizeHint = 0) { return OpenBlock(name, sizeHint, false, ArchiveBlockType::Array); }

    /// Open safe Sequential block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSafeSequentialBlock(const char* name) { return OpenBlock(name, 0, true, ArchiveBlockType::Sequential); }
    /// Open safe Unordered block. Will be automatically closed when returned object is destroyed.
    ArchiveBlock OpenSafeUnorderedBlock(const char* name) { return OpenBlock(name, 0, true, ArchiveBlockType::Unordered); }

    /// @}
};

}
