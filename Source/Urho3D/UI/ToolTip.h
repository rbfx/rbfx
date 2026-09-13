// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/UI/UIElement.h"

namespace Urho3D
{

/// Tooltip %UI element.
class URHO3D_API ToolTip : public UIElement
{
    URHO3D_OBJECT(ToolTip, UIElement);

public:
    /// Construct.
    explicit ToolTip(Context* context);
    /// Destruct.
    ~ToolTip() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Perform UI element update.
    void Update(float timeStep) override;

    /// Hide tooltip if visible and reset parentage.
    void Reset();

    /// Add an alternative hover target.
    void AddAltTarget(UIElement* target);

    /// Set the delay in seconds until the tooltip shows once hovering. Set zero to use the default from the UI subsystem.
    /// @property
    void SetDelay(float delay);

    /// Return the delay in seconds until the tooltip shows once hovering.
    /// @property
    float GetDelay() const { return delay_; }

private:
    /// The element that is being tracked for hovering. Normally the parent element.
    WeakPtr<UIElement> target_;
    /// Alternative targets. Primarily targets parent.
    ea::vector<WeakPtr<UIElement> > altTargets_;
    /// Delay from hover start to displaying the tooltip.
    float delay_;
    /// Hover countdown has started.
    bool hovered_;
    /// Point at which the tooltip was set visible.
    Timer displayAt_;
    /// Original offset position to the parent.
    IntVector2 originalPosition_;
};

}
