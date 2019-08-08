//
// Copyright (c) 2017-2019 the rbfx project.
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
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
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
#include "11_Physics/Physics.h"
#include "12_PhysicsStressTest/PhysicsStressTest.h"
#include "13_Ragdolls/Ragdolls.h"
#include "14_SoundEffects/SoundEffects.h"
#include "15_Navigation/Navigation.h"
#include "16_Chat/Chat.h"
#include "17_SceneReplication/SceneReplication.h"
#include "18_CharacterDemo/CharacterDemo.h"
#include "19_VehicleDemo/VehicleDemo.h"
#include "20_HugeObjectCount/HugeObjectCount.h"
#include "23_Water/Water.h"
#include "24_Urho2DSprite/Urho2DSprite.h"
#include "25_Urho2DParticle/Urho2DParticle.h"
#include "26_ConsoleInput/ConsoleInput.h"
#include "27_Urho2DPhysics/Urho2DPhysics.h"
#include "28_Urho2DPhysicsRope/Urho2DPhysicsRope.h"
#include "29_SoundSynthesis/SoundSynthesis.h"
#include "30_LightAnimation/LightAnimation.h"
#include "31_MaterialAnimation/MaterialAnimation.h"
#include "32_Urho2DConstraints/Urho2DConstraints.h"
#include "33_Urho2DSpriterAnimation/Urho2DSpriterAnimation.h"
#include "34_DynamicGeometry/DynamicGeometry.h"
#include "35_SignedDistanceFieldText/SignedDistanceFieldText.h"
#include "36_Urho2DTileMap/Urho2DTileMap.h"
#include "37_UIDrag/UIDrag.h"
#include "38_SceneAndUILoad/SceneAndUILoad.h"
#include "39_CrowdNavigation/CrowdNavigation.h"
#include "40_Localization/L10n.h"
#include "42_PBRMaterials/PBRMaterials.h"
#include "43_HttpRequestDemo/HttpRequestDemo.h"
#include "44_RibbonTrailDemo/RibbonTrailDemo.h"
#include "45_InverseKinematics/InverseKinematics.h"
#include "46_RaycastVehicle/RaycastVehicleDemo.h"
#include "47_Typography/Typography.h"
#include "48_Hello3DUI/Hello3DUI.h"
#include "49_Urho2DIsometricDemo/Urho2DIsometricDemo.h"
#include "50_Urho2DPlatformer/Urho2DPlatformer.h"
#include "52_NATPunchtrough/NATPunchtrough.h"
#include "53_LANDiscovery/LANDiscovery.h"
#include "100_HelloSystemUI/HelloSystemUI.h"
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
    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_LOG_NAME]     = GetSubsystem<FileSystem>()->GetAppPreferencesDir("rbfx", "samples") + GetTypeName() + ".log";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = true;
    engineParameters_[EP_WINDOW_TITLE] = "rbfx samples";

    if (!engineParameters_.contains(EP_RESOURCE_PREFIX_PATHS))
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
}

void SamplesManager::Start()
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    context_->RegisterFactory<Rotator>();

    SubscribeToEvent(E_EXITREQUESTED, [this](StringHash, VariantMap& args) { OnExitRequested(args); });
    SubscribeToEvent(E_RELEASED, [this](StringHash, VariantMap& args) { OnClickSample(args); });
    SubscribeToEvent(E_KEYUP, [this](StringHash, VariantMap& args) { OnKeyPress(args); });

    GetInput()->SetMouseMode(MM_FREE);
    GetInput()->SetMouseVisible(true);

    GetUI()->GetRoot()->SetDefaultStyle(GetCache()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    GetEngine()->SetAutoExit(false);

    auto* layout = GetUI()->GetRoot()->CreateChild<UIElement>();
    listViewHolder_ = layout;
    layout->SetLayoutMode(LM_VERTICAL);
    layout->SetAlignment(HA_CENTER, VA_CENTER);
    layout->SetSize(300, 600);
    layout->SetStyleAuto();

    auto* list = layout->CreateChild<ListView>();
    list->SetMinSize(300, 600);
    list->SetSelectOnClickEnd(true);
    list->SetHighlightMode(HM_ALWAYS);
    list->SetStyleAuto();
    list->SetName("SampleList");

    // Get logo texture
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/rbfx-logo.png");
    if (!logoTexture)
        return;

    UI* ui = GetSubsystem<UI>();
    logoSprite_ = ui->GetRoot()->CreateChild<Sprite>();
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
    RegisterSample<CharacterDemo>();
    RegisterSample<VehicleDemo>();
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
#if !WEB
    RegisterSample<PBRMaterials>();
#endif
#if URHO3D_NETWORK
    RegisterSample<HttpRequestDemo>();
#endif
    RegisterSample<RibbonTrailDemo>();
#if URHO3D_PHYSICS
#if URHO3D_IK
    RegisterSample<InverseKinematics>();
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
#if URHO3D_SYSTEMUI
    RegisterSample<HelloSystemUi>();
#endif
}

void SamplesManager::Stop()
{
    engine_->DumpResources(true);
}

void SamplesManager::OnExitRequested(VariantMap& args)
{
    if (runningSample_.NotNull())
    {
        runningSample_->Stop();
        runningSample_ = nullptr;
        GetInput()->SetMouseMode(MM_FREE);
        GetInput()->SetMouseVisible(true);
        GetUI()->SetCursor(nullptr);
        GetUI()->GetRoot()->RemoveAllChildren();
        GetUI()->GetRoot()->AddChild(listViewHolder_);
        GetUI()->GetRoot()->AddChild(logoSprite_);
        SubscribeToEvent(E_RELEASED, [this](StringHash, VariantMap& args) { OnClickSample(args); });
        SubscribeToEvent(E_KEYUP, [this](StringHash, VariantMap& args) { OnKeyPress(args); });
        exitTime_ = GetTime()->GetElapsedTime();
    }
    else
        GetEngine()->Exit();
}

void SamplesManager::OnClickSample(VariantMap& args)
{
    using namespace Released;
    StringHash sampleType = static_cast<UIElement*>(args[P_ELEMENT].GetPtr())->GetVar("SampleType").GetStringHash();
    if (!sampleType)
        return;

    UnsubscribeFromEvent(E_RELEASED);
    UnsubscribeFromEvent(E_KEYUP);
    GetUI()->GetRoot()->RemoveAllChildren();
    GetUI()->SetFocusElement(nullptr);

    runningSample_.StaticCast<Object>(context_->CreateObject(sampleType));
    runningSample_->Start();
}

void SamplesManager::OnKeyPress(VariantMap& args)
{
    using namespace KeyUp;

    int key = args[P_KEY].GetInt();

    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_ESCAPE && (GetTime()->GetElapsedTime() - exitTime_) > 1)
        SendEvent(E_EXITREQUESTED);
}

template<typename T>
void SamplesManager::RegisterSample()
{
    context_->RegisterFactory<T>();

    auto* button = context_->CreateObject<Button>().Detach();
    button->SetMinHeight(30);
    button->SetStyleAuto();
    button->SetVar("SampleType", T::GetTypeStatic());

    auto* title = button->CreateChild<Text>();
    title->SetAlignment(HA_CENTER, VA_CENTER);
    title->SetText(T::GetTypeNameStatic());
    title->SetFont(GetCache()->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
    title->SetStyleAuto();

    GetUI()->GetRoot()->GetChildStaticCast<ListView>("SampleList", true)->AddItem(button);
}
}
