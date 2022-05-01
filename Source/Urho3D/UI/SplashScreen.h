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

#include "../Engine/SingleStateApplication.h"
#include "../Audio/SoundSource.h"
#include "../Audio/Sound.h"
#include "../UI/Sprite.h"

namespace Urho3D
{

/// Splash screen application state.
class URHO3D_API SplashScreen : public ApplicationState
{
    URHO3D_OBJECT(SplashScreen, ApplicationState)

    enum class SplashScreenState
    {
        FadeIn,
        Sustain,
        FadeOut,
        Complete
    };
public:
    /// Construct.
    explicit SplashScreen(Context* context);

    /// Activate game screen. Executed by Application.
    void Activate(SingleStateApplication* application) override;

    /// Handle the logic update event.
    void Update(float timeStep) override;

    bool FetchSceneResourcesAsync(const ea::string& fileName);

    void SetBackgroundImage(Texture* image);
    void SetForegroundImage(Texture* image);
    void SetProgressImage(Texture* image);
    void SetProgressColor(const Color& color);
    void SetSound(Sound* sound, float gain);
    void SetNextState(ApplicationState* state);
    void SetFadeInDuration(float durationInSeconds);
    void SetFadeOutDuration(float durationInSeconds);
    void SetDuration(float durationInSeconds);

    Texture* GetBackgroundImage() const;
    Texture* GetForegroundImage() const;
    Texture* GetProgressImage() const;
    const Color& GetProgressColor() const { return progressBar_->GetColor(C_TOPLEFT); }
    ApplicationState* GetNextState() const;
    float GetFadeInDuration() const { return fadeInDuration_; }
    float GetFadeOutDuration() const { return fadeOutDuration_; }
    float GetDuration() const { return duration_; }

private:
    void UpdateLayout(float ratio);
    void UpdateState(float timeStep, unsigned resourceCounter);

    // Splash screen state
    SplashScreenState state_{SplashScreenState ::Complete};
    // Max visited resource counter
    unsigned maxResourceCounter_{0};
    // Current state time left
    float timeLeft_ {0.0f};
    // Current state duration
    float stateDuration_{0.0f};

    // Fade in duration
    float fadeInDuration_{0.0f};
    // Fade out duration
    float fadeOutDuration_{0.0f};
    // Minimum screen time duration
    float duration_{0.0f};

    SharedPtr<Scene> scene_;
    SharedPtr<SoundSource> soundSource_;
    
    SharedPtr<ApplicationState> nextState_;
    SharedPtr<Sprite> background_;
    SharedPtr<Sprite> foreground_;
    SharedPtr<Sprite> progressBar_;
    SharedPtr<Sound> sound_;
};

} // namespace Urho3D
