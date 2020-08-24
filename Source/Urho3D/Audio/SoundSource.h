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

#pragma once

#include "../Audio/AudioDefs.h"
#include "../Scene/Component.h"

namespace Urho3D
{

class Audio;
class Sound;
class SoundStream;

/// Compressed audio decode buffer length in milliseconds.
static const int STREAM_BUFFER_LENGTH = 100;

/// %Sound source component with stereo position. A sound source needs to be created to a node to be considered "enabled" and be able to play, however that node does not need to belong to a scene.
class URHO3D_API SoundSource : public Component
{
    URHO3D_OBJECT(SoundSource, Component);

public:
    /// Construct.
    explicit SoundSource(Context* context);
    /// Destruct. Remove self from the audio subsystem.
    ~SoundSource() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Seek to time.
    void Seek(float seekTime);
    /// Play a sound.
    void Play(Sound* sound);
    /// Play a sound with specified frequency.
    void Play(Sound* sound, float frequency);
    /// Play a sound with specified frequency and gain.
    void Play(Sound* sound, float frequency, float gain);
    /// Play a sound with specified frequency, gain and panning.
    void Play(Sound* sound, float frequency, float gain, float panning);
    /// Start playing a sound stream.
    void Play(SoundStream* stream);
    /// Stop playback.
    void Stop();
    /// Pause playback
    /// It will only be resumed after calling Resume() or if 
    /// Audio->ResumeAudioType(...) is called with the matching sound type
    void Pause();
    /// Resume playback
    void Resume();
    /// Set sound type, determines the master gain group.
    void SetSoundType(const ea::string& type);
    /// Set frequency.
    void SetFrequency(float frequency);
    /// Set pitch (relative speed).
    void SetPitch(float pitch);
    /// Set gain. 0.0 is silence, 1.0 is full volume.
    void SetGain(float gain);
    /// Set attenuation. 1.0 is unaltered. Used for distance attenuated playback.
    void SetAttenuation(float attenuation);
    /// Set stereo panning. -1.0 is full left and 1.0 is full right.
    /// Only implemented for 2D sources. 3D sources will ignore panning
    void SetPanning(float panning);
    /// Set to remove either the sound source component or its owner node from the scene automatically on sound playback completion. Disabled by default.
    void SetAutoRemoveMode(AutoRemoveMode mode);

    /// Return sound.
    Sound* GetSound() const { return sound_; }

    /// Return sound type, determines the master gain group.
    ea::string GetSoundType() const { return soundType_; }

    /// Return playback time position.
    float GetTimePosition() const;

    /// Return frequency.
    float GetFrequency() const { return frequency_; }

    /// Return pitch.
    float GetPitch() const;

    /// Return gain.
    float GetGain() const { return gain_; }

    /// Return attenuation.
    float GetAttenuation() const { return attenuation_; }

    /// Return stereo panning.
    /// Only implemented for 2D sources. 3D sources will ignore panning
    float GetPanning() const { return panning_; }

    /// Return automatic removal mode on sound playback completion.
    AutoRemoveMode GetAutoRemoveMode() const { return autoRemove_; }

    /// Returns whether the source has a sound loaded and in progress.
    /// Note: This function returns true even if the sound is paused
    bool IsPlaying() const;

    /// Returns wether the source is actually playing sound
    bool IsSoundPlaying() const;

    /// Returns wether the audio is paused.
    bool IsPaused() const { return paused_; };

    /// Update the sound source. Perform subclass specific operations. Called by Audio.
    virtual void Update(float timeStep);
    #ifndef URHO3D_USE_OPENAL
    /// Mix sound source output to a 32-bit clipping buffer. Called by Audio.
    void Mix(int dest[], unsigned samples, int mixRate, bool stereo, bool interpolation);
    #endif
    
    /// Update the effective master gain. Called internally and by Audio when the master gain changes.
    void UpdateMasterGain();

    /// Set sound attribute.
    void SetSoundAttr(const ResourceRef& value);
    /// Set sound position attribute.
    void SetPositionAttr(int value);
    /// Return sound attribute.
    ResourceRef GetSoundAttr() const;
    /// Set sound playing attribute.
    void SetPlayingAttr(bool value);
    /// Return sound position attribute.
    int GetPositionAttr() const;

protected:
    /// Audio subsystem.
    WeakPtr<Audio> audio_;
    /// SoundSource type, determines the master gain group.
    ea::string soundType_;
    /// SoundSource type hash.
    StringHash soundTypeHash_;
    /// Frequency.
    float frequency_;
    /// Gain.
    float gain_;
    /// Attenuation.
    float attenuation_;
    /// Stereo panning.
    float panning_;
    /// Effective master gain.
    float masterGain_{};
    /// Whether finished event should be sent on playback stop.
    bool sendFinishedEvent_;
    /// Automatic removal mode.
    AutoRemoveMode autoRemove_;
    /// Is the audio paused (either by Pause() or Audio's subsystem pause sound type)
    bool paused_;

    // Choose reasonable values to avoid audio lag and prevent
    // too much data from being loaded at once
    static constexpr int OPENAL_STREAM_BUFFERS = 5;
    static constexpr float STREAM_WANTED_SECONDS = 0.1f;
    
    uint32_t alsource_;
    uint32_t albuffer_;
    uint32_t alstreamBuffers_[OPENAL_STREAM_BUFFERS];
    int targetBuffer_;
    bool streamFinished_;
    int consumedBuffers_;

    int GetStreamBufferSize() const; 
    int GetStreamSampleCount() const;

private:
    /// Play a sound. Called internally.
    void PlayInternal(Sound* sound);
    /// Play a sound stream. Called internally.
    void PlayInternal(const SharedPtr<SoundStream>& stream);
    /// Stop sound. Called internally.
    void StopInternal();
    /// Set new playback position. Called internally.
    void SetPlayPosition(int pos);

    void UpdateStream(bool reload = false);
    bool LoadBuffer();
    audio_t* buffer_;
    /// Advance playback pointer to simulate audio playback in headless mode.
    void MixNull(float timeStep);

    /// Sound that is being played.
    SharedPtr<Sound> sound_;
    /// Sound stream that is being played.
    SharedPtr<SoundStream> soundStream_;

    // These are used by the null playback
    int position_;
    /// Playback time position.
    float timePosition_;
};

}
