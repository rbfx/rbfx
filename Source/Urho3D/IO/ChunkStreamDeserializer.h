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
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/Serializer.h>

#include <EASTL/shared_array.h>

namespace Urho3D
{
    class URHO3D_API ChunkStreamDeserializer : public Deserializer
    {
    private:
        struct Chunk
        {
            /// Position in encoded stream.
            unsigned position_;
            /// Encoded chunk size;
            unsigned short size_;
            /// Position in decoded stream.
            unsigned decodedPosition_;
            /// Decoded chunk size;
            unsigned short decodedSize_;
        };
    public:
        /// Construct Deserializer.
        ChunkStreamDeserializer(Deserializer& deserializer);
        /// Read bytes from the stream. Return number of bytes actually read.
        unsigned Read(void* dest, unsigned size) override;
        /// Set position from the beginning of the stream. Return actual new position.
        unsigned Seek(unsigned position) override;

    protected:
        /// Read block from underlying deserizalizer.
        virtual bool ReadBlock(Deserializer& deserializer, unsigned short unpackedSize, unsigned short packedSize) = 0;

        /// Read buffer for Android asset or compressed file loading.
        ea::shared_array<unsigned char> readBuffer_;
        /// Read buffer position.
        unsigned readBufferOffset_;
        /// Bytes in the current read buffer.
        unsigned readBufferSize_;

    private:

        /// Register new chunk. All chunk should be sequential in position.
        /// Returns true if chunk is valid and added to the sequence.
        bool Add(const Chunk& chunk);

        /// Find chunk by stream position.
        /// Returns pointer to chunk if found or nullptr if not.
        const Chunk* FindChunk(unsigned decodedPosition) const;

        /// Get last known chunk or nullptr if none.
        const Chunk* GetLastChunk() const { return chunks_.empty() ? nullptr : &chunks_.back(); }

        /// Get last known chunk position or 0 if no chunks known yet.
        unsigned GetLastPosition() const { return chunks_.empty() ? 0 : chunks_.back().position_; }

        /// Original stream reader.
        Deserializer& deserializer_;

        /// Decompression input buffer for compressed file loading.
        ea::shared_array<unsigned char> inputBuffer_;
        /// Bytes in the current input buffer.
        unsigned inputBufferSize_;
        /// Start position within a package file, 0 for regular files.
        unsigned offset_;

        /// Sorted vector of chunks.
        ea::vector<Chunk> chunks_;
    };

    class URHO3D_API ChunkStreamSerializer : public Serializer
    {
    public:
        static const unsigned DefaultChunkSize = 32768;

        ChunkStreamSerializer(Serializer& serializer, unsigned short chunkSize = DefaultChunkSize);

        /// Write bytes to the stream. Return number of bytes actually written.
        unsigned Write(const void* data, unsigned size) override;

        bool Flush();
    protected:
        virtual unsigned char* GetInputBuffer(unsigned chunkSize) = 0;

        virtual bool FlushImpl(Serializer& serializer, unsigned inputBufferSize) = 0;
    private:
        /// Underlying serializer;
        Serializer& serializer_;
        /// input buffer position.
        unsigned chunkSize_;
        /// input buffer position.
        unsigned inputBufferPosition_;
    };

}
