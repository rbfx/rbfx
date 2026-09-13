// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../IO/ArchiveBase.h"
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../IO/VectorBuffer.h"

namespace Urho3D
{

/// Base archive for binary serialization.
template <class BlockType, bool IsInputBool>
class BinaryArchiveBase : public ArchiveBaseT<BlockType, IsInputBool, false>
{
protected:
    using ArchiveBaseT<BlockType, IsInputBool, false>::ArchiveBaseT;

    void CheckResult(bool result, const char* elementName) const
    {
        if (!result)
            throw this->IOFailureException(elementName);
    }
};

/// Binary output archive block.
class URHO3D_API BinaryOutputArchiveBlock : public ArchiveBlockBase
{
public:
    BinaryOutputArchiveBlock(const char* name, ArchiveBlockType type, Serializer* parentSerializer, bool safe);
    Serializer* GetSerializer();

    bool IsUnorderedAccessSupported() const { return false; }
    bool HasElementOrBlock(const char* name) const { return false; }
    void Close(ArchiveBase& archive);

private:
    /// @name For safe blocks only
    /// @{
    ea::unique_ptr<VectorBuffer> outputBuffer_;
    Serializer* parentSerializer_{};
    /// @}
};

/// Binary output archive.
class URHO3D_API BinaryOutputArchive : public BinaryArchiveBase<BinaryOutputArchiveBlock, false>
{
    friend class BinaryOutputArchiveBlock;

public:
    BinaryOutputArchive(Context* context, Serializer& serializer);

    /// @name Archive implementation
    /// @{
    ea::string_view GetName() const final;
    unsigned GetChecksum() final;

    void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;
    void EndBlock() noexcept final;

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
    Serializer* serializer_{};
    /// Serializer used within currently open block.
    Serializer* currentBlockSerializer_{};
};

/// Binary input archive block.
class URHO3D_API BinaryInputArchiveBlock : public ArchiveBlockBase
{
public:
    BinaryInputArchiveBlock(
        const char* name, ArchiveBlockType type, Deserializer* deserializer, bool safe, unsigned nextElementPosition);
    unsigned GetNextElementPosition() const { return nextElementPosition_; }

    bool IsUnorderedAccessSupported() const { return false; }
    bool HasElementOrBlock(const char* name) const { return false; }
    void Close(ArchiveBase& archive);

private:
    Deserializer* deserializer_{};
    /// Whether the block is safe.
    bool safe_{};

    /// @name For safe blocks only
    /// @{
    unsigned blockOffset_{};
    unsigned blockSize_{};
    unsigned nextElementPosition_{};
    /// @}
};

/// Binary input archive.
class URHO3D_API BinaryInputArchive : public BinaryArchiveBase<BinaryInputArchiveBlock, true>
{
public:
    BinaryInputArchive(Context* context, Deserializer& deserializer);

    /// @name Archive implementation
    /// @{
    ea::string_view GetName() const final { return deserializer_->GetName(); }
    unsigned GetChecksum() final { return deserializer_->GetChecksum(); }

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
    /// Deserializer.
    Deserializer* deserializer_{};

};

}
