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
#include <Urho3D/UI/Text3D.h>

namespace
{

SharedPtr<Model> CreateTestSkinnedModel(Context* context)
{
    return Tests::CreateSkinnedQuad_Model(context)->ExportModel();
}

SharedPtr<Animation> CreateTestTranslateXAnimation(Context* context)
{
    return Tests::CreateLoopedTranslationAnimation(context, "", "Quad 2", {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 2.0f);
}

SharedPtr<Animation> CreateTestTranslateZAnimation(Context* context)
{
    return Tests::CreateLoopedTranslationAnimation(context, "", "Quad 2", {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, 2.0f);
}

SharedPtr<Animation> CreateTestTranslateXZAnimation(Context* context)
{
    const auto translateX = Tests::CreateLoopedTranslationAnimation(context, "", "Quad 1", {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 2.0f);
    const auto translateZ = Tests::CreateLoopedTranslationAnimation(context, "", "Quad 2", {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, 2.0f);
    return Tests::CreateCombinedAnimation(context, "", {translateX, translateZ});
}

SharedPtr<Animation> CreateTestRotationAnimation(Context* context)
{
    return Tests::CreateLoopedRotationAnimation(context, "", "Quad 1", Vector3::UP, 2.0f);
}

SharedPtr<Animation> CreateTestUnnamedTranslateXAnimation(Context* context)
{
    return Tests::CreateLoopedTranslationAnimation(context, "", "", { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 2.0f);
}

SharedPtr<Animation> CreateTestVariantAnimation1(Context* context)
{
    auto animation = MakeShared<Animation>(context);
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
    return animation;
}

SharedPtr<Animation> CreateTestVariantAnimation2(Context* context)
{
    auto animation = MakeShared<Animation>(context);
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
    return animation;
}

SharedPtr<Animation> CreateTestVariantAnimation3(Context* context)
{
    auto animation = MakeShared<Animation>(context);
    animation->SetLength(1.0f);
    {
        VariantAnimationTrack* track = animation->CreateVariantTrack("Child Node/@Text3D/Font Size");
        track->AddKeyFrame({ 0.0f, 12.0f });
        track->AddKeyFrame({ 0.4f, 16.0f });
        track->Commit();
    }
    {
        VariantAnimationTrack* track = animation->CreateVariantTrack("@/Variables/Test");
        track->AddKeyFrame({ 0.0f, Variant(12) });
        track->AddKeyFrame({ 0.4f, Variant(16) });
        track->Commit();
    }
    return animation;
}

}

TEST_CASE("Animation of hierarchical AnimatedModels is stable")
{
    const int numNodes = 20;
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateX.ani", CreateTestTranslateXAnimation);

    // Create hierarchical scene
    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<Octree>();

    ea::vector<Node*> nodes;
    Node* parent = scene;
    for (int i = 0; i < numNodes; ++i)
    {
        Node* child = parent->CreateChild("Child");
        nodes.push_back(child);

        auto animatedModel = child->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto controller = child->CreateComponent<AnimationController>();
        controller->Play(animationTranslateX->GetName(), 0, true);

        parent = child->GetChild("Quad 2", true);
    }

    const auto getNodePositions = [&]()
    {
        ea::vector<Vector3> positions;
        for (Node* node : nodes)
            positions.push_back(node->GetWorldPosition());
        return positions;
    };

    // Run time and expect precise animations
    Tests::RunFrame(context, 1.0f / 16, 1.0f / 64);
    const auto positions = getNodePositions();
    for (int i = 0; i < numNodes; ++i)
        REQUIRE(positions[i].x_ == i * -0.125f);
}

TEST_CASE("Animations are blended with linear interpolation")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateX.ani", CreateTestTranslateXAnimation);
    auto animationTranslateZ = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateZ.ani", CreateTestTranslateZAnimation);
    auto animationRotate = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/Rotation.ani", CreateTestRotationAnimation);

    // Test AnimatedModel mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animationRotate}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateZ}.Looped().Layer(1).Weight(0.75f));

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
        animationController->PlayNew(AnimationParameters{animationRotate}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateZ}.Looped().Layer(1).Weight(0.75f));

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

TEST_CASE("Animations are blended additively")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateX.ani", CreateTestTranslateXAnimation);
    auto animationTranslateZ = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateZ.ani", CreateTestTranslateZAnimation);

    // Test AnimatedModel mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateZ}.Looped().Additive().Layer(1).Weight(0.75f));

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
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());
        animationController->PlayNew(AnimationParameters{animationTranslateZ}.Looped().Additive().Layer(1).Weight(0.75f));

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

TEST_CASE("Animation track with empty name is applied to the owner node itself")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/UnnamedTranslateX.ani", CreateTestUnnamedTranslateXAnimation);

    // Test AnimatedModel mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());

        Tests::NodeRef nodeRef{ scene, "Node" };
        Tests::NodeRef rootRef{ scene, "Root" };

        // Time 0.5: Translate X to -1
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(nodeRef);
        CHECK(nodeRef->GetPosition().Equals({-1.0f, 0.0f, 0.0f}, M_LARGE_EPSILON));
        REQUIRE(rootRef);
        CHECK(rootRef->GetPosition().Equals(Vector3::ZERO, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(nodeRef);
        CHECK(nodeRef->GetPosition().Equals({1.0f, 0.0f, 0.0f}, M_LARGE_EPSILON));
        REQUIRE(rootRef);
        CHECK(rootRef->GetPosition().Equals(Vector3::ZERO, M_LARGE_EPSILON));
    }

    // Test Node mode
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto child = node->CreateChild();

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());

        Tests::NodeRef nodeRef{ scene, "Node" };
        Tests::NodeRef childRef{ scene, "" };

        // Time 0.5: Translate X to -1
        Tests::RunFrame(context, 0.5f, 0.05f);
        REQUIRE(nodeRef->GetPosition().Equals({-1.0f, 0.0f, 0.0f}, M_LARGE_EPSILON));
        REQUIRE(childRef->GetPosition().Equals(Vector3::ZERO, M_LARGE_EPSILON));

        // Time 1.5: Translate X to 1
        Tests::SerializeAndDeserializeScene(scene);
        Tests::RunFrame(context, 1.0f, 0.05f);
        REQUIRE(nodeRef->GetPosition().Equals({1.0f, 0.0f, 0.0f}, M_LARGE_EPSILON));
        REQUIRE(childRef->GetPosition().Equals(Vector3::ZERO, M_LARGE_EPSILON));
    }
}

TEST_CASE("Animation is filtered when start bone is specified")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animation = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateXZ.ani", CreateTestTranslateXZAnimation);

    // Test AnimatedModel mode: both animations
    {
        // Setup
        auto scene = MakeShared<Scene>(context);
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Node");
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animation}.Looped().StartBone("Quad 1"));

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
        animationController->PlayNew(AnimationParameters{animation}.Looped().StartBone("Quad 2"));

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
        animationController->PlayNew(AnimationParameters{animation}.Looped().StartBone("Quad 1"));

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
        animationController->PlayNew(AnimationParameters{animation}.Looped().StartBone("Quad 2"));

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

TEST_CASE("VariantCurve is sample with looping and without it")
{
    VariantCurve curve;
    auto white = Color(1.0f, 1.0f, 1.0f, 1.0f);
    auto black = Color(0.0f, 0.0f, 0.0f, 0.0f);
    curve.AddKeyFrame(VariantCurvePoint{0.0f, white});
    curve.AddKeyFrame(VariantCurvePoint{0.99f, white});
    curve.AddKeyFrame(VariantCurvePoint{1.0f, black});
    curve.Commit();

    unsigned frameIndex;
    Color unloopedValue = curve.Sample(1.0f + M_EPSILON / 2.0f, 1.0f, false, frameIndex).GetColor();
    CHECK(unloopedValue.Equals(black));
    Color loopedValue = curve.Sample(1.0f + M_EPSILON / 2.0f, 1.0f, true, frameIndex).GetColor();
    CHECK(loopedValue.Equals(white));
}

TEST_CASE("Variant animation tracks are applied to components with optional blending")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Prepare resources
    auto model = Tests::GetOrCreateResource<Model>(context, "@Tests/AnimationController/SkinnedModel.mdl", CreateTestSkinnedModel);
    auto animation1 = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/VariantAnimation1.ani", CreateTestVariantAnimation1);
    auto animation2 = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/VariantAnimation2.ani", CreateTestVariantAnimation2);
    auto animation3 = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/VariantAnimation3.ani", CreateTestVariantAnimation3);

    // Setup
    auto scene = MakeShared<Scene>(context);
    {
        scene->CreateComponent<Octree>();

        auto node = scene->CreateChild("Root Node");
        node->SetVar("Test", 0);
        node->SetPosition({ 0.0f, 1.0f, 0.0f });
        auto animatedModel = node->CreateComponent<AnimatedModel>();
        animatedModel->SetModel(model);

        auto childNode = node->CreateChild("Child Node");
        auto text = childNode->CreateComponent<Text3D>();

        auto animationController = node->CreateComponent<AnimationController>();
        animationController->PlayNew(AnimationParameters{animation1});
        animationController->PlayNew(AnimationParameters{animation2}.Layer(1).Weight(0.5f));
        animationController->PlayNew(AnimationParameters{animation3}.Layer(2).Additive().Weight(0.5f));
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
    // - Animation3: Lerp(12, 16, 0.75) - 12 = 3
    // - Final: Lerp(Animation1, Animation2, 0.5) + Animation3 * 0.5 = 23
    Tests::RunFrame(context, 0.3f, 0.5f);
    REQUIRE(quad2->GetWorldPosition().Equals({ 0.5f, 1.5f, 0.5f }, M_LARGE_EPSILON));
    REQUIRE(rootNode->GetVar("Test") == Variant(23));
    REQUIRE(childNodeText->GetFontSize() == 24.0f);
    REQUIRE(childNodeText->GetText() == "A");

    // [Time = 1.0]
    // Quad 2: Translate X to 0
    // Test:
    // - Animation1: Lerp(10, 20, 1.0) = 20
    // - Animation2: Lerp(20, 30, 1.0) = 30
    // - Animation3: Lerp(12, 16, 1.0) - 12 = 4
    // - Final: Lerp(Animation1, Animation2, 0.5) + Animation3 * 0.5 = 27
    Tests::SerializeAndDeserializeScene(scene);
    Tests::RunFrame(context, 0.3f, 0.5f);
    REQUIRE(quad2->GetWorldPosition().Equals({ 0.0f, 1.0f, 0.0f }, M_LARGE_EPSILON));
    REQUIRE(rootNode->GetVar("Test") == Variant(27));
    REQUIRE(childNodeText->GetFontSize() == 27.0f);
    REQUIRE(childNodeText->GetText() == "B");
}

TEST_CASE("AnimationController merges animations from external state")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateX.ani", CreateTestTranslateXAnimation);
    auto animationTranslateZ = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateZ.ani", CreateTestTranslateZAnimation);
    auto animationRotate = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/Rotation.ani", CreateTestRotationAnimation);

    // Setup scene
    auto scene = MakeShared<Scene>(context);

    auto node = scene->CreateChild("Node");
    node->CreateChild("Quad 1")->CreateChild("Quad 2");
    auto animationController = node->CreateComponent<AnimationController>();

    // Play animations
    // The only non-looped animation will end in 0.5 seconds and will be fading out 0.5 more seconds
    animationController->PlayNew(AnimationParameters{animationRotate}.Looped(), 0.5f);
    animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped().Speed(2.0f));
    animationController->PlayNew(AnimationParameters{animationTranslateX}.Speed(4.0f).AutoFadeOut(0.5f));
    animationController->PlayNew(AnimationParameters{animationTranslateZ}.Looped().Additive().Layer(1).Weight(0.75f));

    // Spend some time
    animationController->Update(0.25f);
    const auto animations_0_25 = animationController->GetAnimationParameters();

    animationController->Update(0.5f);
    const auto animations_0_75 = animationController->GetAnimationParameters();

    REQUIRE(animations_0_75.size() == 4);

    REQUIRE(animations_0_75[0].GetAnimation() == animationRotate);
    REQUIRE(animations_0_75[0].GetTime() == 0.75f);
    REQUIRE(animations_0_75[0].weight_ == 1.0f);

    REQUIRE(animations_0_75[1].GetAnimation() == animationTranslateX);
    REQUIRE(animations_0_75[1].GetTime() == 1.5f);
    REQUIRE(animations_0_75[1].weight_ == 1.0f);

    REQUIRE(animations_0_75[2].GetAnimation() == animationTranslateX);
    REQUIRE(animations_0_75[2].GetTime() == 2.0f);
    REQUIRE(animations_0_75[2].weight_ == 0.5f);

    REQUIRE(animations_0_75[3].GetAnimation() == animationTranslateZ);
    REQUIRE(animations_0_75[3].GetTime() == 0.75f);
    REQUIRE(animations_0_75[3].weight_ == 0.75f);

    // Merge state from 0.25 with 0.5 delay, except to remain unchanged
    animationController->ReplaceAnimations(animations_0_25, 0.5f, 0.25f);
    animationController->Update(0.0f);
    REQUIRE(animations_0_75 == animationController->GetAnimationParameters());

    // Spend more time
    animationController->Update(0.25f);
    const auto animations_1_0 = animationController->GetAnimationParameters();

    animationController->Update(0.25f);
    const auto animations_1_25 = animationController->GetAnimationParameters();

    REQUIRE(animations_1_25.size() == 3);

    REQUIRE(animations_1_25[0].GetAnimation() == animationRotate);
    REQUIRE(animations_1_25[0].GetTime() == 1.25f);
    REQUIRE(animations_1_25[0].weight_ == 1.0f);

    REQUIRE(animations_1_25[1].GetAnimation() == animationTranslateX);
    REQUIRE(animations_1_25[1].GetTime() == 0.5f);
    REQUIRE(animations_1_25[1].weight_ == 1.0f);

    REQUIRE(animations_1_25[2].GetAnimation() == animationTranslateZ);
    REQUIRE(animations_1_25[2].GetTime() == 1.25f);
    REQUIRE(animations_1_25[2].weight_ == 0.75f);

    // Merge state from 0.25 with 1.0 delay, except to remain unchanged
    animationController->ReplaceAnimations(animations_0_25, 1.0f, 0.25f);
    animationController->Update(0.0f);
    REQUIRE(animations_1_25 == animationController->GetAnimationParameters());

    // Merge state from 0.75 with 0.5 delay, except to remain unchanged
    animationController->ReplaceAnimations(animations_0_75, 0.5f, 0.25f);
    animationController->Update(0.0f);
    REQUIRE(animations_1_25 == animationController->GetAnimationParameters());

    // Play new animation with fade-in, expect animation added
    animationController->PlayNew(AnimationParameters{animationTranslateZ}, 0.5f);
    animationController->Update(0.25f);
    const auto animations_1_5 = animationController->GetAnimationParameters();

    REQUIRE(animations_1_5.size() == 4);

    REQUIRE(animations_1_5[0].GetAnimation() == animationRotate);
    REQUIRE(animations_1_5[0].GetTime() == 1.5f);
    REQUIRE(animations_1_5[0].weight_ == 1.0f);

    REQUIRE(animations_1_5[1].GetAnimation() == animationTranslateX);
    REQUIRE(animations_1_5[1].GetTime() == 1.0f);
    REQUIRE(animations_1_5[1].weight_ == 1.0f);

    REQUIRE(animations_1_5[2].GetAnimation() == animationTranslateZ);
    REQUIRE(animations_1_5[2].GetTime() == 1.5f);
    REQUIRE(animations_1_5[2].weight_ == 0.75f);

    REQUIRE(animations_1_5[3].GetAnimation() == animationTranslateZ);
    REQUIRE(animations_1_5[3].GetTime() == 0.25f);
    REQUIRE(animations_1_5[3].weight_ == 0.5f);

    // Merge state from 0.25 with 0.0 delay, except to fade out newly added animation and fade in already removed animation
    animationController->ReplaceAnimations(animations_0_25, 0.0f, 0.5f);
    animationController->Update(0.0f);

    const auto animations_0_25_B = animationController->GetAnimationParameters();
    REQUIRE(animations_0_25_B.size() == 5);

    REQUIRE(animations_0_25_B[0].GetAnimation() == animationTranslateZ); // removed on merge
    REQUIRE(animations_0_25_B[0].GetTime() == 0.25f);
    REQUIRE(animations_0_25_B[0].weight_ == 0.5f);
    REQUIRE(animations_0_25_B[0].targetWeight_ == 0.0f);
    REQUIRE(animations_0_25_B[0].targetWeightDelay_ == 0.5f);

    REQUIRE(animations_0_25_B[1].GetAnimation() == animationRotate); // merged
    REQUIRE(animations_0_25_B[1].GetTime() == 0.25f);
    REQUIRE(animations_0_25_B[1].weight_ == 1.0f);
    REQUIRE(animations_0_25_B[1].targetWeight_ == 1.0f);

    REQUIRE(animations_0_25_B[2].GetAnimation() == animationTranslateX); // merged
    REQUIRE(animations_0_25_B[2].GetTime() == 0.5f);
    REQUIRE(animations_0_25_B[2].weight_ == 1.0f);
    REQUIRE(animations_0_25_B[2].targetWeight_ == 1.0f);

    REQUIRE(animations_0_25_B[3].GetAnimation() == animationTranslateX); // added
    REQUIRE(animations_0_25_B[3].GetTime() == 1.0f);
    REQUIRE(animations_0_25_B[3].weight_ == 0.0f);
    REQUIRE(animations_0_25_B[3].targetWeight_ == 1.0f);
    REQUIRE(animations_0_25_B[3].targetWeightDelay_ == 0.5f);

    REQUIRE(animations_0_25_B[4].GetAnimation() == animationTranslateZ); // merged
    REQUIRE(animations_0_25_B[4].GetTime() == 0.25f);
    REQUIRE(animations_0_25_B[4].weight_ == 0.75f);
    REQUIRE(animations_0_25_B[4].targetWeight_ == 0.75f);

    // Spend 0.25 seconds, expect continued fading
    animationController->Update(0.25f);
    const auto animations_0_5_B = animationController->GetAnimationParameters();

    REQUIRE(animations_0_5_B[0].GetAnimation() == animationTranslateZ); // removed on merge
    REQUIRE(animations_0_5_B[0].GetTime() == 0.5f);
    REQUIRE(animations_0_5_B[0].weight_ == 0.25f);
    REQUIRE(animations_0_5_B[0].targetWeight_ == 0.0f);
    REQUIRE(animations_0_5_B[0].targetWeightDelay_ == 0.25f);

    REQUIRE(animations_0_5_B[3].GetAnimation() == animationTranslateX); // added
    REQUIRE(animations_0_5_B[3].GetTime() == 2.0f);
    REQUIRE(animations_0_5_B[3].weight_ == 0.5f);
    REQUIRE(animations_0_5_B[3].targetWeight_ == 0.0f);
    REQUIRE(animations_0_5_B[3].targetWeightDelay_ == 0.5f);

    // Merge same state again, expect to remain unchanged
    animationController->ReplaceAnimations(animations_0_25, 0.25f, 0.5f);
    animationController->Update(0.0f);
    REQUIRE(animations_0_5_B == animationController->GetAnimationParameters());

    // Spend 0.25 seconds, expect continued fading
    animationController->Update(0.25f);
    const auto animations_0_75_B = animationController->GetAnimationParameters();

    REQUIRE(animations_0_75_B[2].GetAnimation() == animationTranslateX); // added
    REQUIRE(animations_0_75_B[2].GetTime() == 2.0f);
    REQUIRE(animations_0_75_B[2].weight_ == 0.25f);
    REQUIRE(animations_0_75_B[2].targetWeight_ == 0.0f);
    REQUIRE(animations_0_75_B[2].targetWeightDelay_ == 0.25f);

    REQUIRE(animations_0_75[0] == animations_0_75_B[0]);
    REQUIRE(animations_0_75[1] == animations_0_75_B[1]);
    REQUIRE(animations_0_75[3] == animations_0_75_B[3]);

    // Merge same state again, expect to remain unchanged
    animationController->ReplaceAnimations(animations_0_25, 0.5f, 0.5f);
    animationController->Update(0.0f);
    REQUIRE(animations_0_75_B == animationController->GetAnimationParameters());

    // Spend more time, expect fully converged
    animationController->Update(0.25f);
    REQUIRE(animations_1_0 == animationController->GetAnimationParameters());

    animationController->Update(0.25f);
    REQUIRE(animations_1_25 == animationController->GetAnimationParameters());
}

TEST_CASE("AnimationController merges result in smooth transition")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto animationTranslateX = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateX.ani", CreateTestTranslateXAnimation);
    auto animationTranslateZ = Tests::GetOrCreateResource<Animation>(context, "@Tests/AnimationController/TranslateZ.ani", CreateTestTranslateZAnimation);

    // Setup scene
    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild("Node");
    node->CreateChild("Quad 1")->CreateChild("Quad 2");
    auto animationController = node->CreateComponent<AnimationController>();

    // Play one animation
    animationController->PlayNew(AnimationParameters{animationTranslateX}.Looped());
    animationController->Update(5.0);

    // Play another animation and record smooth transition
    animationController->PlayNewExclusive(AnimationParameters{animationTranslateZ}.Looped(), 1.0f);
    const auto animations1 = animationController->GetAnimationParameters();
    animationController->Update(0.25);
    const auto animations2 = animationController->GetAnimationParameters();
    animationController->Update(0.25);
    const auto animations3 = animationController->GetAnimationParameters();
    animationController->Update(0.25);
    const auto animations4 = animationController->GetAnimationParameters();
    animationController->Update(0.25);
    const auto animations5 = animationController->GetAnimationParameters();

    animationController->Update(0.0);

    // Setup another scene
    auto scene2 = MakeShared<Scene>(context);
    auto node2 = scene2->CreateChild("Node");
    node2->CreateChild("Quad 1")->CreateChild("Quad 2");
    auto animationController2 = node2->CreateComponent<AnimationController>();

    // Play one animation on another scene
    animationController2->PlayNew(AnimationParameters{animationTranslateX}.Looped());
    animationController2->Update(5.0);

    // Reconstruct animation controller animation from the recording
    animationController2->ReplaceAnimations(animations1, 2.0f, 1.0f);
    const auto animations1B = animationController2->GetAnimationParameters();
    REQUIRE(animations1B == animations1);

    animationController2->Update(0.25);
    animationController2->ReplaceAnimations(animations2, 2.0f, 1.0f);
    const auto animations2B = animationController2->GetAnimationParameters();
    REQUIRE(animations2B == animations2);

    animationController2->Update(0.25);
    animationController2->ReplaceAnimations(animations3, 2.0f, 1.0f);
    const auto animations3B = animationController2->GetAnimationParameters();
    REQUIRE(animations3B == animations3);

    animationController2->Update(0.25);
    animationController2->ReplaceAnimations(animations4, 2.0f, 1.0f);
    const auto animations4B = animationController2->GetAnimationParameters();
    REQUIRE(animations4B == animations4);

    animationController2->Update(0.25);
    animationController2->ReplaceAnimations(animations5, 2.0f, 1.0f);
    const auto animations5B = animationController2->GetAnimationParameters();
    REQUIRE(animations5B == animations5);

    animationController2->Update(0.25);
}
