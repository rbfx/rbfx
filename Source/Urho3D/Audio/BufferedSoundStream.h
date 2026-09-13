// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/list.h>
#include <EASTL/shared_array.h>

#include "Urho3D/Audio/SoundStream.h"
#include "Urho3D/Core/Mutex.h"

namespace Urho3D
{

/// %Sound stream that supports manual buffering of data from the main thread.
class URHO3D_API BufferedSoundStream : public SoundStream
{
public:
    /// Construct.
    BufferedSoundStream();
    /// Destruct.
    ~BufferedSoundStream() override;

    /// Produce sound data into destination. Return number of bytes produced. Called by SoundSource from the mixing thread.
    unsigned GetData(signed char* dest, unsigned numBytes) override;

    /// Buffer sound data. Makes a copy of it.
    void AddData(void* data, unsigned numBytes);
    /// Buffer sound data by taking ownership of it.
    void AddData(const ea::shared_array<signed char>& data, unsigned numBytes);
    /// Buffer sound data by taking ownership of it.
    void AddData(const ea::shared_array<signed short>& data, unsigned numBytes);
    /// Remove all buffered audio data.
    void Clear();

    /// Return amount of buffered (unplayed) sound data in bytes.
    unsigned GetBufferNumBytes() const;
    /// Return length of buffered (unplayed) sound data in seconds.
    float GetBufferLength() const;

private:
    /// Buffers and their sizes.
    ea::list<ea::pair<ea::shared_array<signed char>, unsigned> > buffers_;
    /// Byte position in the front most buffer.
    unsigned position_;
    /// Mutex for buffer data.
    mutable Mutex bufferMutex_;
};

}
