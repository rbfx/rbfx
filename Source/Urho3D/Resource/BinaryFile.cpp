// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/BinaryArchive.h"
#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/Serializer.h"
#include "Urho3D/Resource/BinaryFile.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

BinaryFile::BinaryFile(Context* context) :
    Resource(context)
{
}

BinaryFile::~BinaryFile() = default;

void BinaryFile::RegisterObject(Context* context)
{
    context->AddFactoryReflection<BinaryFile>();
}

bool BinaryFile::BeginLoad(Deserializer& source)
{
    source.Seek(0);
    buffer_.SetData(source, source.GetSize());
    SetMemoryUse(buffer_.GetBuffer().capacity());
    return true;
}

bool BinaryFile::Save(Serializer& dest) const
{
    if (dest.Write(buffer_.GetData(), buffer_.GetSize()) != buffer_.GetSize())
    {
        URHO3D_LOGERROR("Can not save binary file" + GetName());
        return false;
    }

    return true;
}

bool BinaryFile::SaveObjectCallback(const ea::function<void(Archive&)> serializeValue)
{
    try
    {
        buffer_.Clear();
        BinaryOutputArchive archive{GetContext(), AsSerializer()};
        serializeValue(archive);
        return true;
    }
    catch (const ArchiveException& e)
    {
        buffer_.Clear();
        URHO3D_LOGERROR("Failed to save object to binary: {}", e.what());
        return false;
    }
}

bool BinaryFile::LoadObjectCallback(const ea::function<void(Archive&)> serializeValue) const
{
    try
    {
        MemoryBuffer readBuffer{buffer_.GetBuffer()};
        BinaryInputArchive archive{GetContext(), readBuffer};
        serializeValue(archive);
        return true;
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Failed to load object from binary: {}", e.what());
        return false;
    }
}

void BinaryFile::Clear()
{
    buffer_.Clear();
}

void BinaryFile::SetData(const ByteVector& data)
{
    buffer_.SetData(data);
    SetMemoryUse(buffer_.GetBuffer().capacity());
}

void BinaryFile::SetText(ea::string_view text)
{
    buffer_.SetData(text.data(), static_cast<unsigned>(text.length()));
    SetMemoryUse(static_cast<unsigned>(buffer_.GetBuffer().capacity()));
}

const ByteVector& BinaryFile::GetData() const
{
    return buffer_.GetBuffer();
}

ea::string_view BinaryFile::GetText() const
{
    const unsigned char* data = buffer_.GetData();
    const unsigned size = buffer_.GetSize();
    return {reinterpret_cast<const char*>(data), size};
}

StringVector BinaryFile::ReadLines() const
{
    StringVector result;
    MemoryBuffer readBuffer{buffer_.GetBuffer()};
    while (!readBuffer.IsEof())
        result.push_back(readBuffer.ReadLine());
    return result;
}

}
