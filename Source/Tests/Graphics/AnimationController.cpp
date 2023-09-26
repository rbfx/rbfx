//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//


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
