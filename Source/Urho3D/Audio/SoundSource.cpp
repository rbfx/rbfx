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

#include <Urho3D/Precompiled.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundStream.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/DebugNew.h>

namespace Urho3D
{

/// Reminder that channels are in WAV order, FL FR FC LFE RL RR
static const int SOUND_SOURCE_LOW_FREQ_CHANNEL[] = {
    0, // SPK_AUTO
    0, // SPK_MONO
    0, // SPK_STEREO
    0, // SPK_QUADROPHONIC
    3, // SPK_SURROUND_5_1
};

#define INC_POS_LOOPED() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_ONESHOT() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define INC_POS_STEREO_LOOPED() \
    pos += ((unsigned)intAdd << 1u); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_STEREO_ONESHOT() \
    pos += ((unsigned)intAdd << 1u); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define GET_IP_SAMPLE() (((((int)pos[1] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_LEFT() (((((int)pos[2] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_RIGHT() (((((int)pos[3] - (int)pos[1]) * fractPos) / 65536) + (int)pos[1])

static const int STREAM_SAFETY_SAMPLES = 4;

extern const char* autoRemoveModeNames[];

SoundSource::SoundSource(Context* context) :
    Component(context),
    soundType_(SOUND_EFFECT),
    frequency_(0.0f),
    gain_(1.0f),
    attenuation_(1.0f),
    panning_(0.0f),
    sendFinishedEvent_(false),
    autoRemove_(REMOVE_DISABLED),
    position_(nullptr),
    fractPosition_(0),
    timePosition_(0.0f),
    unusedStreamSize_(0)
{
    audio_ = GetSubsystem<Audio>();

    if (audio_)
        audio_->AddSoundSource(this);

    UpdateMasterGain();
}

SoundSource::~SoundSource()
{
    if (audio_)
        audio_->RemoveSoundSource(this);
}

void SoundSource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SoundSource>(Category_Audio);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Type", GetSoundType, SetSoundType, ea::string, SOUND_EFFECT, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", float, frequency_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gain", float, gain_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Attenuation", float, attenuation_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Panning", float, panning_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Reach", float, reach_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Low Frequency Effect", bool, lowFrequency_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Ignore Scene Time Scale", bool, ignoreSceneTimeScale_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, false, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Autoremove Mode", autoRemove_, autoRemoveModeNames, REMOVE_DISABLED, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Play Position", GetPositionAttr, SetPositionAttr, int, 0, AM_DEFAULT);
}

void SoundSource::Seek(float seekTime)
{
    // Ignore buffered sound stream
    if (!audio_ || !sound_ || (soundStream_ && !sound_->IsCompressed()))
        return;

    // Set to valid range
    seekTime = Clamp(seekTime, 0.0f, sound_->GetLength());

    if (!soundStream_)
    {
        // Raw or wav format
        SetPositionAttr((int)(seekTime * (sound_->GetSampleSize() * sound_->GetFrequency())));
    }
    else
    {
        // Ogg format
        if (soundStream_->Seek((unsigned)(seekTime * soundStream_->GetFrequency())))
        {
            timePosition_ = seekTime;
        }
    }
}

void SoundSource::Play(Sound* sound)
{
    if (!audio_)
        return;

    // If no frequency set yet, set from the sound's default
    if (frequency_ == 0.0f && sound)
        SetFrequency(sound->GetFrequency());

    // If sound source is currently playing, have to lock the audio mutex
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        PlayLockless(sound);
    }
    else
        PlayLockless(sound);
}

void SoundSource::Play(Sound* sound, float frequency)
{
    SetFrequency(frequency);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain)
{
    SetFrequency(frequency);
    SetGain(gain);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain, float panning)
{
    SetFrequency(frequency);
    SetGain(gain);
    SetPanning(panning);
    Play(sound);
}

void SoundSource::Play(SoundStream* stream)
{
    if (!audio_)
        return;

    // If no frequency set yet, set from the stream's default
    if (frequency_ == 0.0f && stream)
        SetFrequency(stream->GetFrequency());

    SharedPtr<SoundStream> streamPtr(stream);

    // If sound source is currently playing, have to lock the audio mutex. When stream playback is explicitly
    // requested, clear the existing sound if any
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        sound_.Reset();
        PlayLockless(streamPtr);
    }
    else
    {
        sound_.Reset();
        PlayLockless(streamPtr);
    }

    // Stream playback is not supported for network replication, no need to mark network dirty
}

void SoundSource::Stop()
{
    if (!audio_)
        return;

    // If sound source is currently playing, have to lock the audio mutex
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        StopLockless();
    }
    else
        StopLockless();
}

void SoundSource::SetSoundType(const ea::string& type)
{
    if (type == SOUND_MASTER)
        return;

    soundType_ = type;
    soundTypeHash_ = StringHash(type);
    UpdateMasterGain();
}

void SoundSource::SetFrequency(float frequency)
{
    frequency_ = Clamp(frequency, 0.0f, 535232.0f);
}

void SoundSource::SetGain(float gain)
{
    gain_ = Max(gain, 0.0f);
}

void SoundSource::SetAttenuation(float attenuation)
{
    attenuation_ = Clamp(attenuation, 0.0f, 1.0f);
}

void SoundSource::SetPanning(float panning)
{
    panning_ = Clamp(panning, -1.0f, 1.0f);
}

void SoundSource::SetReach(float reach)
{
    reach_ = Clamp(reach, -1.0f, 1.0f);
}

void SoundSource::SetLowFrequency(bool state)
{
    lowFrequency_ = state;
}

void SoundSource::SetAutoRemoveMode(AutoRemoveMode mode)
{
    autoRemove_ = mode;
}

bool SoundSource::IsPlaying() const
{
    return (sound_ || soundStream_) && position_ != nullptr;
}

void SoundSource::SetPlayPosition(signed char* pos)
{
    // Setting play position on a stream is not supported
    if (!audio_ || !sound_ || soundStream_)
        return;

    MutexLock lock(audio_->GetMutex());
    SetPlayPositionLockless(pos);
}

void SoundSource::SetIgnoreSceneTimeScale(bool ignoreSceneTimeScale)
{
    ignoreSceneTimeScale_ = ignoreSceneTimeScale;
}

void SoundSource::Update(float timeStep)
{
    if (!audio_)
        return;

    const float effectiveTimeScale = GetEffectiveTimeScale();
    if (effectiveTimeScale == 0.0f)
        return;

    // If there is no actual audio output, perform fake mixing into a nonexistent buffer to check stopping/looping
    if (!audio_->IsInitialized())
        MixNull(timeStep, frequency_ * effectiveTimeScale);

    // Free the stream if playback has stopped
    if (soundStream_ && !position_)
        StopLockless();

    bool playing = IsPlaying();

    if (!playing && sendFinishedEvent_ && node_ != nullptr)
    {
        sendFinishedEvent_ = false;

        // Make a weak pointer to self to check for destruction during event handling
        WeakPtr<SoundSource> self(this);

        using namespace SoundFinished;

        VariantMap& eventData = context_->GetEventDataMap();
        eventData[P_NODE] = node_;
        eventData[P_SOUNDSOURCE] = this;
        eventData[P_SOUND] = sound_;
        node_->SendEvent(E_SOUNDFINISHED, eventData);

        if (self.Expired())
            return;

        DoAutoRemove(autoRemove_);
    }
}

void SoundSource::Mix(int dest[], unsigned samples, int mixRate, SpeakerMode mode, bool interpolation)
{
    if (!position_ || (!sound_ && !soundStream_))
        return;

    const float effectiveTimeScale = GetEffectiveTimeScale();
    if (effectiveTimeScale == 0.0f)
        return;

    const float effectiveFrequency = frequency_ * effectiveTimeScale;

    int streamFilledSize, outBytes;

    if (soundStream_ && streamBuffer_)
    {
        const int streamBufferSize = streamBuffer_->GetDataSize();
        // Calculate how many bytes of stream sound data is needed
        auto neededSize = (int)((float)samples * effectiveFrequency / (float)mixRate);
        // Add a little safety buffer. Subtract previous unused data
        neededSize += STREAM_SAFETY_SAMPLES;
        neededSize *= soundStream_->GetSampleSize();
        neededSize -= unusedStreamSize_;
        neededSize = Clamp(neededSize, 0, streamBufferSize - unusedStreamSize_);

        // Always start play position at the beginning of the stream buffer
        position_ = streamBuffer_->GetStart();

        // Request new data from the stream
        signed char* destination = streamBuffer_->GetStart() + unusedStreamSize_;
        outBytes = neededSize ? soundStream_->GetData(destination, (unsigned)neededSize) : 0;
        destination += outBytes;
        // Zero-fill rest if stream did not produce enough data
        if (outBytes < neededSize)
            memset(destination, 0, (size_t)(neededSize - outBytes));

        // Calculate amount of total bytes of data in stream buffer now, to know how much went unused after mixing
        streamFilledSize = neededSize + unusedStreamSize_;
    }

    // If streaming, play the stream buffer. Otherwise play the original sound
    Sound* sound = soundStream_ ? streamBuffer_ : sound_;
    if (!sound)
        return;

    // Choose the correct mixing routine
    if (!sound->IsStereo())
    {
        if (interpolation)
        {
            switch (mode)
            {
            case SPK_AUTO:
                assert(!"SPK_AUTO");
                break;
            case SPK_MONO:
                if (!lowFrequency_)
                    MixMonoToMonoIP(sound, dest, samples, mixRate, effectiveFrequency);
                break;
            case SPK_STEREO:
                if (!lowFrequency_)
                    MixMonoToStereoIP(sound, dest, samples, mixRate, effectiveFrequency);
                break;
            case SPK_QUADROPHONIC:
                if (!lowFrequency_)
                    MixMonoToSurroundIP(sound, dest, samples, mixRate, effectiveFrequency, mode);
                break;
            case SPK_SURROUND_5_1:
                if (lowFrequency_)
                    MixMonoToMonoIP(sound, dest, samples, mixRate, effectiveFrequency, SOUND_SOURCE_LOW_FREQ_CHANNEL[mode], 6);
                else
                    MixMonoToSurroundIP(sound, dest, samples, mixRate, effectiveFrequency, mode);
                break;
            }
        }
        else
        {
            switch (mode)
            {
            case SPK_AUTO:
                assert(!"SPK_AUTO");
                break;
            case SPK_MONO:
                if (!lowFrequency_)
                    MixMonoToStereo(sound, dest, samples, mixRate, effectiveFrequency);
                break;
            case SPK_STEREO:
                if (!lowFrequency_)
                    MixMonoToMono(sound, dest, samples, mixRate, effectiveFrequency);
                break;
            case SPK_QUADROPHONIC:
                if (!lowFrequency_)
                    MixMonoToSurround(sound, dest, samples, mixRate, effectiveFrequency, mode);
                break;
            case SPK_SURROUND_5_1:
                if (lowFrequency_)
                    MixMonoToMono(
                        sound, dest, samples, mixRate, effectiveFrequency, SOUND_SOURCE_LOW_FREQ_CHANNEL[mode], 6);
                else
                    MixMonoToSurround(sound, dest, samples, mixRate, effectiveFrequency, mode);
                break;
            }
        }
    }
    else
    {
        if (interpolation)
        {
            if (mode == SPK_MONO)
                MixStereoToMonoIP(sound, dest, samples, mixRate, effectiveFrequency);
            else
                MixStereoToMultiIP(sound, dest, samples, mixRate, effectiveFrequency, mode);
        }
        else
        {
            if (mode == SPK_MONO)
                MixStereoToMono(sound, dest, samples, mixRate, effectiveFrequency);
            else
                MixStereoToMulti(sound, dest, samples, mixRate, effectiveFrequency, mode);
        }
    }

    // Update the time position. In stream mode, copy unused data back to the beginning of the stream buffer
    if (soundStream_)
    {
        timePosition_ += ((float)samples / (float)mixRate) * effectiveFrequency / soundStream_->GetFrequency();

        unusedStreamSize_ = Max(streamFilledSize - (int)(size_t)(position_ - streamBuffer_->GetStart()), 0);
        if (unusedStreamSize_)
            memcpy(streamBuffer_->GetStart(), (const void*)position_, (size_t)unusedStreamSize_);

        // If stream did not produce any data, stop if applicable
        if (!outBytes && soundStream_->GetStopAtEnd())
        {
            position_ = nullptr;
            return;
        }
    }
    else if (sound_)
        timePosition_ = ((float)(int)(size_t)(position_ - sound_->GetStart())) / (sound_->GetSampleSize() * sound_->GetFrequency());
}

void SoundSource::UpdateMasterGain()
{
    if (audio_)
        masterGain_ = audio_->GetSoundSourceMasterGain(soundType_);
}

void SoundSource::SetSoundAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* newSound = cache->GetResource<Sound>(value.name_);
    if (IsPlaying())
        Play(newSound);
    else
    {
        // When changing the sound and not playing, free previous sound stream and stream buffer (if any)
        soundStream_.Reset();
        streamBuffer_.Reset();
        sound_ = newSound;
    }
}

void SoundSource::SetPlayingAttr(bool value)
{
    if (value)
    {
        if (!IsPlaying())
            Play(sound_.Get());
    }
    else
        Stop();
}

void SoundSource::SetPositionAttr(int value)
{
    if (sound_)
        SetPlayPosition(sound_->GetStart() + value);
}

ResourceRef SoundSource::GetSoundAttr() const
{
    return GetResourceRef(sound_, Sound::GetTypeStatic());
}

int SoundSource::GetPositionAttr() const
{
    if (sound_ && position_)
        return (int)(GetPlayPosition() - sound_->GetStart());
    else
        return 0;
}

void SoundSource::PlayLockless(Sound* sound)
{
    // Reset the time position in any case
    timePosition_ = 0.0f;

    if (sound)
    {
        if (!sound->IsCompressed())
        {
            // Uncompressed sound start
            signed char* start = sound->GetStart();
            if (start)
            {
                // Free existing stream & stream buffer if any
                soundStream_.Reset();
                streamBuffer_.Reset();
                sound_ = sound;
                position_ = start;
                fractPosition_ = 0;
                sendFinishedEvent_ = true;
                return;
            }
        }
        else
        {
            // Compressed sound start
            PlayLockless(sound->GetDecoderStream());
            sound_ = sound;
            return;
        }
    }

    // If sound pointer is null or if sound has no data, stop playback
    StopLockless();
    sound_.Reset();
}

void SoundSource::PlayLockless(const SharedPtr<SoundStream>& stream)
{
    // Reset the time position in any case
    timePosition_ = 0.0f;

    if (stream)
    {
        // Setup the stream buffer
        unsigned sampleSize = stream->GetSampleSize();
        unsigned streamBufferSize = sampleSize * stream->GetIntFrequency() * STREAM_BUFFER_LENGTH / 1000;

        streamBuffer_ = MakeShared<Sound>(context_);
        streamBuffer_->SetSize(streamBufferSize);
        streamBuffer_->SetFormat(stream->GetIntFrequency(), stream->IsSixteenBit(), stream->IsStereo());
        streamBuffer_->SetLooped(true);

        soundStream_ = stream;
        unusedStreamSize_ = 0;
        position_ = streamBuffer_->GetStart();
        fractPosition_ = 0;
        sendFinishedEvent_ = true;
        return;
    }

    // If stream pointer is null, stop playback
    StopLockless();
}

void SoundSource::StopLockless()
{
    position_ = nullptr;
    timePosition_ = 0.0f;

    // Free the sound stream and decode buffer if a stream was playing
    soundStream_.Reset();
    streamBuffer_.Reset();
}

void SoundSource::SetPlayPositionLockless(signed char* pos)
{
    // Setting position on a stream is not supported
    if (!sound_ || soundStream_)
        return;

    signed char* start = sound_->GetStart();
    signed char* end = sound_->GetEnd();
    if (pos < start)
        pos = start;
    if (sound_->IsSixteenBit() && (pos - start) & 1u)
        ++pos;
    if (pos > end)
        pos = end;

    position_ = pos;
    timePosition_ = ((float)(int)(size_t)(pos - sound_->GetStart())) / (sound_->GetSampleSize() * sound_->GetFrequency());
}

void SoundSource::MixMonoToMono(
    Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency, int channel, int channelCount)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + (*pos * vol) / 256;
                dest += channelCount - channel;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + (*pos * vol) / 256;
                dest += channelCount - channel;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + *pos * vol;
                dest += channelCount - channel;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + *pos * vol;
                dest += channelCount - channel;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }
    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereo(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = (int)((-panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    auto rightVol = (int)((panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    if (!leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / 256;
                ++dest;
                *dest = *dest + (*pos * rightVol) / 256;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / 256;
                ++dest;
                *dest = *dest + (*pos * rightVol) / 256;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + *pos * leftVol;
                ++dest;
                *dest = *dest + *pos * rightVol;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + *pos * leftVol;
                ++dest;
                *dest = *dest + *pos * rightVol;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToMonoIP(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency, int channel, int channelCount)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + (GET_IP_SAMPLE() * vol) / 256;
                dest += channelCount - channel;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + (GET_IP_SAMPLE() * vol) / 256;
                dest += channelCount - channel;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + GET_IP_SAMPLE() * vol;
                dest += channelCount - channel;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                dest += channel;
                *dest = *dest + GET_IP_SAMPLE() * vol;
                dest += channelCount - channel;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereoIP(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = (int)((-panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    auto rightVol = (int)((panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    if (!leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / 256;
                ++dest;
                *dest = *dest + (s * rightVol) / 256;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / 256;
                ++dest;
                *dest = *dest + (s * rightVol) / 256;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + s * leftVol;
                ++dest;
                *dest = *dest + s * rightVol;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + s * leftVol;
                ++dest;
                *dest = *dest + s * rightVol;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMono(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / 256;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / 256;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + s * vol;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + s * vol;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToStereo(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / 256;
                ++dest;
                *dest = *dest + (pos[1] * vol) / 256;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / 256;
                ++dest;
                *dest = *dest + (pos[1] * vol) / 256;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + pos[0] * vol;
                ++dest;
                *dest = *dest + pos[1] * vol;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + pos[0] * vol;
                ++dest;
                *dest = *dest + pos[1] * vol;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMonoIP(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / 256;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / 256;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + s * vol;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + s * vol;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToStereoIP(Sound* sound, int dest[], unsigned samples, int mixRate, float effectiveFrequency)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + GET_IP_SAMPLE_LEFT() * vol;
                ++dest;
                *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + GET_IP_SAMPLE_LEFT() * vol;
                ++dest;
                *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToSurround(
    Sound* sound, int* dest, unsigned samples, int mixRate, float effectiveFrequency, SpeakerMode speakerMode)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    int frontLeftVol = (int)(((-panning_ + 1.0f) * (reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int frontRightVol = (int)(((panning_ + 1.0f) * (reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int rearLeftVol = (int)(((-panning_ + 1.0f) * (-reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int rearRightVol = (int)(((panning_ + 1.0f) * (-reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int centerVol = Lerp(frontLeftVol, frontRightVol, 0.5f) * Clamp(reach_, 0.0f, 1.0f);

    if (!frontLeftVol && !frontRightVol && !rearLeftVol && !rearRightVol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    int intAdd = (int)add;
    int fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        short* pos = (short*)position_;
        short* end = (short*)sound->GetEnd();
        short* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (*pos * frontLeftVol) / 256; // FL
                ++dest;
                *dest = *dest + (*pos * frontRightVol) / 256; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + (*pos * centerVol) / 256; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + (*pos * rearLeftVol) / 256; // RL
                ++dest;
                *dest = *dest + (*pos * rearRightVol) / 256; // RR
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (*pos * frontLeftVol) / 256; // FL
                ++dest;
                *dest = *dest + (*pos * frontRightVol) / 256; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + (*pos * centerVol) / 256; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + (*pos * rearLeftVol) / 256; // RL
                ++dest;
                *dest = *dest + (*pos * rearRightVol) / 256; // RR
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        signed char* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + *pos * frontLeftVol; // FL
                ++dest;
                *dest = *dest + *pos * frontRightVol; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + *pos * centerVol; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + *pos * rearLeftVol; // RL
                ++dest;
                *dest = *dest + *pos * rearRightVol; // RR
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + *pos * frontLeftVol; // FL
                ++dest;
                *dest = *dest + *pos * frontRightVol; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + *pos * centerVol; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + *pos * rearLeftVol; // RL
                ++dest;
                *dest = *dest + *pos * rearRightVol; // RR
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToSurroundIP(
    Sound* sound, int* dest, unsigned samples, int mixRate, float effectiveFrequency, SpeakerMode speakerMode)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    int frontLeftVol = (int)(((-panning_ + 1.0f) * (reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int frontRightVol = (int)(((panning_ + 1.0f) * (reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int rearLeftVol = (int)(((-panning_ + 1.0f) * (-reach_ + 1.0f)) * (256.0f * totalGain + 0.05f));
    int rearRightVol = (int)(((panning_ + 1.0f) * (-reach_ + 1.0f)) * (256.0f * totalGain + 0.5f));
    int centerVol = Lerp(frontLeftVol, frontRightVol, 0.5f) * Clamp(reach_, 0.0f, 1.0f);

    if (!frontLeftVol && !frontRightVol && !rearLeftVol && !rearRightVol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    int intAdd = (int)add;
    int fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        short* pos = (short*)position_;
        short* end = (short*)sound->GetEnd();
        short* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * frontLeftVol) / 256; // FL
                ++dest;
                *dest = *dest + (s * frontRightVol) / 256; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + (s * centerVol) / 256; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + (s * rearLeftVol) / 256; // RL
                ++dest;
                *dest = *dest + (s * rearRightVol) / 256; // RR
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * frontLeftVol) / 256; // FL
                ++dest;
                *dest = *dest + (s * frontRightVol) / 256; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + (s * centerVol) / 256; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + (s * rearLeftVol) / 256; // RL
                ++dest;
                *dest = *dest + (s * rearRightVol) / 256; // RR
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        signed char* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + s * frontLeftVol; // FL
                ++dest;
                *dest = *dest + s * frontRightVol; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + s * centerVol; // FC
                    ++dest;
                    ++dest; // LFE
                }

                *dest = *dest + s * rearLeftVol; // RL
                ++dest;
                *dest = *dest + s * rearRightVol; // RR
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + s * frontLeftVol; // FL
                ++dest;
                *dest = *dest + s * frontRightVol; // FR
                ++dest;

                if (speakerMode == SPK_SURROUND_5_1)
                {
                    *dest = *dest + s * centerVol; // FC
                    ++dest; // FC
                    ++dest; // LFE
                }

                *dest = *dest + s * rearLeftVol; // RL
                ++dest;
                *dest = *dest + s * rearRightVol; // RR
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMulti(
    Sound* sound, int* dest, unsigned samples, int mixRate, float effectiveFrequency, SpeakerMode speakers)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    int vol = (int)(256.0f * totalGain + 0.5f);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    int intAdd = (int)add;
    int fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        short* pos = (short*)position_;
        short* end = (short*)sound->GetEnd();
        short* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / 256; // FL
                ++dest;
                *dest = *dest + (pos[1] * vol) / 256; // FR
                ++dest;

                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }

                    *dest = *dest + (pos[0] * vol) / 256; // RL
                    ++dest;
                    *dest = *dest + (pos[1] * vol) / 256; // RR
                    ++dest;
                }
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / 256; // FL
                ++dest;
                *dest = *dest + (pos[1] * vol) / 256; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }
                    *dest = *dest + (pos[0] * vol) / 256; // RL
                    ++dest;
                    *dest = *dest + (pos[1] * vol) / 256; // RR
                    ++dest;
                }
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        signed char* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + pos[0] * vol; // FL
                ++dest;
                *dest = *dest + pos[1] * vol; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }
                    *dest = *dest + pos[0] * vol; // RL
                    ++dest;
                    *dest = *dest + pos[1] * vol; // RR
                    ++dest;
                }
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + pos[0] * vol; // FL
                ++dest;
                *dest = *dest + pos[1] * vol; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }

                    *dest = *dest + pos[0] * vol; // RL
                    ++dest;
                    *dest = *dest + pos[1] * vol; // RR
                    ++dest;
                }
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMultiIP(
    Sound* sound, int* dest, unsigned samples, int mixRate, float effectiveFrequency, SpeakerMode speakers)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    int vol = (int)(256.0f * totalGain + 0.5f);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate, effectiveFrequency);
        return;
    }

    float add = effectiveFrequency / (float)mixRate;
    int intAdd = (int)add;
    int fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        short* pos = (short*)position_;
        short* end = (short*)sound->GetEnd();
        short* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256; // FL
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        *dest = *dest + (((GET_IP_SAMPLE_LEFT() * vol) + GET_IP_SAMPLE_RIGHT() * vol) / 256) / 2;
                        ++dest; // FC
                        *dest = *dest + (((GET_IP_SAMPLE_LEFT() * vol) + GET_IP_SAMPLE_RIGHT() * vol) / 256) / 2;
                        ++dest; // LFE
                    }

                    *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256; // RL
                    ++dest;
                    *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256; // RR
                    ++dest;
                }
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256; // FL
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }

                    *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / 256; // RL
                    ++dest;
                    *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / 256; // RR
                    ++dest;
                }
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        signed char* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (samples--)
            {
                *dest = *dest + GET_IP_SAMPLE_LEFT() * vol; // FL
                ++dest;
                *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }

                    *dest = *dest + GET_IP_SAMPLE_LEFT() * vol; // RL
                    ++dest;
                    *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol; // RR
                    ++dest;
                }
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + GET_IP_SAMPLE_LEFT() * vol; // FL
                ++dest;
                *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol; // FR
                ++dest;
                if (speakers > SPK_STEREO)
                {
                    if (speakers == SPK_SURROUND_5_1)
                    {
                        ++dest; // FC
                        ++dest; // LFE
                    }

                    *dest = *dest + GET_IP_SAMPLE_LEFT() * vol; // RL
                    ++dest;
                    *dest = *dest + GET_IP_SAMPLE_RIGHT() * vol; // RR
                    ++dest;
                }
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixZeroVolume(Sound* sound, unsigned samples, int mixRate, float effectiveFrequency)
{
    float add = effectiveFrequency * (float)samples / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    unsigned sampleSize = sound->GetSampleSize();

    fractPosition_ += fractAdd;
    if (fractPosition_ > 65535)
    {
        fractPosition_ &= 65535;
        position_ += sampleSize;
    }
    position_ += intAdd * sampleSize;

    if (position_ > sound->GetEnd())
    {
        if (sound->IsLooped())
        {
            while (position_ >= sound->GetEnd())
            {
                position_ -= (sound->GetEnd() - sound->GetRepeat());
            }
        }
        else
            position_ = nullptr;
    }
}

void SoundSource::MixNull(float timeStep, float effectiveFrequency)
{
    if (!position_ || !sound_ || !IsEnabledEffective())
        return;

    // Advance only the time position
    timePosition_ += timeStep * effectiveFrequency / sound_->GetFrequency();

    if (sound_->IsLooped())
    {
        // For simulated playback, simply reset the time position to zero when the sound loops
        if (timePosition_ >= sound_->GetLength())
            timePosition_ -= sound_->GetLength();
    }
    else
    {
        if (timePosition_ >= sound_->GetLength())
        {
            position_ = nullptr;
            timePosition_ = 0.0f;
        }
    }
}

float SoundSource::GetEffectiveTimeScale() const
{
    if (ignoreSceneTimeScale_)
        return 1.0f;

    if (!node_ || !node_->GetScene())
        return 0.0f;

    if (!IsEnabledEffective())
        return 0.0f;

    return node_->GetScene()->GetEffectiveTimeScale();
}

}
