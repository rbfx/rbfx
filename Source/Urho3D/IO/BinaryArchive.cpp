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

#include "../IO/BinaryArchive.h"
#include "../Core/Assert.h"

#include <cassert>

namespace Urho3D
{

namespace
{

constexpr const char* blockGuardName = "<block guard>";

}

BinaryOutputArchiveBlock::BinaryOutputArchiveBlock(const char* name, ArchiveBlockType type, Serializer* parentSerializer, bool safe)
    : ArchiveBlockBase(name, type)
    , parentSerializer_(parentSerializer)
{
    if (safe)
        outputBuffer_ = ea::make_unique<VectorBuffer>();
}

void BinaryOutputArchiveBlock::Close(ArchiveBase& archive)
{
    if (!outputBuffer_)
    {
        URHO3D_ASSERT(!HasOpenInlineBlock());
        return;
    }

    const unsigned size = outputBuffer_->GetSize();
    if (parentSerializer_->WriteVLE(size))
    {
        if (parentSerializer_->Write(outputBuffer_->GetData(), size) == size)
            return;
    }

    throw archive.IOFailureException(blockGuardName);
}

Serializer* BinaryOutputArchiveBlock::GetSerializer()
{
    if (outputBuffer_)
        return outputBuffer_.get();
    else
        return parentSerializer_;
}

BinaryOutputArchive::BinaryOutputArchive(Context* context, Serializer& serializer)
    : BinaryArchiveBase<BinaryOutputArchiveBlock, false>(context)
    , serializer_(&serializer)
{
}

ea::string_view BinaryOutputArchive::GetName() const
{
    if (Deserializer* deserializer = dynamic_cast<Deserializer*>(serializer_))
        return deserializer->GetName();
    return {};
}

unsigned BinaryOutputArchive::GetChecksum()
{
    if (Deserializer* deserializer = dynamic_cast<Deserializer*>(serializer_))
        return deserializer->GetChecksum();
    return 0;
}

void BinaryOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);

    if (stack_.empty())
    {
        Block block{ name, type, serializer_, safe };
        stack_.push_back(ea::move(block));
        currentBlockSerializer_ = GetCurrentBlock().GetSerializer();
    }
    else
    {
        if (safe)
        {
            Block block{ name, type, GetCurrentBlock().GetSerializer(), safe };
            stack_.push_back(ea::move(block));
            currentBlockSerializer_ = GetCurrentBlock().GetSerializer();
        }
        else
        {
            GetCurrentBlock().OpenInlineBlock();
        }
    }

    if (type == ArchiveBlockType::Array)
    {
        if (!currentBlockSerializer_->WriteVLE(sizeHint))
        {
            EndBlock();
            throw IOFailureException(blockGuardName);
        }
    }
}

void BinaryOutputArchive::EndBlock() noexcept
{
    ArchiveBaseT::EndBlock();
    currentBlockSerializer_ = stack_.empty() ? nullptr : GetCurrentBlock().GetSerializer();
}

void BinaryOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    CheckBeforeElement(name);
    CheckResult(currentBlockSerializer_->Write(bytes, size) == size, name);
}

void BinaryOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    CheckBeforeElement(name);
    CheckResult(currentBlockSerializer_->WriteVLE(value), name);
}

// Generate serialization implementation (binary output)
#define URHO3D_BINARY_OUT_IMPL(type, function) \
    void BinaryOutputArchive::Serialize(const char* name, type& value) \
    { \
        CheckBeforeElement(name); \
        CheckResult(currentBlockSerializer_->function(value), name); \
    }

URHO3D_BINARY_OUT_IMPL(bool, WriteBool);
URHO3D_BINARY_OUT_IMPL(signed char, WriteByte);
URHO3D_BINARY_OUT_IMPL(unsigned char, WriteUByte);
URHO3D_BINARY_OUT_IMPL(short, WriteShort);
URHO3D_BINARY_OUT_IMPL(unsigned short, WriteUShort);
URHO3D_BINARY_OUT_IMPL(int, WriteInt);
URHO3D_BINARY_OUT_IMPL(unsigned int, WriteUInt);
URHO3D_BINARY_OUT_IMPL(long long, WriteInt64);
URHO3D_BINARY_OUT_IMPL(unsigned long long, WriteUInt64);
URHO3D_BINARY_OUT_IMPL(float, WriteFloat);
URHO3D_BINARY_OUT_IMPL(double, WriteDouble);
URHO3D_BINARY_OUT_IMPL(ea::string, WriteString);

#undef URHO3D_BINARY_OUT_IMPL

BinaryInputArchiveBlock::BinaryInputArchiveBlock(const char* name, ArchiveBlockType type,
    Deserializer* deserializer, bool safe, unsigned nextElementPosition)
    : ArchiveBlockBase(name, type)
    , deserializer_(deserializer)
    , safe_(safe)
    , nextElementPosition_(nextElementPosition)
{
    if (safe_)
    {
        blockSize_ = deserializer_->ReadVLE();
        blockOffset_ = deserializer_->GetPosition();
        nextElementPosition_ = ea::min(blockOffset_ + blockSize_, deserializer_->GetSize());
    }
}

void BinaryInputArchiveBlock::Close(ArchiveBase& archive)
{
    if (safe_)
    {
        assert(nextElementPosition_ != M_MAX_UNSIGNED);
        const unsigned currentPosition = deserializer_->GetPosition();
        if (nextElementPosition_ != currentPosition)
            deserializer_->Seek(nextElementPosition_);
    }
}

BinaryInputArchive::BinaryInputArchive(Context* context, Deserializer& deserializer)
    : BinaryArchiveBase<BinaryInputArchiveBlock, true>(context)
    , deserializer_(&deserializer)
{
}

void BinaryInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    CheckBeforeBlock(name);

    if (stack_.empty())
    {
        Block frame{ name, type, deserializer_, safe, M_MAX_UNSIGNED };
        stack_.push_back(frame);
    }
    else
    {
        if (safe)
        {
            Block blockFrame{ name, type, deserializer_, safe, GetCurrentBlock().GetNextElementPosition() };
            stack_.push_back(blockFrame);
        }
        else
        {
            GetCurrentBlock().OpenInlineBlock();
        }
    }

    if (type == ArchiveBlockType::Array)
    {
        sizeHint = deserializer_->ReadVLE();
        if (deserializer_->IsEof() && sizeHint != 0)
        {
            EndBlock();
            throw IOFailureException(blockGuardName);
        }
    }
}

void BinaryInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    CheckBeforeElement(name);
    CheckResult(deserializer_->Read(bytes, size) == size, name);
}

void BinaryInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    CheckBeforeElement(name);
    value = deserializer_->ReadVLE();
    CheckResult(true, name);
}

// Generate serialization implementation (binary input)
#define URHO3D_BINARY_IN_IMPL(type, function) \
    void BinaryInputArchive::Serialize(const char* name, type& value) \
    { \
        CheckBeforeElement(name); \
        value = deserializer_->function(); \
        CheckResult(true, name); \
    }

URHO3D_BINARY_IN_IMPL(bool, ReadBool);
URHO3D_BINARY_IN_IMPL(signed char, ReadByte);
URHO3D_BINARY_IN_IMPL(unsigned char, ReadUByte);
URHO3D_BINARY_IN_IMPL(short, ReadShort);
URHO3D_BINARY_IN_IMPL(unsigned short, ReadUShort);
URHO3D_BINARY_IN_IMPL(int, ReadInt);
URHO3D_BINARY_IN_IMPL(unsigned int, ReadUInt);
URHO3D_BINARY_IN_IMPL(long long, ReadInt64);
URHO3D_BINARY_IN_IMPL(unsigned long long, ReadUInt64);
URHO3D_BINARY_IN_IMPL(float, ReadFloat);
URHO3D_BINARY_IN_IMPL(double, ReadDouble);
URHO3D_BINARY_IN_IMPL(ea::string, ReadString);

#undef URHO3D_BINARY_IN_IMPL

}
