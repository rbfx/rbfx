//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
