// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    explicit MemoryBuffer(const char* text);
    /// Construct as read-only from string.
    explicit MemoryBuffer(ea::string_view text);
    /// Construct as read-only from string.
    explicit MemoryBuffer(const ea::string& text);
    /// Construct from a vector, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(ByteVector& data);
    /// Construct from a read-only vector, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(const ByteVector& data);
    /// Construct from a vector buffer, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(VectorBuffer& data);
    /// Construct from a read-only vector buffer, which must not go out of scope before MemoryBuffer.
    explicit MemoryBuffer(const VectorBuffer& data);
    /// Construct as read-only from a span.
    explicit MemoryBuffer(ea::span<unsigned char> data);
    /// Construct as read-only from a span.
    explicit MemoryBuffer(ea::span<const unsigned char> data);

    /// Read bytes from the memory area. Return number of bytes actually read.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the memory area. Return actual new position.
    unsigned Seek(unsigned position) override;
    /// Write bytes to the memory area.
    unsigned Write(const void* data, unsigned size) override;

    /// Return memory area.
    unsigned char* GetData() const { return buffer_; }

    /// Return whether buffer is read-only.
    bool IsReadOnly() const { return readOnly_; }

private:
    /// Pointer to the memory area.
    unsigned char* buffer_;
    /// Read-only flag.
    bool readOnly_;
};

}
