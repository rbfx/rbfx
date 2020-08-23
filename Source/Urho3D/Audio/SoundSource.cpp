//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Audio/Audio.h"
#include "../Audio/AudioEvents.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundSource.h"
#include "../Audio/SoundStream.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../Scene/ReplicationState.h"

#include "../DebugNew.h"

#ifdef URHO3D_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>

#endif

namespace Urho3D
{

#ifndef URHO3D_USE_OPENAL
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

#endif

extern const char* AUDIO_CATEGORY;

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
    timePosition_(0.0f),

#ifndef URHO3D_USE_OPENAL
    fractPosition_(0),
    unusedStreamSize_(0),
#else
    targetBuffer_(0),
    consumedBuffers_(0),
    streamFinished_(false),
#endif

    paused_(false)
{
    audio_ = GetSubsystem<Audio>();

    if (audio_)
        audio_->AddSoundSource(this);

#ifdef URHO3D_USE_OPENAL

    if(audio_->IsInitialized())
    {
        alGenSources(1, &alsource_); _ALERROR();
        alSourcef(alsource_, AL_PITCH, 1.0f); _ALERROR();
        alSourcef(alsource_, AL_GAIN, 1.0f); _ALERROR();
        alSource3f(alsource_, AL_POSITION, 0.0f, 0.0f, 0.0f); _ALERROR();
        alSource3f(alsource_, AL_VELOCITY, 0.0f, 0.0f, 0.0f); _ALERROR();
        alSourcei(alsource_, AL_LOOPING, AL_FALSE); _ALERROR();
        // This is useful for 2D sources so panning can be implemented
        alSourcei(alsource_, AL_SOURCE_RELATIVE, AL_TRUE); _ALERROR();
    }

#endif

    UpdateMasterGain();
}

SoundSource::~SoundSource()
{
    if (audio_)
        audio_->RemoveSoundSource(this);
}

void SoundSource::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundSource>(AUDIO_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Type", GetSoundType, SetSoundType, ea::string, SOUND_EFFECT, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", float, frequency_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gain", float, gain_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Attenuation", float, attenuation_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Panning", float, panning_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, false, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Autoremove Mode", autoRemove_, autoRemoveModeNames, REMOVE_DISABLED, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Play Position", GetPositionAttr, SetPositionAttr, int, 0, AM_FILE);
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
#ifdef URHO3D_USE_OPENAL
            consumedBuffers_ = (int)ceil(seekTime / STREAM_WANTED_SECONDS);
#endif
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
#ifdef URHO3D_USE_OPENAL
    PlayLockless(sound);
#else
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        PlayLockless(sound);
    }
    else
        PlayLockless(sound);
#endif

    // Forget the Sound & Is Playing attribute previous values so that they will be sent again, triggering
    // the sound correctly on network clients even after the initial playback
    if (networkState_ && networkState_->attributes_ && networkState_->previousValues_.size())
    {
        for (unsigned i = 1; i < networkState_->previousValues_.size(); ++i)
        {
            // The indexing is different for SoundSource & SoundSource3D, as SoundSource3D removes two attributes,
            // so go by attribute types
            VariantType type = networkState_->attributes_->at(i).type_;
            if (type == VAR_RESOURCEREF || type == VAR_BOOL)
                networkState_->previousValues_[i] = Variant::EMPTY;
        }
    }

    MarkNetworkUpdate();
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
#ifdef URHO3D_USE_OPENAL
    sound_.Reset();
    PlayLockless(streamPtr);
#else 
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
#endif

    // Stream playback is not supported for network replication, no need to mark network dirty
}

void SoundSource::Stop()
{
    if (!audio_)
        return;

    // If sound source is currently playing, have to lock the audio mutex
#ifdef URHO3D_USE_OPENAL
    StopLockless();
#else
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        StopLockless();
    }
    else
        StopLockless();
#endif

    MarkNetworkUpdate();
}

void SoundSource::Pause() 
{
#ifdef URHO3D_USE_OPENAL
    alSourcePause(alsource_);
#endif

    paused_ = true;

}

void SoundSource::Resume() 
{
#ifdef URHO3D_USE_OPENAL
    int status;
    alGetSourcei(alsource_, AL_SOURCE_STATE, &status);
    if(status == AL_PAUSED)
    {
        alSourcePlay(alsource_);
    }
#endif

    paused_ = false;
}

void SoundSource::SetSoundType(const ea::string& type)
{
    if (type == SOUND_MASTER)
        return;

    soundType_ = type;
    soundTypeHash_ = StringHash(type);
    UpdateMasterGain();

    MarkNetworkUpdate();
}

void SoundSource::SetFrequency(float frequency)
{
    frequency_ = Clamp(frequency, 0.0f, 535232.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetGain(float gain)
{
    gain_ = Max(gain, 0.0f);

    MarkNetworkUpdate();
}

void SoundSource::SetAttenuation(float attenuation)
{
    attenuation_ = Clamp(attenuation, 0.0f, 1.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetPanning(float panning)
{
    panning_ = Clamp(panning, -1.0f, 1.0f);

#ifdef URHO3D_USE_OPENAL
    // Only makes sense for 2D sources
    alSource3f(alsource_, AL_POSITION, panning_, 0.0f, 0.0f);
#endif

    MarkNetworkUpdate();
}

void SoundSource::SetAutoRemoveMode(AutoRemoveMode mode)
{
    autoRemove_ = mode;
    MarkNetworkUpdate();
}

bool SoundSource::IsPlaying() const
{
#ifdef URHO3D_USE_OPENAL
    if(audio_->IsInitialized())
    {
        int val;
        alGetSourcei(alsource_, AL_SOURCE_STATE, &val);
        return val != AL_STOPPED;
    }
    else
    {
        return (sound_ || soundStream_) && position_ != nullptr;
    }
#else
    return (sound_ || soundStream_) && position_ != nullptr;
#endif
}

bool SoundSource::IsSoundPlaying() const
{
#ifdef URHO3D_USE_OPENAL
    if(audio_->IsInitialized())
    {
        int val;
        alGetSourcei(alsource_, AL_SOURCE_STATE, &val);
        return val == AL_PLAYING;
    }
    else
    {
        return (sound_ || soundStream_) && position_ != nullptr && paused_ == false;
    }
#else
    return (sound_ || soundStream_) && position_ != nullptr && paused_ == false;
#endif
}

void SoundSource::SetPlayPosition(audio_t* pos)
{
    // Setting play position on a stream is not supported
    if (!audio_ || !sound_ || soundStream_)
        return;

#ifndef URHO3D_USE_OPENAL
    MutexLock lock(audio_->GetMutex());
#endif

    SetPlayPositionLockless(pos);
}

volatile audio_t* SoundSource::GetPlayPosition() const
{
#ifdef URHO3D_USE_OPENAL
    if(!audio_->IsInitialized())
    {
        return position_;
    }
    else
    {
        if(soundStream_)
        {

        }
        else
        {
            int byte;
            alGetSourcei(alsource_, AL_BYTE_OFFSET, &byte);
            return sound_->GetStart() + byte;
        }
    }
#else
    return position_;
#endif
}

float SoundSource::GetTimePosition() const
{
#ifdef URHO3D_USE_OPENAL
    if(!audio_->IsInitialized())
    {
        return timePosition_;
    }
    else
    {
        float time;
        alGetSourcef(alsource_, AL_SEC_OFFSET, &time);
        
        if(soundStream_)
        {
            return time + consumedBuffers_ * STREAM_WANTED_SECONDS; 
        }
        else
        {
            return time;
        }
    }
#else 
    return timePosition_;
#endif
}

int SoundSource::GetPositionAttr() const
{
#ifdef URHO3D_USE_OPENAL
    if(sound_)
        return (int)(GetPlayPosition() - sound_->GetStart());
    else
        return 0;

#else
    if (sound_ && position_)
        return (int)(GetPlayPosition() - sound_->GetStart());
    else
        return 0;
#endif
}

#ifdef URHO3D_USE_OPENAL
int SoundSource::GetStreamBufferSize() const
{
    return STREAM_WANTED_SECONDS * sizeof(audio_t) * frequency_ * soundStream_->GetSampleSize();
}
#endif

void SoundSource::Update(float timeStep)
{
    if (!audio_ || (!IsEnabledEffective() && node_ != nullptr))
        return;

    // If there is no actual audio output, perform fake mixing into a nonexistent buffer to check stopping/looping
#ifdef URHO3D_USE_OPENAL
    if(!audio_->IsInitialized())
    {
        MixNull(timeStep);
    }
    else
    {
        // Manual OpenAL attenuation and stream updating
        alSourcef(alsource_, AL_GAIN, gain_ * masterGain_ * attenuation_);
        if(soundStream_)
        {
            UpdateStream();
        }
    }
#else
    if (!audio_->IsInitialized())
        MixNull(timeStep);
#endif

    // Free the stream if playback has stopped
#ifdef URHO3D_USE_OPENAL
    if (soundStream_ && !IsPlaying())
        StopLockless();
#else
    if (soundStream_ && !position_)
        StopLockless();
#endif

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

#ifndef URHO3D_USE_OPENAL
void SoundSource::Mix(int dest[], unsigned samples, int mixRate, bool stereo, bool interpolation)
{
    if (!position_ || (!sound_ && !soundStream_) || (!IsEnabledEffective() && node_ != nullptr))
        return;

    int streamFilledSize, outBytes;

    if (soundStream_ && streamBuffer_)
    {
        int streamBufferSize = streamBuffer_->GetDataSize();
        // Calculate how many bytes of stream sound data is needed
        auto neededSize = (int)((float)samples * frequency_ / (float)mixRate);
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
            if (stereo)
                MixMonoToStereoIP(sound, dest, samples, mixRate);
            else
                MixMonoToMonoIP(sound, dest, samples, mixRate);
        }
        else
        {
            if (stereo)
                MixMonoToStereo(sound, dest, samples, mixRate);
            else
                MixMonoToMono(sound, dest, samples, mixRate);
        }
    }
    else
    {
        if (interpolation)
        {
            if (stereo)
                MixStereoToStereoIP(sound, dest, samples, mixRate);
            else
                MixStereoToMonoIP(sound, dest, samples, mixRate);
        }
        else
        {
            if (stereo)
                MixStereoToStereo(sound, dest, samples, mixRate);
            else
                MixStereoToMono(sound, dest, samples, mixRate);
        }
    }

    // Update the time position. In stream mode, copy unused data back to the beginning of the stream buffer
    if (soundStream_)
    {
        timePosition_ += ((float)samples / (float)mixRate) * frequency_ / soundStream_->GetFrequency();

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

#endif

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
#ifndef URHO3D_USE_OPENAL
        streamBuffer_.Reset();
#endif
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


#ifdef URHO3D_USE_OPENAL

static ALenum GetALSoundFormat(Sound* sound)
{
    if(sound->IsSixteenBit() && sound->IsStereo())
    {
        return AL_FORMAT_STEREO16;
    }
    else if(sound->IsSixteenBit())
    {
        return AL_FORMAT_MONO16;
    }
    else if(sound->IsStereo())
    {
        return AL_FORMAT_STEREO8;
    }
    else
    {
        return AL_FORMAT_MONO8;	
    }
}

void SoundSource::PlayLockless(Sound* sound)
{
    timePosition_ = 0.0f;

    if (sound)
    {
        StopLockless();

        if (!sound->IsCompressed())
        {
            if(audio_->IsInitialized())
            {
                if(sound->IsLooped())
                {	
                    alSourcei(alsource_, AL_LOOPING, AL_TRUE);	
                }
                else
                {
                    alSourcei(alsource_, AL_LOOPING, AL_FALSE);
                }

                // We simply load the full sound into a buffer and pass it to OpenAl
                alGenBuffers(1, &albuffer_);
                ALenum format = GetALSoundFormat(sound);
                alBufferData(albuffer_, format, sound->GetStart(), sound->GetDataSize(), sound->GetIntFrequency());
                alSourcei(alsource_, AL_BUFFER, albuffer_);
                alSourcePlay(alsource_);
                sound_ = sound;
                return;
            }
            else
            {
                position_ = sound->GetStart();
                sound_ = sound;
                return;
            }
        }
        else
        {
            if(audio_->IsInitialized())
            {
                // Looping is manually implemented for streams
                alSourcei(alsource_, AL_LOOPING, AL_FALSE);
                sound_ = sound;
                PlayLockless(sound->GetDecoderStream());
                return;
            }
            else
            {

            }
        }
    }

    // If sound pointer is null or if sound has no data, stop playback
    StopLockless();
    sound_.Reset();
}

void SoundSource::PlayLockless(const SharedPtr<SoundStream>& stream)
{
    if (stream)
    {
        soundStream_ = stream;
        soundStream_->SetStopAtEnd(!sound_->IsLooped());
        streamFinished_ = false;
        targetBuffer_ = 0; 
        // Streaming uses multi buffering

        buffer = (audio_t*)malloc(GetStreamBufferSize()); 

        // We load all buffers initially
        alGenBuffers(OPENAL_STREAM_BUFFERS, alstreamBuffers_);
        for(int i = 0; i < OPENAL_STREAM_BUFFERS; i++)
        {
            LoadBuffer();
        }
        alSourceQueueBuffers(alsource_, OPENAL_STREAM_BUFFERS, alstreamBuffers_);
        alSourcePlay(alsource_);

        return;
    }

    // If stream pointer is null, stop playback
    StopLockless();
}

void SoundSource::UpdateStream() 
{
    int status;
    alGetSourcei(alsource_, AL_SOURCE_STATE, &status);

    int processed;
    // Prevent audio from stopping on the case of lag
    if(status == AL_STOPPED && !streamFinished_)
    {
        processed = OPENAL_STREAM_BUFFERS;
    }
    else
    {
        alGetSourcei(alsource_, AL_BUFFERS_PROCESSED, &processed);
        consumedBuffers_ += processed;
    }

    for(int i = 0; i < processed; i++)
    {
        uint32_t unqueued;
        alSourceUnqueueBuffers(alsource_, 1, &unqueued);
        LoadBuffer();
        if(streamFinished_)
        {
            break;
        }
        alSourceQueueBuffers(alsource_, 1, &unqueued);
    }

    if(status == AL_STOPPED && !streamFinished_)
    {
        alSourcePlay(alsource_);
    }
}   

void SoundSource::LoadBuffer() 
{
    uint32_t target = alstreamBuffers_[targetBuffer_];
    int needed = GetStreamBufferSize();
    int produced = soundStream_->GetData(buffer, needed);
   
    // Fill empty space with 0s to avoid trash data
    // Only happens on non-looping streams
    if (produced < needed)
    {
        memset(buffer + produced, 0, (size_t)(needed - produced));
        streamFinished_ = true;
    }
    
    // Load the audio to OpenAL
    alBufferData(target, GetALSoundFormat(sound_), buffer, needed, sound_->GetIntFrequency());

    targetBuffer_++;
    if(targetBuffer_ >= OPENAL_STREAM_BUFFERS)
    {
        targetBuffer_ = 0;
    }
}

#else

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

        streamBuffer_ = context_->CreateObject<Sound>();
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

#endif



void SoundSource::StopLockless()
{
#ifdef URHO3D_USE_OPENAL
    alSourceStop(alsource_);
    if(soundStream_)
    {
        int queued;
        alGetSourcei(alsource_, AL_BUFFERS_QUEUED, &queued);
        // Unqueue all buffers
        for(int i = 0; i < queued; i++)
        {
            uint32_t _;
            alSourceUnqueueBuffers(alsource_, 1, &_);
        }
        alDeleteBuffers(OPENAL_STREAM_BUFFERS, alstreamBuffers_);
        free(buffer);
    }
    else if(sound_)
    {
        alDeleteBuffers(1, &albuffer_);
    }
#else
    position_ = nullptr;
    timePosition_ = 0.0f;
#endif
    // Free the sound stream and decode buffer if a stream was playing
    soundStream_.Reset();
    sound_.Reset();
#ifndef URHO3D_USE_OPENAL
    streamBuffer_.Reset();
#endif
}

void SoundSource::SetPlayPositionLockless(audio_t* pos)
{
    // Setting position on a stream is not supported
    if (!sound_ || soundStream_)
        return;

    audio_t* start = sound_->GetStart();
    audio_t* end = sound_->GetEnd();
    if (pos < start)
        pos = start;
    if (sound_->IsSixteenBit() && (pos - start) & 1u)
        ++pos;
    if (pos > end)
        pos = end;

#ifdef URHO3D_USE_OPENAL
    ALint rel_pos = pos - start;
    alSourcei(alsource_, AL_BYTE_OFFSET, rel_pos);
#else
    position_ = pos;
    timePosition_ = ((float)(int)(size_t)(pos - sound_->GetStart())) / (sound_->GetSampleSize() * sound_->GetFrequency());
#endif
}


#ifndef URHO3D_USE_OPENAL

void SoundSource::MixMonoToMono(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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
                *dest = *dest + (*pos * vol) / 256;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (*pos * vol) / 256;
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
                *dest = *dest + *pos * vol;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + *pos * vol;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }
    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereo(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = (int)((-panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    auto rightVol = (int)((panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    if (!leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixMonoToMonoIP(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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
                *dest = *dest + (GET_IP_SAMPLE() * vol) / 256;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE() * vol) / 256;
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
                *dest = *dest + GET_IP_SAMPLE() * vol;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (samples--)
            {
                *dest = *dest + GET_IP_SAMPLE() * vol;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereoIP(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = (int)((-panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    auto rightVol = (int)((panning_ + 1.0f) * (256.0f * totalGain + 0.5f));
    if (!leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixStereoToMono(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixStereoToStereo(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixStereoToMonoIP(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixStereoToStereoIP(Sound* sound, int dest[], unsigned samples, int mixRate)
{
    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt(256.0f * totalGain);
    if (!vol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    float add = frequency_ / (float)mixRate;
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

void SoundSource::MixZeroVolume(Sound* sound, unsigned samples, int mixRate)
{
    float add = frequency_ * (float)samples / (float)mixRate;
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

#endif

void SoundSource::MixNull(float timeStep)
{
    if (!position_ || !sound_ || !IsEnabledEffective())
        return;

    // Advance only the time position
    timePosition_ += timeStep * frequency_ / sound_->GetFrequency();

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

}
