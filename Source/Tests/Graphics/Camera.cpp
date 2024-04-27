//
// Copyright (c) 2024-2024 the rbfx project.
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
#include "Urho3D/Scene/Scene.h"

#include <Urho3D/Graphics/Camera.h>

TEST_CASE("Camera FocusOn")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    node->SetRotation(Quaternion(Vector3(10,20,30)));
    {
        auto camera = node->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetAspectRatio(0.6f);
    }
    {
        auto camera = node->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetFov(160);
        camera->SetNearClip(10);
    }
    {
        auto camera = node->CreateComponent<Camera>();
        camera->SetOrthographic(false);
        camera->SetAspectRatio(1.6f);
        camera->SetZoom(2.0f);
    }
    {
        const auto camera = node->CreateComponent<Camera>();
        camera->SetOrthographic(true);
        camera->SetAspectRatio(0.6f);
    }
    {
        const auto camera = node->CreateComponent<Camera>();
        camera->SetOrthographic(true);
        camera->SetAspectRatio(1.6f);
        camera->SetZoom(2.0f);
    }

    ea::vector<Camera*> cameras;
    node->GetComponents<Camera>(cameras);

    ea::array<BoundingBox, 2> boxes{
        BoundingBox(Vector3(-1, -2, -1), Vector3(1, 2, 1)),
        BoundingBox(Vector3(0, 0, 0), Vector3(1, 2, 1)),
    };

    for (auto* camera : cameras)
    {
        for (auto& box : boxes)
        {
            camera->GetNode()->SetWorldPosition(Vector3(-1, -1, 2));
            camera->FocusOn(box);

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
