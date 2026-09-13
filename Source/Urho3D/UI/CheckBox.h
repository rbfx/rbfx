// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../UI/BorderImage.h"

namespace Urho3D
{

/// %UI element that can be toggled between unchecked and checked state.
class URHO3D_API CheckBox : public BorderImage
{
    URHO3D_OBJECT(CheckBox, BorderImage);

public:
    /// Construct.
    explicit CheckBox(Context* context);
    /// Destruct.
    ~CheckBox() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Return UI rendering batches.
    void GetBatches(ea::vector<UIBatch>& batches, ea::vector<float>& vertexData, const IntRect& currentScissor) override;
    /// React to mouse click begin.
    void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to a key press.
    void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    /// Set checked state.
    /// @property
    void SetChecked(bool enable);
    /// Set checked image offset.
    /// @property
    void SetCheckedOffset(const IntVector2& offset);
    /// Set checked image offset.
    void SetCheckedOffset(int x, int y);

    /// Return whether is checked.
    /// @property
    bool IsChecked() const { return checked_; }

    /// Return checked image offset.
    /// @property
    const IntVector2& GetCheckedOffset() const { return checkedOffset_; }

protected:
    /// Checked image offset.
    IntVector2 checkedOffset_;
    /// Current checked state.
    bool checked_;
};

}
