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

#include "../Precompiled.h"

#include <EASTL/shared_array.h>

#include "../IO/Compression.h"
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../IO/VectorBuffer.h"

#include <LZ4/lz4.h>
#include <LZ4/lz4hc.h>

namespace Urho3D
{

unsigned EstimateCompressBound(unsigned srcSize)
{
    return (unsigned)LZ4_compressBound(srcSize);
}

unsigned CompressData(void* dest, const void* src, unsigned srcSize)
{
    if (!dest || !src || !srcSize)
        return 0;
    else
        return (unsigned)LZ4_compress_HC((const char*)src, (char*)dest, srcSize, LZ4_compressBound(srcSize), 0);
}

unsigned DecompressData(void* dest, const void* src, unsigned destSize)
{
    if (!dest || !src || !destSize)
        return 0;
    else
        return (unsigned)LZ4_decompress_fast((const char*)src, (char*)dest, destSize);
}

bool CompressStream(Serializer& dest, Deserializer& src, unsigned short maxBlockSize)
{
    auto maxDestSize = (unsigned)LZ4_compressBound(maxBlockSize);
    const ea::shared_array<unsigned char> srcBuffer(new unsigned char[maxBlockSize]);
    const ea::shared_array<unsigned char> destBuffer(new unsigned char[maxDestSize]);

    unsigned pos = 0;
    bool success = true;
    unsigned dataSize = src.GetSize() - src.GetPosition();
    while (pos < dataSize)
    {
        const unsigned blockSize = std::min(static_cast<unsigned>(maxBlockSize), dataSize - pos);
        const unsigned unpackedSize = src.Read(srcBuffer.get(), blockSize);
        if (unpackedSize != blockSize)
            return false;

        auto packedSize = static_cast<unsigned>(LZ4_compress_HC(reinterpret_cast<const char*>(srcBuffer.get()),
                                                                reinterpret_cast<char*>(destBuffer.get()),
                                                                unpackedSize,
                                                                maxDestSize, 0));
        if (!packedSize)
        {
            return false;
        }

        success &= dest.WriteUShort(static_cast<unsigned short>(unpackedSize));
        success &= dest.WriteUShort(static_cast<unsigned short>(packedSize));
        success &= (dest.Write(destBuffer.get(), packedSize) == packedSize);

        pos += unpackedSize;
    }

    return success;
}

bool DecompressStream(Serializer& dest, Deserializer& src)
{
    size_t srcBufferSize = 0;
    size_t dstBufferSize = 0;
    ea::shared_array<unsigned char> srcBuffer;
    ea::shared_array<unsigned char> destBuffer;

    while (!src.IsEof())
    {
        const unsigned destSize = src.ReadUShort();
        const unsigned srcSize = src.ReadUShort();
        if (!srcSize || !destSize)
            return true; // No data

        if (srcSize > src.GetSize())
            return false; // Illegal source (packed data) size reported, possibly not valid data

        if (srcBufferSize < srcSize)
        {
            srcBufferSize = srcSize;
            srcBuffer = new unsigned char[srcBufferSize];
        }
        if (dstBufferSize < destSize)
        {
            dstBufferSize = destSize;
            destBuffer = new unsigned char[dstBufferSize];
        }

        if (src.Read(srcBuffer.get(), srcSize) != srcSize)
            return false;

        if (LZ4_decompress_fast(reinterpret_cast<const char*>(srcBuffer.get()), reinterpret_cast<char*>(destBuffer.get()), destSize) < 0)
            return false;
        if (dest.Write(destBuffer.get(), destSize) != destSize)
            return false;
    }
    return true;
}

VectorBuffer CompressVectorBuffer(VectorBuffer& src)
{
    VectorBuffer ret;
    src.Seek(0);
    CompressStream(ret, src);
    ret.Seek(0);
    return ret;
}

VectorBuffer DecompressVectorBuffer(VectorBuffer& src)
{
    VectorBuffer ret;
    src.Seek(0);
    DecompressStream(ret, src);
    ret.Seek(0);
    return ret;
}

CompressedStreamDeserializer::CompressedStreamDeserializer(Deserializer& deserializer):
    ChunkStreamDeserializer(deserializer),
    inputBufferSize_(0),
    readBufferCapacity_(0)
{
}

bool CompressedStreamDeserializer::ReadBlock(Deserializer& deserializer_, unsigned short unpackedSize, unsigned short packedSize)
{
    if (!readBuffer_ || readBufferCapacity_ < unpackedSize)
    {
        readBuffer_ = new unsigned char[unpackedSize];
        readBufferCapacity_ = unpackedSize;
    }
    if (!inputBuffer_ || inputBufferSize_ < packedSize)
    {
        const unsigned recommendedInputBufferSize = std::max<unsigned>(packedSize, LZ4_compressBound(unpackedSize));
        inputBuffer_ = new unsigned char[recommendedInputBufferSize];
        inputBufferSize_ = recommendedInputBufferSize;
    }

    if (deserializer_.Read(inputBuffer_.get(), packedSize) != packedSize)
    {
        return false;
    }

    if (LZ4_decompress_fast(reinterpret_cast<const char*>(inputBuffer_.get()), reinterpret_cast<char*>(readBuffer_.get()), unpackedSize) != packedSize)
    {
        return false;
    }
    readBufferSize_ = unpackedSize;
    readBufferOffset_ = 0;
    return true;
}

CompressedStreamSerializer::CompressedStreamSerializer(Serializer& serializer, unsigned short chunkSize):
    ChunkStreamSerializer(serializer, chunkSize),
    inputBufferSize_(0)
{
}

unsigned char* CompressedStreamSerializer::GetInputBuffer(unsigned chunkSize)
{
    if (!inputBuffer_)
    {
        inputBuffer_ = new unsigned char[chunkSize];
        compressedBuffer_ = new unsigned char[LZ4_compressBound(chunkSize)];
    }
    return inputBuffer_.get();
}

bool CompressedStreamSerializer::FlushImpl(Serializer& serializer, unsigned unpackedSize)
{
    auto packedSize = (unsigned)LZ4_compress_HC((const char*)inputBuffer_.get(), (char*)compressedBuffer_.get(), unpackedSize, LZ4_compressBound(unpackedSize), 0);
    if (!packedSize)
        return false;

    serializer.WriteUShort((unsigned short)unpackedSize);
    serializer.WriteUShort((unsigned short)packedSize);
    serializer.Write(compressedBuffer_.get(), packedSize);
    
    return true;
}

}
