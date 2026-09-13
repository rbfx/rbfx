// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/Serializer.h"
#include "Urho3D/IO/Deserializer.h"

namespace Urho3D
{
/// File open mode.
enum FileMode
{
    FILE_READ = 0,
    FILE_WRITE,
    FILE_READWRITE
};

/// A common root class for objects that implement both Serializer and Deserializer.
class URHO3D_API AbstractFile : public Deserializer, public Serializer
{
public:
    /// Construct.
    AbstractFile() : Deserializer() { }
    /// Construct.
    explicit AbstractFile(unsigned int size) : Deserializer(size) { }
    /// Destruct.
    ~AbstractFile() override = default;
    /// Change the file name. Used by the resource system.
    /// @property
    virtual void SetName(const ea::string& name) { name_ = name; }
    /// Return whether is open.
    /// @property
    virtual bool IsOpen() const { return true; }
    /// Return absolute file name in file system.
    /// @property
    virtual const ea::string& GetAbsoluteName() const { return name_; }
    /// Close the file.
    virtual void Close() {}

#ifndef SWIG
    // A workaround for SWIG failing to generate bindings because both IAbstractFile and IDeserializer provide GetName() method. This is
    // fine because IAbstractFile inherits GetName() from IDeserializer anyway.

    /// Return the file name.
    const ea::string& GetName() const override { return name_; }
#endif

protected:
    /// File name.
    ea::string name_;
};

using AbstractFilePtr = SharedPtr<AbstractFile, RefCounted>;

}
