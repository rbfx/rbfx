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

#include "../Precompiled.h"

#include "../Audio/OggVorbisSoundStream.h"
#include "../Audio/Sound.h"

#include <STB/stb_vorbis.h>

#include "../DebugNew.h"

namespace Urho3D
{

OggVorbisSoundStream::OggVorbisSoundStream(const Sound* sound)
{
    assert(sound && sound->IsCompressed());

    SetFormat(sound->GetIntFrequency(), sound->IsSixteenBit(), sound->IsStereo());
    // If the sound is looped, the stream will automatically rewind at end
    SetStopAtEnd(!sound->IsLooped());

    // Initialize decoder
    data_ = sound->GetData();
    dataSize_ = sound->GetDataSize();
    int error;
    decoder_ = stb_vorbis_open_memory((unsigned char*)data_.get(), dataSize_, &error, nullptr);
}

OggVorbisSoundStream::~OggVorbisSoundStream()
{
    // Close decoder
    if (decoder_)
    {
        auto* vorbis = static_cast<stb_vorbis*>(decoder_);

        stb_vorbis_close(vorbis);
        decoder_ = nullptr;
    }
}

bool OggVorbisSoundStream::Seek(unsigned sample_number)
{
    if (!decoder_)
        return false;

    auto* vorbis = static_cast<stb_vorbis*>(decoder_);

    return stb_vorbis_seek(vorbis, sample_number) == 1;
}

unsigned OggVorbisSoundStream::GetData(signed char* dest, unsigned numBytes)
{
    if (!decoder_)
        return 0;

    auto* vorbis = static_cast<stb_vorbis*>(decoder_);

    unsigned channels = stereo_ ? 2 : 1;
    auto outSamples = (unsigned)stb_vorbis_get_samples_short_interleaved(vorbis, channels, (short*)dest, numBytes >> 1u);
    unsigned outBytes = (outSamples * channels) << 1u;

    // Rewind and retry if is looping and produced less output than should have
    if (outBytes < numBytes && !stopAtEnd_)
    {
        numBytes -= outBytes;
        stb_vorbis_seek_start(vorbis);
        outSamples =
            (unsigned)stb_vorbis_get_samples_short_interleaved(vorbis, channels, (short*)(dest + outBytes), numBytes >> 1u);
        outBytes += (outSamples * channels) << 1u;
    }

    return outBytes;
}

}
