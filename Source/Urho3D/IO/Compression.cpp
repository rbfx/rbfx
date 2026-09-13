// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

bool CompressStream(Serializer& dest, Deserializer& src, unsigned compressionLevel)
{
    unsigned srcSize = src.GetSize() - src.GetPosition();
    // Prepend the source and dest. data size in the stream so that we know to buffer & uncompress the right amount
    if (!srcSize)
    {
        dest.WriteUInt(0);
        dest.WriteUInt(0);
        return true;
    }

    auto maxDestSize = (unsigned)LZ4_compressBound(srcSize);
    ea::shared_array<unsigned char> srcBuffer(new unsigned char[srcSize]);
    ea::shared_array<unsigned char> destBuffer(new unsigned char[maxDestSize]);

    if (src.Read(srcBuffer.get(), srcSize) != srcSize)
        return false;

    auto destSize = (unsigned)LZ4_compress_HC((const char*)srcBuffer.get(), (char*)destBuffer.get(), srcSize, LZ4_compressBound(srcSize), compressionLevel);
    bool success = true;
    success &= dest.WriteUInt(srcSize);
    success &= dest.WriteUInt(destSize);
    success &= dest.Write(destBuffer.get(), destSize) == destSize;
    return success;
}

bool DecompressStream(Serializer& dest, Deserializer& src)
{
    if (src.IsEof())
        return false;

    unsigned destSize = src.ReadUInt();
    unsigned srcSize = src.ReadUInt();
    if (!srcSize || !destSize)
        return true; // No data

    if (srcSize > src.GetSize())
        return false; // Illegal source (packed data) size reported, possibly not valid data

    ea::shared_array<unsigned char> srcBuffer(new unsigned char[srcSize]);
    ea::shared_array<unsigned char> destBuffer(new unsigned char[destSize]);

    if (src.Read(srcBuffer.get(), srcSize) != srcSize)
        return false;

    LZ4_decompress_fast((const char*)srcBuffer.get(), (char*)destBuffer.get(), destSize);
    return dest.Write(destBuffer.get(), destSize) == destSize;
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

}
