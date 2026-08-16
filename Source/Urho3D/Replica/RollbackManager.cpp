// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RollbackManager.h"

#include "SnapshotBuffer.h"

namespace Urho3D
{

RollbackManager::RollbackManager(unsigned capacity)
{
    SetCapacity(capacity);
}

void RollbackManager::SetCapacity(unsigned capacity)
{
    capacity_ = Max(capacity, 1u);
    while (inputs_.size() > capacity_)
        inputs_.erase(inputs_.begin());
    while (states_.size() > capacity_)
        states_.erase(states_.begin());
}

void RollbackManager::Clear()
{
    inputs_.clear();
    states_.clear();
}

void RollbackManager::RecordInput(const RollbackInput& input)
{
    for (RollbackInput& current : inputs_)
    {
        if (current.frame == input.frame)
        {
            current = input;
            return;
        }
    }

    inputs_.push_back(input);
    while (inputs_.size() > capacity_)
        inputs_.erase(inputs_.begin());
}

void RollbackManager::SaveState(NetworkFrame frame, const StringVariantMap& state)
{
    for (NetworkSnapshot& current : states_)
    {
        if (current.frame == frame)
        {
            current.values = state;
            return;
        }
    }

    states_.push_back({frame, static_cast<float>(static_cast<long long>(frame)), state});
    while (states_.size() > capacity_)
        states_.erase(states_.begin());
}

const StringVariantMap* RollbackManager::FindState(NetworkFrame frame) const
{
    for (const NetworkSnapshot& state : states_)
    {
        if (state.frame == frame)
            return &state.values;
    }
    return nullptr;
}

bool RollbackManager::Reconcile(NetworkFrame authoritativeFrame, const StringVariantMap& authoritativeState,
    const RollbackSimulator& simulator, StringVariantMap& correctedState)
{
    correctedState = authoritativeState;
    SaveState(authoritativeFrame, correctedState);

    for (const RollbackInput& input : inputs_)
    {
        if (input.frame <= authoritativeFrame)
            continue;
        if (!simulator || !simulator(input.values, correctedState))
            return false;
        SaveState(input.frame, correctedState);
    }
    return true;
}

} // namespace Urho3D
