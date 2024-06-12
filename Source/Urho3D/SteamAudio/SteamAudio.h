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

#pragma once

#include "../SteamAudio/SteamAudioDefs.h"
#include "../Core/Object.h"

#include <phonon.h>

namespace Urho3D
{

class SteamAudioBufferPool;
class SteamSoundListener;
class SteamSoundSource;
class SteamSoundMesh;
class SteamSoundBufferPool;

/// %Audio subsystem.
class URHO3D_API SteamAudio : public Object
{
    URHO3D_OBJECT(SteamAudio, Object);

public:
    /// Construct.
    explicit SteamAudio(Context* context);
    /// Destruct. Terminate the audio thread and free the audio buffer.
    ~SteamAudio() override;

    /// Initialize sound output with specified buffer length and output mode.
    bool SetMode(int mixRate, SpeakerMode mode);
    /// Re-initialize sound output with same parameters.
    bool RefreshMode();
    /// Shutdown this audio device, likely because we've lost it.
    void Close();
    /// Run update on sound sources. Required for continued playback.
    void Update(float timeStep);
    /// Restart sound output.
    void Play();
    /// Suspend sound output.
    void Stop();
    /// Set master gain.
    /// @property
    void SetMasterGain(float gain);
    /// Set active sound listener for 3D sounds.
    /// @property
    void SetListener(SteamSoundListener* listener);

    /// Set reflection simulation active state.
    void SetReflectionSimulationActive(bool active);
    /// Returns reflection simulation active state.
    bool ReflectionSimulationActive() const { return simulateReflections_; }
    /// Set impulse response duration.
    void SetImpulseResponseDuration(float duration = 2.0f);
    /// Returns impulse response duration.
    float ImpulseResponseDuration() const { return sharedInputs_.duration; }

    /// Return phonon context.
    IPLContext GetPhononContext() const { return phononContext_; }
    /// Return HRTF.
    IPLHRTF GetHRTF() const { return hrtf_; }
    /// Return scene.
    IPLScene GetScene() const { return scene_; }
    /// Return phonon audio settings.
    const IPLAudioSettings& GetAudioSettings() const { return audioSettings_; }
    /// Return audio buffer pool.
    SteamAudioBufferPool& GetAudioBufferPool() { return *audioBufferPool_; }
    /// Return simulator.
    IPLSimulator GetSimulator() { return simulator_; }
    /// Return simulator outputs.
    bool GetSimulatorOutputs(IPLSource source, IPLSimulationOutputs& outputs) noexcept;
    /// Return channel count.
    /// @property
    unsigned GetChannelCount() const { return channelCount_; }
    /// Return byte size of one frame.
    /// @property
    unsigned GetFrameSize() const { return audioSettings_.frameSize; }

    /// Return master gain for a specific sound source type. Unknown sound types will return full gain (1).
    /// @property
    float GetMasterGain() const;

    /// Return mode of output.
    /// @property
    SpeakerMode GetSpeakerMode() const;

    /// Return active sound listener.
    /// @property
    SteamSoundListener* GetListener() const;

    /// Mark scene dirty (after changes)
    void MarkSceneDirty() { sceneDirty_ = true; }
    /// Mark scene simulator (after changes)
    void MarkSimulatorDirty() { simulatorDirty_ = true; }

    /// Return all sound sources.
    const ea::vector<SteamSoundSource*>& GetSoundSources() const { return soundSources_; }
    /// Add a sound source to keep track of. Called by SteamSoundSource.
    void AddSoundSource(SteamSoundSource* soundSource);
    /// Remove a sound source. Called by SteamSoundSource.
    void RemoveSoundSource(SteamSoundSource* soundSource);

    /// Return audio thread mutex.
    Mutex& GetMutex() { return audioMutex_; }

    /// Mix sound sources into the buffer.
    void MixOutput(float* dest);

private:
    /// Returns simulation flags.
    IPLSimulationFlags SimulationFlags() const;

    /// Handle render update event.
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Stop sound output and release the sound buffer.
    void Release();

    /// Phonon context.
    IPLContext phononContext_{};
    /// Phonon simulator.
    IPLSimulator simulator_{};
    /// Phonon audio settings.
    IPLAudioSettings audioSettings_{};
    /// Phonon HRTF.
    IPLHRTF hrtf_{};
    /// Phonon final output frame buffer.
    IPLAudioBuffer phononFrameBuffer_{};
    /// Phonon scene.
    IPLScene scene_{};
    /// Simulation inputs.
    IPLSimulationSharedInputs sharedInputs_{{}, 4096, 16, 2.0f, 1, 1.0f};
    /// Is reflection simulation active?
    bool simulateReflections_{};
    /// Is phonon scene dirty?
    bool sceneDirty_{};
    /// Is simulator dirty?
    bool simulatorDirty_{};
    /// Interleaved output frame buffer for SDL.
    ea::vector<float> finalFrameBuffer_{};
    /// Audio thread mutex.
    Mutex audioMutex_;
    /// Simulator mutex.
    Mutex simulatorMutex_;
    /// Channel count
    unsigned channelCount_{};
    /// Master gain.
    float masterGain_{};
    /// Sound sources.
    ea::vector<SteamSoundSource*> soundSources_;
    /// Sound listener.
    WeakPtr<SteamSoundListener> listener_;
    /// Audio buffer pool.
    ea::unique_ptr<SteamAudioBufferPool> audioBufferPool_;
};

/// %Audio buffer pool.
class SteamAudioBufferPool {
    SteamAudio *audio_;

public:
    SteamAudioBufferPool(SteamAudio* audio);
    ~SteamAudioBufferPool();

    IPLAudioBuffer *GetCurrentBuffer();
    IPLAudioBuffer *GetNextBuffer();
    void SwitchToNextBuffer();

private:
    /// Returns next buffer index.
    unsigned GetNextBufferIndex() const;

    /// Current buffer index.
    unsigned bufferIdx_;
    /// Different audio buffers for processing.
    ea::array<IPLAudioBuffer, 4> buffers_;
};

/// Register Audio library objects.
/// @nobind
void URHO3D_API RegisterSteamAudioLibrary(Context* context);

}
