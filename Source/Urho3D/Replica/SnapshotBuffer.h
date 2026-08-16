// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/vector.h>

#include <Urho3D/Core/Variant.h>
#include <Urho3D/Replica/NetworkId.h>

namespace Urho3D
{

/// One authoritative or predicted network snapshot.
struct URHO3D_API NetworkSnapshot
{
    NetworkFrame frame{NetworkFrame::Min};
    float time{};
    StringVariantMap values;
};

/// Bounded snapshot history with deterministic interpolation and limited extrapolation.
class URHO3D_API SnapshotBuffer
{
public:
    explicit SnapshotBuffer(unsigned capacity = 32);

    void SetCapacity(unsigned capacity);
    unsigned GetCapacity() const { return capacity_; }
    void SetMaxExtrapolation(float seconds) { maxExtrapolation_ = Max(seconds, 0.0f); }
    float GetMaxExtrapolation() const { return maxExtrapolation_; }

    /// Insert or replace a snapshot while keeping frame order.
    void Push(const NetworkSnapshot& snapshot);
    void Clear() { snapshots_.clear(); }
    bool IsEmpty() const { return snapshots_.empty(); }
    unsigned GetSize() const { return snapshots_.size(); }

    /// Sample at a presentation time. Extrapolation is capped by maxExtrapolation.
    bool Sample(float time, StringVariantMap& values) const;
    /// Return the most recent snapshot.
    const NetworkSnapshot* GetLatest() const;
    const ea::vector<NetworkSnapshot>& GetSnapshots() const { return snapshots_; }

private:
    static Variant Interpolate(const Variant& from, const Variant& to, float alpha);
    static void InterpolateMaps(const StringVariantMap& from, const StringVariantMap& to, float alpha,
        StringVariantMap& result);

    unsigned capacity_{32};
    float maxExtrapolation_{0.25f};
    ea::vector<NetworkSnapshot> snapshots_;
};

} // namespace Urho3D
