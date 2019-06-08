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

#include "../IO/BinaryArchive.h"

#include <cassert>

namespace Urho3D
{

BinaryOutputArchiveBlock::BinaryOutputArchiveBlock(const char* name, ArchiveBlockType type, Serializer* parentSerializer, bool checked, unsigned sizeHint)
    : name_(name)
    , type_(type)
    , parentSerializer_(parentSerializer)
{
    if (checked)
        checkedData_ = ea::make_shared<VectorBuffer>();

    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
        expectedElementCount_ = sizeHint;
}

bool BinaryOutputArchiveBlock::Open(ArchiveBase& archive)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
    {
        if (!parentSerializer_->WriteVLE(expectedElementCount_))
        {
            archive.SetError(ArchiveBase::errorWriteEOF_blockName_elementName, name_, ArchiveBase::blockElementName_);
            return false;
        }
    }

    return true;
}

Serializer* BinaryOutputArchiveBlock::CreateElementKey(ArchiveBase& archive)
{
    if (type_ != ArchiveBlockType::Map)
    {
        archive.SetError(ArchiveBase::fatalUnexpectedKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (keySet_)
    {
        archive.SetError(ArchiveBase::fatalDuplicateKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    keySet_ = true;
    return GetSerializer();
}

Serializer* BinaryOutputArchiveBlock::CreateElement(ArchiveBase& archive, const char* elementName)
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ >= expectedElementCount_)
    {
        archive.SetError(ArchiveBase::fatalBlockOverflow_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Map && !keySet_)
    {
        archive.SetError(ArchiveBase::fatalMissingKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetError(ArchiveBase::fatalMissingElementName_blockName, name_);
        assert(0);
        return nullptr;
    }

    ++numElements_;
    keySet_ = false;
    return GetSerializer();
}

bool BinaryOutputArchiveBlock::Close(ArchiveBase& archive)
{
    if (expectedElementCount_ != M_MAX_UNSIGNED && numElements_ != expectedElementCount_)
    {
        if (numElements_ < expectedElementCount_)
            archive.SetError(ArchiveBase::fatalBlockUnderflow_blockName, name_);
        else
            archive.SetError(ArchiveBase::fatalBlockOverflow_blockName, name_);
        assert(0);
        return false;
    }

    if (!checkedData_)
        return true;

    const unsigned size = checkedData_->GetSize();
    if (parentSerializer_->WriteVLE(size))
    {
        if (parentSerializer_->Write(checkedData_->GetData(), size) == size)
            return true;
    }

    archive.SetError(ArchiveBase::errorCannotWriteData_blockName_elementName, name_, ArchiveBase::checkedBlockGuardElementName_);
    return false;

}

Serializer* BinaryOutputArchiveBlock::GetSerializer()
{
    if (checkedData_)
        return checkedData_.get();
    else
        return parentSerializer_;
}

BinaryOutputArchive::BinaryOutputArchive(Context* context, Serializer& serializer)
    : BinaryArchiveBase<BinaryOutputArchiveBlock>(context)
    , serializer_(serializer)
{
}

bool BinaryOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        Block block{ name, type, &serializer_, true, sizeHint };
        if (block.Open(*this))
        {
            stack_.push_back(ea::move(block));
            return true;
        }
    }

    if (Serializer* parentSerializer = GetCurrentBlock().CreateElement(*this, name))
    {
        Block block{ name, type, parentSerializer, true, sizeHint };
        if (block.Open(*this))
        {
            stack_.push_back(ea::move(block));
            return true;
        }
    }

    return false;
}

bool BinaryOutputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    const bool blockClosed = GetCurrentBlock().Close(*this);

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();

    return blockClosed;
}

bool BinaryOutputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Serializer* serializer = GetCurrentBlock().CreateElementKey(*this))
        return CheckElementWrite(serializer->WriteString(key), ArchiveBase::keyElementName_);

    return false;
}

bool BinaryOutputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Serializer* serializer = GetCurrentBlock().CreateElementKey(*this))
        return CheckElementWrite(serializer->WriteUInt(key), ArchiveBase::keyElementName_);

    return false;
}

bool BinaryOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Serializer* serializer = GetCurrentBlock().CreateElement(*this, name))
        return CheckElementWrite(serializer->Write(bytes, size) == size, name);

    return false;
}

bool BinaryOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Serializer* serializer = GetCurrentBlock().CreateElement(*this, name))
        return CheckElementWrite(serializer->WriteVLE(value), name);

    return false;
}

bool BinaryOutputArchive::CheckEOF(const char* elementName)
{
    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetError(ArchiveBase::errorReadEOF_blockName_elementName, blockName, elementName);
        return false;
    }
    return true;
}

bool BinaryOutputArchive::CheckEOFAndRoot(const char* elementName)
{
    if (!CheckEOF(elementName))
        return false;

    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

bool BinaryOutputArchive::CheckElementWrite(bool result, const char* elementName)
{
    if (result)
        return true;

    SetError(ArchiveBase::errorCannotWriteData_blockName_elementName, GetCurrentBlock().GetName(), elementName);
    return false;
}

// Generate serialization implementation (binary output)
#define URHO3D_BINARY_OUT_IMPL(type, function) \
    bool BinaryOutputArchive::Serialize(const char* name, type& value) \
    { \
        if (!CheckEOFAndRoot(ArchiveBase::keyElementName_)) \
            return false; \
        if (Serializer* serializer = GetCurrentBlock().CreateElement(*this, name)) \
            return CheckElementWrite(serializer->function(value), name); \
        return false; \
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
    Deserializer* deserializer, bool checked, unsigned nextElementPosition)
    : name_(name)
    , type_(type)
    , deserializer_(deserializer)
    , checked_(checked)
    , nextElementPosition_(nextElementPosition)
{
}

void BinaryInputArchiveBlock::Open(ArchiveBase& archive)
{
    if (type_ == ArchiveBlockType::Map || type_ == ArchiveBlockType::Array)
    {
        numElements_ = deserializer_->ReadVLE();
    }

    if (checked_)
    {
        blockSize_ = deserializer_->ReadVLE();
        blockOffset_ = deserializer_->GetPosition();
        nextElementPosition_ = blockOffset_ + blockSize_;
    }
}

Deserializer* BinaryInputArchiveBlock::ReadElementKey(ArchiveBase& archive)
{
    if (deserializer_->GetPosition() > nextElementPosition_)
    {
        archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, ArchiveBase::keyElementName_);
        return nullptr;
    }

    if (type_ != ArchiveBlockType::Map)
    {
        archive.SetError(ArchiveBase::fatalUnexpectedKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (keyRead_)
    {
        archive.SetError(ArchiveBase::fatalDuplicateKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Array || type_ == ArchiveBlockType::Map)
    {
        if (numElementsRead_ >= numElements_)
        {
            archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, ArchiveBase::keyElementName_);
            return nullptr;
        }
    }

    keyRead_ = true;
    return deserializer_;
}

Deserializer* BinaryInputArchiveBlock::ReadElement(ArchiveBase& archive, const char* elementName)
{
    if (deserializer_->GetPosition() > nextElementPosition_)
    {
        archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, elementName);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Array || type_ == ArchiveBlockType::Map)
    {
        if (numElementsRead_ >= numElements_)
        {
            archive.SetError(ArchiveBase::errorElementNotFound_blockName_elementName, name_, ArchiveBase::keyElementName_);
            return nullptr;
        }
    }

    if (type_ == ArchiveBlockType::Unordered && !elementName)
    {
        archive.SetError(ArchiveBase::fatalMissingElementName_blockName, name_);
        assert(0);
        return nullptr;
    }

    if (type_ == ArchiveBlockType::Map && !keyRead_)
    {
        archive.SetError(ArchiveBase::fatalMissingKeySerialization_blockName, name_);
        assert(0);
        return nullptr;
    }

    keyRead_ = false;
    ++numElementsRead_;
    return deserializer_;
}

void BinaryInputArchiveBlock::Close(ArchiveBase& archive)
{
    if (checked_)
    {
        assert(nextElementPosition_ != M_MAX_UNSIGNED);
        const unsigned currentPosition = deserializer_->GetPosition();
        if (nextElementPosition_ != currentPosition)
            deserializer_->Seek(nextElementPosition_);
    }
}

BinaryInputArchive::BinaryInputArchive(Context* context, Deserializer& deserializer)
    : BinaryArchiveBase<BinaryInputArchiveBlock>(context)
    , deserializer_(deserializer)
{
}

bool BinaryInputArchive::BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

    // Open root block
    if (stack_.empty())
    {
        Block frame{ name, type, &deserializer_, true, M_MAX_UNSIGNED };
        frame.Open(*this);
        sizeHint = frame.GetSizeHint();
        stack_.push_back(frame);
        return true;
    }

    // Try open block
    if (Deserializer* deserializer = GetCurrentBlock().ReadElement(*this, name))
    {
        Block blockFrame{ name, type, deserializer, true, GetCurrentBlock().GetNextElementPosition() };
        blockFrame.Open(*this);
        sizeHint = blockFrame.GetSizeHint();
        stack_.push_back(blockFrame);
        return true;
    }

    return false;
}

bool BinaryInputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    GetCurrentBlock().Close(*this);

    stack_.pop_back();
    if (stack_.empty())
        CloseArchive();
    return true;
}

bool BinaryInputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Deserializer* deserializer = GetCurrentBlock().ReadElementKey(*this))
    {
        key = deserializer->ReadString();
        return true;
    }

    return false;
}

bool BinaryInputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Deserializer* deserializer = GetCurrentBlock().ReadElementKey(*this))
    {
        key = deserializer->ReadUInt();
        return true;
    }

    return false;
}

bool BinaryInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Deserializer* deserializer = GetCurrentBlock().ReadElementKey(*this))
        return CheckElementRead(deserializer->Read(bytes, size) == size, name);

    return false;
}

bool BinaryInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    if (Deserializer* deserializer = GetCurrentBlock().ReadElementKey(*this))
    {
        value = deserializer->ReadVLE();
        return true;
    }

    return false;
}

bool BinaryInputArchive::CheckEOF(const char* elementName)
{
    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetError(ArchiveBase::errorReadEOF_blockName_elementName, blockName, elementName);
        return false;
    }
    return true;
}

bool BinaryInputArchive::CheckEOFAndRoot(const char* elementName)
{
    if (!CheckEOF(elementName))
        return false;

    if (stack_.empty())
    {
        SetError(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

bool BinaryInputArchive::CheckElementRead(bool result, const char* elementName)
{
    if (result)
        return true;

    SetError(ArchiveBase::errorCannotWriteData_blockName_elementName, GetCurrentBlock().GetName(), elementName);
    return false;
}

// Generate serialization implementation (binary input)
#define URHO3D_BINARY_IN_IMPL(type, function) \
    bool BinaryInputArchive::Serialize(const char* name, type& value) \
    { \
        if (!CheckEOFAndRoot(ArchiveBase::keyElementName_)) \
            return false; \
        if (Deserializer* deserializer = GetCurrentBlock().ReadElement(*this, name)) \
        { \
            value = deserializer->function(); \
            return true; \
        } \
        return false; \
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
