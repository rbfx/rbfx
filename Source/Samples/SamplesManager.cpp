//
// Copyright (c) 2017-2022 the rbfx project.
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
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/RenderPipeline/RenderPipeline.h>
#include <Urho3D/Resource/ResourceCache.h>
#if URHO3D_RMLUI
#include <Urho3D/RmlUI/RmlSerializableInspector.h>
#include <Urho3D/RmlUI/RmlUI.h>
#endif
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "01_HelloWorld/HelloWorld.h"
#include "02_HelloGUI/HelloGUI.h"
#include "03_Sprites/Sprites.h"
#include "04_StaticScene/StaticScene.h"
#include "05_AnimatingScene/AnimatingScene.h"
#include "06_SkeletalAnimation/SkeletalAnimation.h"
#include "07_Billboards/Billboards.h"
#include "08_Decals/Decals.h"
#include "09_MultipleViewports/MultipleViewports.h"
#include "10_RenderToTexture/RenderToTexture.h"
#if URHO3D_PHYSICS
#include "11_Physics/Physics.h"
#include "12_PhysicsStressTest/PhysicsStressTest.h"
#include "13_Ragdolls/Ragdolls.h"
#endif
#include "14_SoundEffects/SoundEffects.h"
#if URHO3D_NAVIGATION
#include "15_Navigation/Navigation.h"
#endif
#if URHO3D_NETWORK
#include "16_Chat/Chat.h"
#include "17_SceneReplication/SceneReplication.h"
#endif
#if URHO3D_PHYSICS
#include "18_CharacterDemo/CharacterDemo.h"
#include "19_VehicleDemo/VehicleDemo.h"
#endif
#include "20_HugeObjectCount/HugeObjectCount.h"
#include "23_Water/Water.h"
#if URHO3D_URHO2D
#include "24_Urho2DSprite/Urho2DSprite.h"
#include "25_Urho2DParticle/Urho2DParticle.h"
#endif
#if URHO3D_SYSTEMUI
#include "26_ConsoleInput/ConsoleInput.h"
#endif
#if URHO3D_PHYSICS2D
#include "27_Physics2D/Urho2DPhysics.h"
#include "28_Physics2DRope/Urho2DPhysicsRope.h"
#endif
#include "29_SoundSynthesis/SoundSynthesis.h"
#include "30_LightAnimation/LightAnimation.h"
#include "31_MaterialAnimation/MaterialAnimation.h"
#if URHO3D_PHYSICS2D
#include "32_Physics2DConstraints/Urho2DConstraints.h"
#endif
#if URHO3D_URHO2D
#include "33_Urho2DSpriterAnimation/Urho2DSpriterAnimation.h"
#endif
#include "34_DynamicGeometry/DynamicGeometry.h"
#include "35_SignedDistanceFieldText/SignedDistanceFieldText.h"
#if URHO3D_URHO2D
#include "36_Urho2DTileMap/Urho2DTileMap.h"
#endif
#include "37_UIDrag/UIDrag.h"
#include "38_SceneAndUILoad/SceneAndUILoad.h"
#if URHO3D_NAVIGATION
#include "39_CrowdNavigation/CrowdNavigation.h"
#endif
#include "40_Localization/L10n.h"
#if URHO3D_NETWORK
#include "43_HttpRequestDemo/HttpRequestDemo.h"
#endif
#include "44_RibbonTrailDemo/RibbonTrailDemo.h"
#if URHO3D_PHYSICS
#include "46_RaycastVehicle/RaycastVehicleDemo.h"
#endif
#include "47_Typography/Typography.h"
#include "48_Hello3DUI/Hello3DUI.h"
#if URHO3D_URHO2D
#include "49_Urho2DIsometricDemo/Urho2DIsometricDemo.h"
#include "50_Urho2DPlatformer/Urho2DPlatformer.h"
#endif
#if URHO3D_NETWORK
#include "52_NATPunchtrough/NATPunchtrough.h"
#include "53_LANDiscovery/LANDiscovery.h"
#endif
#include "54_WindowSettingsDemo/WindowSettingsDemo.h"
#if URHO3D_SYSTEMUI
#include "100_HelloSystemUI/HelloSystemUI.h"
#endif
#if URHO3D_NAVIGATION
#include "106_BakedLighting/BakedLighting.h"
#endif
#if URHO3D_RMLUI
#include "107_HelloRmlUI/HelloRmlUI.h"
#endif
#include "108_RenderingShowcase/RenderingShowcase.h"
#if URHO3D_PHYSICS
#include "109_KinematicCharacter/KinematicCharacterDemo.h"
#endif
#if URHO3D_RMLUI
#if URHO3D_NETWORK
#if URHO3D_PHYSICS
#include "110_AdvancedNetworking/AdvancedNetworking.h"
#endif
#endif
#endif
#include "111_SplashScreen/SplashScreenDemo.h"
#include "112_AggregatedInput/AggregatedInput.h"
#if URHO3D_ACTIONS
#include "113_Actions/ActionDemo.h"
#endif
#if URHO3D_RMLUI
#include "114_AdvancedUI/AdvancedUI.h"
#endif
#if URHO3D_PHYSICS
#include "115_RayCast/RayCastSample.h"
#endif
#include "116_VirtualFileSystem/VFSSample.h"
#if URHO3D_PHYSICS
#include "117_PointerAdapter/PointerAdapterSample.h"
#include "118_CameraShake/CameraShake.h"
#endif
#include "Rotator.h"

#include "SamplesManager.h"

// Expands to this example's entry-point
URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::SamplesManager);

namespace Urho3D
{

SamplesManager::SamplesManager(Context* context) :
    Application(context)
{
}

void SamplesManager::Setup()
{
    // Modify engine startup parameters
    engineParameters_[EP_WINDOW_TITLE] = "Samples";
    engineParameters_[EP_APPLICATION_NAME] = "Built-in Samples";
    engineParameters_[EP_LOG_NAME]     = "conf://Samples.log";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = true;
    engineParameters_[EP_HIGH_DPI]     = true;
    engineParameters_[EP_RESOURCE_PATHS] = "CoreData;Data";
#if MOBILE
    engineParameters_[EP_ORIENTATIONS] = "Portrait";
#endif
    if (!engineParameters_.contains(EP_RESOURCE_PREFIX_PATHS))
    {
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
        if (GetPlatform() == PlatformId::MacOS ||
            GetPlatform() == PlatformId::iOS)
            engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../Resources;../..";
        else
            engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
    }
    engineParameters_[EP_AUTOLOAD_PATHS] = "Autoload";
#if DESKTOP
    GetCommandLineParser().add_option("--sample", commandLineArgsTemp_);
#endif
}

SampleSelectionScreen::SampleSelectionScreen(Context* context)
    : ApplicationState(context)
    , dpadAdapter_(context)
{
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void SampleSelectionScreen::Activate(StringVariantMap& bundle)
{
    ApplicationState::Activate(bundle);
    dpadAdapter_.SetEnabled(true);
}

void SampleSelectionScreen::Deactivate()
{
    ApplicationState::Deactivate();
    dpadAdapter_.SetEnabled(false);
}

void SamplesManager::Start()
{
    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    VirtualFileSystem* vfs = context_->GetSubsystem<VirtualFileSystem>();
    vfs->SetWatching(true);

    UI* ui = context_->GetSubsystem<UI>();

#if MOBILE
    // Scale UI for high DPI mobile screens
    auto dpi = GetSubsystem<Graphics>()->GetDisplayDPI();
    if (dpi.z_ >= 200)
        ui->SetScale(2.0f);
#endif

    // Parse command line arguments
    ea::transform(commandLineArgsTemp_.begin(), commandLineArgsTemp_.end(), ea::back_inserter(commandLineArgs_),
        [](const std::string& str) { return ea::string(str.c_str()); });

    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    context_->AddFactoryReflection<Rotator>();
    context_->AddFactoryReflection<SampleSelectionScreen>();

    inspectorNode_ = MakeShared<Scene>(context_);
    sampleSelectionScreen_ = MakeShared<SampleSelectionScreen>(context_);
    // Keyboard arrow keys are already handled by UI
    DirectionalPadAdapterFlags flags = sampleSelectionScreen_->dpadAdapter_.GetSubscriptionMask();
    flags.Set(DirectionalPadAdapterMask::Keyboard, false);
    sampleSelectionScreen_->dpadAdapter_.SetSubscriptionMask(static_cast<DirectionalPadAdapterMask>(flags));
    context_->GetSubsystem<StateManager>()->EnqueueState(sampleSelectionScreen_);

#if URHO3D_SYSTEMUI
    if (DebugHud* debugHud = context_->GetSubsystem<Engine>()->CreateDebugHud())
        debugHud->ToggleAll();
#endif
    auto* input = context_->GetSubsystem<Input>();
    SubscribeToEvent(E_RELEASED, &SamplesManager::OnClickSample);
    SubscribeToEvent(&sampleSelectionScreen_->dpadAdapter_, E_KEYUP, &SamplesManager::OnArrowKeyPress);
    SubscribeToEvent(input, E_KEYUP, &SamplesManager::OnKeyPress);
    SubscribeToEvent(E_SAMPLE_EXIT_REQUESTED, &SamplesManager::OnCloseCurrentSample);
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, &SamplesManager::OnButtonPress);
    SubscribeToEvent(E_BEGINFRAME, &SamplesManager::OnFrameStart);

#if URHO3D_RMLUI
    auto* rmlUi = context_->GetSubsystem<RmlUI>();
    rmlUi->LoadFont("Fonts/NotoSans-Condensed.ttf", false);
    rmlUi->LoadFont("Fonts/NotoSans-CondensedBold.ttf", false);
    rmlUi->LoadFont("Fonts/NotoSans-CondensedBoldItalic.ttf", false);
    rmlUi->LoadFont("Fonts/NotoSans-CondensedItalic.ttf", false);
#endif

    sampleSelectionScreen_->GetUIRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    IntVector2 listSize = VectorMin(IntVector2(300, 600), ui->GetRoot()->GetSize());
    auto* layout = sampleSelectionScreen_->GetUIRoot()->CreateChild<UIElement>();
    listViewHolder_ = layout;
    layout->SetLayoutMode(LM_VERTICAL);
    layout->SetAlignment(HA_CENTER, VA_CENTER);
    layout->SetSize(listSize);
    layout->SetStyleAuto();

    auto* list = layout->CreateChild<ListView>();
    list->SetMinSize(listSize);
    list->SetSelectOnClickEnd(true);
    list->SetHighlightMode(HM_ALWAYS);
    list->SetStyleAuto();
    list->SetName("SampleList");
    list->SetFocus(true);

    // Get logo texture
    Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/FishBoneLogo.png");
    if (!logoTexture)
        return;

    logoSprite_ = sampleSelectionScreen_->GetUIRoot()->CreateChild<Sprite>();
    logoSprite_->SetTexture(logoTexture);

    int textureWidth = logoTexture->GetWidth();
    int textureHeight = logoTexture->GetHeight();
    logoSprite_->SetScale(256.0f / textureWidth);
    logoSprite_->SetSize(textureWidth, textureHeight);
    logoSprite_->SetHotSpot(textureWidth, textureHeight);
    logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);
    logoSprite_->SetOpacity(0.9f);
    logoSprite_->SetPriority(-100);

    RegisterSample<HelloWorld>();
    RegisterSample<HelloGUI>();
    RegisterSample<Sprites>();
    RegisterSample<StaticScene>();
    RegisterSample<AnimatingScene>();
    RegisterSample<SkeletalAnimation>();
    RegisterSample<Billboards>();
    RegisterSample<Decals>();
    RegisterSample<MultipleViewports>();
    RegisterSample<RenderToTexture>();
#if URHO3D_PHYSICS
    RegisterSample<Physics>();
    RegisterSample<PhysicsStressTest>();
    RegisterSample<Ragdolls>();
#endif
    RegisterSample<SoundEffects>();
#if URHO3D_NAVIGATION
    RegisterSample<Navigation>();
#endif
#if URHO3D_NETWORK
    RegisterSample<Chat>();
    RegisterSample<SceneReplication>();
#endif
#if URHO3D_PHYSICS
    RegisterSample<CharacterDemo>();
    RegisterSample<VehicleDemo>();
#endif
    RegisterSample<HugeObjectCount>();
    RegisterSample<Water>();
#if URHO3D_URHO2D
    RegisterSample<Urho2DSprite>();
    RegisterSample<Urho2DParticle>();
#endif
#if URHO3D_SYSTEMUI
    RegisterSample<ConsoleInput>();
#endif
#if URHO3D_URHO2D
    RegisterSample<Urho2DPhysics>();
    RegisterSample<Urho2DPhysicsRope>();
#endif
    RegisterSample<SoundSynthesis>();
    RegisterSample<LightAnimation>();
    RegisterSample<MaterialAnimation>();
#if URHO3D_URHO2D
    RegisterSample<Urho2DConstraints>();
    RegisterSample<Urho2DSpriterAnimation>();
#endif
    RegisterSample<DynamicGeometry>();
    RegisterSample<SignedDistanceFieldText>();
#if URHO3D_URHO2D
    RegisterSample<Urho2DTileMap>();
#endif
    RegisterSample<UIDrag>();
    RegisterSample<SceneAndUILoad>();
#if URHO3D_NAVIGATION
    RegisterSample<CrowdNavigation>();
#endif
    RegisterSample<L10n>();
#if URHO3D_NETWORK
    RegisterSample<HttpRequestDemo>();
#endif
    RegisterSample<RibbonTrailDemo>();
#if URHO3D_PHYSICS
#if URHO3D_IK
    //RegisterSample<InverseKinematics>();
#endif
    RegisterSample<RaycastVehicleDemo>();
#endif
    RegisterSample<Typography>();
    RegisterSample<Hello3DUI>();
#if URHO3D_URHO2D
    RegisterSample<Urho2DIsometricDemo>();
    RegisterSample<Urho2DPlatformer>();
#endif
#if URHO3D_NETWORK
    RegisterSample<NATPunchtrough>();
    RegisterSample<LANDiscovery>();
#endif
    RegisterSample<WindowSettingsDemo>();
#if URHO3D_SYSTEMUI
    RegisterSample<HelloSystemUi>();
#endif
#if URHO3D_NAVIGATION
    RegisterSample<BakedLighting>();
#endif
#if URHO3D_RMLUI
    RegisterSample<HelloRmlUI>();
#endif
    RegisterSample<RenderingShowcase>();
#if URHO3D_PHYSICS
    RegisterSample<KinematicCharacterDemo>();
#endif
#if URHO3D_RMLUI
#if URHO3D_NETWORK
#if URHO3D_PHYSICS
    RegisterSample<AdvancedNetworking>();
#endif
#endif
#endif
    RegisterSample<SplashScreenDemo>();
    RegisterSample<AggregatedInput>();
#if URHO3D_ACTIONS
    RegisterSample<ActionDemo>();
#endif
#if URHO3D_RMLUI
    RegisterSample<AdvancedUI>();
#endif
#if URHO3D_PHYSICS
    RegisterSample<RayCastSample>();
#endif
    RegisterSample<VFSSample>();
#if URHO3D_PHYSICS
    RegisterSample<PointerAdapterSample>();
#endif
    RegisterSample<CameraShake>();

    if (!commandLineArgs_.empty())
        StartSample(commandLineArgs_[0]);
}

void SamplesManager::Stop()
{
    engine_->DumpResources(true);
}

void SamplesManager::OnClickSample(VariantMap& args)
{
    using namespace Released;
    StringHash sampleType = static_cast<UIElement*>(args[P_ELEMENT].GetPtr())->GetVar("SampleType").GetStringHash();
    if (!sampleType)
        return;

    StartSample(sampleType);
}

void SamplesManager::StartSample(StringHash sampleType)
{
    UI* ui = context_->GetSubsystem<UI>();
    ui->SetFocusElement(nullptr);

#if MOBILE
    Graphics* graphics = context_->GetSubsystem<Graphics>();
    graphics->SetOrientations("LandscapeLeft LandscapeRight");
    IntVector2 screenSize = graphics->GetSize();
    graphics->SetMode(Max(screenSize.x_, screenSize.y_), Min(screenSize.x_, screenSize.y_));
#endif
    StringVariantMap args;
    args["Args"] = GetArgs();
    context_->GetSubsystem<StateManager>()->EnqueueState(sampleType, args);
}

UIElement* SamplesManager::GetSampleButtonAt(int index)
{
    if (index < 0)
        return nullptr;
    ListView* listView = listViewHolder_->GetChildStaticCast<ListView>("SampleList", true);
    const auto& children = listView->GetItems();
    if (index < children.size())
        return children.at(index);
    return nullptr;
}

int SamplesManager::GetSelectedIndex() const
{
    ListView* listView = listViewHolder_->GetChildStaticCast<ListView>("SampleList", true);
    if (listView)
    {
        const auto& children = listView->GetItems();
        const eastl_size_t numButtons = children.size();
        if (numButtons > 0)
        {
            eastl_size_t currentIndex = numButtons - 1;
            for (eastl_size_t i = 0; i < numButtons; ++i)
            {
                if (children.at(i)->IsSelected())
                {
                    return static_cast<int>(i);
                    break;
                }
            }
        }
    }
    return -1;
}

void SamplesManager::OnButtonPress(VariantMap& args)
{
    if (!sampleSelectionScreen_->IsActive())
        return;
    using namespace JoystickButtonDown;
    int key = args[P_BUTTON].GetInt();
    int joystick = args[P_JOYSTICKID].GetInt();

    Input* input = context_->GetSubsystem<Input>();
    auto* state = input->GetJoystickByIndex(joystick);
    if (state->IsController())
    {
        switch (key)
        {
            case CONTROLLER_BUTTON_A:
            {
                UIElement* button = GetSampleButtonAt(GetSelectedIndex());

                if (button)
                {
                    using namespace Released;
                    StringHash sampleType = button->GetVar("SampleType").GetStringHash();
                    if (!sampleType)
                        return;

                    StartSample(sampleType);
                }
                break;
            }
        }
    }
}

void SamplesManager::OnKeyPress(VariantMap& args)
{
    using namespace KeyUp;

    int key = args[P_KEY].GetInt();

    // Close console (if open) or exit when ESC is pressed
    StateManager* stateManager = GetSubsystem<StateManager>();
    auto* currentSample = dynamic_cast<Sample*>(stateManager->GetState());
    if (key == KEY_ESCAPE && (!currentSample || currentSample->IsEscapeEnabled()))
        isClosing_ = true;

#if URHO3D_RMLUI
    if (key == KEY_I)
    {
        auto* renderer = GetSubsystem<Renderer>();
        auto* input = GetSubsystem<Input>();
        Viewport* viewport = renderer ? renderer->GetViewport(0) : nullptr;
        RenderPipelineView* renderPipelineView = viewport ? viewport->GetRenderPipelineView() : nullptr;

        if (inspectorNode_->HasComponent<RmlSerializableInspector>())
        {
            inspectorNode_->RemoveComponent<RmlSerializableInspector>();

            input->SetMouseVisible(oldMouseVisible_);
            input->SetMouseMode(oldMouseMode_);
        }
        else if (renderPipelineView)
        {
            auto inspector = inspectorNode_->CreateComponent<RmlSerializableInspector>();
            inspector->Connect(renderPipelineView->GetRenderPipeline());

            oldMouseVisible_ = input->IsMouseVisible();
            oldMouseMode_ = input->GetMouseMode();
            input->SetMouseVisible(true);
            input->SetMouseMode(MM_ABSOLUTE);
        }
    }
#endif

    if (!sampleSelectionScreen_->IsActive())
        return;

    if (key == KEY_SPACE)
    {
        UIElement* button = GetSampleButtonAt(GetSelectedIndex());

        if (button)
        {
            using namespace Released;
            StringHash sampleType = button->GetVar("SampleType").GetStringHash();
            if (!sampleType)
                return;

            StartSample(sampleType);
        }
    }
}

void SamplesManager::OnArrowKeyPress(VariantMap& args)
{
    using namespace KeyUp;

    int key = args[P_KEY].GetInt();

    if (key == KEY_DOWN)
    {
        int currentIndex = GetSelectedIndex();
        UIElement* currentSelection = GetSampleButtonAt(currentIndex);
        if (currentSelection)
            currentSelection->SetSelected(false);
        UIElement* nextSelection = GetSampleButtonAt(currentIndex + 1);
        if (nextSelection)
            nextSelection->SetSelected(true);
    }

    if (key == KEY_UP)
    {
        int currentIndex = GetSelectedIndex();
        UIElement* currentSelection = GetSampleButtonAt(currentIndex);
        if (currentSelection)
            currentSelection->SetSelected(false);
        UIElement* nextSelection = GetSampleButtonAt(currentIndex - 1);
        if (nextSelection)
            nextSelection->SetSelected(true);
    }
}

void SamplesManager::OnFrameStart()
{
    if (isClosing_)
    {
        StateManager* stateManager = context_->GetSubsystem<StateManager>();
        isClosing_ = false;
        if (stateManager->GetTargetState() != SampleSelectionScreen::GetTypeNameStatic())
        {
            Input* input = context_->GetSubsystem<Input>();
            UI* ui = context_->GetSubsystem<UI>();
            stateManager->EnqueueState(sampleSelectionScreen_);
#if MOBILE
            Graphics* graphics = context_->GetSubsystem<Graphics>();
            graphics->SetOrientations("Portrait");
            IntVector2 screenSize = graphics->GetSize();
            graphics->SetMode(Min(screenSize.x_, screenSize.y_), Max(screenSize.x_, screenSize.y_));
#endif
        }
        else
        {
#if URHO3D_SYSTEMUI
            if (auto* console = GetSubsystem<Console>())
            {
                if (console->IsVisible())
                {
                    console->SetVisible(false);
                    return;
                }
            }
#endif
#if !defined(__EMSCRIPTEN__)
            context_->GetSubsystem<Engine>()->Exit();
#endif
        }

#if URHO3D_RMLUI
        // Always close inspector
        inspectorNode_->RemoveComponent<RmlSerializableInspector>();
#endif
    }
}

void SamplesManager::OnCloseCurrentSample()
{
    isClosing_ = true;
}

template<typename T>
void SamplesManager::RegisterSample()
{
    context_->AddFactoryReflection<T>();

    auto button = MakeShared<Button>(context_);
    button->SetMinHeight(30);
    button->SetStyleAuto();
    button->SetVar("SampleType", T::GetTypeStatic());

    auto* title = button->CreateChild<Text>();
    title->SetAlignment(HA_CENTER, VA_CENTER);
    title->SetText(T::GetTypeNameStatic());
    title->SetFont(context_->GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
    title->SetStyleAuto();

    sampleSelectionScreen_->GetUIRoot()->GetChildStaticCast<ListView>("SampleList", true)->AddItem(button);
}

}
