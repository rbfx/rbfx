// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../ModelUtils.h"
#include "Urho3D/Resource/ResourceCache.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/AnimationController.h>

TEST_CASE("AnimationController should remove animation on completion")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild();
    auto controller = node->CreateComponent<AnimationController>();
    AnimationParameters params(
        context->GetSubsystem<ResourceCache>()->GetResource<Animation>("Animations/SlidingDoor/Open.xml"));
    params.removeOnCompletion_ = true;
    controller->PlayNewExclusive(params, 0.0f);
    const auto length = params.GetAnimation()->GetLength();
    Tests::RunFrame(context, length + 1.0f);

    CHECK(0 == controller->GetNumAnimations());
}
