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

#include "Encription.h"

#include "Urho3D/Core/StringUtils.h"

extern "C"
{
#include <tweetnacl/tweetnacl.h>

}

namespace Urho3D
{
    static_assert(sizeof(EncryptionKey) == crypto_box_BEFORENMBYTES, "EncryptionKey size should match crypto_box_BEFORENMBYTES");

    EncryptionKey::EncryptionKey(const unsigned char* key, unsigned size)
    {
        int i = 0;
        for (; i < size && i < sizeof(key_); ++i)
        {
            key_[i] = key[i];
        }
        for (; i < sizeof(key_); ++i)
        {
            key_[i] = 0;
        }
    }

    EncryptionKey::EncryptionKey(const ea::vector<unsigned char>& key)
        : EncryptionKey(key.data(), key.size())
    {
    }


    EncryptionKey::EncryptionKey(const ea::string& base64key)
        : EncryptionKey(DecodeBase64(base64key))
    {
    }

    ea::string EncryptionKey::ToString() const
    {
        return EncodeBase64(ea::vector<unsigned char>(key_, key_ + sizeof(key_)));
    }

    static_assert(sizeof(EncryptionNonce) == crypto_box_NONCEBYTES, "EncryptionKey size should match crypto_box_NONCEBYTES");

    const EncryptionNonce EncryptionNonce::ZERO;

    EncryptionNonce::EncryptionNonce()
    {
        memset(nonce_, 0, sizeof nonce_);
    }

    EncryptionNonce::EncryptionNonce(const unsigned char* nonce, unsigned size)
    {
        int i = 0;
        for (; i < size && i < sizeof(nonce_); ++i)
        {
            nonce_[i] = nonce[i];
        }
        for (; i < sizeof(nonce_); ++i)
        {
            nonce_[i] = 0;
        }
    }

    EncryptionNonce::EncryptionNonce(const ea::vector<unsigned char>& nonce)
        : EncryptionNonce(nonce.data(), nonce.size())
    {
    }


    EncryptionNonce::EncryptionNonce(const ea::string& base64key)
        : EncryptionNonce(DecodeBase64(base64key))
    {
    }

    ea::string EncryptionNonce::ToString() const
    {
        return EncodeBase64(ea::vector<unsigned char>(nonce_, nonce_ + sizeof(nonce_)));
    }

    EncryptionKey GenerateSymmetricEncryptionKey()
    {
        unsigned char k[crypto_box_BEFORENMBYTES];
        unsigned char publickey[crypto_box_PUBLICKEYBYTES];
        unsigned char secretkey[crypto_box_SECRETKEYBYTES];
        crypto_box_keypair(publickey, secretkey);
        crypto_box_beforenm(k, publickey, secretkey);
        return EncryptionKey(k, crypto_box_BEFORENMBYTES);
    }

    unsigned EstimateEncryptBound(unsigned srcSize)
    {
        return srcSize + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES;
    }

    unsigned EncryptData(void* dest, const void* src, unsigned srcSize, const EncryptionKey& key, const EncryptionNonce& nonce)
    {
        if (!dest || !src || !srcSize)
            return 0;
        const ea::unique_ptr<unsigned char> src_padded(new unsigned char[srcSize + crypto_box_ZEROBYTES]);
        const ea::unique_ptr<unsigned char> dst_padded(new unsigned char[srcSize + crypto_box_ZEROBYTES]);
        memset(src_padded.get(), 0, crypto_box_ZEROBYTES);
        memcpy(src_padded.get() + crypto_box_ZEROBYTES, src, srcSize);

        if (0 != crypto_box_afternm(dst_padded.get(), src_padded.get(), srcSize + crypto_box_ZEROBYTES, nonce.nonce_, key.key_))
            return 0;

        for (int i = 0; i < crypto_box_BOXZEROBYTES; ++i)
            if (dst_padded.get()[i] != 0)
                return 0;
        const unsigned dstSize = srcSize + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES;
        memcpy(dest, dst_padded.get() + crypto_box_BOXZEROBYTES, dstSize);
        return dstSize;
    }

    unsigned DecryptData(void* dest, const void* src, unsigned srcSize, const EncryptionKey& key, const EncryptionNonce& nonce)
    {
        if (!dest || !src || !srcSize)
            return 0;
        const unsigned dstSize = srcSize + crypto_box_BOXZEROBYTES - crypto_box_ZEROBYTES;
        const ea::unique_ptr<unsigned char> src_padded(new unsigned char[dstSize + crypto_box_ZEROBYTES]);
        const ea::unique_ptr<unsigned char> dst_padded(new unsigned char[dstSize + crypto_box_ZEROBYTES]);
        memset(src_padded.get(), 0, crypto_box_BOXZEROBYTES);
        memcpy(src_padded.get() + crypto_box_BOXZEROBYTES, src, srcSize);
        if (0 != crypto_box_open_afternm(dst_padded.get(), src_padded.get(), dstSize + crypto_box_ZEROBYTES, nonce.nonce_, key.key_))
            return 0;
        for (int i = 0; i < crypto_box_ZEROBYTES; ++i)
            if (dst_padded.get()[i] != 0)
                return 0;
        memcpy(dest, dst_padded.get() + crypto_box_ZEROBYTES, dstSize);
        return dstSize;
    }

    EncryptedStreamDeserializer::EncryptedStreamDeserializer(Deserializer& deserializer, const EncryptionKey& key, const EncryptionNonce& nonce):
        ChunkStreamDeserializer(deserializer),
        inputBufferSize_(0),
        readBufferCapacity_(0),
        key_(key),
        nonce_(nonce)
    {
    }

    bool EncryptedStreamDeserializer::ReadBlock(Deserializer& deserializer_, unsigned short unpackedSize, unsigned short packedSize)
    {
        if (unpackedSize != packedSize - crypto_box_BOXZEROBYTES)
            return false;
        const unsigned recommendedBufferSize = unpackedSize + crypto_box_ZEROBYTES;
        if (!readBuffer_ || readBufferCapacity_ < recommendedBufferSize)
        {
            readBuffer_ = new unsigned char[recommendedBufferSize];
            readBufferCapacity_ = recommendedBufferSize;
        }
        if (!inputBuffer_ || inputBufferSize_ < recommendedBufferSize)
        {
            inputBuffer_ = new unsigned char[recommendedBufferSize];
            inputBufferSize_ = recommendedBufferSize;
        }

        memset(inputBuffer_.get(), 0, crypto_box_BOXZEROBYTES);
        if (deserializer_.Read(inputBuffer_.get() + crypto_box_BOXZEROBYTES, packedSize) != packedSize)
        {
            return false;
        }

        if (0 != crypto_box_open_afternm(readBuffer_.get(), inputBuffer_.get(), recommendedBufferSize, nonce_.nonce_, key_.key_))
            return false;

        readBufferOffset_ = crypto_box_ZEROBYTES;
        readBufferSize_ = recommendedBufferSize;
        return true;
    }

    EncryptedStreamSerializer::EncryptedStreamSerializer(Serializer& serializer, const EncryptionKey& key, const EncryptionNonce& nonce, unsigned short chunkSize) :
        ChunkStreamSerializer(serializer, chunkSize),
        inputBufferSize_(0),
        key_(key),
        nonce_(nonce)
    {
    }

    unsigned char* EncryptedStreamSerializer::GetInputBuffer(unsigned chunkSize)
    {
        if (!inputBuffer_)
        {
            inputBuffer_ = new unsigned char[chunkSize + crypto_box_ZEROBYTES];
            compressedBuffer_ = new unsigned char[chunkSize + crypto_box_ZEROBYTES];
            memset(inputBuffer_.get(), 0, crypto_box_ZEROBYTES);
        }
        return inputBuffer_.get() + crypto_box_ZEROBYTES;
    }

    bool EncryptedStreamSerializer::FlushImpl(Serializer& serializer, unsigned unpackedSize)
    {
        if (0 != crypto_box_afternm(compressedBuffer_.get(), inputBuffer_.get(), unpackedSize + crypto_box_ZEROBYTES, nonce_.nonce_, key_.key_))
            return false;

        const unsigned short packedSize = unpackedSize + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES;
        serializer.WriteUShort(static_cast<unsigned short>(unpackedSize));
        serializer.WriteUShort(packedSize);
        serializer.Write(compressedBuffer_.get() + crypto_box_BOXZEROBYTES, packedSize);

        return true;
    }

}
