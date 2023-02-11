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

#pragma once

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Plugins/PluginApplication.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text.h>

#include <EASTL/array.h>

namespace Urho3D
{

class InputLogger : public MainPluginApplication
{
    URHO3D_OBJECT(InputLogger, MainPluginApplication);

public:
    /// Construct.
    explicit InputLogger(Context* context);

protected:
    /// Implement MainPluginApplication
    /// @{
    void Load() override;
    void Start(bool isMain) override;
    void Stop() override;
    void Unload() override;
    /// @}

private:
    struct LoggedEvent
    {
        ea::string eventType_;
        ea::map<ea::string, ea::string> parameters_;
        unsigned count_{};
        unsigned timeStamp_{};
    };

    struct ViewportData
    {
        SharedPtr<Viewport> viewport_;
        SharedPtr<Scene> scene_;
        Camera* camera_;
    };

    ViewportData CreateViewport(const Color& color, const IntRect& rect) const;

    void OnInputEvent(StringHash eventType, VariantMap& eventData);
    LoggedEvent DecodeEvent(StringHash eventType, VariantMap& eventData) const;
    bool MergeEvent(const LoggedEvent& event);
    void AddEvent(const LoggedEvent& event);
    bool CanMergeEventWith(const LoggedEvent& event, const LoggedEvent& existingEvent) const;

    void Update();
    void UpdateText();

    ea::array<ViewportData, 2> viewports_;
    SharedPtr<Text> text_;

    ea::vector<LoggedEvent> eventLog_;
};


}
