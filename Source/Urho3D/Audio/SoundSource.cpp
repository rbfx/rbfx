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

#include <AL/al.h>
#include <AL/alc.h>

namespace Urho3D
{

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
    position_(-1),
    timePosition_(0.0f),
    targetBuffer_(0),
    consumedBuffers_(0),
    streamFinished_(false),
    paused_(false)
{
    audio_ = GetSubsystem<Audio>();

    if (audio_)
        audio_->AddSoundSource(this);

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
    // Set to valid range
    if(!sound_)
        return;

    seekTime = Clamp(seekTime, 0.0f, sound_->GetLength());
    SetPositionAttr((int)(seekTime * (sound_->GetFrequency())));
}

void SoundSource::Play(Sound* sound)
{
    if (!audio_)
        return;


    PlayInternal(sound);
    
    // If no frequency set yet, set from the sound's default
    if (frequency_ == 0.0f && sound)
        SetFrequency(sound->GetFrequency());

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
    Play(sound);
    SetFrequency(frequency);
}

void SoundSource::Play(Sound* sound, float frequency, float gain)
{
    SetGain(gain);
    Play(sound);
    SetFrequency(frequency);
}

void SoundSource::Play(Sound* sound, float frequency, float gain, float panning)
{
    SetGain(gain);
    SetPanning(panning);
    Play(sound);
    SetFrequency(frequency);
}

void SoundSource::Play(SoundStream* stream)
{
    if (!audio_)
        return;


    SharedPtr<SoundStream> streamPtr(stream);

    // When stream playback is explicitly requested, clear the existing sound if any
    sound_.Reset();
    PlayInternal(streamPtr);
    
    // If no frequency set yet, set from the stream's default
    if (frequency_ == 0.0f && stream)
        SetFrequency(stream->GetFrequency());

    // Stream playback is not supported for network replication, no need to mark network dirty
}

void SoundSource::Stop()
{
    if (!audio_)
        return;

    StopInternal();
    MarkNetworkUpdate();
}

void SoundSource::Pause() 
{
    alSourcePause(alsource_);
    paused_ = true;
}

void SoundSource::Resume() 
{
    int status;
    alGetSourcei(alsource_, AL_SOURCE_STATE, &status);
    if(status == AL_PAUSED)
    {
        alSourcePlay(alsource_);
    }
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
    frequency_ = Max(frequency, 0.0f);
    if(audio_->IsInitialized())
    {
        float pitch;
        if(sound_)
        {
            pitch = frequency_ / sound_->GetFrequency();
        }
        else if(soundStream_)
        {
            pitch = frequency_ / soundStream_->GetFrequency();
        }
        
        alSourcef(alsource_, AL_PITCH, pitch);
    }
    MarkNetworkUpdate();
}

void SoundSource::SetPitch(float pitch) 
{
    if(sound_)
    {
        SetFrequency(sound_->GetFrequency() * pitch);
    }   
    else if(soundStream_)
    {
        SetFrequency(soundStream_->GetFrequency() * pitch);
    }
     
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
    alSource3f(alsource_, AL_POSITION, panning_, 0.0f, 0.0f);

    MarkNetworkUpdate();
}

void SoundSource::SetAutoRemoveMode(AutoRemoveMode mode)
{
    autoRemove_ = mode;
    MarkNetworkUpdate();
}

bool SoundSource::IsPlaying() const
{
    if(audio_->IsInitialized())
    {
        int val;
        alGetSourcei(alsource_, AL_SOURCE_STATE, &val);
        return val != AL_STOPPED;
    }
    else
    {
        return (sound_ || soundStream_) && position_ != -1;
    }
}

bool SoundSource::IsSoundPlaying() const
{
    if(audio_->IsInitialized())
    {
        int val;
        alGetSourcei(alsource_, AL_SOURCE_STATE, &val);
        return val == AL_PLAYING;
    }
    else
    {
        return (sound_ || soundStream_) && position_ != -1 && paused_ == false;
    }
}


float SoundSource::GetTimePosition() const
{
    if(!sound_)
        return 0.0f;

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
}

float SoundSource::GetPitch() const
{
    if(sound_)
    {
        return frequency_ / sound_->GetFrequency();
    }
    else if(soundStream_)
    {
        return frequency_ / soundStream_->GetFrequency();
    }

    return 0.0f;
}

int SoundSource::GetPositionAttr() const
{
    if(!sound_)
        return 0;

    if(!audio_->IsInitialized())
    {
        return position_;
    }
    else
    {
        int sample;
        alGetSourcei(alsource_, AL_SAMPLE_OFFSET, &sample);

        if(soundStream_)
        {
            return sample + consumedBuffers_ * GetStreamSampleCount();
        }
        else
        {
            return sample;
        }
        
        
    }
}

int SoundSource::GetStreamBufferSize() const
{
    return Max(STREAM_WANTED_SECONDS * sizeof(audio_t) * soundStream_->GetFrequency() * soundStream_->GetSampleSize(), 1);
}

int SoundSource::GetStreamSampleCount() const 
{
    return Max(STREAM_WANTED_SECONDS * soundStream_->GetFrequency(), 1);
}

void SoundSource::Update(float timeStep)
{
    if (!audio_ || (!IsEnabledEffective() && node_ != nullptr))
        return;

    // If there is no actual audio output, perform fake mixing into a nonexistent buffer to check stopping/looping
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

    bool playing = IsPlaying();

    // Free the stream if playback has stopped
    if (soundStream_ && !playing)
        StopInternal();

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
        PlayInternal(newSound);
    else
    {
        // When changing the sound and not playing, free previous sound stream and stream buffer (if any)
        soundStream_.Reset();
        sound_ = newSound;
    }
}

void SoundSource::SetPlayingAttr(bool value)
{
    if (value)
    {
        if (!IsPlaying())
            PlayInternal(sound_.Get());
    }
    else
        Stop();
}

void SoundSource::SetPositionAttr(int value)
{
    if (sound_)
        SetPlayPosition(value);
}

ResourceRef SoundSource::GetSoundAttr() const
{
    return GetResourceRef(sound_, Sound::GetTypeStatic());
}


static ALenum GetALSoundFormat(SoundStream* sound)
{
    if(sound->IsSixteenBit() && sound->IsStereo())
        return AL_FORMAT_STEREO16;
    else if(sound->IsSixteenBit())
        return AL_FORMAT_MONO16;
    else if(sound->IsStereo())
        return AL_FORMAT_STEREO8;
    else
        return AL_FORMAT_MONO8;	
}

static ALenum GetALSoundFormat(Sound* sound)
{
    if(sound->IsSixteenBit() && sound->IsStereo())
        return AL_FORMAT_STEREO16;
    else if(sound->IsSixteenBit())
        return AL_FORMAT_MONO16;
    else if(sound->IsStereo())
        return AL_FORMAT_STEREO8;
    else
        return AL_FORMAT_MONO8;	
}

void SoundSource::PlayInternal(Sound* sound)
{
    timePosition_ = 0.0f;

    if (sound)
    {
        StopInternal();
        sound_ = sound;

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
                return;
            }
            else
            {
                position_ = 0;
                return;
            }
        }
        else
        {
            if(audio_->IsInitialized())
            {
                // Looping is manually implemented for streams
                alSourcei(alsource_, AL_LOOPING, AL_FALSE);
            }

            PlayInternal(sound->GetDecoderStream());
            return;
        }
    }

    // If sound pointer is null or if sound has no data, stop playback
    StopInternal();
    sound_.Reset();
}

void SoundSource::PlayInternal(const SharedPtr<SoundStream>& stream)
{
    if (stream)
    {
        soundStream_ = stream;
        if(sound_)
        {
            soundStream_->SetStopAtEnd(!sound_->IsLooped());
        }
        streamFinished_ = false;
        targetBuffer_ = 0; 

        if(audio_->IsInitialized())
        {
            // Streams require a buffer that depends on frequency
            if(frequency_ == 0.0f)
                frequency_ = soundStream_->GetFrequency();

            // Streaming uses multi buffering
            buffer_ = (audio_t*)malloc(GetStreamBufferSize()); 

            alGenBuffers(OPENAL_STREAM_BUFFERS, alstreamBuffers_);
            UpdateStream(true);
            alSourcePlay(alsource_);
        }
        else
        {
            position_ = 0;
            timePosition_ = 0.0f;
        }

        return;
    }

    // If stream pointer is null, stop playback
    StopInternal();
}

void SoundSource::UpdateStream(bool reload) 
{
    if(reload)
    {
        // Unqueue all buffers. This stops audio.
        alSourceStop(alsource_);

        int queued;
        alGetSourcei(alsource_, AL_BUFFERS_QUEUED, &queued);
        for(int i = 0; i < queued; i++)
        {
            uint32_t _;
            alSourceUnqueueBuffers(alsource_, 1, &_);
        }
       
        targetBuffer_ = 0;
        int loaded = 0;
        for(int i = 0; i < OPENAL_STREAM_BUFFERS; i++)
        {
            if(!LoadBuffer())  
                break; 
            loaded++;
        }
        alSourceQueueBuffers(alsource_, loaded, alstreamBuffers_);

        alSourcePlay(alsource_);

    }
    else
    {
        int status;
        alGetSourcei(alsource_, AL_SOURCE_STATE, &status);


        // Buffered sound streams may not have loaded all data
        if (!sound_ || (soundStream_ && !sound_->IsCompressed()))
        {
            int queued;
            alGetSourcei(alsource_, AL_BUFFERS_QUEUED, &queued);

            if(queued < OPENAL_STREAM_BUFFERS)
            {
                for(int i = 0; i < OPENAL_STREAM_BUFFERS - queued; i++)
                {
                    if(!LoadBuffer())
                        break;
                }
                alSourceQueueBuffers(alsource_, OPENAL_STREAM_BUFFERS - queued, &alstreamBuffers_[queued]);
            }
        }
        
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

            // Looping timekeeping
            if(sound_ && sound_->IsLooped())
            {
                int total_buffers = ceil(sound_->GetLength() / STREAM_WANTED_SECONDS);
                if(consumedBuffers_ >= total_buffers)
                {
                    consumedBuffers_ = 0;
                }
            }
        }

        for(int i = 0; i < processed; i++)
        {    
            uint32_t unqueued;
            alSourceUnqueueBuffers(alsource_, 1, &unqueued);
            if(!LoadBuffer())
            {
                alSourceQueueBuffers(alsource_, 1, &unqueued);
                break;
            }
            if(streamFinished_)
            {
                break;
            }
            alSourceQueueBuffers(alsource_, 1, &unqueued);
        }

        if(status == AL_STOPPED && !streamFinished_)
        {
            alSourcePlay(alsource_);
            consumedBuffers_ += OPENAL_STREAM_BUFFERS;
        }
    }
    
}   

bool SoundSource::LoadBuffer() 
{
    uint32_t target = alstreamBuffers_[targetBuffer_];
    int needed = GetStreamBufferSize();
    int produced = soundStream_->GetData(buffer_, needed);
   
    // Fill empty space with 0s to avoid trash data
    // Only happens on non-looping streams
    if (produced < needed - 1)
    {
        // Ignore buffered sound streams as they may not have generated
        // all sound just yet
        // TODO: Improve this? This could be a custom sound source
        if (!sound_ || (soundStream_ && !sound_->IsCompressed()))
            return false;

        memset(buffer_ + produced, 0, (size_t)(needed - produced));
        streamFinished_ = true;
    }
    
    // Load the audio to OpenAL
    alBufferData(target, GetALSoundFormat(soundStream_), buffer_, needed, soundStream_->GetIntFrequency());

    targetBuffer_++;
    if(targetBuffer_ >= OPENAL_STREAM_BUFFERS)
    {
        targetBuffer_ = 0;
    }

    return true;
}

void SoundSource::StopInternal()
{
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
        free(buffer_);
        buffer_ = nullptr;
    }
    else if(sound_)
    {
        alDeleteBuffers(1, &albuffer_);
    }
    // Free the sound stream and decode buffer if a stream was playing
    soundStream_.Reset();
    sound_.Reset();
}

void SoundSource::SetPlayPosition(int pos)
{
    // Ignore buffered sound stream
    if (!audio_ || !sound_ || (soundStream_ && !sound_->IsCompressed()))
        return;
        
    if (pos < 0)
        pos = 0;

    if (sound_->IsSixteenBit() && (pos) & 1u)
        ++pos;

    if(soundStream_)
    {
        int end = (int)ceil(sound_->GetLength() * sound_->GetFrequency());
        if(pos > end)
            pos = end;

        if(audio_->IsInitialized())
        {
            if (soundStream_->Seek(pos))
            {
                // We must also reload all buffers, otherwise a delay will occur
                UpdateStream(true);
                consumedBuffers_ = (int)ceil(pos / GetStreamSampleCount());
            }
        }
    }
    else
    {
        int end = sound_->GetEnd() - sound_->GetStart();
        if (pos > end)
            pos = end;

        if(audio_->IsInitialized())
        {
            alSourcei(alsource_, AL_SAMPLE_OFFSET, pos);
        }
    }
    
    position_ = pos;
    timePosition_ = ((float)(int)(size_t)(pos) / (sound_->GetFrequency()));

}


void SoundSource::MixNull(float timeStep)
{
    if (position_ == -1 || !sound_ || !IsEnabledEffective())
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
            position_ = -1;
            timePosition_ = 0.0f;
        }
    }
}

}
