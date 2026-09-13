// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Container/ByteVector.h"
#include "../IO/Archive.h"
#include "../IO/VectorBuffer.h"
#include "../Resource/Resource.h"

#include <EASTL/functional.h>

namespace Urho3D
{

/// Resource for generic binary file.
class URHO3D_API BinaryFile : public Resource
{
    URHO3D_OBJECT(BinaryFile, Resource);

public:
    /// Construct empty.
    explicit BinaryFile(Context* context);
    /// Destruct.
    ~BinaryFile() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource to a stream.
    bool Save(Serializer& dest) const override;

    /// Save/load objects using Archive serialization.
    /// @{
    bool SaveObjectCallback(const ea::function<void(Archive&)> serializeValue);
    bool LoadObjectCallback(const ea::function<void(Archive&)> serializeValue) const;
    template <class T, class ... Args> bool SaveObject(const char* name, const T& object, Args &&... args);
    template <class T, class ... Args> bool LoadObject(const char* name, T& object, Args &&... args) const;
    bool SaveObject(const Object& object) { return SaveObject(object.GetTypeName().c_str(), object); }
    bool LoadObject(Object& object) const { return LoadObject(object.GetTypeName().c_str(), object); }
    /// @}

    /// Clear data.
    void Clear();
    /// Set data.
    void SetData(const ByteVector& data);
    /// Set data from text.
    void SetText(ea::string_view text);
    /// Return immutable data.
    const ByteVector& GetData() const;
    /// Return immutable data as string view.
    ea::string_view GetText() const;
    /// Return data as text lines.
    StringVector ReadLines() const;

    /// Return mutable internal buffer.
    VectorBuffer& GetMutableBuffer() { return buffer_; }
    /// Cast to Serializer.
    Serializer& AsSerializer() { return buffer_; }
    /// Cast to Deserializer.
    Deserializer& AsDeserializer() { return buffer_; }

private:
    VectorBuffer buffer_;
};

template <class T, class ... Args>
bool BinaryFile::SaveObject(const char* name, const T& object, Args &&... args)
{
    return SaveObjectCallback([&](Archive& archive)
    {
        SerializeValue(archive, name, const_cast<T&>(object), ea::forward<Args>(args)...);
    });
}

template <class T, class ... Args>
bool BinaryFile::LoadObject(const char* name, T& object, Args &&... args) const
{
    return LoadObjectCallback([&](Archive& archive)
    {
        SerializeValue(archive, name, object, ea::forward<Args>(args)...);
    });
}

}
