//
// Copyright (c) 2008-2019 the Urho3D project.
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

Sample::Sample(Context* context) :
    Object(context),
    yaw_(0.0f),
    pitch_(0.0f),
    touchEnabled_(false),
    useMouseMode_(MM_ABSOLUTE),
    screenJoystickIndex_(M_MAX_UNSIGNED),
    screenJoystickSettingsIndex_(M_MAX_UNSIGNED),
    paused_(false)
{
}

void Sample::Setup()
{
}

void Sample::Start()
{
    if (GetPlatform() == "Android" || GetPlatform() == "iOS")
        // On mobile platform, enable touch by adding a screen joystick
        InitTouchInput();
    else if (GetSubsystem<Input>()->GetNumJoysticks() == 0)
        // On desktop platform, do not detect touch when we already got a joystick
        SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Sample, HandleTouchBegin));

    // Create logo
    CreateLogo();

    // Set custom window Title & Icon
    SetWindowTitleAndIcon();

    // Create console and debug HUD
    CreateConsoleAndDebugHud();

    // Subscribe key down event
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Sample, HandleKeyDown));
    // Subscribe key up event
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(Sample, HandleKeyUp));
    // Subscribe scene update event
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Sample, HandleSceneUpdate));
}

void Sample::Stop()
{
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

void Sample::InitMouseMode(MouseMode mode)
{
    useMouseMode_ = mode;

    Input* input = GetSubsystem<Input>();

    if (GetPlatform() != "Web")
    {
        if (useMouseMode_ == MM_FREE)
            input->SetMouseVisible(true);

        if (useMouseMode_ != MM_ABSOLUTE)
        {
            input->SetMouseMode(useMouseMode_);
#if URHO3D_SYSTEMUI
            Console* console = GetSubsystem<Console>();
            if (console && console->IsVisible())
                input->SetMouseMode(MM_ABSOLUTE, true);
#endif
        }
    }
    else
    {
        input->SetMouseVisible(true);
        SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Sample, HandleMouseModeRequest));
        SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(Sample, HandleMouseModeChange));
    }
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
    logoSprite_ = ui->GetRoot()->CreateChild<Sprite>();

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

        // Material quality
        else if (key == '2')
        {
            auto quality = (unsigned)renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetMaterialQuality((MaterialQuality)quality);
        }

        // Specular lighting
        else if (key == '3')
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

        // Shadow rendering
        else if (key == '4')
            renderer->SetDrawShadows(!renderer->GetDrawShadows());

        // Shadow map resolution
        else if (key == '5')
        {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }

        // Shadow depth and filtering quality
        else if (key == '6')
        {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }

        // Occlusion culling
        else if (key == '7')
        {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }

        // Instancing
        else if (key == '8')
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

        // Take screenshot
        else if (key == '9')
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                Time::GetTimeStamp().replaced(':', '_').replaced('.', '_').replaced(' ', '_') + ".png");
        }
    }
}

void Sample::HandleSceneUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    // Move the camera by touch, if the camera node is initialized by descendant sample class
    if (touchEnabled_ && cameraNode_)
    {
        Input* input = GetSubsystem<Input>();
        for (unsigned i = 0; i < input->GetNumTouches(); ++i)
        {
            TouchState* state = input->GetTouch(i);
            if (!state->touchedElement_)    // Touch on empty space
            {
                if (state->delta_.x_ ||state->delta_.y_)
                {
                    Camera* camera = cameraNode_->GetComponent<Camera>();
                    if (!camera)
                        return;

                    Graphics* graphics = GetSubsystem<Graphics>();
                    yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                    pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;

                    // Construct new orientation for the camera scene node from yaw and pitch; roll is fixed to zero
                    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
                }
                else
                {
                    // Move the cursor to the touch position
                    Cursor* cursor = GetSubsystem<UI>()->GetCursor();
                    if (cursor && cursor->IsVisible())
                        cursor->SetPosition(state->position_);
                }
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

// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void Sample::HandleMouseModeRequest(StringHash /*eventType*/, VariantMap& eventData)
{
#if URHO3D_SYSTEMUI
    Console* console = GetSubsystem<Console>();
    if (console && console->IsVisible())
        return;
#endif
    Input* input = GetSubsystem<Input>();
    if (useMouseMode_ == MM_ABSOLUTE)
        input->SetMouseVisible(false);
    else if (useMouseMode_ == MM_FREE)
        input->SetMouseVisible(true);
    input->SetMouseMode(useMouseMode_);
}

void Sample::HandleMouseModeChange(StringHash /*eventType*/, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}

void Sample::CloseSample()
{
    VariantMap args;
    {
        using namespace KeyDown;
        args[P_KEY] = KEY_ESCAPE;
        args[P_SCANCODE] = SCANCODE_ESCAPE;
        args[P_BUTTONS] = 0;
        args[P_QUALIFIERS] = 0;
        args[P_REPEAT] = false;
        SendEvent(E_KEYDOWN, args);
    }
    {
        using namespace KeyUp;
        args[P_KEY] = KEY_ESCAPE;
        args[P_SCANCODE] = SCANCODE_ESCAPE;
        args[P_BUTTONS] = 0;
        args[P_QUALIFIERS] = 0;
        SendEvent(E_KEYUP, args);
    }
}
