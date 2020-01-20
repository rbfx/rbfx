//
// Copyright (c) 2020-2020 the rbfx project.
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
#include "../Resource/Resource.h"

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
    /// Save resource to a file.
    bool SaveFile(const ea::string& fileName) const override;

    /// Set data.
    void SetData(const ByteVector& data) { data_ = data; }
    /// Return immutable data.
    const ByteVector& GetData() const { return data_; }
    /// Return mutable data.
    ByteVector& GetMutableData() { return data_; }

private:
    /// Data.
    ByteVector data_;
};

}
