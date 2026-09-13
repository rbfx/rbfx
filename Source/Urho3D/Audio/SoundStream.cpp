// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Audio/SoundStream.h"

namespace Urho3D
{

SoundStream::SoundStream() :
    frequency_(44100),
    stopAtEnd_(false),
    sixteenBit_(false),
    stereo_(false)
{
}

SoundStream::~SoundStream() = default;

bool SoundStream::Seek(unsigned int sample_number)
{
    return false;
}

void SoundStream::SetFormat(unsigned frequency, bool sixteenBit, bool stereo)
{
    frequency_ = frequency;
    sixteenBit_ = sixteenBit;
    stereo_ = stereo;
}

void SoundStream::SetStopAtEnd(bool enable)
{
    stopAtEnd_ = enable;
}

unsigned SoundStream::GetSampleSize() const
{
    unsigned size = 1;
    if (sixteenBit_)
        size <<= 1;
    if (stereo_)
        size <<= 1;
    return size;
}

}
