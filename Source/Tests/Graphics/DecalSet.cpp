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
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Resource/ResourceCache.h"

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/DecalSet.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

TEST_CASE("Static model decal simple test")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);
    auto octree = scene->CreateComponent<Octree>();
    auto node = scene->CreateChild();

    auto staticModel = node->CreateComponent<StaticModel>();
    staticModel->SetModel(context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl"));
    Vector3 rayStart{0, 0, -4};
    Vector3 rayDirection = (node->GetWorldPosition() - rayStart).Normalized();
    ea::vector<RayQueryResult> results;
    RayOctreeQuery query{results, Ray(rayStart, rayDirection), RAY_TRIANGLE};
    octree->Raycast(query);
    REQUIRE(!results.empty());

    RayQueryResult& result = results[0];

    Quaternion lookAt;
    lookAt.FromLookRotation(rayDirection);

    auto decalSet = node->CreateComponent<DecalSet>();
    decalSet->AddDecal(staticModel, result.position_, lookAt, 0.2f, 1.1f, 0.1f, Vector2::ZERO, Vector2::ONE);

    REQUIRE(decalSet->GetNumDecals() == 1);
    auto* decal = decalSet->GetDecal(0);
    REQUIRE(decal);
    REQUIRE(decal->indices_.size() == 12);
    REQUIRE(decal->vertices_.size() == 6);

    ea::array<DecalVertex, 6> expectedVertices = {{
        {{-0.1f, -0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {0.0454545f, 1.f}, {1.f, -5.20357e-07f, -3.42285e-08f, 1.f}},
        {{0.1f, 0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {0.954545f, 0.f}, {1.f, -4.73052e-07f, -3.42285e-08f, 1.f}},
        {{0.11f, 0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {1.f, 0.f}, {1.f, -7.80536e-07f, -3.42285e-08f, 1.f}},
        {{0.11f, -0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {1.f, 1.f}, {1.f, -7.09579e-08f, -3.42285e-08f, 1.f}},
        {{-0.11f, -0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {-1.78814e-07f, 1.f}, {1.f, 3.54789e-08f, -3.42285e-08f, 1.f}},
        {{-0.11f, 0.1f, -0.5f}, {-3.42285e-08f, 0.f, -1.f}, {-1.19209e-07f, 0.f}, {1.f, 7.09579e-08f, -3.42285e-08f, 1.f}},
    }};

    for (unsigned i = 0; i < expectedVertices.size(); ++i)
    {
         UNSCOPED_INFO("{ {" << decal->vertices_[i].position_.x_ << "f, " << decal->vertices_[i].position_.y_ << "f, "
                             << decal->vertices_[i].position_.z_ << "f },");
         UNSCOPED_INFO("{" << decal->vertices_[i].normal_.x_ << "f, " << decal->vertices_[i].normal_.y_ << "f, "
                           << decal->vertices_[i].normal_.z_ << "f},");
         UNSCOPED_INFO("{" << decal->vertices_[i].texCoord_.x_ << "f, " << decal->vertices_[i].texCoord_.y_ << "f},");
         UNSCOPED_INFO("{" << decal->vertices_[i].tangent_.x_ << "f, " << decal->vertices_[i].tangent_.y_ << "f, "
                           << decal->vertices_[i].tangent_.z_ << "f, " << decal->vertices_[i].tangent_.w_ << "f} }");
        CHECK(expectedVertices[i].Equals(decal->vertices_[i], 1e-3f));
    }
}

TEST_CASE("Static model decal projection test")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);
    auto octree = scene->CreateComponent<Octree>();
    auto node = scene->CreateChild();
    node->SetPosition({1, 2, 3});
    node->SetRotation(Quaternion(Vector3{10, 20, 30}));

    auto staticModel = node->CreateComponent<StaticModel>();
    staticModel->SetModel(context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl"));
    Vector3 rayStart{-1, 0, -3};
    Vector3 rayDirection = (node->GetWorldPosition() - rayStart).Normalized();
    ea::vector<RayQueryResult> results;
    RayOctreeQuery query{results, Ray(rayStart, rayDirection), RAY_TRIANGLE};
    octree->Raycast(query);
    REQUIRE(!results.empty());

    RayQueryResult& result = results[0];

    Quaternion lookAt;
    lookAt.FromLookRotation(rayDirection);
        
    auto decalSet = node->CreateComponent<DecalSet>();
    decalSet->AddDecal(staticModel, result.position_, lookAt, 0.5f, 1.0f, 1.0f, Vector2::ZERO, Vector2::ONE);

    REQUIRE(decalSet->GetNumDecals() == 1);
    auto* decal = decalSet->GetDecal(0);
    REQUIRE(decal);
    REQUIRE(decal->indices_.size() == 18);
    REQUIRE(decal->vertices_.size() == 10);

    ea::array<DecalVertex, 10> expectedVertices = {{
        {{-0.396309f, -0.396309f, -0.5f}, {1.49012e-08f, 0.f, -1.f},
            {0.177958f, 1.f}, {0.861344f, -0.508022f, 1.2835e-08f, 1.f}},
        {{0.015489f, 0.015489f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {0.484638f, 1.78814e-07f},
            {0.861344f, -0.508022f, 1.2835e-08f, 1.f}},
        {{0.237543f, -0.115479f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {1.f, 1.19209e-07f},
            {0.861344f, -0.508022f, 1.2835e-08f, 1.f}},
        {{0.0179654f, -0.5f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {1.f, 0.785163f},
            {0.861344f, -0.508022f, 1.2835e-08f, 1.f}},
        {{-0.220503f, -0.5f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {0.585984f, 1.f},
            {0.861344f, -0.508022f, 1.2835e-08f, 1.f}},
        {{-0.472986f, -0.351085f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {8.9407e-08f, 1.f},
            {0.861344f, -0.508021f, 1.2835e-08f, 1.f}},
        {{-0.193327f, 0.138649f, -0.5f}, {1.49012e-08f, 0.f, -1.f}, {-1.19209e-07f, 0.f},
            {0.861344f, -0.508021f, 1.2835e-08f, 1.f}},
        {{-0.220503f, -0.5f, -0.5f}, {7.45058e-09f, -1.f, 0.f}, {0.585984f, 1.f},
            {0.716291f, 5.33678e-09f, 0.697802f, 1.f}},
        {{0.0179654f, -0.5f, -0.5f}, {7.45058e-09f, -1.f, 0.f}, {1.f, 0.785163f},
            {0.716291f, 5.33678e-09f, 0.697802f, 1.f}},
        {{0.0109782f, -0.5f, -0.274494f}, {7.45058e-09f, -1.f, 0.f}, {1.f, 1.f},
            {0.716291f, 5.33678e-09f, 0.697802f, 1.f}}}};

    for (unsigned i=0; i<expectedVertices.size(); ++i)
    {
        UNSCOPED_INFO("{ {" << decal->vertices_[i].position_.x_ << "f, " << decal->vertices_[i].position_.y_ << "f, "
                            << decal->vertices_[i].position_.z_ << "f },");
        UNSCOPED_INFO("{" << decal->vertices_[i].normal_.x_ << "f, " << decal->vertices_[i].normal_.y_ << "f, "
                          << decal->vertices_[i].normal_.z_ << "f},");
        UNSCOPED_INFO("{" << decal->vertices_[i].texCoord_.x_ << "f, " << decal->vertices_[i].texCoord_.y_ << "f},");
        UNSCOPED_INFO("{" << decal->vertices_[i].tangent_.x_ << "f, " << decal->vertices_[i].tangent_.y_ << "f, "
                          << decal->vertices_[i].tangent_.z_ << "f, " << decal->vertices_[i].tangent_.w_ << "f} }");
        CHECK(expectedVertices[i].Equals(decal->vertices_[i], 1e-3f));
    }
}

TEST_CASE("Animated model decal projection test")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);
    auto octree = scene->CreateComponent<Octree>();
    auto node = scene->CreateChild();
    node->SetPosition({1, 2, 3});
    node->SetRotation(Quaternion(Vector3{10, 20, 30}));

    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/NinjaSnowWar/Ninja.mdl"));
    auto animationController = node->CreateComponent<AnimationController>();
    animationController->PlayNew(AnimationParameters{context->GetSubsystem<ResourceCache>()->GetResource<Animation>("Models/NinjaSnowWar/Ninja_Walk.ani")});
    animationController->UpdatePose();
    auto* bone = node->FindChild("Joint12", true);

    Vector3 rayStart = bone->GetWorldPosition() + Vector3{-1, -1, -1};
    Vector3 rayDirection = (bone->GetWorldPosition() - rayStart).Normalized();
    ea::vector<RayQueryResult> results;
    RayOctreeQuery query{results, Ray(rayStart, rayDirection), RAY_TRIANGLE};
    octree->Raycast(query);
    REQUIRE(!results.empty());

    RayQueryResult& result = results[0];

    Quaternion lookAt;
    lookAt.FromLookRotation(rayDirection);

    auto decalSet = node->CreateComponent<DecalSet>();
    decalSet->AddDecal(animatedModel, result.position_, lookAt, 0.1f, 1.1f, 1.0f, Vector2::ZERO, Vector2::ONE);

    REQUIRE(decalSet->GetNumDecals() == 1);
    auto* decal = decalSet->GetDecal(0);
    REQUIRE(decal);
    REQUIRE(decal->indices_.size() == 162);
    REQUIRE(decal->vertices_.size() == 68);


    // Logging actual values in a copy-pastable format.
    ea::array<unsigned, 5> indices = {{0, 1u, 2u, 50u, 67u}};
    for (unsigned i : indices)
    {
        UNSCOPED_INFO("{ auto& vertex = decal->vertices_[" << i << "];");
        UNSCOPED_INFO("CHECK(vertex.position_.Equals(Vector3{" << decal->vertices_[i].position_.x_ << "f, "
                                                               << decal->vertices_[i].position_.y_ << "f, "
                                                               << decal->vertices_[i].position_.z_ << "f }));");
        UNSCOPED_INFO("CHECK(vertex.normal_.Equals(Vector3{" << decal->vertices_[i].normal_.x_ << "f, "
                                                             << decal->vertices_[i].normal_.y_ << "f, "
                                                             << decal->vertices_[i].normal_.z_ << "f }));");
        UNSCOPED_INFO("CHECK(vertex.texCoord_.Equals(Vector2{" << decal->vertices_[i].texCoord_.x_ << "f, "
                                                               << decal->vertices_[i].texCoord_.y_ << "f }));");
        UNSCOPED_INFO("CHECK(vertex.tangent_.Equals(Vector4{"
            << decal->vertices_[i].tangent_.x_ << "f, " << decal->vertices_[i].tangent_.y_ << "f, "
            << decal->vertices_[i].tangent_.z_ << "f, " << decal->vertices_[i].tangent_.w_ << "f }));");
        UNSCOPED_INFO("CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{"
            << decal->vertices_[i].blendWeights_[0] << "f, " << decal->vertices_[i].blendWeights_[1] << "f, "
            << decal->vertices_[i].blendWeights_[2] << "f, " << decal->vertices_[i].blendWeights_[3] << "f }));");
        UNSCOPED_INFO("CHECK(vertex.blendIndices_[0] == " << (unsigned)decal->vertices_[i].blendIndices_[0] << ");");
        UNSCOPED_INFO("CHECK(vertex.blendIndices_[1] == " << (unsigned)decal->vertices_[i].blendIndices_[1] << ");");
        UNSCOPED_INFO("CHECK(vertex.blendIndices_[2] == " << (unsigned)decal->vertices_[i].blendIndices_[2] << ");");
        UNSCOPED_INFO("CHECK(vertex.blendIndices_[3] == " << (unsigned)decal->vertices_[i].blendIndices_[3] << "); }");
    }

    // There are too many vertices to compare. Let's compare first few random vertices, that should be "good enough".

    const float eps = 1e-3f;
    {
        auto& vertex = decal->vertices_[0];
        CHECK(vertex.position_.Equals(Vector3{0.234493f, 0.996382f, 0.0135708f}, eps));
        CHECK(vertex.normal_.Equals(Vector3{-0.528885f, -0.496562f, -0.688263f}, eps));
        CHECK(vertex.texCoord_.Equals(Vector2{0.463876f, 0.427102f}, eps));
        CHECK(vertex.tangent_.Equals(Vector4{0.835496f, -0.447094f, -0.319458f, 1.f}, eps));
        CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{1.f, 0.f, 0.f, 0.f}, eps));
        CHECK(vertex.blendIndices_[0] == 0);
        CHECK(vertex.blendIndices_[1] == 0);
        CHECK(vertex.blendIndices_[2] == 0);
        CHECK(vertex.blendIndices_[3] == 0);
    }
    {
        auto& vertex = decal->vertices_[1];
        CHECK(vertex.position_.Equals(Vector3{0.240903988f, 1.03326809f, -0.00848847814f}, eps));
        CHECK(vertex.normal_.Equals(Vector3{-0.422799f, -0.503043f, -0.753783f}, eps));
        CHECK(vertex.texCoord_.Equals(Vector2{0.417679f, 0.f}, eps));
        CHECK(vertex.tangent_.Equals(Vector4{0.892582f, -0.374943f, -0.250431f, 1.f}, eps));
        CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{1.f, 0.f, 0.f, 0.f}, eps));
        CHECK(vertex.blendIndices_[0] == 0);
        CHECK(vertex.blendIndices_[1] == 0);
        CHECK(vertex.blendIndices_[2] == 0);
        CHECK(vertex.blendIndices_[3] == 0);
    }
    {
        auto& vertex = decal->vertices_[2];
        CHECK(vertex.position_.Equals(Vector3{0.264088690f, 1.02781534f, -0.00949959457f}, eps));
        CHECK(vertex.normal_.Equals(Vector3{-0.152182f, -0.487659f, -0.859668f}, eps));
        CHECK(vertex.texCoord_.Equals(Vector2{0.604814f, -9.53674e-07f}, eps));
        CHECK(vertex.tangent_.Equals(Vector4{0.972564f, -0.228736f, -0.0424133f, 1.f}, eps));
        CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{1.f, 0.f, 0.f, 0.f}, eps));
        CHECK(vertex.blendIndices_[0] == 0);
        CHECK(vertex.blendIndices_[1] == 0);
        CHECK(vertex.blendIndices_[2] == 0);
        CHECK(vertex.blendIndices_[3] == 0);
    }
    {
        auto& vertex = decal->vertices_[50];
        CHECK(vertex.position_.Equals(Vector3{0.238162f, 0.939239f, 0.0372444f}, eps));
        CHECK(vertex.normal_.Equals(Vector3{-0.853497f, -0.0454199f, 0.519115f}, eps));
        CHECK(vertex.texCoord_.Equals(Vector2{0.66768f, 1.f}, eps));
        CHECK(vertex.tangent_.Equals(Vector4{-0.320773f, -0.739288f, -0.592079f, 1.f}, eps));
        CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{1.f, 0.f, 0.f, 0.f}, eps));
        CHECK(vertex.blendIndices_[0] == 5);
        CHECK(vertex.blendIndices_[1] == 0);
        CHECK(vertex.blendIndices_[2] == 0);
        CHECK(vertex.blendIndices_[3] == 0);
    }
    {
        auto& vertex = decal->vertices_[67];
        CHECK(vertex.position_.Equals(Vector3{0.252909f, 0.982886f, 0.0896384f}, eps));
        CHECK(vertex.normal_.Equals(Vector3{-0.00110826f, 0.752038f, -0.659118f}, eps));
        CHECK(vertex.texCoord_.Equals(Vector2{0.364721f, 1.f}, eps));
        CHECK(vertex.tangent_.Equals(Vector4{0.988480389f, -0.0989458859f, -0.1145261f, 1.f}, eps));
        CHECK(Vector4{vertex.blendWeights_}.Equals(Vector4{1.f, 0.f, 0.f, 0.f}, eps));
        CHECK(vertex.blendIndices_[0] == 5);
        CHECK(vertex.blendIndices_[1] == 0);
        CHECK(vertex.blendIndices_[2] == 0);
        CHECK(vertex.blendIndices_[3] == 0);
    }
}
