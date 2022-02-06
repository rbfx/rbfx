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

#pragma once

#include <EASTL/list.h>
#include <EASTL/shared_array.h>

#include "../Audio/SoundStream.h"
#include "../Core/Mutex.h"

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
