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

#include "../Engine/StateManager.h"
#include "../Audio/SoundSource.h"
#include "../Audio/Sound.h"
#include "../UI/Sprite.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

/// Splash screen application state.
class URHO3D_API SplashScreen : public ApplicationState
{
    URHO3D_OBJECT(SplashScreen, ApplicationState)

public:
    /// Construct.
    explicit SplashScreen(Context* context);

    /// Activate game screen. Executed by StateManager.
    void Activate(StringVariantMap& bundle) override;

    /// Return true if state is ready to be deactivated. Executed by StateManager.
    bool CanLeaveState() const override;

    /// Deactivate game screen. Executed by StateManager.
    void Deactivate() override;

    /// Handle the logic update event.
    void Update(float timeStep) override;

    /// Background load all resources referenced by a scene.
    bool QueueSceneResourcesAsync(const ea::string& fileName);
    /// Template version of queueing a resource background load.
    template <class T> bool QueueResource(const ea::string& name, bool sendEventOnFailure = true);
    /// Background load a resource. Return true if successfully stored to the load
    /// queue, false if eg. already exists. Can be called from outside the main thread.
    bool QueueResource(StringHash type, const ea::string& name, bool sendEventOnFailure = true);

    void SetBackgroundImage(Texture* image);
    void SetForegroundImage(Texture* image);
    void SetProgressImage(Texture* image);
    void SetProgressColor(const Color& color);
    void SetSound(Sound* sound, float gain);
    void SetDuration(float durationInSeconds);
    void SetSkippable(bool skippable);

    Texture* GetBackgroundImage() const;
    Texture* GetForegroundImage() const;
    Texture* GetProgressImage() const;
    const Color& GetProgressColor() const { return progressBar_->GetColor(C_TOPLEFT); }
    float GetDuration() const { return duration_; }
    bool IsSkippable() const { return skippable_; }

private:
    void HandleKeyUp(StringHash eventType, VariantMap& args);

    void UpdateLayout(float ratio);
    void UpdateState(float timeStep, unsigned resourceCounter);

    // Is splash screen skippable.
    bool skippable_{true};
    // Is exit requested by user.
    bool exitRequested_{};

    // Max visited resource counter
    unsigned maxResourceCounter_{0};
    // Accumulated on-screen time.
    float timeAcc_ {0.0f};

    // Minimum screen time duration
    float duration_{0.0f};

    SharedPtr<Scene> scene_;
    SharedPtr<SoundSource> soundSource_;

    SharedPtr<Sprite> background_;
    SharedPtr<Sprite> foreground_;
    SharedPtr<Sprite> progressBar_;
    SharedPtr<Sound> sound_;
};

template <class T> bool SplashScreen::QueueResource(const ea::string& name, bool sendEventOnFailure)
{
    StringHash type = T::GetTypeStatic();
    return QueueResource(type, name, sendEventOnFailure);
}


} // namespace Urho3D
