//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../UI/ToolTip.h"
#include "../UI/UI.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

ToolTip::ToolTip(Context* context) :
    UIElement(context),
    delay_(0.0f),
    hovered_(false)
{
    SetVisible(false);
}

ToolTip::~ToolTip() = default;

void ToolTip::RegisterObject(Context* context)
{
    context->RegisterFactory<ToolTip>(UI_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(UIElement);
    URHO3D_ACCESSOR_ATTRIBUTE("Delay", GetDelay, SetDelay, float, 0.0f, AM_FILE);
}

void ToolTip::Update(float timeStep)
{
    // Track the element we are parented to for hovering. When we display, we move ourself to the root element
    // to ensure displaying on top
    UIElement* root = GetRoot();
    if (!root)
        return;
    if (parent_ != root)
        target_ = parent_;

    // If target is removed while we are displaying, we have no choice but to destroy ourself
    if (target_.Expired())
    {
        Remove();
        return;
    }

    bool hovering = target_->IsHovering() && target_->IsVisibleEffective();
    if (!hovering)
    {
        for (auto it = altTargets_.begin(); it != altTargets_.end();)
        {
            SharedPtr<UIElement> target = it->Lock();
            if (!target)
                it = altTargets_.erase(it);
            else
            {
                hovering = target->IsHovering() && target->IsVisibleEffective();

                if (hovering)
                    break;
                else
                    ++it;
            }
        }
    }

    if (hovering)
    {
        float effectiveDelay = delay_ > 0.0f ? delay_ : GetSubsystem<UI>()->GetDefaultToolTipDelay();

        if (!hovered_)
        {
            hovered_ = true;
            displayAt_.Reset();
        }
        else if (displayAt_.GetMSec(false) >= (unsigned)(effectiveDelay * 1000.0f) && parent_ == target_.Get())
        {
            originalPosition_ = GetPosition();
            IntVector2 screenPosition = GetScreenPosition();
            SetParent(root);
            SetPosition(screenPosition);
            SetVisible(true);
            // BringToFront() is unreliable in this case as it takes into account only input-enabled elements.
            // Rather just force priority to max
            SetPriority(M_MAX_INT);
        }
    }
    else
    {
        Reset();
    }
}

void ToolTip::Reset()
{
    if (IsVisible() && parent_ == GetRoot())
    {
        SetParent(target_.Get());
        SetPosition(originalPosition_);
        SetVisible(false);
    }
    hovered_ = false;
    displayAt_.Reset();
}

void ToolTip::AddAltTarget(UIElement* target)
{
    altTargets_.push_back(WeakPtr<UIElement>(target));
}

void ToolTip::SetDelay(float delay)
{
    delay_ = delay;
}

}
