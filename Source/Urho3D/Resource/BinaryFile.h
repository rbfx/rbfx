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

/// \file

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
