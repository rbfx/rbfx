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
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../IO/VectorBuffer.h"

namespace Urho3D
{

/// Base archive for binary serialization.
template <class T, bool IsInputBool>
class BinaryArchiveBase : public ArchiveBaseT<IsInputBool, false>
{
public:
    /// Construct.
    explicit BinaryArchiveBase(Context* context)
        : context_(context)
    {}

    /// Get context.
    Context* GetContext() final { return context_; }

    /// Whether the unordered element access is supported for Unordered blocks.
    bool IsUnorderedSupportedNow() const final { return false; }

    /// Return current string stack.
    ea::string GetCurrentStackString() final
    {
        ea::string result;
        for (const Block& block : stack_)
        {
            if (!result.empty())
                result += "/";
            result += ea::string{ block.GetName() };
            if (block.HasNestedBlocks())
                result += "/?";
        }
        return result;
    }

protected:
    /// Block type.
    using Block = T;

    /// Get current block.
    Block& GetCurrentBlock() { return stack_.back(); }

    /// Context.
    Context* context_{};
    /// Blocks stack.
    ea::vector<Block> stack_;
};

/// XML output archive block. Internal.
class BinaryOutputArchiveBlock
{
public:
    /// Construct.
    BinaryOutputArchiveBlock(const char* name, ArchiveBlockType type, Serializer* parentSerializer, bool safe);
    /// Get block name.
    ea::string_view GetName() const { return name_; }
    /// Close block.
    bool Close(ArchiveBase& archive);

    /// Get serializer.
    Serializer* GetSerializer();
    /// Open nested block.
    void OpenNestedBlock() { ++nesting_; }
    /// Close nested block.
    bool CloseNestedBlock()
    {
        if (nesting_ > 0)
        {
            --nesting_;
            return true;
        }
        return false;
    }
    /// Whether the block has nested blocks.
    bool HasNestedBlocks() const { return nesting_ > 0; }

private:
    /// Block name.
    ea::string_view name_;
    /// Block type.
    ArchiveBlockType type_{};
    /// Block checked data (safe blocks only).
    ea::unique_ptr<VectorBuffer> checkedData_;
    /// Depth of nesting.
    unsigned nesting_{};

    /// Parent serializer object.
    Serializer* parentSerializer_{};
};

/// XML output archive.
class URHO3D_API BinaryOutputArchive : public BinaryArchiveBase<BinaryOutputArchiveBlock, false>
{
public:
    /// Construct.
    BinaryOutputArchive(Context* context, Serializer& serializer);

    /// Return name of the archive.
    ea::string_view GetName() const final
    {
        if (Deserializer* deserializer = dynamic_cast<Deserializer*>(serializer_))
            return deserializer->GetName();
        return {};
    }
    /// Return a checksum if applicable.
    unsigned GetChecksum() final
    {
        if (Deserializer* deserializer = dynamic_cast<Deserializer*>(serializer_))
            return deserializer->GetChecksum();
        return 0;
    }

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
    bool CheckEOF(const char* elementName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName);
    /// Check result of the action.
    bool CheckElementWrite(bool result, const char* elementName);

    /// Serializer.
    Serializer* serializer_{};
    /// Current block serializer.
    Serializer* currentBlockSerializer_{};
};

/// XML input archive block. Internal.
class BinaryInputArchiveBlock
{
public:
    /// Construct valid.
    BinaryInputArchiveBlock(const char* name, ArchiveBlockType type, Deserializer* deserializer, bool safe, unsigned nextElementPosition);
    /// Return name.
    const ea::string_view GetName() const { return name_; }
    /// Return next element position.
    unsigned GetNextElementPosition() const { return nextElementPosition_; }
    /// Open block.
    bool Open(ArchiveBase& archive);
    /// Close block.
    void Close(ArchiveBase& archive);

    /// Open nested block.
    void OpenNestedBlock() { ++nesting_; }
    /// Close nested block.
    bool CloseNestedBlock()
    {
        if (nesting_ > 0)
        {
            --nesting_;
            return true;
        }
        return false;
    }
    /// Whether the block has nested blocks.
    bool HasNestedBlocks() const { return nesting_ > 0; }

private:
    /// Block name.
    ea::string_view name_;
    /// Block type.
    ArchiveBlockType type_{};

    /// Deserializer.
    Deserializer* deserializer_{};
    /// Whether the block is safe.
    bool safe_{};
    /// Block nesting.
    unsigned nesting_{};

    /// Block offset.
    unsigned blockOffset_{};
    /// Block size.
    unsigned blockSize_{};
    /// Next element position.
    unsigned nextElementPosition_{};
};

/// XML input archive.
class URHO3D_API BinaryInputArchive : public BinaryArchiveBase<BinaryInputArchiveBlock, true>
{
public:
    /// Construct.
    BinaryInputArchive(Context* context, Deserializer& deserializer);

    /// Return name of the archive.
    ea::string_view GetName() const final
    {
        return deserializer_->GetName();
    }
    /// Return a checksum if applicable.
    unsigned GetChecksum() final
    {
        return deserializer_->GetChecksum();
    }

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
    bool CheckEOF(const char* elementName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName);
    /// Check element read.
    bool CheckElementRead(bool result, const char* elementName);

    /// Deserializer.
    Deserializer* deserializer_{};

};

}
