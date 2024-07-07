// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Scene/ShakeComponent.h>

TEST_CASE("ShakeComponent can be applied outside origin")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto node = scene->CreateChild();
    node->SetPosition(Vector3(1, 2, 3));
    node->SetRotation(Quaternion(Vector3(10, 20, 30)));
    auto shake = node->CreateComponent<ShakeComponent>();
    shake->SetRotationRange(Vector3(20, 20, 20));
    shake->SetShiftRange(Vector3(20, 20, 20));
    shake->SetTraumaFalloff(1.0f);
    shake->AddTrauma(0.25f);

    Tests::RunFrame(context, 0.1f);

    CHECK(!Vector3(1, 2, 3).Equals(node->GetPosition()));
    CHECK(!Quaternion(Vector3(10, 20, 30)).Equals(node->GetRotation()));

    node->Translate(Vector3(11, 22, 33), TS_PARENT);
    node->Rotate(Quaternion(Vector3(11, 22, 32)), TS_PARENT);

    Tests::RunFrame(context, 0.1f);

    Tests::RunFrame(context, 0.1f);

    CHECK(Vector3(12, 24, 36).Equals(node->GetPosition()));
    auto expected = Quaternion(Vector3(11, 22, 32)) * Quaternion(Vector3(10, 20, 30));
    CHECK(expected.Equals(node->GetRotation(), 1e-2f));
}
