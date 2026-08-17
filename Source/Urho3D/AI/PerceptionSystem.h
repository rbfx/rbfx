// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Math/Vector3.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class PerceptionSense
{
    Sight,
    Hearing,
    Damage
};

/// A sensory stimulus retained by PerceptionSystem until its TTL expires.
struct URHO3D_API PerceptionStimulus
{
    ea::string id;
    PerceptionSense sense{PerceptionSense::Sight};
    ea::string source;
    Vector3 position{Vector3::ZERO};
    float strength{1.0f};
    float age{};
    float ttl{1.0f};
    bool successfullySensed{true};
};

/// Runtime perception memory for sight, hearing and damage stimuli.
class URHO3D_API PerceptionSystem
{
public:
    bool AddStimulus(const PerceptionStimulus& stimulus, ea::string* error = nullptr);
    bool RemoveStimulus(const ea::string& id);
    void Clear();
    unsigned Update(float deltaSeconds);

    void SetObserverPosition(const Vector3& position) { observerPosition_ = position; }
    const Vector3& GetObserverPosition() const { return observerPosition_; }
    void SetSightRadius(float radius) { sightRadius_ = Max(radius, 0.0f); }
    void SetHearingRadius(float radius) { hearingRadius_ = Max(radius, 0.0f); }
    float GetSightRadius() const { return sightRadius_; }
    float GetHearingRadius() const { return hearingRadius_; }

    bool IsPerceived(const PerceptionStimulus& stimulus) const;
    ea::vector<PerceptionStimulus> Query(PerceptionSense sense) const;
    const ea::vector<PerceptionStimulus>& GetStimuli() const { return stimuli_; }
    unsigned BindOnStimulus(ea::function<void(const PerceptionStimulus&)> callback);
    bool UnbindOnStimulus(unsigned callbackId);

private:
    void Notify(const PerceptionStimulus& stimulus);

    Vector3 observerPosition_;
    float sightRadius_{50.0f};
    float hearingRadius_{25.0f};
    ea::vector<PerceptionStimulus> stimuli_;
    ea::unordered_map<unsigned, ea::function<void(const PerceptionStimulus&)>> callbacks_;
    unsigned nextCallbackId_{1};
};

} // namespace Urho3D
