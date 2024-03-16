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

#pragma once

#include "../Scene/Component.h"

#include <phonon.h>

namespace Urho3D
{

class SteamAudio;
class Sound;
class SoundStream;

/// %Sound source component with stereo position. A sound source needs to be created to a node to be considered "enabled" and be able to play, however that node does not need to belong to a scene.
class URHO3D_API SteamSoundSource : public Component
{
    URHO3D_OBJECT(SteamSoundSource, Component);

public:
    /// Construct.
    explicit SteamSoundSource(Context* context);
    /// Destruct. Remove self from the audio subsystem.
    ~SteamSoundSource() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Play a sound.
    void Play(Sound *sound);

    /// Return whether is playing.
    /// @property
    bool IsPlaying() const;

    /// Set playing attribute.
    void SetPlayingAttr(bool playing);

    /// Set sound attribute.
    void SetSoundAttr(const ResourceRef& value);
    /// Return sound attribute.
    ResourceRef GetSoundAttr() const;

    /// Generate sound.
    IPLAudioBuffer *GenerateAudioBuffer(float gain);

private:
    /// Handle render update event.
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle transform change.
    void OnMarkedDirty(Node *) override;

    /// Mark effects dirty.
    void MarkEffectsDirty() { effectsDirty_ = true; }

    /// Recreate effects
    void UpdateEffects();
    /// Destroy effects
    void UnlockedDestroyEffects();
    /// Update simulation inputs.
    void UpdateSourcePosition();

    /// Steam audio subsystem.
    WeakPtr<SteamAudio> audio_;
    /// Currently playing sound.
    SharedPtr<Sound> sound_;
    /// Binaural effect.
    IPLBinauralEffect binauralEffect_;
    /// Direct effect.
    IPLDirectEffect directEffect_;
    /// Sound source.
    IPLSource source_;
    /// Last simulator outputs.
    IPLSimulationOutputs simulatorOutputs_;
    /// Mutex for effects.
    Mutex effectsMutex_;
    /// Audio gain.
    float gain_;
    /// Is playback paused?
    bool paused_;
    /// Will playback loop?
    bool loop_;
    /// Enable binaural effect?
    bool binaural_;
    /// Enable distance attenuation.
    bool distanceAttenuation_;
    /// Enable air absorption.
    bool airAbsorption_;
    /// Enable occlusion.
    bool occlusion_;
    /// Enable transmission.
    bool transmission_;
    /// Binaural spatial blend.
    float binauralSpatialBlend_;
    /// Bilinear interpolation for binaural effect.
    bool binauralBilinearInterpolation_;
    /// Playback position.
    unsigned frame_;
    // Are the effects loaded?
    bool effectsLoaded_;
    /// Are effects dirty?
    bool effectsDirty_;
};

}
