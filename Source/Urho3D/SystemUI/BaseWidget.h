// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

/// SystemUI base class for a widget.
class URHO3D_API BaseWidget : public Object
{
    URHO3D_OBJECT(BaseWidget, Object)

public:
    BaseWidget(Context* context);
    ~BaseWidget() override;

    virtual void RenderContent() = 0;
};

} // namespace Urho3D
