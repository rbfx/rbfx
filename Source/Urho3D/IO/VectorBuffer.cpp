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

#include "../Precompiled.h"

#include "../IO/VectorBuffer.h"
#include "VectorBuffer.h"


namespace Urho3D
{

static ea::string vectorBufferName{"VectorBuffer"};

VectorBuffer::VectorBuffer() = default;

VectorBuffer::VectorBuffer(const ByteVector& data)
{
    SetData(data);
}

VectorBuffer::VectorBuffer(const void* data, unsigned size)
{
    SetData(data, size);
}

VectorBuffer::VectorBuffer(Deserializer& source, unsigned size)
{
    SetData(source, size);
}

unsigned VectorBuffer::Read(void* dest, unsigned size)
{
    if (size + position_ > size_)
        size = size_ - position_;
    if (!size)
        return 0;

    unsigned char* srcPtr = &buffer_[position_];
    auto* destPtr = (unsigned char*)dest;
    position_ += size;

    memcpy(destPtr, srcPtr, size);

    return size;
}

unsigned VectorBuffer::Seek(unsigned position)
{
    if (position > size_)
        position = size_;

    position_ = position;
    return position_;
}

unsigned VectorBuffer::Write(const void* data, unsigned size)
{
    if (!size)
        return 0;

    if (size + position_ > size_)
    {
        size_ = size + position_;
        buffer_.resize(size_);
    }

    auto* srcPtr = (unsigned char*)data;
    unsigned char* destPtr = &buffer_[position_];
    position_ += size;

    memcpy(destPtr, srcPtr, size);

    return size;
}

void VectorBuffer::SetData(const ea::vector<unsigned char>& data)
{
    buffer_ = data;
    position_ = 0;
    size_ = data.size();
}

void VectorBuffer::SetData(const void* data, unsigned size)
{
    if (!data)
        size = 0;

    buffer_.resize(size);
    if (size)
        memcpy(&buffer_[0], data, size);

    position_ = 0;
    size_ = size;
}

void VectorBuffer::SetData(Deserializer& source, unsigned size)
{
    buffer_.resize(size);
    unsigned actualSize = source.Read(buffer_.data(), size);
    if (actualSize != size)
        buffer_.resize(actualSize);

    position_ = 0;
    size_ = actualSize;
}

void VectorBuffer::Clear()
{
    buffer_.clear();
    position_ = 0;
    size_ = 0;
}

void VectorBuffer::Resize(unsigned size)
{
    buffer_.resize(size);
    size_ = size;
    if (position_ > size_)
        position_ = size_;
}

const ea::string& VectorBuffer::GetName() const
{
    return vectorBufferName;
}

}
