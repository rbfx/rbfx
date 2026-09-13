// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
