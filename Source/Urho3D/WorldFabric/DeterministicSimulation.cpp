// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "DeterministicSimulation.h"

#include <algorithm>

namespace Urho3D
{

namespace
{

void HashByte(unsigned long long& hash, unsigned char value)
{
    hash ^= value;
    hash *= 1099511628211ull;
}

void HashString(unsigned long long& hash, const ea::string& value)
{
    for (unsigned char byte : value)
        HashByte(hash, byte);
    HashByte(hash, 0);
}

} // namespace

DeterministicSimulation::DeterministicSimulation(unsigned capacity)
{
    Configure(fixedDelta_, capacity);
}

void DeterministicSimulation::Configure(float fixedDelta, unsigned capacity)
{
    fixedDelta_ = fixedDelta > 0.0f ? fixedDelta : 1.0f / 60.0f;
    capacity_ = Max(2u, capacity);
    if (snapshots_.size() > capacity_)
    {
        const unsigned removeCount = snapshots_.size() - capacity_;
        snapshots_.erase(snapshots_.begin(), snapshots_.begin() + removeCount);
        if (inputs_.size() > removeCount)
            inputs_.erase(inputs_.begin(), inputs_.begin() + removeCount);
    }
}

bool DeterministicSimulation::Start(const StringVariantMap& initialState)
{
    Clear();
    state_ = initialState;
    StoreSnapshot();
    return true;
}

bool DeterministicSimulation::Advance(const StringVariantMap& input, const DeterministicStep& step)
{
    if (!step || snapshots_.empty())
        return false;
    return ApplyFrame(currentFrame_ + 1, input, step);
}

bool DeterministicSimulation::ApplyFrame(unsigned frame, const StringVariantMap& input, const DeterministicStep& step)
{
    StringVariantMap nextState = state_;
    if (!step(frame, fixedDelta_, input, state_, nextState))
        return false;

    state_ = nextState;
    currentFrame_ = frame;
    inputs_.push_back(input);
    StoreSnapshot();
    return true;
}

void DeterministicSimulation::StoreSnapshot()
{
    DeterministicSnapshot snapshot;
    snapshot.frame = currentFrame_;
    snapshot.state = state_;
    snapshot.digest = ComputeDigest(state_);
    snapshots_.push_back(snapshot);
    while (snapshots_.size() > capacity_)
        snapshots_.erase(snapshots_.begin());
    while (inputs_.size() > snapshots_.size() - 1)
        inputs_.erase(inputs_.begin());
}

bool DeterministicSimulation::Restore(unsigned frame)
{
    const DeterministicSnapshot* snapshot = FindSnapshot(frame);
    if (!snapshot)
        return false;
    state_ = snapshot->state;
    currentFrame_ = snapshot->frame;
    return true;
}

bool DeterministicSimulation::ReplayTo(unsigned targetFrame, const DeterministicStep& step)
{
    if (!step || targetFrame < currentFrame_)
        return false;

    unsigned frame = currentFrame_ + 1;
    while (frame <= targetFrame)
    {
        const unsigned firstStoredFrame = snapshots_.empty() ? 0 : snapshots_.front().frame;
        if (frame <= firstStoredFrame || frame - firstStoredFrame > inputs_.size())
            return false;
        const unsigned inputIndex = frame - firstStoredFrame - 1;
        if (inputIndex >= inputs_.size())
            return false;
        if (!ApplyFrame(frame, inputs_[inputIndex], step))
            return false;
        ++frame;
    }
    return true;
}

const DeterministicSnapshot* DeterministicSimulation::FindSnapshot(unsigned frame) const
{
    for (const DeterministicSnapshot& snapshot : snapshots_)
    {
        if (snapshot.frame == frame)
            return &snapshot;
    }
    return nullptr;
}

unsigned long long DeterministicSimulation::ComputeDigest(const StringVariantMap& state)
{
    ea::vector<ea::string> keys;
    keys.reserve(state.size());
    for (const auto& entry : state)
        keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end());

    unsigned long long hash = 1469598103934665603ull;
    for (const ea::string& key : keys)
    {
        HashString(hash, key);
        const auto it = state.find(key);
        if (it != state.end())
        {
            HashByte(hash, static_cast<unsigned char>(it->second.GetType()));
            HashString(hash, it->second.ToString());
        }
    }
    return hash;
}

unsigned long long DeterministicSimulation::ComputeStateDigest() const
{
    return ComputeDigest(state_);
}

void DeterministicSimulation::Clear()
{
    currentFrame_ = 0;
    state_.clear();
    snapshots_.clear();
    inputs_.clear();
}

} // namespace Urho3D
