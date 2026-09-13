// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "SplashScreenDemo.h"
#include "SamplesManager.h"

#include <Urho3D/Scene/PrefabResource.h>

SplashScreenDemo::SplashScreenDemo(Context* context)
    : Sample(context)
{
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void SplashScreenDemo::Activate(StringVariantMap& bundle)
{
    Sample::Activate(bundle);

    auto splashScreen = MakeShared<SplashScreen>(context_);

    splashScreen->QueueSceneResourcesAsync("Scenes/RenderingShowcase_0.xml");
    splashScreen->QueueResource<PrefabResource>("Prefabs/AdvancedNetworkingPlayer.prefab");
    splashScreen->SetBackgroundImage(
        context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/StoneDiffuse.dds"));
    splashScreen->SetForegroundImage(
        context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/LogoLarge.png"));
    splashScreen->SetProgressImage(
        context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/TerrainDetail2.dds"));
    splashScreen->SetDuration(1.0f);
    splashScreen->SetSkippable(true);

    auto * stateManager = context_->GetSubsystem<StateManager>();
    stateManager->EnqueueState(splashScreen);
    stateManager->EnqueueState(SampleSelectionScreen::GetTypeStatic());
}
