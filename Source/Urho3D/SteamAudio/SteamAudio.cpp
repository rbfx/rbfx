//
// Copyright (c) 2008-2024 the Urho3D project.
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
#include "../SteamAudio/SteamSoundMesh.h"
#include "../SteamAudio/SteamSoundListener.h"
#include "../Audio/Sound.h"
#include "../IO/Log.h"
#include "../Scene/Node.h"
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"

#include <SDL.h>

namespace Urho3D
{

void SDLSteamAudioCallback(void* userdata, Uint8* stream, int);

SteamAudio::SteamAudio(Context* context) :
    Object(context), audioBufferPool_(nullptr)
{
    context_->RequireSDL(SDL_INIT_AUDIO);

    // Set the master to the default value
    masterGain_ = 1.0f;

    // Register Audio library object factories
    RegisterSteamAudioLibrary(context_);

    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(SteamAudio, HandleRenderUpdate));
}

SteamAudio::~SteamAudio()
{
    Release();
    context_->ReleaseSDL();
}

bool SteamAudio::SetMode(int mixRate, SpeakerMode mode)
{
    // Clean up first
    Release();

    // Reset master gain
    masterGain_ = 1.0f;

    // Convert speaker mode to channel count
    switch (mode) {
    case SPK_MONO: channelCount_ = 1; break;
    case SPK_AUTO:
    case SPK_STEREO: channelCount_ = 2; break;
    case SPK_QUADROPHONIC: channelCount_ = 4; break;
    case SPK_SURROUND_5_1: channelCount_ = 6; break;
    }

    // Create context
    IPLContextSettings contextSettings {
        .version = STEAMAUDIO_VERSION,
        #ifndef NDEBUG
        .flags = IPL_CONTEXTFLAGS_VALIDATION
        #endif
    };
    iplContextCreate(&contextSettings, &phononContext_);

    // Typical audio settings...
    audioSettings_ = IPLAudioSettings {
        .samplingRate = mixRate,
        .frameSize = 1024
    };

    // Create single frame buffer
    // This buffer one individual audio frame per channel at a time.
    finalFrameBuffer_.resize(channelCount_ * audioSettings_.frameSize);

    // Create the HRTF
    // The HRTF basically describes the "set of filters that is applied to audio in order to spatialize it"
    IPLHRTFSettings hrtfSettings {
        .type = IPL_HRTFTYPE_DEFAULT,
        .volume = 1.0f
    };
    iplHRTFCreate(phononContext_, &audioSettings_, &hrtfSettings, &hrtf_);

    // Create the scene
    IPLSceneSettings sceneSettings {
        .type = IPL_SCENETYPE_DEFAULT
    };
    iplSceneCreate(phononContext_, &sceneSettings, &scene_);

    // Create the simulator
    IPLSimulationSettings simulationSettings {
        .flags = static_cast<IPLSimulationFlags>(IPL_SIMULATIONFLAGS_DIRECT | IPL_SIMULATIONFLAGS_REFLECTIONS),
        .sceneType = IPL_SCENETYPE_DEFAULT,
        .reflectionType = IPL_REFLECTIONEFFECTTYPE_CONVOLUTION,
        .maxNumOcclusionSamples = 12,
        .maxNumRays = 16384,
        .numDiffuseSamples = 8, //TODO: No idea about this, find a good default value
        .maxDuration = 4.0f,
        .maxOrder = 8,
        .maxNumSources = 16, //TODO: This should dynamically increase if limit is reached
        .numThreads = 3,
        .numVisSamples = 8, //TODO: No idea about this, find a good default value
        .samplingRate = audioSettings_.samplingRate,
        .frameSize = audioSettings_.frameSize
    };
    iplSimulatorCreate(phononContext_, &simulationSettings, &simulator_);
    iplSimulatorSetScene(simulator_, scene_);
    MarkSimulatorDirty();

    // Allocate an output buffer
    // That buffer is "deinterleaved", which means that it's actually one buffer for each channel
    phononFrameBuffer_ = IPLAudioBuffer {};
    iplAudioBufferAllocate(phononContext_, channelCount_, audioSettings_.frameSize, &phononFrameBuffer_);

    // Create the buffer pool
    audioBufferPool_ = ea::make_unique<SteamAudioBufferPool>(this);

    // Set up SDL
    SDL_AudioSpec spec {
        .freq = audioSettings_.samplingRate,
        .format = AUDIO_F32,
        .channels = 2,
        .samples = static_cast<unsigned short>(audioSettings_.frameSize),
        .callback = *SDLSteamAudioCallback,
        .userdata = this
    };

    // Open the audio channel
    if (SDL_OpenAudio(&spec, NULL) < 0) {
        URHO3D_LOGERROR(ea::string("Failed to open SDL2 audio: ")+SDL_GetError());
        return false;
    }

    // Start playing audio
    audioMutex_.Release();
    Play();

    // Update once
    Update(0.0f);

    return true;
}

bool SteamAudio::RefreshMode()
{
    return SetMode(audioSettings_.samplingRate, GetSpeakerMode());
}

void SteamAudio::Close()
{
    Release();
}

void SteamAudio::Update(float timeStep)
{
    if (sceneDirty_) {
        iplSceneCommit(scene_);
        iplSceneSaveOBJ(scene_, "scene-base.obj");
        sceneDirty_ = false;
    }
    if (simulatorDirty_) {
        iplSimulatorCommit(simulator_);
        simulatorDirty_ = false;
    }

    // Update listener coordinates in simulator
    if (listener_) {
        const auto lUp = listener_->GetNode()->GetWorldUp();
        const auto lDir = listener_->GetNode()->GetWorldDirection();
        const auto lRight = listener_->GetNode()->GetWorldRight();
        const auto lPos = listener_->GetNode()->GetWorldPosition();
        sharedInputs_.listener = {
            .right = {lRight.x_, lRight.y_, lRight.z_},
            .up = {lUp.x_, lUp.y_, lUp.z_},
            .ahead = {lDir.x_, lDir.y_, lDir.z_},
            .origin = {lPos.x_, lPos.y_, lPos.z_}
        },
        iplSimulatorSetSharedInputs(simulator_, SimulationFlags(), &sharedInputs_);

        // Run simulations
        {
            MutexLock Lock(simulatorMutex_);
            iplSimulatorRunDirect(simulator_);
        }
        {
            MutexLock Lock(simulatorMutex_);
            iplSimulatorRunReflections(simulator_);
        }
    }
}

void SteamAudio::Play()
{
    SDL_PauseAudio(0);
}

void SteamAudio::Stop()
{
    SDL_PauseAudio(1);
}

void SteamAudio::SetMasterGain(float gain)
{
    masterGain_ = gain;
}

void SteamAudio::SetListener(SteamSoundListener *listener)
{
    MutexLock Lock(audioMutex_);
    listener_ = listener;
}

void SteamAudio::SetReflectionSimulationActive(bool active)
{
    simulateReflections_ = active;
}

void SteamAudio::SetImpulseResponseDuration(float duration)
{
    sharedInputs_.duration = duration;
}

bool SteamAudio::GetSimulatorOutputs(IPLSource source, IPLSimulationOutputs& outputs) noexcept
{
    if (!simulatorMutex_.TryAcquire())
        return false;
    outputs = IPLSimulationOutputs {};
    iplSourceGetOutputs(source, SimulationFlags(), &outputs);
    simulatorMutex_.Release();
    return true;
}

float SteamAudio::GetMasterGain() const
{
    return masterGain_;
}

SpeakerMode SteamAudio::GetSpeakerMode() const
{
    switch (channelCount_) {
    case 1: return SPK_MONO;
    case 2: return SPK_STEREO;
    case 4: return SPK_QUADROPHONIC;
    case 6: return SPK_SURROUND_5_1;
    default: return SPK_AUTO;
    }
}

SteamSoundListener *SteamAudio::GetListener() const {
    if (!(listener_ && listener_->IsEnabledEffective()))
        return nullptr;
    return listener_;
}

void SteamAudio::AddSoundSource(SteamSoundSource *soundSource)
{
    MutexLock Lock(audioMutex_);
    soundSources_.push_back(soundSource);
}

void SteamAudio::RemoveSoundSource(SteamSoundSource *soundSource)
{
    MutexLock Lock(audioMutex_);
    auto i = soundSources_.find(soundSource);
    if (i != soundSources_.end())
    {
        MutexLock lock(audioMutex_);
        soundSources_.erase(i);
    }
}

void SteamAudio::MixOutput(float *dest) noexcept
{
    if (!audioMutex_.TryAcquire())
        return;

    // Stop if no listener
    if (!GetListener()) {
        memset(dest, 0, audioSettings_.frameSize*channelCount_*sizeof(float));
        return;
    }

    // Clear frame buffer
    for (unsigned channel = 0; channel != phononFrameBuffer_.numChannels; channel++)
        for (unsigned sample = 0; sample != phononFrameBuffer_.numSamples; sample++)
            phononFrameBuffer_.data[channel][sample] = 0.0f;

    // Iterate over all sound sources
    for (auto source : soundSources_) {
        // Skip disabled ones
        if (!source->IsEnabledEffective())
            continue;

        // Generate audio buffer
        auto audioBuffer = source->GenerateAudioBuffer(masterGain_);

        // Skip if none generated
        if (!audioBuffer)
            continue;

        // Mix into frame buffer
        iplAudioBufferMix(phononContext_, audioBuffer, &phononFrameBuffer_);
    }

    // Interleave into our buffer
    iplAudioBufferInterleave(phononContext_, &phononFrameBuffer_, dest);

    audioMutex_.Release();
}

IPLSimulationFlags SteamAudio::SimulationFlags() const
{
    int fres = IPL_SIMULATIONFLAGS_DIRECT;
    if (simulateReflections_)
        fres |= IPL_SIMULATIONFLAGS_REFLECTIONS;
    return static_cast<IPLSimulationFlags>(fres);
}

void SteamAudio::HandleRenderUpdate(StringHash eventType, VariantMap &eventData)
{
    using namespace RenderUpdate;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void SteamAudio::Release()
{
    iplSimulatorRelease(&simulator_);
    iplAudioBufferFree(phononContext_, &phononFrameBuffer_);
    iplHRTFRelease(&hrtf_);
    iplContextRelease(&phononContext_);
    audioBufferPool_.reset();
    finalFrameBuffer_.clear();
    SDL_CloseAudio();
    audioMutex_.Acquire();
}

void SDLSteamAudioCallback(void* userdata, Uint8 *stream, int)
{
    auto* audio = static_cast<SteamAudio*>(userdata);
    {
        audio->MixOutput(reinterpret_cast<float*>(stream));
    }
}


SteamAudioBufferPool::SteamAudioBufferPool(SteamAudio *audio) : audio_(audio), bufferIdx_(0)
{
    const auto phononContext = audio_->GetPhononContext();
    const auto& audioSettings = audio_->GetAudioSettings();
    const auto channelCount = audio_->GetChannelCount();
    for (auto& buffer : buffers_)
        iplAudioBufferAllocate(phononContext, channelCount, audioSettings.frameSize, &buffer);
}

SteamAudioBufferPool::~SteamAudioBufferPool()
{
    const auto phononContext = audio_->GetPhononContext();
    for (auto& buffer : buffers_)
        iplAudioBufferFree(phononContext, &buffer);
}

IPLAudioBuffer *SteamAudioBufferPool::GetCurrentBuffer()
{
    return &buffers_[bufferIdx_];
}

IPLAudioBuffer *SteamAudioBufferPool::GetNextBuffer()
{
    return &buffers_[GetNextBufferIndex()];
}

void SteamAudioBufferPool::SwitchToNextBuffer()
{
    bufferIdx_ = GetNextBufferIndex();
}

unsigned int SteamAudioBufferPool::GetNextBufferIndex() const
{
    auto bufferIdx = bufferIdx_;
    if (++bufferIdx == buffers_.size())
        bufferIdx = 0;
    return bufferIdx;
}


void RegisterSteamAudioLibrary(Context* context)
{
    Sound::RegisterObject(context);
    SteamSoundListener::RegisterObject(context);
    SteamSoundSource::RegisterObject(context);
    SteamSoundMesh::RegisterObject(context);
}

}
