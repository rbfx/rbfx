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

#include "../Container/ByteVector.h"
#include "../IO/AbstractFile.h"

namespace Urho3D
{

/// Dynamically sized buffer that can be read and written to as a stream.
class URHO3D_API VectorBuffer : public AbstractFile
{
public:
    /// Construct an empty buffer.
    VectorBuffer();
    /// Construct from another buffer.
    explicit VectorBuffer(const ByteVector& data);
    /// Construct from a memory area.
    VectorBuffer(const void* data, unsigned size);
    /// Construct from a stream.
    VectorBuffer(Deserializer& source, unsigned size);

    /// Returns name of this object.
    const ea::string& GetName() const override;

    /// Read bytes from the buffer. Return number of bytes actually read.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the buffer. Return actual new position.
    unsigned Seek(unsigned position) override;
    /// Write bytes to the buffer. Return number of bytes actually written.
    unsigned Write(const void* data, unsigned size) override;

    /// Set data from another buffer.
    void SetData(const ByteVector& data);
    /// Set data from a memory area.
    void SetData(const void* data, unsigned size);
    /// Set data from a stream.
    void SetData(Deserializer& source, unsigned size);
    /// Reset to zero size.
    void Clear();
    /// Set size.
    void Resize(unsigned size);

    /// Return data.
    const unsigned char* GetData() const { return size_ ? &buffer_[0] : nullptr; }

    /// Return non-const data.
    unsigned char* GetModifiableData() { return size_ ? &buffer_[0] : nullptr; }

    /// Return the buffer.
    const ByteVector& GetBuffer() const { return buffer_; }
    /// Return the buffer.
    ByteVector& GetBuffer() { return buffer_; }

private:
    /// Dynamic data buffer.
    ByteVector buffer_;
};

}
