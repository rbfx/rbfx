// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/LogicComponent.h"

namespace Urho3D
{

/// Component that maintains world origin around the node.
class URHO3D_API WorldOrigin : public LogicComponent
{
    URHO3D_OBJECT(WorldOrigin, LogicComponent)

public:
    static constexpr unsigned MaxDistance = 8192;
    static constexpr unsigned DefaultStep = 128;

    explicit WorldOrigin(Context* context);
    ~WorldOrigin() override;

    static void RegisterObject(Context* context);

    /// Implement LogicComponent.
    /// @{
    void Update(float timeStep) override;
    /// @}

    /// Attributes.
    /// @{
    void SetMaxDistance(unsigned distance) { maxDistance_ = distance; }
    unsigned GetMaxDistance() const { return maxDistance_; }
    void SetStep(unsigned step) { step_ = step; }
    unsigned GetStep() const { return step_; }
    void SetUpdateX(unsigned value) { updateX_ = value; }
    unsigned GetUpdateX() const { return updateX_; }
    void SetUpdateY(unsigned value) { updateY_ = value; }
    unsigned GetUpdateY() const { return updateY_; }
    void SetUpdateZ(unsigned value) { updateZ_ = value; }
    unsigned GetUpdateZ() const { return updateZ_; }
    /// @}

private:
    unsigned maxDistance_{MaxDistance};
    unsigned step_{DefaultStep};
    bool updateX_{true};
    bool updateY_{false};
    bool updateZ_{true};
};

} // namespace Urho3D
