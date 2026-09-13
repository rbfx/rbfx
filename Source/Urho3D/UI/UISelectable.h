// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/Color.h"
#include "Urho3D/UI/UIElement.h"

namespace Urho3D
{

class URHO3D_API UISelectable : public UIElement
{
public:
    URHO3D_OBJECT(UISelectable, UIElement);

    /// Construct.
    using UIElement::UIElement;
    /// Destruct.
    ~UISelectable() override = default;

    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Return UI rendering batches.
    void GetBatches(ea::vector<UIBatch>& batches, ea::vector<float>& vertexData, const IntRect& currentScissor) override;

    /// Set selection background color. Color with 0 alpha (default) disables.
    /// @property
    void SetSelectionColor(const Color& color);
    /// Set hover background color. Color with 0 alpha (default) disables.
    /// @property
    void SetHoverColor(const Color& color);

    /// Return selection background color.
    /// @property
    const Color& GetSelectionColor() const { return selectionColor_; }
    /// Return hover background color.
    /// @property
    const Color& GetHoverColor() const { return hoverColor_; }

protected:
    /// Selection background color.
    Color selectionColor_{Color::TRANSPARENT_BLACK};
    /// Hover background color.
    Color hoverColor_{Color::TRANSPARENT_BLACK};
};

}
