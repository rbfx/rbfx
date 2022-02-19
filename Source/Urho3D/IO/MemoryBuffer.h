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

/// Memory area that can be read and written to as a stream.
/// @nobind
class URHO3D_API MemoryBuffer : public AbstractFile
{
public:
    /// Construct with a pointer and size.
    MemoryBuffer(void* data, unsigned size);
    /// Construct as read-only with a pointer and size.
    MemoryBuffer(const void* data, unsigned size);
    /// Construct as read-only from string.
    explicit MemoryBuffer(ea::string_view text);
    /// Construct from a vector, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(ByteVector& data);
    /// Construct from a read-only vector, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(const ByteVector& data);

    /// Read bytes from the memory area. Return number of bytes actually read.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the memory area. Return actual new position.
    unsigned Seek(unsigned position) override;
    /// Write bytes to the memory area.
    unsigned Write(const void* data, unsigned size) override;

    /// Return memory area.
    unsigned char* GetData() { return buffer_; }

    /// Return whether buffer is read-only.
    bool IsReadOnly() { return readOnly_; }

private:
    /// Pointer to the memory area.
    unsigned char* buffer_;
    /// Read-only flag.
    bool readOnly_;
};

}
