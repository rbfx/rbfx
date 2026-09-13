// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
