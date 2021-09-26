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
#include <Urho3D/UI/Text3D.h>

TEST_CASE("Lerp animation blending")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel("@/SkinnedQuad.mdl");
    cache->AddManualResource(model);

    auto animationRotate = Tests::CreateLoopedRotationAnimation(context,
        "Tests/Rotate.ani", "Quad 1", Vector3::UP, 2.0f);
    auto animationTranslateX = Tests::CreateLoopedTranslationAnimation(context,
        "Tests/TranslateX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto animationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "Tests/TranslateZ.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);

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
        animationController->Play("Tests/Rotate.ani", 0, true);
        animationController->Play("Tests/TranslateX.ani", 0, true);
        animationController->Play("Tests/TranslateZ.ani", 1, true);
        animationController->SetWeight("Tests/TranslateZ.ani", 0.75f);

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
        animationController->Play("Tests/Rotate.ani", 0, true);
        animationController->Play("Tests/TranslateX.ani", 0, true);
        animationController->Play("Tests/TranslateZ.ani", 1, true);
        animationController->SetWeight("Tests/TranslateZ.ani", 0.75f);

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
        "Tests/TranslateX.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto modelAnimationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "Tests/TranslateZ_Model.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);
    auto nodeAnimationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "Tests/TranslateZ_Node.ani", "Quad 2", { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);

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
        animationController->Play("Tests/TranslateX.ani", 0, true);
        animationController->Play("Tests/TranslateZ_Model.ani", 1, true);
        animationController->SetWeight("Tests/TranslateZ_Model.ani", 0.75f);
        animationController->SetBlendMode("Tests/TranslateZ_Model.ani", ABM_ADDITIVE);

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
        animationController->Play("Tests/TranslateX.ani", 0, true);
        animationController->Play("Tests/TranslateZ_Node.ani", 1, true);
        animationController->SetWeight("Tests/TranslateZ_Node.ani", 0.75f);
        animationController->SetBlendMode("Tests/TranslateZ_Node.ani", ABM_ADDITIVE);

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
        "Tests/TranslateX.ani", "Quad 1", { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
    auto animationTranslateZ = Tests::CreateLoopedTranslationAnimation(context,
        "Tests/TranslateZ.ani", "Quad 2", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 2.0f);
    auto animation = Tests::CreateCombinedAnimation(context,
        "Tests/TranslateXZ.ani", { animationTranslateX, animationTranslateZ });

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
        animationController->Play("Tests/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("Tests/TranslateXZ.ani", "Quad 1");

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
        animationController->Play("Tests/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("Tests/TranslateXZ.ani", "Quad 2");

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
        animationController->Play("Tests/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("Tests/TranslateXZ.ani", "Quad 1");

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
        animationController->Play("Tests/TranslateXZ.ani", 0, true);
        animationController->SetStartBone("Tests/TranslateXZ.ani", "Quad 2");

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

TEST_CASE("Variant animation tracks")
{
    auto context = Tests::CreateCompleteTestContext();
    auto cache = context->GetSubsystem<ResourceCache>();

    // Prepare resources
    auto model = Tests::CreateSkinnedQuad_Model(context)->ExportModel("@/SkinnedQuad.mdl");
    cache->AddManualResource(model);

    {
        auto animation = MakeShared<Animation>(context);
        animation->SetName("Tests/Animation1.ani");
        animation->SetLength(1.0f);
        {
            AnimationTrack* track = animation->CreateTrack("Quad 2");
            track->channelMask_ = CHANNEL_POSITION;

            track->AddKeyFrame({ 0.0f, Vector3::ONE });
            track->AddKeyFrame({ 0.6f, Vector3::ZERO });
        }
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("Child Node/@Text3D/Text");
            track->AddKeyFrame({ 0.0f, Variant("A") });
            track->AddKeyFrame({ 0.4f, Variant("B") });
            track->Commit();
        }
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("Child Node/@Text3D/Font Size");
            track->AddKeyFrame({ 0.0f, 10.0f });
            track->AddKeyFrame({ 0.4f, 20.0f });
            track->Commit();
        }
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("@/Variables/Test");
            track->AddKeyFrame({ 0.0f, Variant(10) });
            track->AddKeyFrame({ 0.4f, Variant(20) });
            track->Commit();
        }
        cache->AddManualResource(animation);
    }
    {
        auto animation = MakeShared<Animation>(context);
        animation->SetName("Tests/Animation2.ani");
        animation->SetLength(1.0f);
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("Child Node/@Text3D/Font Size");
            track->AddKeyFrame({ 0.0f, 20.0f });
            track->AddKeyFrame({ 0.4f, 30.0f });
            track->Commit();
        }
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("@/Variables/Test");
            track->AddKeyFrame({ 0.0f, Variant(20) });
            track->AddKeyFrame({ 0.4f, Variant(30) });
            track->Commit();
        }
        cache->AddManualResource(animation);
    }
    {
        auto animation = MakeShared<Animation>(context);
        animation->SetName("Tests/Animation3.ani");
        animation->SetLength(1.0f);
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("Child Node/@Text3D/Font Size");
            track->baseValue_ = 11.0f;
            track->AddKeyFrame({ 0.0f, 12.0f });
            track->AddKeyFrame({ 0.4f, 16.0f });
            track->Commit();
        }
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("@/Variables/Test");
            track->baseValue_ = 11;
            track->AddKeyFrame({ 0.0f, Variant(12) });
            track->AddKeyFrame({ 0.4f, Variant(16) });
            track->Commit();
        }
        cache->AddManualResource(animation);
    }

    // Setup
    auto scene = MakeShared<Scene>(context);
    {
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Root Node");
        node->SetPosition({ 0.0f, 1.0f, 0.0f });
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto childNode = node->CreateChild("Child Node");
        auto text = childNode->CreateComponent<Text3D>();

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->Play("Tests/Animation1.ani", 0, false);
        animationController->Play("Tests/Animation2.ani", 1, false);
        animationController->SetWeight("Tests/Animation2.ani", 0.5f);
        animationController->Play("Tests/Animation3.ani", 2, false);
        animationController->SetBlendMode("Tests/Animation3.ani", ABM_ADDITIVE);
        animationController->SetWeight("Tests/Animation3.ani", 0.5f);
    }

    // Assert
    Tests::NodeRef rootNode{ scene, "Root Node" };
    Tests::NodeRef quad2{ scene, "Quad 2" };
    Tests::ComponentRef<Text3D> childNodeText{ scene, "Child Node" };

    // [Time = 0.3]
    // Quad 2: Translate to 0.5
    // Test:
    // - Animation1: Lerp(10, 20, 0.75) = 17(.5)
    // - Animation2: Lerp(20, 30, 0.75) = 27(.5)
    // - Animation3: Lerp(12, 16, 0.75) - 11 = 4
    // - Final: Lerp(Animation1, Animation2, 0.5) + Animation3 * 0.5 = 24(.5)
    Tests::RunFrame(context, 0.3f, 0.5f);
    REQUIRE(quad2->GetWorldPosition().Equals({ 0.5f, 1.5f, 0.5f }, M_LARGE_EPSILON));
    REQUIRE(rootNode->GetVar("Test") == Variant(24));
    REQUIRE(childNodeText->GetFontSize() == 24.5f);
    REQUIRE(childNodeText->GetText() == "A");

    // [Time = 1.0]
    // Quad 2: Translate X to 0
    // Test:
    // - Animation1: Lerp(10, 20, 1.0) = 20
    // - Animation2: Lerp(20, 30, 1.0) = 30
    // - Animation3: Lerp(12, 16, 1.0) - 11 = 5
    // - Final: Lerp(Animation1, Animation2, 0.5) + Animation3 * 0.5 = 27(.5)
    Tests::SerializeAndDeserializeScene(scene);
    Tests::RunFrame(context, 0.3f, 0.5f);
    REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    REQUIRE(rootNode->GetVar("Test") == Variant(27));
    REQUIRE(childNodeText->GetFontSize() == 27.5f);
    REQUIRE(childNodeText->GetText() == "B");
}
