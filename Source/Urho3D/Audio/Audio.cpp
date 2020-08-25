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

#include "../Scene/Node.h"
#include "../Audio/Audio.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundListener.h"
#include "../Audio/SoundSource3D.h"
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Profiler.h"
#include "../IO/Log.h"

#include <SDL/SDL.h>

#include "../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

#include <AL/al.h>
#include <AL/alc.h>

namespace Urho3D
{

void _ALERROR()
{
    ALenum error = alGetError();
    if(error != AL_NO_ERROR)
    {
        switch(error)
        {
        case AL_INVALID_NAME:
            URHO3D_LOGERROR("AL_INVALID_NAME");
            break;
        case AL_INVALID_ENUM:
            URHO3D_LOGERROR("AL_INVALID_ENUM");
            break;
        case AL_INVALID_VALUE:
            URHO3D_LOGERROR("AL_INVALID_VALUE");
            break;
        case AL_INVALID_OPERATION:
            URHO3D_LOGERROR("AL_INVALID_OPERATION");
            break;
        case AL_OUT_OF_MEMORY:
            URHO3D_LOGERROR("AL_OUT_OF_MEMORY");
            break;
        default:
            URHO3D_LOGERROR("UNKNOWN AL ERROR");
        }
    }

}

const char* AUDIO_CATEGORY = "Audio";

static const int MIN_BUFFERLENGTH = 20;
static const int MIN_MIXRATE = 11025;
static const int MAX_MIXRATE = 48000;
static const StringHash SOUND_MASTER_HASH("Master");

Audio::Audio(Context* context) :
    Object(context)
{

    // Set the master to the default value
    masterGain_[SOUND_MASTER_HASH] = 1.0f;

    // Register Audio library object factories
    RegisterAudioLibrary(context_);

    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Audio, HandleRenderUpdate));
}

Audio::~Audio()
{
    Release();
}

bool Audio::SetMode(int bufferLengthMSec, int mixRate, bool stereo, bool interpolation)
{
    Release();

    // We simply create the mode and ignore all paramaters
    // because they cannot be configured in OpenAL
    // TODO: Better handling? OpenAL offers some customization possibilities


    ALCdevice *aldevice;
    aldevice = alcOpenDevice(NULL);
    if(!aldevice)
    {
        URHO3D_LOGERROR("Could not create OpenAL device");
        return false;
    }

    ALint attribs[4] = { 0 };

    ALCcontext *alcontext;
    alcontext = alcCreateContext(aldevice, nullptr);
    if(!alcontext)
    {
        URHO3D_LOGERROR("Could not create OpenAL context");
        return false;
    }

    if(!alcMakeContextCurrent(alcontext))
    {
        URHO3D_LOGERROR("Could not make OpenAL context current context");
        return false;
    }
    
    isInitialized_ = true;
    // We disable attenuation as we implement it using gain
    alDistanceModel(AL_NONE);
    URHO3D_LOGINFO("OpenAL context created");
    
    return Play();
}

void Audio::Update(float timeStep)
{
    if (!playing_ && IsInitialized())
        return;

    UpdateInternal(timeStep);
}

bool Audio::Play()
{
    if (playing_)
        return true;

    if (!IsInitialized())
    {
        URHO3D_LOGERROR("No audio mode set, can not start playback");
        return false;
    }

    // Update sound sources before resuming playback to make sure 3D positions are up to date
    UpdateInternal(0.0f);

    playing_ = true;
    return true;
}

void Audio::Stop()
{
    playing_ = false;
}

void Audio::SetMasterGain(const ea::string& type, float gain)
{
    masterGain_[type] = Clamp(gain, 0.0f, 1.0f);

    for (auto i = soundSources_.begin(); i != soundSources_.end(); ++i)
        (*i)->UpdateMasterGain();
    
}

void Audio::PauseSoundType(const ea::string& type)
{
    for (auto i = soundSources_.begin(); i != soundSources_.end(); ++i)
    {
        auto* ip = (*i);
        if(ip->GetSoundType() == type)
        {
            ip->Pause();
        }
    }

    pausedSoundTypes_.insert(type);
}

void Audio::ResumeSoundType(const ea::string& type)
{
    for (auto i = soundSources_.begin(); i != soundSources_.end(); ++i)
    {
        auto* ip = (*i);
        if(ip->GetSoundType() == type)
        {
            ip->Resume();
        }
    }
    pausedSoundTypes_.erase(type);
    // Update sound sources before resuming playback to make sure 3D positions are up to date
    // Done under mutex to ensure no mixing happens before we are ready
    UpdateInternal(0.0f);
}

void Audio::ResumeAll()
{
    pausedSoundTypes_.clear();
    UpdateInternal(0.0f);
}

void Audio::SetListener(SoundListener* listener)
{
    listener_ = listener;
}

void Audio::StopSound(Sound* sound)
{
    for (auto i = soundSources_.begin(); i != soundSources_.end(); ++i)
    {
        if ((*i)->GetSound() == sound)
            (*i)->Stop();
    }
}

float Audio::GetMasterGain(const ea::string& type) const
{
    // By definition previously unknown types return full volume
    auto findIt = masterGain_.find(type);
    if (findIt == masterGain_.end())
        return 1.0f;

    return findIt->second.GetFloat();
}

bool Audio::IsSoundTypePaused(const ea::string& type) const
{
    return pausedSoundTypes_.contains(type);
}

SoundListener* Audio::GetListener() const
{
    return listener_;
}

void Audio::AddSoundSource(SoundSource* soundSource)
{
    soundSources_.push_back(soundSource);
}

void Audio::RemoveSoundSource(SoundSource* soundSource)
{
    auto i = soundSources_.find(soundSource);
    if (i != soundSources_.end())
    {
        soundSources_.erase(i);
    }
}

float Audio::GetSoundSourceMasterGain(StringHash typeHash) const
{
    auto masterIt = masterGain_.find(SOUND_MASTER_HASH);

    if (!typeHash)
        return masterIt->second.GetFloat();

    auto typeIt = masterGain_.find(typeHash);

    if (typeIt == masterGain_.end() || typeIt == masterIt)
        return masterIt->second.GetFloat();

    return masterIt->second.GetFloat() * typeIt->second.GetFloat();
}

void Audio::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace RenderUpdate;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Audio::Release()
{
    Stop();

    if(IsInitialized())
    {
        ALCcontext* context = alcGetCurrentContext();
        ALCdevice* device = alcGetContextsDevice(context);
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }
}

void Audio::UpdateInternal(float timeStep)
{
    URHO3D_PROFILE("UpdateAudio");

    // Update the listener position if any
    if(listener_)
    {
        Vector3 pos = listener_->GetNode()->GetPosition();
        alListener3f(AL_POSITION, pos.x_, pos.y_, -pos.z_);
        Vector3 at = listener_->GetNode()->GetWorldDirection();
        Vector3 up = listener_->GetNode()->GetWorldUp();
        float orientation[6] = {at.x_, at.y_, -at.z_, up.x_, up.y_, -up.z_};
        alListenerfv(AL_ORIENTATION, orientation);
    }

    // Update in reverse order, because sound sources might remove themselves
    for (unsigned i = soundSources_.size() - 1; i < soundSources_.size(); --i)
    {
        SoundSource* source = soundSources_[i];

        // Check for pause if necessary; do not update paused sound sources
        if (!pausedSoundTypes_.empty())
        {
            if (pausedSoundTypes_.contains(source->GetSoundType()))
                continue;
        }

        source->Update(timeStep);
    }
}

void RegisterAudioLibrary(Context* context)
{
    Sound::RegisterObject(context);
    SoundSource::RegisterObject(context);
    SoundSource3D::RegisterObject(context);
    SoundListener::RegisterObject(context);
}

}
