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

#include <EASTL/unique_ptr.h>
#include <EASTL/hash_set.h>

#include "../Audio/AudioDefs.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"

namespace Urho3D
{

class AudioImpl;
class Microphone;
class Sound;
class SoundListener;
class SoundSource;

/// %Audio subsystem.
class URHO3D_API Audio : public Object
{
    URHO3D_OBJECT(Audio, Object);

public:
    /// Construct.
    explicit Audio(Context* context);
    /// Destruct. Terminate the audio thread and free the audio buffer.
    ~Audio() override;

    /// Initialize sound output with specified buffer length and output mode.
    bool SetMode(int bufferLengthMSec, int mixRate, SpeakerMode mode, bool interpolation = true);
    /// Re-initialize sound output with same parameters.
    bool RefreshMode();
    /// Shutdown this audio device, likely because we've lost it.
    void Close();
    /// Run update on sound sources. Not required for continued playback, but frees unused sound sources & sounds and updates 3D positions.
    void Update(float timeStep);
    /// Restart sound output.
    bool Play();
    /// Suspend sound output.
    void Stop();
    /// Set master gain on a specific sound type such as sound effects, music or voice.
    /// @property
    void SetMasterGain(const ea::string& type, float gain);
    /// Pause playback of specific sound type. This allows to suspend e.g. sound effects or voice when the game is paused. By default all sound types are unpaused.
    void PauseSoundType(const ea::string& type);
    /// Resume playback of specific sound type.
    void ResumeSoundType(const ea::string& type);
    /// Resume playback of all sound types.
    void ResumeAll();
    /// Set active sound listener for 3D sounds.
    /// @property
    void SetListener(SoundListener* listener);
    /// Stop any sound source playing a certain sound clip.
    void StopSound(Sound* sound);

    /// Return byte size of one sample.
    /// @property
    unsigned GetSampleSize() const { return sampleSize_; }

    /// Return mixing rate.
    /// @property
    int GetMixRate() const { return mixRate_; }

    /// Return millseconds of buffer length.
    /// @property
    unsigned GetBufferLengthMS() const { return bufferLengthMSec_; }

    /// Return whether output is interpolated.
    /// @property
    bool GetInterpolation() const { return interpolation_; }

    /// Return mode of output.
    /// @property
    SpeakerMode GetSpeakerMode() const { return speakerMode_; }

    /// Return whether audio is being output.
    /// @property
    bool IsPlaying() const { return playing_; }

    /// Return whether an audio stream has been reserved.
    /// @property
    bool IsInitialized() const { return deviceID_ != 0; }

    /// Return master gain for a specific sound source type. Unknown sound types will return full gain (1).
    /// @property
    float GetMasterGain(const ea::string& type) const;

    /// Return whether specific sound type has been paused.
    bool IsSoundTypePaused(const ea::string& type) const;

    /// Return active sound listener.
    /// @property
    SoundListener* GetListener() const;

    /// Return all sound sources.
    const ea::vector<SoundSource*>& GetSoundSources() const { return soundSources_; }

    /// Return whether the specified master gain has been defined.
    bool HasMasterGain(const ea::string& type) const { return masterGain_.contains(type); }

    /// Add a sound source to keep track of. Called by SoundSource.
    void AddSoundSource(SoundSource* soundSource);
    /// Remove a sound source. Called by SoundSource.
    void RemoveSoundSource(SoundSource* soundSource);

    /// Return audio thread mutex.
    Mutex& GetMutex() { return audioMutex_; }

    /// Return sound type specific gain multiplied by master gain.
    float GetSoundSourceMasterGain(StringHash typeHash) const;

    /// Mix sound sources into the buffer.
    void MixOutput(void* dest, unsigned samples);

    /// Returns a pretty-name list of all attached microphones.
    StringVector EnumerateMicrophones() const;
    /// Constructs a microphone from a pretty-name (found via EnumerateMicrophones()).
    SharedPtr<Microphone> CreateMicrophone(const ea::string& name, bool forSpeechRecog, unsigned wantedFreq, unsigned silenceLevelLimit = 0);
    /// Disables a microphone that has been lost.
    void CloseMicrophoneForLoss(unsigned which);

private:
    /// Handle render update event.
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Stop sound output and release the sound buffer.
    void Release();
    /// Actually update sound sources with the specific timestep. Called internally.
    void UpdateInternal(float timeStep);

    /// Clipping buffer for mixing.
    ea::unique_ptr<int[]> clipBuffer_;
    /// Audio thread mutex.
    Mutex audioMutex_;
    /// SDL audio device ID.
    unsigned deviceID_{};
    /// Sample size.
    unsigned sampleSize_{};
    /// Clip buffer size in samples.
    unsigned fragmentSize_{};
    /// Clip buffer size in milliseconds.
    unsigned bufferLengthMSec_{};
    /// Mixing rate.
    int mixRate_{};
    /// Mixing interpolation flag.
    bool interpolation_{};
    /// Speaker configuration.
    SpeakerMode speakerMode_{SpeakerMode::SPK_AUTO};
    /// Playing flag.
    bool playing_{};
    /// Master gain by sound source type.
    ea::unordered_map<StringHash, Variant> masterGain_;
    /// Paused sound types.
    ea::hash_set<StringHash> pausedSoundTypes_;
    /// Sound sources.
    ea::vector<SoundSource*> soundSources_;
    /// Sound listener.
    WeakPtr<SoundListener> listener_;
    /// List of microphones being tracked.
    ea::vector< WeakPtr<Microphone> > microphones_;
};

/// Register Audio library objects.
/// @nobind
void URHO3D_API RegisterAudioLibrary(Context* context);

}
