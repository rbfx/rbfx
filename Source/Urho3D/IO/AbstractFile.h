//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../IO/Serializer.h"
#include "../IO/Deserializer.h"

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
