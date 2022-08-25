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

#include "../Precompiled.h"

#include "MultitouchAdapter.h"

#include "../Input/Input.h"
#include "../Input/InputEvents.h"

namespace Urho3D
{

MultitouchAdapter::MultitouchAdapter(Context* context)
    : Object(context)
{
}

void MultitouchAdapter::SetEnabled(bool enabled)
{
    if (enabled != enabled_)
    {
        enabled_ = enabled;
        if (enabled_)
            SubscribeToEvents();
        else
            UnsubscribeFromEvents();
    }
}
void MultitouchAdapter::SubscribeToEvents()
{
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(MultitouchAdapter, HandleTouchBegin));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(MultitouchAdapter, HandleTouchEnd));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(MultitouchAdapter, HandleTouchMove));
}

void MultitouchAdapter::UnsubscribeFromEvents()
{
    UnsubscribeFromEvent(E_TOUCHBEGIN);
    UnsubscribeFromEvent(E_TOUCHEND);
    UnsubscribeFromEvent(E_TOUCHMOVE);
}

void MultitouchAdapter::HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData)
{
    if (!acceptTouches_)
        return;

    using namespace TouchBegin;

    const auto touchId = eventData[P_TOUCHID].GetInt();


    Input* input = GetSubsystem<Input>();
    for (unsigned i=0; i<input->GetNumTouches(); ++i)
    {
        auto* touchState = input->GetTouch(i);
        if (touchState->touchID_ == touchId)
        {
            // Ignore touches that start over UI elements
            if (touchState->touchedElement_)
                return;

            for (auto& activeTouch : touches_)
            {
                if (activeTouch.touchID_ == touchId)
                {
                    return;
                }
            }

            SendEvent(MULTITOUCH_CANCEL);

            auto& touch = touches_.emplace_back();
            touch.touchID_ = touchId;
            touch.pos_ = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
            SendEvent(MULTITOUCH_BEGIN);
        }
    }
}

void MultitouchAdapter::HandleTouchEnd(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace TouchEnd;

    const auto touchId = eventData[P_TOUCHID].GetInt();

    for (auto activeTouch = touches_.begin(); activeTouch != touches_.end(); ++activeTouch)
    {
        if (activeTouch->touchID_ == touchId)
        {
            activeTouch->pos_ = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
            SendEvent(MULTITOUCH_END);

            touches_.erase_unsorted(activeTouch);

            // Don't accept new touches until all fingers released.
            acceptTouches_ = touches_.empty();

            return;
        }
    }
}

void MultitouchAdapter::HandleTouchMove(StringHash /*eventType*/, VariantMap& eventData)
{
    if (!acceptTouches_)
        return;

    using namespace TouchMove;

    const auto touchId = eventData[P_TOUCHID].GetInt();

    for (auto& activeTouch : touches_)
    {
        if (activeTouch.touchID_ == touchId)
        {
            activeTouch.pos_ = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
            SendEvent(MULTITOUCH_MOVE);
            return;
        }
    }
}

void MultitouchAdapter::SendEvent(MultitouchEventType event)
{
    if (touches_.empty())
        return;

    using namespace Multitouch;

    VariantMap evt;
    evt[Multitouch::P_EVENTTYPE] = event;
    evt[Multitouch::P_NUMFINGERS] = static_cast<unsigned>(touches_.size());

    IntVector2 center(0, 0);
    IntVector2 min(INT_MAX, INT_MAX);
    IntVector2 max(INT_MIN, INT_MIN);
    for (auto& activeTouch : touches_)
    {
        center += activeTouch.pos_;
        min = VectorMin(min, activeTouch.pos_);
        max = VectorMax(max, activeTouch.pos_);
    }
    center = IntVector2(center.x_ / touches_.size(), center.y_ / touches_.size());
    auto size = max - min;

    evt[Multitouch::P_X] = center.x_;
    evt[Multitouch::P_Y] = center.y_;
    evt[Multitouch::P_SIZE] = size;
    if (event == MULTITOUCH_BEGIN)
    {
        lastKnownPosition_ = center;
        lastKnownSize_ = size;
    }
    auto delta = center - lastKnownPosition_;
    evt[Multitouch::P_DX] = delta.x_;
    evt[Multitouch::P_DY] = delta.y_;
    evt[Multitouch::P_DSIZE] = size - lastKnownSize_;
    lastKnownPosition_ = center;
    lastKnownSize_ = size;

    Object::SendEvent(E_MULTITOUCH, evt);
}

} // namespace Urho3D
