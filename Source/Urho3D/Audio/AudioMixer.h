// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class AudioDspType
{
    LowPass,
    HighPass,
    Equalizer,
    Compressor,
    Reverb,
    Delay
};

struct URHO3D_API AudioDspEffect
{
    AudioDspType type{AudioDspType::LowPass};
    bool enabled{true};
    float mix{1.0f};
    float parameterA{};
    float parameterB{};
};

struct URHO3D_API AudioBus
{
    ea::string name;
    ea::string parent;
    float volume{1.0f};
    bool muted{};
    bool soloed{};
    ea::vector<AudioDspEffect> effects;
};

struct URHO3D_API AudioVoice
{
    ea::string id;
    ea::string bus{"Master"};
    float volume{1.0f};
    float pan{};
    float pitch{1.0f};
    float distanceAttenuation{1.0f};
    bool playing{true};
};

struct URHO3D_API AudioMeter
{
    float rms{};
    float peak{};
    unsigned activeVoices{};
};

/// Deterministic professional mixer model for buses, DSP routing and voice meters.
class URHO3D_API AudioMixer
{
public:
    bool AddBus(const AudioBus& bus);
    bool RemoveBus(const ea::string& name);
    AudioBus* GetBus(const ea::string& name);
    const AudioBus* GetBus(const ea::string& name) const;
    bool SetBusVolume(const ea::string& name, float volume);
    bool SetBusMuted(const ea::string& name, bool muted);
    bool SetBusSoloed(const ea::string& name, bool soloed);
    bool AddEffect(const ea::string& bus, const AudioDspEffect& effect);

    bool AddVoice(const AudioVoice& voice);
    bool RemoveVoice(const ea::string& id);
    AudioVoice* GetVoice(const ea::string& id);
    const AudioVoice* GetVoice(const ea::string& id) const;
    bool SetVoiceBus(const ea::string& id, const ea::string& bus);
    bool SetVoiceVolume(const ea::string& id, float volume);
    bool SetVoicePlaying(const ea::string& id, bool playing);

    void Update(float deltaSeconds);
    float GetEffectiveBusVolume(const ea::string& name) const;
    AudioMeter GetBusMeter(const ea::string& name) const;
    const ea::vector<AudioBus>& GetBuses() const { return buses_; }
    const ea::vector<AudioVoice>& GetVoices() const { return voices_; }

private:
    bool HasSoloBus() const;
    bool IsBusAudible(const ea::string& name) const;
    unsigned GetBusIndex(const ea::string& name) const;

    ea::vector<AudioBus> buses_;
    ea::vector<AudioVoice> voices_;
    ea::unordered_map<ea::string, AudioMeter> meters_;
};

} // namespace Urho3D
