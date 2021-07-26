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

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/ChunkStreamDeserializer.h>

namespace Urho3D
{
class Serializer;
class VectorBuffer;

struct EncryptionKey
{
    EncryptionKey(const unsigned char* key, unsigned size);
    EncryptionKey(const ea::vector<unsigned char>& key);
    EncryptionKey(const ea::string& base64key);

    ea::string ToString() const;

    unsigned char key_[32];
};

/// Generate random symmetric encryption key.
URHO3D_API EncryptionKey GenerateSymmetricEncryptionKey();

/// Estimate and return worst case LZ4 compressed output size in bytes for given input size.
URHO3D_API unsigned EstimateEncryptBound(unsigned srcSize);
/// Encrypt data using the LZ4 algorithm and return the compressed data size. The needed destination buffer worst-case size is given by EstimateEncryptBound().
URHO3D_API unsigned EncryptData(void* dest, const void* src, unsigned srcSize, const EncryptionKey& key);
/// Uncompress data using the LZ4 algorithm. The uncompressed data size must be known. Return the number of compressed data bytes consumed.
URHO3D_API unsigned DecryptData(void* dest, const void* src, unsigned srcSize, const EncryptionKey& key);
/// Encrypt a source stream (from current position to the end) to the destination stream using the LZ4 algorithm. Return true on success.
URHO3D_API bool EncryptStream(Serializer& dest, Deserializer& src, unsigned short maxBlockSize = 32768);
/// Decrypt a compressed source stream produced using EncryptStream() to the destination stream. Return true on success.
URHO3D_API bool DecryptStream(Serializer& dest, Deserializer& src);
/// Encrypt a VectorBuffer using the LZ4 algorithm and return the compressed result buffer.
URHO3D_API VectorBuffer EncryptVectorBuffer(VectorBuffer& src);
/// Decrypt a VectorBuffer produced using EncryptVectorBuffer().
URHO3D_API VectorBuffer DecryptVectorBuffer(VectorBuffer& src);


class URHO3D_API EncryptedStreamDeserializer : public ChunkStreamDeserializer
{
public:
    /// Construct Deserializer.
    EncryptedStreamDeserializer(Deserializer& deserializer, const EncryptionKey& key);
protected:
    /// Read block from underlying deserizalizer.
    bool ReadBlock(Deserializer& deserializer, unsigned short unpackedSize, unsigned short packedSize) override;
private:
    /// input buffer.
    ea::shared_array<unsigned char> inputBuffer_;
    /// Bytes in the current input buffer.
    unsigned inputBufferSize_;
    /// Encryption key.
    EncryptionKey key_;
    /// Read buffer capacity.
    unsigned readBufferCapacity_;
};

class URHO3D_API EncryptedStreamSerializer : public ChunkStreamSerializer
{
public:
    /// Construct Serializer.
    EncryptedStreamSerializer(Serializer& serializer, const EncryptionKey& key, unsigned short chunkSize);
protected:
    unsigned char* GetInputBuffer(unsigned chunkSize) override;

    bool FlushImpl(Serializer& serializer, unsigned inputBufferSize) override;
private:
    /// input buffer.
    ea::shared_array<unsigned char> inputBuffer_;
    /// Compressed data.
    ea::shared_array<unsigned char> compressedBuffer_;
    /// Bytes in the current input buffer.
    unsigned inputBufferSize_;
    /// Encryption key.
    EncryptionKey key_;
};

}
