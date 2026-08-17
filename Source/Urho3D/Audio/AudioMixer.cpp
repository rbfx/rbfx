// SPDX-License-Identifier: MIT

#include "AudioMixer.h"
#include <Urho3D/Math/MathDefs.h>

#include <algorithm>
#include <cmath>

namespace Urho3D
{

bool AudioMixer::AddBus(const AudioBus& bus)
{
    if (bus.name.empty() || GetBus(bus.name) || (!bus.parent.empty() && !GetBus(bus.parent)))
        return false;
    AudioBus normalized = bus;
    normalized.volume = std::max(0.0f, normalized.volume);
    buses_.push_back(normalized);
    meters_[normalized.name] = {};
    return true;
}

bool AudioMixer::RemoveBus(const ea::string& name)
{
    if (name == "Master")
        return false;
    const unsigned index = GetBusIndex(name);
    if (index == M_MAX_UNSIGNED)
        return false;
    for (const AudioBus& bus : buses_)
    {
        if (bus.parent == name)
            return false;
    }
    for (const AudioVoice& voice : voices_)
    {
        if (voice.bus == name)
            return false;
    }
    buses_.erase(buses_.begin() + index);
    meters_.erase(name);
    return true;
}

AudioBus* AudioMixer::GetBus(const ea::string& name)
{
    const unsigned index = GetBusIndex(name);
    return index != M_MAX_UNSIGNED ? &buses_[index] : nullptr;
}

const AudioBus* AudioMixer::GetBus(const ea::string& name) const
{
    const unsigned index = GetBusIndex(name);
    return index != M_MAX_UNSIGNED ? &buses_[index] : nullptr;
}

bool AudioMixer::SetBusVolume(const ea::string& name, float volume)
{
    AudioBus* bus = GetBus(name);
    if (!bus)
        return false;
    bus->volume = std::max(0.0f, volume);
    return true;
}

bool AudioMixer::SetBusMuted(const ea::string& name, bool muted)
{
    AudioBus* bus = GetBus(name);
    if (!bus)
        return false;
    bus->muted = muted;
    return true;
}

bool AudioMixer::SetBusSoloed(const ea::string& name, bool soloed)
{
    AudioBus* bus = GetBus(name);
    if (!bus)
        return false;
    bus->soloed = soloed;
    return true;
}

bool AudioMixer::AddEffect(const ea::string& busName, const AudioDspEffect& effect)
{
    AudioBus* bus = GetBus(busName);
    if (!bus)
        return false;
    AudioDspEffect normalized = effect;
    normalized.mix = std::max(0.0f, std::min(1.0f, normalized.mix));
    bus->effects.push_back(normalized);
    return true;
}

bool AudioMixer::AddVoice(const AudioVoice& voice)
{
    if (voice.id.empty() || GetVoice(voice.id) || !GetBus(voice.bus))
        return false;
    AudioVoice normalized = voice;
    normalized.volume = std::max(0.0f, normalized.volume);
    normalized.pitch = std::max(0.01f, normalized.pitch);
    normalized.distanceAttenuation = std::max(0.0f, normalized.distanceAttenuation);
    normalized.pan = std::max(-1.0f, std::min(1.0f, normalized.pan));
    voices_.push_back(normalized);
    return true;
}

bool AudioMixer::RemoveVoice(const ea::string& id)
{
    const auto iter = std::find_if(voices_.begin(), voices_.end(), [&](const AudioVoice& voice) { return voice.id == id; });
    if (iter == voices_.end())
        return false;
    voices_.erase(iter);
    return true;
}

AudioVoice* AudioMixer::GetVoice(const ea::string& id)
{
    const auto iter = std::find_if(voices_.begin(), voices_.end(), [&](const AudioVoice& voice) { return voice.id == id; });
    return iter != voices_.end() ? &*iter : nullptr;
}

const AudioVoice* AudioMixer::GetVoice(const ea::string& id) const
{
    const auto iter = std::find_if(voices_.begin(), voices_.end(), [&](const AudioVoice& voice) { return voice.id == id; });
    return iter != voices_.end() ? &*iter : nullptr;
}

bool AudioMixer::SetVoiceBus(const ea::string& id, const ea::string& bus)
{
    AudioVoice* voice = GetVoice(id);
    if (!voice || !GetBus(bus))
        return false;
    voice->bus = bus;
    return true;
}

bool AudioMixer::SetVoiceVolume(const ea::string& id, float volume)
{
    AudioVoice* voice = GetVoice(id);
    if (!voice)
        return false;
    voice->volume = std::max(0.0f, volume);
    return true;
}

bool AudioMixer::SetVoicePlaying(const ea::string& id, bool playing)
{
    AudioVoice* voice = GetVoice(id);
    if (!voice)
        return false;
    voice->playing = playing;
    return true;
}

void AudioMixer::Update(float)
{
    for (const AudioBus& bus : buses_)
    {
        AudioMeter meter;
        for (const AudioVoice& voice : voices_)
        {
            if (!voice.playing || voice.bus != bus.name || !IsBusAudible(bus.name))
                continue;
            const float gain = voice.volume * voice.distanceAttenuation * GetEffectiveBusVolume(bus.name);
            meter.peak = std::max(meter.peak, gain);
            meter.rms += gain * gain;
            ++meter.activeVoices;
        }
        if (meter.activeVoices != 0)
            meter.rms = std::sqrt(meter.rms / meter.activeVoices);
        meters_[bus.name] = meter;
    }
}

float AudioMixer::GetEffectiveBusVolume(const ea::string& name) const
{
    const AudioBus* bus = GetBus(name);
    if (!bus || bus->muted)
        return 0.0f;
    if (HasSoloBus() && !IsBusAudible(name))
        return 0.0f;
    float volume = bus->volume;
    ea::string parent = bus->parent;
    unsigned guard = 0;
    while (!parent.empty() && guard++ < buses_.size())
    {
        const AudioBus* parentBus = GetBus(parent);
        if (!parentBus || parentBus->muted)
            return 0.0f;
        volume *= parentBus->volume;
        parent = parentBus->parent;
    }
    return volume;
}

AudioMeter AudioMixer::GetBusMeter(const ea::string& name) const
{
    const auto iter = meters_.find(name);
    return iter != meters_.end() ? iter->second : AudioMeter{};
}

bool AudioMixer::HasSoloBus() const
{
    for (const AudioBus& bus : buses_)
    {
        if (bus.soloed)
            return true;
    }
    return false;
}

bool AudioMixer::IsBusAudible(const ea::string& name) const
{
    const AudioBus* bus = GetBus(name);
    if (!bus)
        return false;
    if (bus->soloed)
        return true;
    for (const AudioBus& candidate : buses_)
    {
        if (candidate.soloed && IsBusAudible(bus->parent))
            return false;
    }
    return !HasSoloBus();
}

unsigned AudioMixer::GetBusIndex(const ea::string& name) const
{
    for (unsigned i = 0; i < buses_.size(); ++i)
    {
        if (buses_[i].name == name)
            return i;
    }
    return M_MAX_UNSIGNED;
}

} // namespace Urho3D
