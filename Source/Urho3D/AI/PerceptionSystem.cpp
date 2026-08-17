// SPDX-License-Identifier: MIT

#include "PerceptionSystem.h"

#include <algorithm>

namespace Urho3D
{

bool PerceptionSystem::AddStimulus(const PerceptionStimulus& stimulus, ea::string* error)
{
    if (stimulus.id.empty())
    {
        if (error)
            *error = "Perception stimulus id cannot be empty.";
        return false;
    }
    if (stimulus.ttl <= 0.0f || stimulus.strength < 0.0f)
    {
        if (error)
            *error = "Perception stimulus TTL must be positive and strength cannot be negative.";
        return false;
    }
    for (PerceptionStimulus& existing : stimuli_)
    {
        if (existing.id == stimulus.id)
        {
            existing = stimulus;
            existing.age = 0.0f;
            Notify(existing);
            return true;
        }
    }
    PerceptionStimulus inserted = stimulus;
    inserted.age = 0.0f;
    stimuli_.push_back(inserted);
    Notify(stimuli_.back());
    return true;
}

bool PerceptionSystem::RemoveStimulus(const ea::string& id)
{
    const auto iter = std::find_if(stimuli_.begin(), stimuli_.end(),
        [&](const PerceptionStimulus& stimulus) { return stimulus.id == id; });
    if (iter == stimuli_.end())
        return false;
    stimuli_.erase(iter);
    return true;
}

void PerceptionSystem::Clear()
{
    stimuli_.clear();
}

unsigned PerceptionSystem::Update(float deltaSeconds)
{
    if (deltaSeconds < 0.0f)
        return 0;
    for (PerceptionStimulus& stimulus : stimuli_)
        stimulus.age += deltaSeconds;
    const unsigned oldSize = stimuli_.size();
    stimuli_.erase(std::remove_if(stimuli_.begin(), stimuli_.end(),
        [](const PerceptionStimulus& stimulus) { return stimulus.age >= stimulus.ttl; }), stimuli_.end());
    return oldSize - stimuli_.size();
}

bool PerceptionSystem::IsPerceived(const PerceptionStimulus& stimulus) const
{
    if (!stimulus.successfullySensed || stimulus.age >= stimulus.ttl)
        return false;
    if (stimulus.sense == PerceptionSense::Damage)
        return true;
    const float radius = stimulus.sense == PerceptionSense::Sight ? sightRadius_ : hearingRadius_;
    return (stimulus.position - observerPosition_).LengthSquared() <= radius * radius;
}

ea::vector<PerceptionStimulus> PerceptionSystem::Query(PerceptionSense sense) const
{
    ea::vector<PerceptionStimulus> result;
    for (const PerceptionStimulus& stimulus : stimuli_)
    {
        if (stimulus.sense == sense && IsPerceived(stimulus))
            result.push_back(stimulus);
    }
    std::sort(result.begin(), result.end(), [](const PerceptionStimulus& lhs, const PerceptionStimulus& rhs)
    {
        if (lhs.strength != rhs.strength)
            return lhs.strength > rhs.strength;
        if (lhs.age != rhs.age)
            return lhs.age < rhs.age;
        return lhs.id < rhs.id;
    });
    return result;
}

unsigned PerceptionSystem::BindOnStimulus(ea::function<void(const PerceptionStimulus&)> callback)
{
    if (!callback)
        return 0;
    const unsigned id = nextCallbackId_++;
    callbacks_[id] = ea::move(callback);
    return id;
}

bool PerceptionSystem::UnbindOnStimulus(unsigned callbackId)
{
    return callbacks_.erase(callbackId) != 0;
}

void PerceptionSystem::Notify(const PerceptionStimulus& stimulus)
{
    ea::vector<unsigned> ids;
    ids.reserve(callbacks_.size());
    for (const auto& entry : callbacks_)
        ids.push_back(entry.first);
    std::sort(ids.begin(), ids.end());
    for (unsigned id : ids)
    {
        const auto iter = callbacks_.find(id);
        if (iter != callbacks_.end() && iter->second)
            iter->second(stimulus);
    }
}

} // namespace Urho3D
