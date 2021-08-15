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

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/ModelView.h>

namespace
{

ModelVertex MakeModelVertex(const Vector3& position, const Vector3& normal, const Color& color)
{
    ModelVertex vertex;
    vertex.SetPosition(position);
    vertex.SetNormal(normal);
    vertex.color_[0] = color.ToVector4();
    return vertex;
}

void AppendQuad(GeometryLODView& dest,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color)
{
    static constexpr unsigned NumVertices = 4;
    static constexpr unsigned NumIndices = 2 * 3;

    const Vector3 vertexPositions[NumVertices] = {
        { -size.x_ / 2, -size.y_ / 2, 0.0f },
        {  size.x_ / 2, -size.y_ / 2, 0.0f },
        { -size.x_ / 2,  size.y_ / 2, 0.0f },
        {  size.x_ / 2,  size.y_ / 2, 0.0f },
    };
    const Vector3 vertexNorm = { 0.0f, 0.0f, -1.0f };

    const unsigned baseIndex = dest.vertices_.size();
    for (unsigned i = 0; i < NumVertices; ++i)
    {
        const Vector3 pos = rotation * vertexPositions[i] + position;
        const Vector3 norm = rotation * vertexNorm;
        dest.vertices_.push_back(MakeModelVertex(pos, norm, color));
    }

    const unsigned indices[NumIndices] = {
        0, 2, 1,
        1, 2, 3
    };
    for (unsigned i = 0; i < NumIndices; ++i)
        dest.indices_.push_back(baseIndex + indices[i]);
}

void AppendSkinnedQuad(GeometryLODView& dest, const Vector4& blendIndices, const Vector4& blendWeights,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color)
{
    const unsigned beginVertex = dest.vertices_.size();
    AppendQuad(dest, position, rotation, size, color);
    const unsigned endVertex = dest.vertices_.size();

    for (unsigned i = beginVertex; i < endVertex; ++i)
    {
        dest.vertices_[i].blendIndices_ = blendIndices;
        dest.vertices_[i].blendWeights_ = blendWeights;
    }
}

}

TEST_CASE("Simple model constructed and desconstructed")
{
    auto context = Tests::CreateCompleteTestContext();
    auto modelView = MakeShared<ModelView>(context);

    VectorBuffer modelData;
    {
        // Set metadata
        modelView->AddMetadata("Metadata1", Vector3{ 1.5f, 0.5f, 1.0f });
        modelView->AddMetadata("Metadata2", "[tag]");

        // Set vertex format
        ModelVertexFormat format;
        format.position_ = TYPE_VECTOR3;
        format.normal_ = TYPE_VECTOR3;
        format.color_[0] = TYPE_UBYTE4_NORM;
        modelView->SetVertexFormat(format);

        // Set LODs
        auto& geometries = modelView->GetGeometries();
        geometries.resize(2);
        geometries[0].lods_.resize(1);
        geometries[1].lods_.resize(2);

        // Set geometry data
        AppendQuad(geometries[0].lods_[0], { 0.0f, 0.5f, 0.0f }, { 0.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::WHITE);
        AppendQuad(geometries[0].lods_[0], { 0.0f, 0.5f, 0.0f }, { 90.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::BLACK);
        AppendQuad(geometries[1].lods_[0], { 0.0f, 0.5f, 1.0f }, Quaternion::IDENTITY, { 2.0f, 2.0f }, Color::RED);
        AppendQuad(geometries[1].lods_[1], { 0.0f, 0.5f, 1.0f }, Quaternion::IDENTITY, { 2.0f, 2.0f }, Color::BLUE);

        geometries[1].lods_[0].lodDistance_ = 10.0f;
        geometries[1].lods_[1].lodDistance_ = 20.0f;

        // Convert
        const auto model = modelView->ExportModel();
        REQUIRE(model);

        // Assert metadata here because metadata cannot be serialized to memory buffer
        CHECK(model->GetMetadata("Metadata1") == Variant(Vector3{ 1.5f, 0.5f, 1.0f }));
        CHECK(model->GetMetadata("Metadata2") == Variant("[tag]"));

        // Serialize
        model->RemoveAllMetadata();
        const bool modelSaved = model->Save(modelData);
        REQUIRE(modelSaved);

        modelData.Seek(0);
    }

    // Assert loaded
    auto model = MakeShared<Model>(context);
    const bool modelLoaded = model->Load(modelData);
    REQUIRE(modelLoaded);

    // Assert vertex data
    {
        const auto& vertexBuffers = model->GetVertexBuffers();
        REQUIRE(vertexBuffers.size() == 1);

        const auto vertexBuffer = vertexBuffers[0];
        REQUIRE(vertexBuffer->GetVertexCount() == 16);
        REQUIRE(vertexBuffer->GetVertexSize() == 28);

        const auto vertexElements = vertexBuffer->GetElements();
        REQUIRE(vertexElements.size() == 3);
        CHECK(vertexElements[0].semantic_ == SEM_POSITION);
        CHECK(vertexElements[0].type_ == TYPE_VECTOR3);
        CHECK(vertexElements[0].offset_ == 0);
        CHECK(vertexElements[1].semantic_ == SEM_NORMAL);
        CHECK(vertexElements[1].type_ == TYPE_VECTOR3);
        CHECK(vertexElements[1].offset_ == 12);
        CHECK(vertexElements[2].semantic_ == SEM_COLOR);
        CHECK(vertexElements[2].type_ == TYPE_UBYTE4_NORM);
        CHECK(vertexElements[2].offset_ == 24);

        const auto vertexData = vertexBuffer->GetUnpackedData();
        CHECK(vertexData[0] == Vector4{ -0.5f, 0.0f, 0.0f, 1.0f });
        CHECK(vertexData[1] == Vector4{ 0.0f, 0.0f, -1.0f, 0.0f });
        CHECK(vertexData[2] == Color::WHITE.ToVector4());
        CHECK(vertexData[4 * 3].Equals(Vector4{ 0.0f, 0.0f, 0.5f, 1.0f }));
        CHECK(vertexData[8 * 3] == Vector4{ -1.0f, -0.5f, 1.0f, 1.0f });
        CHECK(vertexData[12 * 3] == Vector4{ -1.0f, -0.5f, 1.0f, 1.0f });
    }

    // Assert index data
    {
        const auto& indexBuffers = model->GetIndexBuffers();
        REQUIRE(indexBuffers.size() == 1);

        const auto indexBuffer = indexBuffers[0];
        REQUIRE(indexBuffer->GetIndexCount() == 4 * 6);
        REQUIRE(indexBuffer->GetIndexSize() == 2);

        const auto indexData = indexBuffer->GetUnpackedData();
        CHECK(indexData[0] == 0);
        CHECK(indexData[1] == 2);
        CHECK(indexData[2] == 1);
        CHECK(indexData[6] == 4);
    }

    // Assert geometries
    {
        const auto& geometries = model->GetGeometries();
        REQUIRE(geometries.size() == 2);
        REQUIRE(geometries[0].size() == 1);
        REQUIRE(geometries[1].size() == 2);

        {
            const auto geometry1 = geometries[0][0];
            CHECK(geometry1->GetVertexStart() == 0);
            CHECK(geometry1->GetVertexCount() == 8);
            CHECK(geometry1->GetIndexStart() == 0);
            CHECK(geometry1->GetIndexCount() == 12);
            CHECK(geometry1->GetLodDistance() == 0.0f);
            CHECK(geometry1->GetPrimitiveType() == TRIANGLE_LIST);
            REQUIRE(geometry1->GetVertexBuffers().size() == 1);
            CHECK(geometry1->GetVertexBuffer(0) == model->GetVertexBuffers()[0]);
            CHECK(geometry1->GetIndexBuffer() == model->GetIndexBuffers()[0]);
        }

        {
            const auto geometry2 = geometries[1][0];
            CHECK(geometry2->GetVertexStart() == 8);
            CHECK(geometry2->GetVertexCount() == 4);
            CHECK(geometry2->GetIndexStart() == 12);
            CHECK(geometry2->GetIndexCount() == 6);
            CHECK(geometry2->GetLodDistance() == 10.0f);
            CHECK(geometry2->GetPrimitiveType() == TRIANGLE_LIST);
            REQUIRE(geometry2->GetVertexBuffers().size() == 1);
            CHECK(geometry2->GetVertexBuffer(0) == model->GetVertexBuffers()[0]);
            CHECK(geometry2->GetIndexBuffer() == model->GetIndexBuffers()[0]);
        }

        {
            const auto geometry3 = geometries[1][1];
            CHECK(geometry3->GetVertexStart() == 12);
            CHECK(geometry3->GetVertexCount() == 4);
            CHECK(geometry3->GetIndexStart() == 18);
            CHECK(geometry3->GetIndexCount() == 6);
            CHECK(geometry3->GetLodDistance() == 20.0f);
            CHECK(geometry3->GetPrimitiveType() == TRIANGLE_LIST);
            REQUIRE(geometry3->GetVertexBuffers().size() == 1);
            CHECK(geometry3->GetVertexBuffer(0) == model->GetVertexBuffers()[0]);
            CHECK(geometry3->GetIndexBuffer() == model->GetIndexBuffers()[0]);
        }
    }

    // Assert ModelView parsing
    {
        auto secondModelView = MakeShared<ModelView>(context);
        const bool modelImported = secondModelView->ImportModel(model);
        REQUIRE(modelImported);
        CHECK(modelView->GetVertexFormat() == secondModelView->GetVertexFormat());
        CHECK(modelView->GetGeometries() == secondModelView->GetGeometries());
        CHECK(modelView->GetBones() == secondModelView->GetBones());
    }
}

TEST_CASE("Skeletal model constructed and desconstructed")
{
    auto context = Tests::CreateCompleteTestContext();
    auto modelView = Tests::CreateSkinnedQuad_Model(context);

    VectorBuffer modelData;
    {
        // Convert
        const auto model = modelView->ExportModel();
        REQUIRE(model);
        REQUIRE(model->GetSkeleton().GetRootBone()->name_ == "Root");

        // Serialize
        const bool modelSaved = model->Save(modelData);
        REQUIRE(modelSaved);

        modelData.Seek(0);
    }

    // Assert loaded
    auto model = MakeShared<Model>(context);
    const bool modelLoaded = model->Load(modelData);
    REQUIRE(modelLoaded);

    // Assert ModelView parsing
    {
        auto secondModelView = MakeShared<ModelView>(context);
        const bool modelImported = secondModelView->ImportModel(model);
        REQUIRE(modelImported);
        CHECK(modelView->GetVertexFormat() == secondModelView->GetVertexFormat());
        CHECK(modelView->GetGeometries() == secondModelView->GetGeometries());
        CHECK(modelView->GetBones() == secondModelView->GetBones());
    }
}
