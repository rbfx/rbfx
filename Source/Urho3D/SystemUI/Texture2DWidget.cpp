// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../SystemUI/Texture2DWidget.h"

namespace Urho3D
{

Texture2DWidget::Texture2DWidget(Context* context, Texture2D* resource)
    : BaseClassName(context)
    , resource_(resource)
{
}

Texture2DWidget::~Texture2DWidget()
{
}

void Texture2DWidget::RenderContent()
{
    Texture2D* texture = GetTexture2D();
    if (!texture)
        return;

    const ImVec2 contentPosition = ui::GetCursorPos();
    auto reg = ui::GetContentRegionAvail();
    const auto contentSize = ImVec2(reg.x, reg.x);
    const ImVec2 previewSize = Widgets::FitContent(contentSize, ToImGui(texture->GetSize()));
    ui::SetCursorPos(contentPosition + ImVec2((contentSize.x - previewSize.x) * 0.5f, 0.0f));
    Widgets::Image(texture, previewSize);
}

} // namespace Urho3D
