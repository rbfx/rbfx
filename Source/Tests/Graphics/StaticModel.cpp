// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../ModelUtils.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/CustomGeometry.h>

TEST_CASE("Empty geometry skipped in batch")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild();
    auto staticModel = node->CreateComponent<StaticModel>();

    // Create model with empty geometry
    auto geometry = MakeShared<Geometry>(context);
    auto model = MakeShared<Model>(context);
    auto vb = MakeShared<VertexBuffer>(context);
    vb->SetShadowed(true);
    vb->SetSize(0, 0);
    model->SetVertexBuffers({vb}, {}, {});
    auto ib = MakeShared<IndexBuffer>(context);
    ib->SetShadowed(true);
    ib->SetSize(0, false);
    REQUIRE(model->SetIndexBuffers({ib}));
    REQUIRE(geometry->SetVertexBuffer(0, vb));
    geometry->SetIndexBuffer(ib);
    REQUIRE(geometry->SetDrawRange(PrimitiveType::LINE_LIST, 0, 0));
    model->SetNumGeometries(1);
    REQUIRE(model->SetNumGeometryLodLevels(0, 1));
    REQUIRE(model->SetGeometry(0, 0, geometry));

    // Check that geometry is set correctly to null
    staticModel->SetModel(model);
    auto& batches = staticModel->GetBatches();
    REQUIRE(batches.size() == 1);
    REQUIRE(batches[0].geometry_ == nullptr);
}


TEST_CASE("Non-empty geometry is present at batch")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild();
    auto staticModel = node->CreateComponent<StaticModel>();

    // Create model with geometry
    auto geometry = MakeShared<Geometry>(context);
    auto model = MakeShared<Model>(context);
    auto vb = MakeShared<VertexBuffer>(context);
    vb->SetShadowed(true);
    vb->SetSize(0, 0);
    model->SetVertexBuffers({vb}, {}, {});
    auto ib = MakeShared<IndexBuffer>(context);
    ib->SetShadowed(true);
    ib->SetSize(2, false);
    REQUIRE(model->SetIndexBuffers({ib}));
    REQUIRE(geometry->SetVertexBuffer(0, vb));
    geometry->SetIndexBuffer(ib);
    REQUIRE(geometry->SetDrawRange(PrimitiveType::LINE_LIST, 0, 2));
    model->SetNumGeometries(1);
    REQUIRE(model->SetNumGeometryLodLevels(0, 1));
    REQUIRE(model->SetGeometry(0, 0, geometry));

    // Check that geometry is set correctly to null
    staticModel->SetModel(model);
    auto& batches = staticModel->GetBatches();
    REQUIRE(batches.size() == 1);
    REQUIRE(batches[0].geometry_ == geometry);
}
