//
// Copyright (c) 2017-2020 the rbfx project.
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
#include <Urho3D/UI/UIElement.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "Inspector/UIElementInspector.h"

namespace Urho3D
{

UIElementInspector::UIElementInspector(Context* context)
    : SerializableInspector(context)
{
}

void UIElementInspector::RenderInspector(const char* filter)
{
    if (inspected_.Expired())
        return;

    auto element = static_cast<UIElement*>(inspected_.Get());
    const char* name = element->GetName().empty() ? element->GetTypeName().c_str() : element->GetName().c_str();
    if (ui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen))
        RenderAttributes(element, filter, const_cast<UIElement*>(element->GetRoot()));
}

}
