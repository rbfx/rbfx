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
