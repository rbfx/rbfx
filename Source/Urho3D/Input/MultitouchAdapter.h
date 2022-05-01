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
#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

enum MultitouchEventType
{
    MULTITOUCH_BEGIN,
    MULTITOUCH_END,
    MULTITOUCH_MOVE,
    MULTITOUCH_CANCEL,
};

class URHO3D_API MultitouchAdapter : public Object
{

    URHO3D_OBJECT(MultitouchAdapter, Object)

    struct ActiveTouch
    {
        int touchID_;
        IntVector2 pos_;
    };

public:
    /// Construct.
    explicit MultitouchAdapter(Context* context);

    void SetEnabled(bool enabled);

    bool IsEnabled() const { return enabled_; }

private:
    void SubscribeToEvents();

    void UnsubscribeFromEvents();

    void HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData);

    void HandleTouchEnd(StringHash /*eventType*/, VariantMap& eventData);

    void HandleTouchMove(StringHash /*eventType*/, VariantMap& eventData);

    void SendEvent(MultitouchEventType event);

    bool enabled_{false};

    bool acceptTouches_{true};
    IntVector2 lastKnownPosition_{0,0};
    IntVector2 lastKnownSize_{0,0};

    ea::vector<ActiveTouch> touches_;
};

} // namespace Urho3D
