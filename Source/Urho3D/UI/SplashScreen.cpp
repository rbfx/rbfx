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

#include "SplashScreen.h"

#include "Sprite.h"
#include "../Resource/ResourceCache.h"
#include "../Audio/SoundSource.h"
#include "../IO/FileSystem.h"
#include "../Scene/Scene.h"
#include "Urho3D/Graphics/Renderer.h"

namespace Urho3D
{

namespace
{
void UpdateSizeAndPosition(IntVector2 screenSize, Sprite* sprite, bool stretch)
{
    Texture* texture = sprite->GetTexture();
    if (!texture)
        return;
    auto imageSize = texture->GetSize();
    auto horisontalScale = screenSize.x_ / static_cast<float>(imageSize.x_);
    auto verticalScale = screenSize.y_ / static_cast<float>(imageSize.y_);
    float scale = 1.0f;
    if (stretch)
    {
        scale = Max(horisontalScale, verticalScale);
    }
    else
    {
        scale = Min(Min(horisontalScale, verticalScale), 1.0f);
    }
    sprite->SetSize(static_cast<int>(scale * imageSize.x_), static_cast<int>(scale * imageSize.y_));
    Vector2 pos = Vector2((screenSize.x_ - sprite->GetWidth()) / 2.0f, (screenSize.y_ - sprite->GetHeight()) / 2.0f);
    UIElement* parent = sprite->GetParent();
    sprite->SetPosition(pos - parent->GetScreenPosition().ToVector2());
}

}

SplashScreen::SplashScreen(Context* context)
    : ApplicationState(context)
{
    SetMouseGrabbed(false);
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);

    background_ = GetUIRoot()->CreateChild<Sprite>();
    foreground_ = background_->CreateChild<Sprite>();
    progressBar_ = foreground_->CreateChild<Sprite>();
    scene_ = MakeShared<Scene>(context);
    soundSource_ = scene_->CreateComponent<SoundSource>();
}

void SplashScreen::Activate(StringVariantMap& bundle)
{
    ApplicationState::Activate(bundle);
    auto* input = context_->GetSubsystem<Input>();
    SubscribeToEvent(input, E_KEYUP, URHO3D_HANDLER(SplashScreen, HandleKeyUp));
    SubscribeToEvent(input, E_MOUSEBUTTONUP, URHO3D_HANDLER(SplashScreen, HandleKeyUp));
    SubscribeToEvent(input, E_JOYSTICKBUTTONUP, URHO3D_HANDLER(SplashScreen, HandleKeyUp));

    timeAcc_ = 0.0;
    exitRequested_ = false;

    auto* cache = GetSubsystem<ResourceCache>();

    maxResourceCounter_ = Max(cache->GetNumBackgroundLoadResources(), 1);

    SetViewport(0, MakeShared<Viewport>(context_, scene_, nullptr));
    if (sound_)
        soundSource_->Play(sound_);

    Update(0);
}

/// Return true if state is ready to be deactivated. Executed by StateManager.
bool SplashScreen::CanLeaveState() const
{
    if (exitRequested_)
        return true;

    auto* cache = GetSubsystem<ResourceCache>();
    const unsigned resourceCounter = cache->GetNumBackgroundLoadResources();
    return resourceCounter == 0 && timeAcc_ > duration_;
}

void SplashScreen::Deactivate()
{
    ApplicationState::Deactivate();

    auto* input = context_->GetSubsystem<Input>();
    UnsubscribeFromEvent(input, E_KEYUP);
    UnsubscribeFromEvent(input, E_MOUSEBUTTONUP);
    UnsubscribeFromEvent(input ,E_JOYSTICKBUTTONUP);
}

void SplashScreen::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    if (eventType == E_KEYUP)
    {
        using namespace KeyUp;
        switch (args[P_KEY].GetInt())
        {
        case KEY_SPACE:
        case KEY_ESCAPE:
        case KEY_BACKSPACE:
            exitRequested_ |= skippable_; break;
        }
        return;
    }
    if (eventType == E_MOUSEBUTTONUP)
    {
        exitRequested_ |= skippable_;
        return;
    }
    if (eventType == E_JOYSTICKBUTTONUP)
    {
        exitRequested_ |= skippable_;
        return;
    }
}

void SplashScreen::UpdateLayout(float ratio)
{
    const UIElement* root = GetUIRoot();
    const IntVector2 screenSize = root->GetSize();

    if (ratio >= 1.0f)
        ratio = 0.0f;

    UpdateSizeAndPosition(screenSize, background_, true);
    UpdateSizeAndPosition(screenSize, foreground_, false);

    IntVector2 progressBarAreaSize = IntVector2(screenSize.x_, screenSize.y_ / 10);
    const Texture* barTexture = progressBar_->GetTexture();
    if (barTexture)
    {
        UpdateSizeAndPosition(progressBarAreaSize, progressBar_, false);
        progressBarAreaSize = progressBar_->GetSize();
        progressBar_->SetImageRect(IntRect(0, 0, barTexture->GetWidth() * ratio, barTexture->GetHeight()));
    }
    progressBar_->SetPosition(
        Vector2((screenSize.x_ - progressBarAreaSize.x_) * 0.5, screenSize.y_ - progressBarAreaSize.y_)
        - foreground_->GetScreenPosition().ToVector2());
    progressBar_->SetSize(progressBarAreaSize.x_ * ratio, progressBarAreaSize.y_);
}

/// Handle the logic update event.
void SplashScreen::Update(float timeStep)
{
    auto* cache = GetSubsystem<ResourceCache>();

    const unsigned resourceCounter = cache->GetNumBackgroundLoadResources();
    if (resourceCounter > maxResourceCounter_)
    {
        maxResourceCounter_ = resourceCounter;
    }
    UpdateLayout(static_cast<float>(maxResourceCounter_ - resourceCounter) / maxResourceCounter_);

    timeAcc_ += timeStep;
};

void SplashScreen::SetDuration(float durationInSeconds)
{
    duration_ = durationInSeconds;
}

void SplashScreen::SetSkippable(bool skippable)
{
    skippable_ = skippable;
}

void SplashScreen::SetSound(Sound* sound, float gain)
{
    sound_ = sound;
    soundSource_->SetGain(gain);
    if (IsActive())
        soundSource_->Play(sound_);
}

void SplashScreen::SetBackgroundImage(Texture* image)
{
    background_->SetTexture(image);
}

void SplashScreen::SetForegroundImage(Texture* image)
{
    foreground_->SetTexture(image);
}

void SplashScreen::SetProgressImage(Texture* image)
{
    progressBar_->SetTexture(image);
}

void SplashScreen::SetProgressColor(const Color& color)
{
    progressBar_->SetColor(color);
}

Texture* SplashScreen::GetBackgroundImage() const
{
    return background_->GetTexture();
}

Texture* SplashScreen::GetForegroundImage() const
{
    return foreground_->GetTexture();
}

Texture* SplashScreen::GetProgressImage() const
{
    return progressBar_->GetTexture();
}


/// Background load a resource. Return true if successfully stored to the load
/// queue, false if eg. already exists. Can be called from outside the main thread.
bool SplashScreen::QueueResource(StringHash type, const ea::string& name, bool sendEventOnFailure)
{
    auto* cache = GetSubsystem<ResourceCache>();
    return cache->BackgroundLoadResource(type, name, sendEventOnFailure);
}

bool SplashScreen::QueueSceneResourcesAsync(const ea::string& fileName)
{
    auto* cache = GetSubsystem<ResourceCache>();
    scene_ = MakeShared<Scene>(context_);
    AbstractFilePtr file = cache->GetFile(fileName);
    if (file)
    {
        ea::string extension = GetExtension(fileName);
        if (extension == ".xml")
            return scene_->LoadAsyncXML(file, LOAD_RESOURCES_ONLY);
        else if (extension == ".json")
            return scene_->LoadAsyncJSON(file, LOAD_RESOURCES_ONLY);
        else
            return scene_->LoadAsync(file, LOAD_RESOURCES_ONLY);
    }
    return false;
}

} // namespace Urho3D
