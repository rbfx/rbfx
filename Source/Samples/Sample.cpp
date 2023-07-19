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

#if URHO3D_RMLUI
#   include <Urho3D/RmlUI/RmlUI.h>
#endif

#include "Sample.h"
#include "SamplesManager.h"
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Model.h>

Sample::Sample(Context* context) :
    ApplicationState(context),
    yaw_(0.0f),
    pitch_(0.0f),
    touchEnabled_(false),
    screenJoystickIndex_(M_MAX_UNSIGNED),
    screenJoystickSettingsIndex_(M_MAX_UNSIGNED),
    paused_(false)
{
    SetMouseMode(MM_ABSOLUTE);
    SetMouseVisible(false);
}

/// Activate game state. Executed by StateManager.
void Sample::Activate(StringVariantMap& bundle)
{
    ApplicationState::Activate(bundle);
    Start(bundle["Args"].GetStringVector());
}

/// Deactivate game screen. Executed by Application.
void Sample::Deactivate()
{
    Stop();
    ApplicationState::Deactivate();
}

void Sample::Start(const ea::vector<ea::string>& args)
{
    Start();
}

void Sample::Start()
{
    auto* input = context_->GetSubsystem<Input>();

    if (GetPlatform() == PlatformId::Android || GetPlatform() == PlatformId::iOS)
        // On mobile platform, enable touch by adding a screen joystick
        InitTouchInput();
    else if (GetSubsystem<Input>()->GetNumJoysticks() == 0)
        // On desktop platform, do not detect touch when we already got a joystick
        SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Sample, HandleTouchBegin));

    if (!GetSubsystem<Engine>()->IsHeadless())
    {
        // Create logo
        CreateLogo();
        // Set custom window Title & Icon
        SetWindowTitleAndIcon();
        // Create console and debug HUD
        CreateConsoleAndDebugHud();
    }

    // Subscribe key down event
    SubscribeToEvent(input, E_KEYDOWN, URHO3D_HANDLER(Sample, HandleKeyDown));
    // Subscribe key up event
    SubscribeToEvent(input, E_KEYUP, URHO3D_HANDLER(Sample, HandleKeyUp));
    // Subscribe scene update event
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Sample, HandleSceneUpdate));
}

void Sample::Stop()
{
    if (screenJoystickIndex_ != M_MAX_UNSIGNED)
    {
        Input* input = GetSubsystem<Input>();
        input->RemoveScreenJoystick((SDL_JoystickID)screenJoystickIndex_);
        screenJoystickIndex_ = M_MAX_UNSIGNED;
    }
}

void Sample::SetDefaultSkybox(Scene* scene)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    const auto skybox = scene->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));
}

void Sample::InitTouchInput()
{
    touchEnabled_ = true;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Input* input = GetSubsystem<Input>();
    XMLFile* layout = cache->GetResource<XMLFile>("UI/ScreenJoystick_Samples.xml");
    const ea::string& patchString = GetScreenJoystickPatchString();
    if (!patchString.empty())
    {
        // Patch the screen joystick layout further on demand
        SharedPtr<XMLFile> patchFile(new XMLFile(context_));
        if (patchFile->FromString(patchString))
            layout->Patch(patchFile);
    }
    screenJoystickIndex_ = (unsigned)input->AddScreenJoystick(layout, cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, true);
}

void Sample::SetLogoVisible(bool enable)
{
    if (logoSprite_)
        logoSprite_->SetVisible(enable);
}

void Sample::CreateLogo()
{
    // Get logo texture
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/FishBoneLogo.png");
    if (!logoTexture)
        return;

    // Create logo sprite and add to the UI layout
    UI* ui = GetSubsystem<UI>();
    logoSprite_ = GetUIRoot()->CreateChild<Sprite>();

    // Set logo sprite texture
    logoSprite_->SetTexture(logoTexture);

    int textureWidth = logoTexture->GetWidth();
    int textureHeight = logoTexture->GetHeight();

    // Set logo sprite scale
    logoSprite_->SetScale(256.0f / textureWidth);

    // Set logo sprite size
    logoSprite_->SetSize(textureWidth, textureHeight);

    // Set logo sprite hot spot
    logoSprite_->SetHotSpot(textureWidth, textureHeight);

    // Set logo sprite alignment
    logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);

    // Make logo not fully opaque to show the scene underneath
    logoSprite_->SetOpacity(0.9f);

    // Set a low priority for the logo so that other UI elements can be drawn on top
    logoSprite_->SetPriority(-100);
}

void Sample::SetWindowTitleAndIcon()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Graphics* graphics = GetSubsystem<Graphics>();
    Image* icon = cache->GetResource<Image>("Textures/UrhoIcon.png");
    graphics->SetWindowIcon(icon);
    graphics->SetWindowTitle("rbfx Sample");
}

void Sample::CreateConsoleAndDebugHud()
{
    // Create console
    Console* console = context_->GetSubsystem<Engine>()->CreateConsole();

    // Create debug HUD.
    DebugHud* debugHud = context_->GetSubsystem<Engine>()->CreateDebugHud();
}


void Sample::HandleKeyUp(StringHash /*eventType*/, VariantMap& eventData)
{
}

void Sample::HandleKeyDown(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    // Toggle console with F1 or backquote
#if URHO3D_SYSTEMUI
    if (key == KEY_F1 || key == KEY_BACKQUOTE)
    {
#if URHO3D_RMLUI
        if (auto* ui = GetSubsystem<RmlUI>())
        {
            if (ui->IsInputCaptured())
                return;
        }
#endif
        if (auto* ui = GetSubsystem<UI>())
        {
            if (UIElement* element = ui->GetFocusElement())
            {
                if (element->IsEditable())
                    return;
            }
        }
        GetSubsystem<Console>()->Toggle();
        return;
    }
    // Toggle debug HUD with F2
    else if (key == KEY_F2)
    {
        context_->GetSubsystem<Engine>()->CreateDebugHud()->ToggleAll();
        return;
    }
#endif

    // Common rendering quality controls, only when UI has no focused element
    if (!GetSubsystem<UI>()->GetFocusElement())
    {
        Renderer* renderer = GetSubsystem<Renderer>();

        // Preferences / Pause
        if (key == KEY_SELECT && touchEnabled_)
        {
            paused_ = !paused_;

            Input* input = GetSubsystem<Input>();
            if (screenJoystickSettingsIndex_ == M_MAX_UNSIGNED)
            {
                // Lazy initialization
                ResourceCache* cache = GetSubsystem<ResourceCache>();
                screenJoystickSettingsIndex_ = (unsigned)input->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings_Samples.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
            }
            else
                input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, paused_);
        }

        // Texture quality
        else if (key == '1')
        {
            auto quality = (unsigned)renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetTextureQuality((MaterialQuality)quality);
        }

        // Default texture filter
        else if (key == '2')
        {
            auto filterMode = (unsigned)renderer->GetTextureFilterMode();
            ++filterMode;
            if (filterMode > FILTER_ANISOTROPIC)
                filterMode = FILTER_NEAREST;
            renderer->SetTextureFilterMode((TextureFilterMode)filterMode);
        }

        // Take screenshot
        else if (key == '9')
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);

            // Save into app directory
            const ea::string screenshotPath = GetSubsystem<Engine>()->GetAppPreferencesDir() + "Screenshots/";
            const ea::string screenshotName =
                "Screenshot_" + Time::GetTimeStamp().replaced(':', '_').replaced('.', '_').replaced(' ', '_') + ".png";

            GetSubsystem<FileSystem>()->CreateDirsRecursive(screenshotPath);
            screenshot.SavePNG(screenshotPath + screenshotName);
        }
    }
}

void Sample::HandleSceneUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    if (touchEnabled_)
    {
        Input* input = GetSubsystem<Input>();
        for (unsigned i = 0; i < input->GetNumTouches(); ++i)
        {
            TouchState* state = input->GetTouch(i);
            if (!state->touchedElement_)    // Touch on empty space
            {
                // Move the cursor to the touch position
                Cursor* cursor = GetSubsystem<UI>()->GetCursor();
                if (cursor && cursor->IsVisible())
                    cursor->SetPosition(state->position_);
            }
        }
    }
}

void Sample::HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData)
{
    // On some platforms like Windows the presence of touch input can only be detected dynamically
    InitTouchInput();
    UnsubscribeFromEvent("TouchBegin");
}

void Sample::CloseSample()
{
    SendEvent(E_SAMPLE_EXIT_REQUESTED);
}
