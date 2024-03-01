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
#include "../SteamAudio/SteamSoundListener.h"
#include "../Audio/Sound.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../Scene/SceneEvents.h"

#include "../DebugNew.h"

#include <phonon.h>

namespace Urho3D
{

SteamSoundSource::SteamSoundSource(Context* context) :
    Component(context), sound_(nullptr), binauralEffect_(nullptr), directEffect_(nullptr), source_(nullptr), gain_(1.0f), paused_(false), loop_(false), binaural_(false), distanceAttenuation_(false), airAbsorption_(false), occlusion_(false), transmission_(false), binauralSpatialBlend_(1.0f), binauralBilinearInterpolation_(false), effectsLoaded_(false)
{
    audio_ = GetSubsystem<SteamAudio>();

    if (audio_)
        // Add this sound source
        audio_->AddSoundSource(this);
}

SteamSoundSource::~SteamSoundSource()
{
    if (audio_)
        // Remove this sound source
        audio_->RemoveSoundSource(this);
}

void SteamSoundSource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SteamSoundSource>(Category_Audio);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gain", float, gain_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Loop", bool, loop_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Binaural", bool, binaural_, UpdateEffects, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Binaural Spacial Blend", float, binauralSpatialBlend_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Binaural Bilinear Interpolation", bool, binauralBilinearInterpolation_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Distance Attenuation", bool, distanceAttenuation_, UpdateEffects, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Air absorption", bool, airAbsorption_, UpdateEffects, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Occlusion", bool, occlusion_, UpdateEffects, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Transmission", bool, transmission_, UpdateEffects, false, AM_DEFAULT);
}

void SteamSoundSource::Play(Sound *sound)
{
    // Reset current frame (playback position)
    frame_ = 0;

    // Set sound
    sound_ = sound;

    // Update effects
    UpdateEffects();
}

bool SteamSoundSource::IsPlaying() const
{
    const auto& audioSettings = audio_->GetAudioSettings();
    return sound_ && !paused_;
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

IPLAudioBuffer *SteamSoundSource::GenerateAudioBuffer(float gain)
{
    // Return nothing if not playing
    if (!IsPlaying())
        return nullptr;

    // Get phonon context and audio settings
    const auto phononContext = audio_->GetPhononContext();
    const auto hrtf = audio_->GetHRTF();
    const auto& audioSettings = audio_->GetAudioSettings();
    auto& pool = audio_->GetAudioBufferPool();

    // Calculate size of one full interleaved frame
    const auto fullFrameSize = audioSettings.frameSize*(sound_->IsStereo()?2:1);

    // Stop or reset if file ends before end of frame
    if ((frame_ + 1)*fullFrameSize*(sound_->IsSixteenBit()?2:1) > sound_->GetDataSize()) {
        if (loop_)
            frame_ = 0;
        else
            return nullptr;
    }

    // Convert sound data to interleaved float buffer
    ea::vector<float> rawInputBuffer(fullFrameSize);
    for (unsigned sample = 0; sample != fullFrameSize; sample++) {
        // Convert to float32
        if (sound_->IsSixteenBit()) {
            int16_t integerData = reinterpret_cast<const int16_t*>(sound_->GetData().get())[frame_*fullFrameSize + sample];
            rawInputBuffer[sample] = float(integerData)/32767.0f;
        } else {
            int8_t integerData = sound_->GetData()[frame_*fullFrameSize + sample];
            rawInputBuffer[sample] = float(integerData)/128.f;
        }
        // Apply gain
        rawInputBuffer[sample] *= gain_*gain;
    }

    // Get listener node
    auto listener = audio_->GetListener()->GetNode();

    // Get positions
    const auto lPos = listener->GetWorldPosition();
    const auto lDir = listener->GetWorldDirection();
    const auto lUp = listener->GetWorldUp();
    const auto sPos = GetNode()->GetWorldPosition();
    const IPLVector3 psPos {sPos.x_, sPos.y_, sPos.z_};
    const IPLVector3 plPos {lPos.x_, lPos.y_, lPos.z_};

    // Deinterleave sound data into buffer
    iplAudioBufferDeinterleave(phononContext, rawInputBuffer.data(), pool.GetCurrentBuffer());

    // Apply binaural effect
    if (binaural_) {
        IPLBinauralEffectParams binauralEffectParams {
            .interpolation = binauralBilinearInterpolation_?IPL_HRTFINTERPOLATION_BILINEAR:IPL_HRTFINTERPOLATION_NEAREST,
            .spatialBlend = binauralSpatialBlend_,
            .hrtf = hrtf
        };
        binauralEffectParams.direction = iplCalculateRelativeDirection(phononContext, psPos, plPos, {lDir.x_, lDir.y_, lDir.z_}, {lUp.x_, lUp.y_, lUp.z_});
        binauralEffectParams.direction.x = -binauralEffectParams.direction.x; // Why is this required?
        iplBinauralEffectApply(binauralEffect_, &binauralEffectParams, pool.GetCurrentBuffer(), pool.GetNextBuffer());
        pool.SwitchToNextBuffer();
    }

    // Create direct effect properties
    IPLDirectEffectParams directEffectParams {};

    // Apply simulation results
    if (source_)
        directEffectParams = audio_->GetSimulatorOutputs(source_).direct;

    // Apply distance attenuation
    if (distanceAttenuation_) {
        IPLDistanceAttenuationModel distanceAttenuationModel {
            .type = IPL_DISTANCEATTENUATIONTYPE_DEFAULT
        };

        directEffectParams.flags = static_cast<IPLDirectEffectFlags>(directEffectParams.flags | IPL_DIRECTEFFECTFLAGS_APPLYDISTANCEATTENUATION);
        directEffectParams.distanceAttenuation = iplDistanceAttenuationCalculate(phononContext, psPos, plPos, &distanceAttenuationModel);
    }

    // Apply air absorption
    if (airAbsorption_) {
        IPLAirAbsorptionModel airAbsorptionModel {
            .type = IPL_AIRABSORPTIONTYPE_DEFAULT
        };

        directEffectParams.flags = static_cast<IPLDirectEffectFlags>(directEffectParams.flags | IPL_DIRECTEFFECTFLAGS_APPLYAIRABSORPTION);
        iplAirAbsorptionCalculate(phononContext, psPos, plPos, &airAbsorptionModel, directEffectParams.airAbsorption);
    }

    // Apply direct effect
    if (directEffectParams.flags) {
        iplDirectEffectApply(directEffect_, &directEffectParams, pool.GetCurrentBuffer(), pool.GetNextBuffer());
        pool.SwitchToNextBuffer();
    }

    // Increment to next frame
    frame_++;

    // Don't process any further for now, just return that buffer as is...
    return pool.GetCurrentBuffer();
}

void SteamSoundSource::UpdateEffects()
{
    DestroyEffects();

    if (!sound_)
        return;

    const auto phononContext = audio_->GetPhononContext();
    const auto& audioSettings = audio_->GetAudioSettings();

    if (binaural_) {
        // Create binaural effect
        IPLBinauralEffectSettings binauralEffectSettings {
            .hrtf = audio_->GetHRTF()
        };
        iplBinauralEffectCreate(phononContext, const_cast<IPLAudioSettings*>(&audioSettings), &binauralEffectSettings, &binauralEffect_);
    }
    if (occlusion_ || transmission_) {
        IPLSourceSettings sourceSettings {
            .flags = IPL_SIMULATIONFLAGS_DIRECT
        };
        iplSourceCreate(audio_->GetSimulator(), &sourceSettings, &source_);
        iplSourceAdd(source_, audio_->GetSimulator());
        audio_->MarkSimulatorDirty();

        SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(SteamSoundSource, UpdateSimulator));
    }
    if (airAbsorption_ || distanceAttenuation_ || source_) {
        // Create direct effect
        IPLDirectEffectSettings directEffectSettings {
            .numChannels = sound_->IsStereo()?2:1
        };
        iplDirectEffectCreate(phononContext, const_cast<IPLAudioSettings*>(&audioSettings), &directEffectSettings, &directEffect_);
    }
}

void SteamSoundSource::DestroyEffects()
{
    if (!effectsLoaded_)
        return;

    if (binauralEffect_)
        iplBinauralEffectRelease(&binauralEffect_);
    if (directEffect_)
        iplDirectEffectRelease(&directEffect_);
    if (source_) {
        iplSourceRemove(source_, audio_->GetSimulator());
        audio_->MarkSimulatorDirty();
        iplSourceRelease(&source_);

        UnsubscribeFromEvent(E_SCENEUPDATE);
    }
}

void SteamSoundSource::UpdateSimulator(StringHash eventType, VariantMap &eventData)
{
    if (!source_)
        return;

    const auto lUp = GetNode()->GetWorldUp();
    const auto lDir = GetNode()->GetWorldDirection();
    const auto lRight = GetNode()->GetWorldRight();
    const auto lPos = GetNode()->GetWorldPosition();

    IPLSimulationInputs inputs {
        .flags = IPL_SIMULATIONFLAGS_DIRECT,
        .directFlags = static_cast<IPLDirectSimulationFlags>((occlusion_?IPL_DIRECTSIMULATIONFLAGS_OCCLUSION:0) | (transmission_?IPL_DIRECTSIMULATIONFLAGS_TRANSMISSION:0)),
        .source = {
            .right = {lRight.x_, lRight.y_, lRight.z_},
            .up = {lUp.x_, lUp.y_, lUp.z_},
            .ahead = {lDir.x_, lDir.y_, lDir.z_},
            .origin = {lPos.x_, lPos.y_, lPos.z_}
        },
        .occlusionType = IPL_OCCLUSIONTYPE_RAYCAST
    };
    iplSourceSetInputs(source_, IPL_SIMULATIONFLAGS_DIRECT, &inputs);
}

}
