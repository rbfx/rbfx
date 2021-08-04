//
// Copyright (c) 2021-2021 the rbfx project.
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

#include "ChunkStreamDeserializer.h"

#include "Log.h"
#include "EASTL/algorithm.h"

namespace Urho3D
{


ChunkStreamDeserializer::ChunkStreamDeserializer(Deserializer& deserializer) :
    deserializer_(deserializer),
    readBufferOffset_(0),
    readBufferSize_(0),
    offset_(deserializer.GetPosition())
{
    size_ = deserializer.GetSize() - offset_;
    position_ = 0;
}

unsigned ChunkStreamDeserializer::Read(void* dest, unsigned size)
{
    if (!size)
        return 0;

    unsigned sizeLeft = size;
    auto* destPtr = static_cast<unsigned char*>(dest);
    unsigned readCount = 0;
    while (sizeLeft)
    {
        if (!readBuffer_ || readBufferOffset_ >= readBufferSize_)
        {
            if (deserializer_.IsEof())
            {
                return readCount;
            }
            unsigned short unpackedSize = deserializer_.ReadUShort();
            unsigned short packedSize = deserializer_.ReadUShort();
            const auto chankStartPos = deserializer_.GetPosition();
            if (GetLastPosition() < chankStartPos)
            {
                Add(Chunk{ chankStartPos, packedSize, position_, unpackedSize });
            }

            if (!ReadBlock(deserializer_, unpackedSize, packedSize))
                return readCount;
        }

        const unsigned copySize = Min((readBufferSize_ - readBufferOffset_), sizeLeft);
        memcpy(destPtr, readBuffer_.get() + readBufferOffset_, copySize);
        destPtr += copySize;
        sizeLeft -= copySize;
        readBufferOffset_ += copySize;
        position_ += copySize;
        readCount += copySize;
    }
    return readCount;
}

/// Set position from the beginning of the stream. Return actual new position.
unsigned ChunkStreamDeserializer::Seek(unsigned position)
{
    if (position == position_)
        return position;

    // Start over from the beginning
    if (position == 0)
    {
        position_ = 0;
        readBufferOffset_ = 0;
        readBufferSize_ = 0;
        if (deserializer_.Seek(offset_) != offset_)
            return -1;
        return position;
    }

    // If we know where to seek
    auto* chunk = FindChunk(position);
    if (chunk)
    {
        if (deserializer_.Seek(chunk->position_) != chunk->position_)
            return -1;
        if (!ReadBlock(deserializer_, chunk->decodedSize_, chunk->size_))
            return -1;
        readBufferSize_ = chunk->decodedSize_;
        readBufferOffset_ = position - chunk->decodedPosition_;
        return position;
    }

    // Seek beyond last known chunk.
    chunk = GetLastChunk();
    if (!chunk)
    {
        position_ = 0;
        readBufferOffset_ = 0;
        readBufferSize_ = 0;
        if (deserializer_.Seek(offset_) != offset_)
        {
            return -1;
        }
    }
    else
    {
        position_ = chunk->decodedPosition_ + chunk->decodedSize_;
        if (deserializer_.Seek(chunk->position_ + chunk->size_) != chunk->position_ + chunk->size_)
        {
            return -1;
        }
    }

    // Read chunks until we reach end of file or find a chunk where data belongs to.
    for (;;)
    {
        if (deserializer_.IsEof())
        {
            if (position_ == position)
                return position;
            return -1;
        }
        unsigned short unpackedSize = deserializer_.ReadUShort();
        unsigned short packedSize = deserializer_.ReadUShort();
        const auto chunkStartPos = deserializer_.GetPosition();
        Chunk newChunk{ chunkStartPos, packedSize, position_, unpackedSize };
        Add(newChunk);

        // Check if we reached destination.
        if (position < position_ + unpackedSize)
        {
            if (!ReadBlock(deserializer_, unpackedSize, packedSize))
                return -1;
            readBufferSize_ = unpackedSize;
            readBufferOffset_ = position - newChunk.decodedPosition_;
            return position;
        }

        position_ += unpackedSize;
        if (deserializer_.Seek(newChunk.position_ + newChunk.size_) != newChunk.position_ + newChunk.size_)
        {
            return -1;
        }

    }

    return position_;
}

/// Register new chunk. All chunk should be sequential in position.
bool ChunkStreamDeserializer::Add(const Chunk& chunk)
{
    int index = chunks_.size();
    if (index > 0)
    {
        if (chunks_[index - 1].position_ >= chunk.position_ ||
            chunks_[index - 1].decodedPosition_ >= chunk.decodedPosition_)
        {
            return false;
        }
    }
    chunks_.push_back(chunk);
    return true;
}

/// Find chunk by stream position.
/// Returns nullptr if matching chunk is not visited yet.
const ChunkStreamDeserializer::Chunk* ChunkStreamDeserializer::FindChunk(unsigned decodedPosition) const
{
    // Early out for an empty collection.
    if (chunks_.empty())
        return nullptr;

    // Use binary search to find the destination.
    Chunk value;
    value.decodedPosition_ = decodedPosition;
    auto candidate = ea::upper_bound(chunks_.begin(), chunks_.end(), value, [](const Chunk& a, const Chunk& b) { return a.decodedPosition_ < b.decodedPosition_; });

    if (candidate == chunks_.begin())
    {
        URHO3D_LOGERROR("Seek position is before first known chunk. This should never happen!");
        return nullptr;
    }

    --candidate;
    // If it is also within the last chunk then we found it.
    if (decodedPosition < (*candidate).decodedPosition_ + (*candidate).decodedSize_)
        return &*candidate;
    return nullptr;
}

ChunkStreamSerializer::ChunkStreamSerializer(Serializer& serializer, unsigned short chunkSize)
    : serializer_(serializer)
    , chunkSize_(chunkSize)
    , inputBufferPosition_(0)

{
}

/// Write bytes to the stream. Return number of bytes actually written.
unsigned ChunkStreamSerializer::Write(const void* data, unsigned size)
{
    unsigned char* const inputBuffer = GetInputBuffer(chunkSize_);
    unsigned written = 0;
    while (written != size)
    {
        const unsigned dataSize = std::min(size- written, (chunkSize_ - inputBufferPosition_));
        memcpy(inputBuffer + inputBufferPosition_,  static_cast<const unsigned char*>(data) + written, dataSize);
        inputBufferPosition_ += dataSize;
        if (written + dataSize != size)
        {
            if (!Flush())
            {
                return written;
            }
        }
        written += dataSize;
    }
    return written;
}

bool ChunkStreamSerializer::Flush()
{
    if (inputBufferPosition_ > 0)
    {
        if (!FlushImpl(serializer_, inputBufferPosition_))
            return false;
        inputBufferPosition_ = 0;
    }
    return true;
}

}
