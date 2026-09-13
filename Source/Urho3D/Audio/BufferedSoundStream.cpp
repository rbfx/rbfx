// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Audio/BufferedSoundStream.h"

#include "../DebugNew.h"

namespace Urho3D
{

BufferedSoundStream::BufferedSoundStream() :
    position_(0)
{
}

BufferedSoundStream::~BufferedSoundStream() = default;

unsigned BufferedSoundStream::GetData(signed char* dest, unsigned numBytes)
{
    MutexLock lock(bufferMutex_);

    unsigned outBytes = 0;

    while (numBytes && buffers_.size())
    {
        // Copy as much from the front buffer as possible, then discard it and move to the next
        auto front = buffers_.begin();

        unsigned copySize = front->second - position_;
        if (copySize > numBytes)
            copySize = numBytes;

        memcpy(dest, front->first.get() + position_, copySize);
        position_ += copySize;
        if (position_ >= front->second)
        {
            buffers_.pop_front();
            position_ = 0;
        }

        dest += copySize;
        outBytes += copySize;
        numBytes -= copySize;
    }

    return outBytes;
}

void BufferedSoundStream::AddData(void* data, unsigned numBytes)
{
    if (data && numBytes)
    {
        MutexLock lock(bufferMutex_);

        ea::shared_array<signed char> newBuffer(new signed char[numBytes]);
        memcpy(newBuffer.get(), data, numBytes);
        buffers_.push_back(ea::make_pair(newBuffer, numBytes));
    }
}

void BufferedSoundStream::AddData(const ea::shared_array<signed char>& data, unsigned numBytes)
{
    if (data && numBytes)
    {
        MutexLock lock(bufferMutex_);

        buffers_.push_back(ea::make_pair(data, numBytes));
    }
}

void BufferedSoundStream::AddData(const ea::shared_array<signed short>& data, unsigned numBytes)
{
    if (data && numBytes)
    {
        MutexLock lock(bufferMutex_);

        buffers_.push_back(ea::make_pair(ea::do_reinterpret_cast<signed char>(data), numBytes));
    }
}

void BufferedSoundStream::Clear()
{
    MutexLock lock(bufferMutex_);

    buffers_.clear();
    position_ = 0;
}

unsigned BufferedSoundStream::GetBufferNumBytes() const
{
    MutexLock lock(bufferMutex_);

    unsigned ret = 0;
    for (const auto& buffer : buffers_)
        ret += buffer.second;
    // Subtract amount of sound data played from the front buffer
    ret -= position_;

    return ret;
}

float BufferedSoundStream::GetBufferLength() const
{
    return (float)GetBufferNumBytes() / (GetFrequency() * (float)GetSampleSize());
}

}
