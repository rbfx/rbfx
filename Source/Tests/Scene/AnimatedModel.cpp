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
#include "../SceneUtils.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>

TEST_CASE("Lerp animation blending")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel("@/SkinnedQuad.mdl");
    cache->AddManualResource(model);

    auto animationRotate = Tests::CreateLoopedRotationAnimation(context,
        "@/Rotate.ani", "Quad 1", Vector3::UP, 2.0f);
    auto animationTranslateX = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto animationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateZ.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);

    cache->AddManualResource(animationRotate);
    cache->AddManualResource(animationTranslateX);
    cache->AddManualResource(animationTranslateZ);

    // Test AnimatedModel mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/Rotate.ani", 0, true);
        animationController->Play("@/TranslateX.ani", 0, true);
        animationController->Play("@/TranslateZ.ani", 1, true);
        animationController->SetWeight("@/TranslateZ.ani", 0.75f);

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1 * 25%, Translate Z to -2 * 75%, Rotate 90 degrees (X to -Z, Z to X)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.5f, 1.0f, 0.25f }, M_LARGE_EPSILON));

        // Time 1.0: Translate X to 0 * 25%, Translate Z to 0 * 75%, Rotate 180 degrees (X to -X, Z to -Z)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1 * 25%, Translate Z to 2 * 75%, Rotate 270 degrees (X to Z, Z to -X)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.5f, 1.0f, 0.25f }, M_LARGE_EPSILON));

        // Time 2.0: Translate X to 0 * 25%, Translate Z to 0 * 75%, Rotate 360 degrees (identity)
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    }

    // Test Node mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto nodeQuad1 = node->CreateChild("Quad 1");
        auto nodeQuad2 = nodeQuad1->CreateChild("Quad 2");

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/Rotate.ani", 0, true);
        animationController->Play("@/TranslateX.ani", 0, true);
        animationController->Play("@/TranslateZ.ani", 1, true);
        animationController->SetWeight("@/TranslateZ.ani", 0.75f);

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1 * 25%, Translate Z to -2 * 75%, Rotate 90 degrees (X to -Z, Z to X)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.5f, 1.0f, 0.25f }, M_LARGE_EPSILON));

        // Time 1.0: Translate X to 0 * 25%, Translate Z to 0 * 75%, Rotate 180 degrees (X to -X, Z to -Z)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1 * 25%, Translate Z to 2 * 75%, Rotate 270 degrees (X to Z, Z to -X)
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.5f, 1.0f, 0.25f }, M_LARGE_EPSILON));

        // Time 2.0: Translate X to 0 * 25%, Translate Z to 0 * 75%, Rotate 360 degrees (identity)
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    }
}

TEST_CASE("Additive animation blending")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel("@/SkinnedQuad.mdl");
    cache->AddManualResource(model);

    auto modelAnimationTranslateX = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto modelAnimationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateZ_Model.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);
    auto nodeAnimationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateZ_Node.ani", "Quad 2", { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);

    cache->AddManualResource(modelAnimationTranslateX);
    cache->AddManualResource(modelAnimationTranslateZ);
    cache->AddManualResource(nodeAnimationTranslateZ);

    // Test AnimatedModel mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateX.ani", 0, true);
        animationController->Play("@/TranslateZ_Model.ani", 1, true);
        animationController->SetWeight("@/TranslateZ_Model.ani", 0.75f);
        animationController->SetBlendMode("@/TranslateZ_Model.ani", ABM_ADDITIVE);

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1 * 100%, Translate Z to -2 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.0f, 1.0f, -1.5f }, M_LARGE_EPSILON));

        // Time 1.0: Translate X to 0 * 100%, Translate Z to 0 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1 * 100%, Translate Z to 2 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 1.0f, 1.0f, 1.5f }, M_LARGE_EPSILON));

        // Time 2.0: Translate X to 0 * 100%, Translate Z to 0 * 75%
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    }

    // Test Node mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto nodeQuad1 = node->CreateChild("Quad 1");
        auto nodeQuad2 = nodeQuad1->CreateChild("Quad 2");

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateX.ani", 0, true);
        animationController->Play("@/TranslateZ_Node.ani", 1, true);
        animationController->SetWeight("@/TranslateZ_Node.ani", 0.75f);
        animationController->SetBlendMode("@/TranslateZ_Node.ani", ABM_ADDITIVE);

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1 * 100%, Translate Z to -2 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.0f, 1.0f, -1.5f }, M_LARGE_EPSILON));

        // Time 1.0: Translate X to 0 * 100%, Translate Z to 0 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1 * 100%, Translate Z to 2 * 75%
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 1.0f, 1.0f, 1.5f }, M_LARGE_EPSILON));

        // Time 2.0: Translate X to 0 * 100%, Translate Z to 0 * 75%
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    }
}

TEST_CASE("Animation start bone")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel("@/SkinnedQuad.mdl");
    cache->AddManualResource(model);

    auto animationTranslateX = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateX.ani", "Quad 1", { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto animationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "@/TranslateZ.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);
    auto animation = Tests::CreateCombinedAnimation(context,
        "@/TranslateXZ.ani", { animationTranslateX, animationTranslateZ });

    cache->AddManualResource(animation);

    // Test AnimatedModel mode: both animations
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("@/TranslateXZ.ani", "Quad 1");

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1, Translate Z to -2
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.0f, 1.0f, -2.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1, Translate Z to 2
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 1.0f, 1.0f, 2.0f }, M_LARGE_EPSILON));
    }

    // Test AnimatedModel mode: one animation
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("@/TranslateXZ.ani", "Quad 2");

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate Z to -2
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, -2.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate Z to 2
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 2.0f }, M_LARGE_EPSILON));
    }

    // Test Node mode: both animations
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto nodeQuad1 = node->CreateChild("Quad 1");
        auto nodeQuad2 = nodeQuad1->CreateChild("Quad 2");

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("@/TranslateXZ.ani", "Quad 1");

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate X to -1, Translate Z to -2
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ -1.0f, 1.0f, -2.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1, Translate Z to 2
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 1.0f, 1.0f, 2.0f }, M_LARGE_EPSILON));
    }

    // Test Node mode: one animation
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto nodeQuad1 = node->CreateChild("Quad 1");
        auto nodeQuad2 = nodeQuad1->CreateChild("Quad 2");

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("@/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("@/TranslateXZ.ani", "Quad 2");

        // Assert
        Tests::NodeRef quad2{ scene, "Quad 2" };

        // Time 0.5: Translate Z to -2
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, -2.0f }, M_LARGE_EPSILON));

        // Time 1.5: Translate Z to 2
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 2.0f }, M_LARGE_EPSILON));
    }
}
