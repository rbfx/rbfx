//
// Copyright (c) 2024-2024 the Urho3D project.
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

#include "../SteamAudio/SteamAudio.h"
#include "../SteamAudio/SteamSoundSource.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundStream.h"
#include "../Audio/OggVorbisSoundStream.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"

#include "../DebugNew.h"

#include <phonon.h>

namespace Urho3D
{

SteamSoundSource::SteamSoundSource(Context* context) :
    Component(context), paused_(false), sound_(nullptr)
{
    audio_ = GetSubsystem<SteamAudio>();

    if (audio_) {
        // Create buffers
        const auto phononContext = audio_->GetPhononContext();
        const auto& audioSettings = audio_->GetAudioSettings();
        const auto channelCount = audio_->GetChannelCount();
        iplAudioBufferAllocate(phononContext, channelCount, audioSettings.frameSize, &stageABuffer_);
        iplAudioBufferAllocate(phononContext, channelCount, audioSettings.frameSize, &stageBBuffer_);
        iplAudioBufferAllocate(phononContext, channelCount, audioSettings.frameSize, &stageCBuffer_);
        iplAudioBufferAllocate(phononContext, channelCount, audioSettings.frameSize, &outputBuffer_);

        // Add this sound source
        audio_->AddSoundSource(this);
    }
}

SteamSoundSource::~SteamSoundSource()
{
    if (audio_) {
        // Remove this sound source
        audio_->RemoveSoundSource(this);

        // Delete buffers
        const auto phononContext = audio_->GetPhononContext();
        iplAudioBufferFree(phononContext, &stageABuffer_);
        iplAudioBufferFree(phononContext, &stageBBuffer_);
        iplAudioBufferFree(phononContext, &stageCBuffer_);
        iplAudioBufferFree(phononContext, &outputBuffer_);
    }
}

void SteamSoundSource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SteamSoundSource>(Category_Audio);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
}

void SteamSoundSource::Play(Sound *sound)
{
    // Reset current frame (playback position)
    frame_ = 0;

    // Set sound
    sound_ = sound;
}

bool SteamSoundSource::IsPlaying() const
{
    const auto& audioSettings = audio_->GetAudioSettings();
    return sound_ && !paused_ && frame_ < sound_->GetDataSize()/audioSettings.frameSize;
}

void SteamSoundSource::SetPlayingAttr(bool playing)
{
    paused_ = !playing;
}

void SteamSoundSource::SetSoundAttr(const ResourceRef &value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    Play(cache->GetResource<Sound>(value.name_));
}

ResourceRef SteamSoundSource::GetSoundAttr() const
{
    return GetResourceRef(sound_, Sound::GetTypeStatic());
}

IPLAudioBuffer *SteamSoundSource::GenerateAudioBuffer()
{
    // Return nothing if not playing
    if (!IsPlaying())
        return nullptr;

    // Get phonon context and audio settings
    const auto phononContext = audio_->GetPhononContext();
    const auto& audioSettings = audio_->GetAudioSettings();

    // Convert sound data to interleaved float buffer
    ea::vector<float> rawInputBuffer(audioSettings.frameSize*audio_->GetChannelCount());
    for (unsigned sample = 0; sample != audioSettings.frameSize; sample++)
        rawInputBuffer[sample] = float(sound_->GetData()[frame_*audioSettings.frameSize + sample])/(sound_->IsSixteenBit()?32767.0f:128.f);

    // Deinterleave sound data into stage A buffer
    iplAudioBufferDeinterleave(phononContext, rawInputBuffer.data(), &stageABuffer_);

    // Increment to next frame
    frame_++;

    // Don't process any further for now, just return that buffer as is...
    return &stageABuffer_;
}

}
