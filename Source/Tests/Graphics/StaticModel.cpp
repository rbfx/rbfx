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
