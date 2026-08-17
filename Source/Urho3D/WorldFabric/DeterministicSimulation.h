// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/vector.h>

namespace Urho3D
{

struct URHO3D_API DeterministicSnapshot
{
    unsigned frame{};
    StringVariantMap state;
    unsigned long long digest{};
};

using DeterministicStep = ea::function<bool(unsigned frame, float fixedDelta,
    const StringVariantMap& input, const StringVariantMap& currentState, StringVariantMap& nextState)>;

/// Fixed-step deterministic simulation history shared by gameplay, AI, physics and networking.
class URHO3D_API DeterministicSimulation
{
public:
    explicit DeterministicSimulation(unsigned capacity = 128);

    void Configure(float fixedDelta, unsigned capacity);
    float GetFixedDelta() const { return fixedDelta_; }
    unsigned GetCapacity() const { return capacity_; }
    unsigned GetCurrentFrame() const { return currentFrame_; }
    const StringVariantMap& GetState() const { return state_; }

    bool Start(const StringVariantMap& initialState);
    bool Advance(const StringVariantMap& input, const DeterministicStep& step);
    bool Restore(unsigned frame);
    bool ReplayTo(unsigned targetFrame, const DeterministicStep& step);
    const DeterministicSnapshot* FindSnapshot(unsigned frame) const;
    const ea::vector<DeterministicSnapshot>& GetSnapshots() const { return snapshots_; }
    const ea::vector<StringVariantMap>& GetInputs() const { return inputs_; }
    unsigned long long ComputeStateDigest() const;
    void Clear();

private:
    static unsigned long long ComputeDigest(const StringVariantMap& state);
    void StoreSnapshot();
    bool ApplyFrame(unsigned frame, const StringVariantMap& input, const DeterministicStep& step);

    float fixedDelta_{1.0f / 60.0f};
    unsigned capacity_{128};
    unsigned currentFrame_{};
    StringVariantMap state_;
    ea::vector<DeterministicSnapshot> snapshots_;
    ea::vector<StringVariantMap> inputs_;
};

} // namespace Urho3D
