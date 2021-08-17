//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>

TEST_CASE("Animation played for skinned model")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel();
    model->SetName("@/Models/SkinnedQuad.mdl");

    auto animation_2tx = Tests::CreateLoopedTranslationAnimation(context,
        "@/Models/SkinnedQuad_2TX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 0.0f }, 2.0f);
    auto animation_2tz = Tests::CreateLoopedTranslationAnimation(context,
        "@/Models/SkinnedQuad_2TZ.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.5f }, 2.0f);
    auto animation_1ry = Tests::CreateLoopedRotationAnimation(context,
        "@/Models/SkinnedQuad_1RY.ani", "Quad 1", Vector3::UP, 1.0f);

    cache->AddManualResource(model);
    cache->AddManualResource(animation_2tx);
    cache->AddManualResource(animation_2tz);
    cache->AddManualResource(animation_1ry);

    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<Octree>();

    auto node = scene->CreateChild("Node");
    auto animatedModel = node->CreateComponent<AnimatedModel>();
    auto animationController = node->CreateComponent<AnimationController>();

    animatedModel->SetModel(model);
    animationController->Play(animation_2tx->GetName(), 0, true);
    animationController->Play(animation_2tz->GetName(), 1, true);
    animationController->SetBlendMode(animation_2tz->GetName(), ABM_ADDITIVE);
    animationController->Play(animation_1ry->GetName(), 2, true);

    Tests::RunFrame(context, 0.5f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ 0.5f, 1.0f, 0.5f }, M_LARGE_EPSILON));

    Tests::RunFrame(context, 1.0f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ -0.5f, 1.0f, -0.5f }, M_LARGE_EPSILON));

    Tests::SerializeAndDeserializeScene(scene);

    Tests::RunFrame(context, 0.5f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
}

TEST_CASE("Animation played for nodes")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto animation_2tx = Tests::CreateLoopedTranslationAnimation(context,
        "@/Models/SkinnedQuad_2TX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 0.0f }, 2.0f);
    auto animation_1ry = Tests::CreateLoopedRotationAnimation(context,
        "@/Models/SkinnedQuad_1RY.ani", "Quad 1", Vector3::UP, 1.0f);

    cache->AddManualResource(animation_2tx);
    cache->AddManualResource(animation_1ry);

    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<Octree>();

    auto node = scene->CreateChild("Node");
    auto animationController = node->CreateComponent<AnimationController>();

    auto root = node->CreateChild("Root");
    auto quad1 = root->CreateChild("Quad 1");
    auto quad2 = quad1->CreateChild("Quad 2");

    animationController->Play(animation_2tx->GetName(), 0, true);
    animationController->Play(animation_1ry->GetName(), 1, true);

    Tests::RunFrame(context, 0.5f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ 0.5f, 1.0f, 0.0f }, M_LARGE_EPSILON));

    Tests::RunFrame(context, 1.0f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ -0.5f, 1.0f, 0.0f }, M_LARGE_EPSILON));

    // TODO(animation): It doesn't work in Urho
    //Tests::SerializeAndDeserializeScene(scene);

    Tests::RunFrame(context, 0.5f, 0.05f);
    REQUIRE(scene->GetChild("Quad 2", true)->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
}
