//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Resource/Resource.h"
#include "../Resource/JSONValue.h"

#include <EASTL/functional.h>

namespace Urho3D
{

/// JSON document resource.
class URHO3D_API JSONFile : public Resource
{
    URHO3D_OBJECT(JSONFile, Resource);

public:
    /// Construct.
    explicit JSONFile(Context* context);
    /// Destruct.
    ~JSONFile() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource with default indentation (one tab). Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save resource with user-defined indentation, only the first character (if any) of the string is used and the length of the string defines the character count. Return true if successful.
    bool Save(Serializer& dest, const ea::string& indendation) const;

    /// Save/load objects using Archive serialization.
    /// @{
    bool SaveObjectCallback(const ea::function<void(Archive&)> serializeValue);
    bool LoadObjectCallback(const ea::function<void(Archive&)> serializeValue) const;
    template <class T> bool SaveObject(const char* name, const T& object);
    template <class T> bool LoadObject(const char* name, T& object) const;
    bool SaveObject(const Object& object) { return SaveObject(object.GetTypeName().c_str(), object); }
    bool LoadObject(Object& object) const { return LoadObject(object.GetTypeName().c_str(), object); }
    /// @}

    /// Deserialize from a string. Return true if successful.
    bool FromString(const ea::string& source);
    /// Save to a string.
    ea::string ToString(const ea::string& indendation = "\t") const;

    /// Return root value.
    /// @property
    JSONValue& GetRoot() { return root_; }
    /// Return root value.
    const JSONValue& GetRoot() const { return root_; }

    /// Return true if parsing json string into JSONValue succeeds.
    static bool ParseJSON(const ea::string& json, JSONValue& value, bool reportError = true);

private:
    /// JSON root value.
    JSONValue root_;
};

template <class T>
bool JSONFile::SaveObject(const char* name, const T& object)
{
    return SaveObjectCallback([&](Archive& archive) { SerializeValue(archive, name, const_cast<T&>(object)); });
}

template <class T>
bool JSONFile::LoadObject(const char* name, T& object) const
{
    return LoadObjectCallback([&](Archive& archive) { SerializeValue(archive, name, object); });
}

}
