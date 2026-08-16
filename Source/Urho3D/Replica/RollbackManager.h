// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/functional.h>
#include <EASTL/vector.h>

#include <Urho3D/Core/Variant.h>
#include <Urho3D/Replica/NetworkId.h>
#include <Urho3D/Replica/SnapshotBuffer.h>

namespace Urho3D
{

/// Local input recorded against a simulation frame.
struct URHO3D_API RollbackInput
{
    NetworkFrame frame{NetworkFrame::Min};
    StringVariantMap values;
};

using RollbackSimulator = ea::function<bool(const StringVariantMap&, StringVariantMap&)>;

/// Bounded client-prediction history and authoritative-state reconciliation.
class URHO3D_API RollbackManager
{
public:
    explicit RollbackManager(unsigned capacity = 64);

    void SetCapacity(unsigned capacity);
    unsigned GetCapacity() const { return capacity_; }
    void Clear();

    void RecordInput(const RollbackInput& input);
    void SaveState(NetworkFrame frame, const StringVariantMap& state);
    const StringVariantMap* FindState(NetworkFrame frame) const;

    /// Replace state at authoritativeFrame and replay later inputs through simulator.
    bool Reconcile(NetworkFrame authoritativeFrame, const StringVariantMap& authoritativeState,
        const RollbackSimulator& simulator, StringVariantMap& correctedState);

    const ea::vector<RollbackInput>& GetInputs() const { return inputs_; }
    const ea::vector<NetworkSnapshot>& GetStates() const { return states_; }

private:
    unsigned capacity_{64};
    ea::vector<RollbackInput> inputs_;
    ea::vector<NetworkSnapshot> states_;
};

} // namespace Urho3D
