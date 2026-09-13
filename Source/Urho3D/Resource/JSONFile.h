// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    template <class T, class ... Args> bool SaveObject(const char* name, const T& object, Args &&... args);
    template <class T, class ... Args> bool LoadObject(const char* name, T& object, Args &&... args) const;
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

template <class T, class ... Args>
bool JSONFile::SaveObject(const char* name, const T& object, Args &&... args)
{
    return SaveObjectCallback([&](Archive& archive)
    {
        SerializeValue(archive, name, const_cast<T&>(object), ea::forward<Args>(args)...);
    });
}

template <class T, class ... Args>
bool JSONFile::LoadObject(const char* name, T& object, Args &&... args) const
{
    return LoadObjectCallback([&](Archive& archive)
    {
        SerializeValue(archive, name, object, ea::forward<Args>(args)...);
    });
}

}
