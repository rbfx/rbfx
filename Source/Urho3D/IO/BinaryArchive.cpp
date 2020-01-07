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

#include <cassert>

namespace Urho3D
{

BinaryOutputArchiveBlock::BinaryOutputArchiveBlock(const char* name, ArchiveBlockType type, Serializer* parentSerializer, bool safe)
    : name_(name)
    , type_(type)
    , parentSerializer_(parentSerializer)
{
    if (safe)
        checkedData_ = ea::make_unique<VectorBuffer>();
}

bool BinaryOutputArchiveBlock::Close(ArchiveBase& archive)
{
    if (!checkedData_)
    {
        assert(nesting_ == 0);
        return true;
    }

    const unsigned size = checkedData_->GetSize();
    if (parentSerializer_->WriteVLE(size))
    {
        if (parentSerializer_->Write(checkedData_->GetData(), size) == size)
            return true;
    }

    archive.SetErrorFormatted(ArchiveBase::errorUnspecifiedFailure_elementName, ArchiveBase::blockElementName_);
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
    : BinaryArchiveBase<BinaryOutputArchiveBlock, false>(context)
    , serializer_(&serializer)
{
}

bool BinaryOutputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

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
            GetCurrentBlock().OpenNestedBlock();
        }
    }

    if (type == ArchiveBlockType::Array || type == ArchiveBlockType::Map)
    {
        if (!currentBlockSerializer_->WriteVLE(sizeHint))
        {
            SetErrorFormatted(ArchiveBase::errorUnspecifiedFailure_elementName, ArchiveBase::blockElementName_);
            EndBlock();
            return false;
        }
    }

    return true;
}

bool BinaryOutputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetErrorFormatted(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    if (GetCurrentBlock().CloseNestedBlock())
        return true;

    const bool blockClosed = GetCurrentBlock().Close(*this);

    stack_.pop_back();
    if (!stack_.empty())
        currentBlockSerializer_ = GetCurrentBlock().GetSerializer();
    else
    {
        CloseArchive();
        currentBlockSerializer_ = nullptr;
    }

    return blockClosed;
}

bool BinaryOutputArchive::SerializeKey(ea::string& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    return CheckElementWrite(currentBlockSerializer_->WriteString(key), ArchiveBase::keyElementName_);
}

bool BinaryOutputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    return CheckElementWrite(currentBlockSerializer_->WriteUInt(key), ArchiveBase::keyElementName_);
}

bool BinaryOutputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (!CheckEOFAndRoot(name))
        return false;

    return CheckElementWrite(currentBlockSerializer_->Write(bytes, size) == size, name);
}

bool BinaryOutputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (!CheckEOFAndRoot(name))
        return false;

    return CheckElementWrite(currentBlockSerializer_->WriteVLE(value), name);
}

bool BinaryOutputArchive::CheckEOF(const char* elementName)
{
    if (HasError())
        return false;

    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetErrorFormatted(ArchiveBase::errorEOF_elementName, elementName);
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
        SetErrorFormatted(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

bool BinaryOutputArchive::CheckElementWrite(bool result, const char* elementName)
{
    if (result)
        return true;

    SetErrorFormatted(ArchiveBase::errorUnspecifiedFailure_elementName, elementName);
    return false;
}

// Generate serialization implementation (binary output)
#define URHO3D_BINARY_OUT_IMPL(type, function) \
    bool BinaryOutputArchive::Serialize(const char* name, type& value) \
    { \
        if (!CheckEOFAndRoot(name)) \
            return false; \
        return CheckElementWrite(currentBlockSerializer_->function(value), name); \
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
    : name_(name)
    , type_(type)
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

bool BinaryInputArchive::BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type)
{
    if (!CheckEOF(name))
        return false;

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
            GetCurrentBlock().OpenNestedBlock();
        }
    }

    if (type == ArchiveBlockType::Array || type == ArchiveBlockType::Map)
    {
        sizeHint = deserializer_->ReadVLE();
        if (deserializer_->IsEof() && sizeHint != 0)
        {
            SetErrorFormatted(ArchiveBase::errorUnspecifiedFailure_elementName, ArchiveBase::blockElementName_);
            EndBlock();
            return false;
        }
    }

    return true;
}

bool BinaryInputArchive::EndBlock()
{
    if (stack_.empty())
    {
        SetErrorFormatted(ArchiveBase::fatalUnexpectedEndBlock);
        return false;
    }

    if (GetCurrentBlock().CloseNestedBlock())
        return true;

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

    key = deserializer_->ReadString();
    return CheckElementRead(true, ArchiveBase::keyElementName_);
}

bool BinaryInputArchive::SerializeKey(unsigned& key)
{
    if (!CheckEOFAndRoot(ArchiveBase::keyElementName_))
        return false;

    key = deserializer_->ReadUInt();
    return CheckElementRead(true, ArchiveBase::keyElementName_);
}

bool BinaryInputArchive::SerializeBytes(const char* name, void* bytes, unsigned size)
{
    if (!CheckEOFAndRoot(name))
        return false;

    return CheckElementRead(deserializer_->Read(bytes, size) == size, name);
}

bool BinaryInputArchive::SerializeVLE(const char* name, unsigned& value)
{
    if (!CheckEOFAndRoot(name))
        return false;

    value = deserializer_->ReadVLE();
    return CheckElementRead(true, name);
}

bool BinaryInputArchive::CheckEOF(const char* elementName)
{
    if (HasError())
        return false;

    if (IsEOF())
    {
        const ea::string_view blockName = !stack_.empty() ? GetCurrentBlock().GetName() : "";
        SetErrorFormatted(ArchiveBase::errorEOF_elementName, elementName);
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
        SetErrorFormatted(ArchiveBase::fatalRootBlockNotOpened_elementName, elementName);
        assert(0);
        return false;
    }

    return true;
}

bool BinaryInputArchive::CheckElementRead(bool result, const char* elementName)
{
    if (!result || deserializer_->GetPosition() > GetCurrentBlock().GetNextElementPosition())
    {
        SetErrorFormatted(ArchiveBase::errorUnspecifiedFailure_elementName, elementName);
        return false;
    }
    return true;
}

// Generate serialization implementation (binary input)
#define URHO3D_BINARY_IN_IMPL(type, function) \
    bool BinaryInputArchive::Serialize(const char* name, type& value) \
    { \
        if (!CheckEOFAndRoot(name)) \
            return false; \
        value = deserializer_->function(); \
        return CheckElementRead(true, name); \
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
