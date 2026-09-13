// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Signal.h"
#include "../Graphics/Animation.h"
#include "../SystemUI/BaseWidget.h"
#include "../SystemUI/Widgets.h"

namespace Urho3D
{

/// SystemUI widget to preview texture.
class URHO3D_API Texture2DWidget : public BaseWidget
{
    URHO3D_OBJECT(Texture2DWidget, BaseWidget)

public:
    Texture2DWidget(Context* context, Texture2D* resource);
    ~Texture2DWidget() override;

    void RenderContent() override;

    Texture2D* GetTexture2D() const { return resource_; }

private:
    SharedPtr<Texture2D> resource_;
};

} // namespace Urho3D
