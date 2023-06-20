//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Input.h"
#include "Urho3D/Input/InputMap.h"
#include "Urho3D/Resource/Resource.h"

namespace Urho3D
{
namespace Detail
{
struct ActionState
{
    float lastKnownValue_{};
};
} // namespace Detail

class URHO3D_API InputTranslator : public Object
{
    URHO3D_OBJECT(InputTranslator, Object);

public:
    /// Construct.
    explicit InputTranslator(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Returns true if the device is actively recording.
    bool IsEnabled() const { return enabled_; }
    /// Starts or stops recording.
    void SetEnabled(bool state);

    /// Get input map.
    InputMap* GetMap() const { return map_; }
    /// Set input map.
    void SetMap(InputMap* map);

    /// Evaluate action state based on input.
    float EvaluateActionState(const ea::string& action) const;

private:
    /// Handle scene update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Input map.
    SharedPtr<InputMap> map_;
    /// Is translator enabled.
    bool enabled_{};
    /// Last known action states.
    ea::unordered_map<ea::string, Detail::ActionState> states_;
};

} // namespace Urho3D
