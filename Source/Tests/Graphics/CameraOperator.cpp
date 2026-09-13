// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../ModelUtils.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/CameraOperator.h>
#include <Urho3D/Graphics/Camera.h>

TEST_CASE("CameraOperator orthographic test")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto scene = MakeShared<Scene>(context);
    const auto rootNode = scene->CreateChild();
    const auto cameraOperator = rootNode->CreateComponent<CameraOperator>();
    auto camera = rootNode->CreateComponent<Camera>();
    camera->SetOrthographic(true);
    cameraOperator->SetBoundingBox(BoundingBox(Vector3(-1, -1, -1), Vector3(1, 1, 1)));
    cameraOperator->SetBoundingBoxTrackingEnabled(true);
    camera->SetAspectRatio(0.5f);
    cameraOperator->MoveCamera();
    CHECK(camera->GetOrthoSize() == 4.0);
    camera->SetAspectRatio(2.0f);
    cameraOperator->MoveCamera();
    CHECK(camera->GetOrthoSize() == 2.0);

    camera->SetAspectRatio(0.5f);
    camera->SetAutoAspectRatio(true);
    cameraOperator->MoveCamera();
    CHECK(camera->GetAutoAspectRatio());
    CHECK(Equals(camera->GetAspectRatio(), 0.5f));
    CHECK(Equals(camera->GetOrthoSize(), 4.0f));
    camera->SetAspectRatio(2.0f);
    camera->SetAutoAspectRatio(true);
    cameraOperator->MoveCamera();
    CHECK(camera->GetAutoAspectRatio());
    CHECK(Equals(camera->GetAspectRatio(), 2.0f));
    CHECK(Equals(camera->GetOrthoSize(), 2.0f));
}

TEST_CASE("CameraOperator focus on bounding box")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto scene = MakeShared<Scene>(context);
    const auto rootNode = scene->CreateChild();
    rootNode->SetRotation(Quaternion(Vector3(10, 20, 30)));
    rootNode->SetPosition(Vector3(0.1f, 0.2f, 0.3f));
    {
        auto childNode = rootNode->CreateChild();
        childNode->SetRotation(Quaternion(Vector3(30, 20, 10)));
        childNode->CreateComponent<CameraOperator>();
        auto camera = childNode->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetAspectRatio(0.6f);
    }
    {
        auto childNode = rootNode->CreateChild();
        childNode->CreateComponent<CameraOperator>();
        auto camera = childNode->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetFov(160);
        camera->SetNearClip(10);
    }
    {
        auto childNode = rootNode->CreateChild();
        childNode->CreateComponent<CameraOperator>();
        auto camera = childNode->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetAspectRatio(1.6f);
        camera->SetZoom(2.0f);
    }
    {
        auto childNode = rootNode->CreateChild();
        childNode->CreateComponent<CameraOperator>();
        auto camera = childNode->CreateComponent<Camera>();
        camera->SetOrthographic(true);
        camera->SetAspectRatio(0.6f);
    }
    {
        auto childNode = rootNode->CreateChild();
        childNode->CreateComponent<CameraOperator>();
        auto camera = childNode->CreateComponent<Camera>();
        camera->SetOrthographic(true);
        camera->SetAspectRatio(1.6f);
        camera->SetZoom(2.0f);
    }

    ea::vector<CameraOperator*> cameras;
    rootNode->FindComponents<CameraOperator>(cameras);

    ea::array<BoundingBox, 2> boxes{
        BoundingBox(Vector3(-1, -2, -1), Vector3(1, 2, 1)),
        BoundingBox(Vector3(0, 0, 0), Vector3(1, 2, 1)),
    };

    for (auto* cameraOperator : cameras)
    {
        for (auto& box : boxes)
        {
            cameraOperator->GetNode()->SetWorldPosition(Vector3(-1, -1, 2));
            cameraOperator->SetBoundingBox(box);
            cameraOperator->SetBoundingBoxTrackingEnabled(true);
            cameraOperator->MoveCamera();

            auto* camera = cameraOperator->GetComponent<Camera>();
            const Frustum frustum = camera->GetFrustum();
            float minDistance = ea::numeric_limits<float>::max();
            for (int mask = 0; mask < 8; ++mask)
            {
                Vector3 point{(mask & 1) ? box.min_.x_ : box.max_.x_, (mask & 2) ? box.min_.y_ : box.max_.y_,
                    (mask & 4) ? box.min_.z_ : box.max_.z_};

                for (int planeIndex = PLANE_NEAR; planeIndex <= PLANE_DOWN; ++planeIndex)
                {
                    const auto& plane = frustum.planes_[planeIndex];
                    const float distanceToFrustumPlane = plane.Distance(point);
                    if (minDistance > distanceToFrustumPlane)
                        minDistance = distanceToFrustumPlane;

                    CHECK(distanceToFrustumPlane > -1e-4f);
                }
            }
            CHECK(Urho3D::Abs(minDistance) < 1e-4f);
        }
    }
}
