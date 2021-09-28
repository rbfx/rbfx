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
        Tests::AppendQuad(geometries[0].lods_[0], { 0.0f, 0.5f, 0.0f }, { 0.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::WHITE);
        Tests::AppendQuad(geometries[0].lods_[0], { 0.0f, 0.5f, 0.0f }, { 90.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::BLACK);
        Tests::AppendQuad(geometries[1].lods_[0], { 0.0f, 0.5f, 1.0f }, Quaternion::IDENTITY, { 2.0f, 2.0f }, Color::RED);
        Tests::AppendQuad(geometries[1].lods_[1], { 0.0f, 0.5f, 1.0f }, Quaternion::IDENTITY, { 2.0f, 2.0f }, Color::BLUE);

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

TEST_CASE("Animation serialized")
{
    auto context = Tests::CreateCompleteTestContext();
    auto animation = MakeShared<Animation>(context);

    VectorBuffer animationData;
    {
        animation->SetAnimationName("Test Animation");
        animation->SetLength(2.0f);

        {
            AnimationTrack* track = animation->CreateTrack("Track 1");
            track->channelMask_ = CHANNEL_POSITION;

            track->AddKeyFrame(AnimationKeyFrame{ 0.0f, Vector3::ONE });
            track->AddKeyFrame(AnimationKeyFrame{ 1.0f, Vector3::ONE * 1.5f });
            track->AddKeyFrame(AnimationKeyFrame{ 2.0f, Vector3::ONE * 2.0f });
        }

        {
            AnimationTrack* track = animation->CreateTrack("Track 2");
            track->channelMask_ = CHANNEL_POSITION | CHANNEL_ROTATION | CHANNEL_SCALE;

            track->AddKeyFrame(AnimationKeyFrame{ 0.0f, Vector3::ONE,
                Quaternion{ 30.0f, Vector3::UP }, Vector3::ONE * 0.2f });
            track->AddKeyFrame(AnimationKeyFrame{ 1.0f, Vector3::ONE * 1.5f,
                Quaternion{ 60.0f, Vector3::UP }, Vector3::ONE * 0.5f });
            track->AddKeyFrame(AnimationKeyFrame{ 2.0f, Vector3::ONE * 2.0f,
                Quaternion{ 90.0f, Vector3::UP }, Vector3::ONE * 0.8f });
        }

        {
            VariantAnimationTrack* track = animation->CreateVariantTrack("Track 3");

            track->AddKeyFrame(VariantAnimationKeyFrame{ 0.0f, Variant("A") });
            track->AddKeyFrame(VariantAnimationKeyFrame{ 1.0f, Variant("B") });
            track->AddKeyFrame(VariantAnimationKeyFrame{ 2.0f, Variant("C") });
        }

        const bool animationSaved = animation->Save(animationData);
        REQUIRE(animationSaved);

        animationData.Seek(0);
    }

    {
        auto secondAnimation = MakeShared<Animation>(context);
        const bool animationLoaded = secondAnimation->Load(animationData);
        REQUIRE(animationLoaded);

        CHECK(secondAnimation->GetAnimationName() == animation->GetAnimationName());
        CHECK(secondAnimation->GetLength() == animation->GetLength());
        CHECK(secondAnimation->GetNumTracks() == animation->GetNumTracks());
        CHECK(secondAnimation->GetNumVariantTracks() == animation->GetNumVariantTracks());

        {
            auto track = secondAnimation->GetTrack(ea::string{ "Track 1" });
            REQUIRE(track);
            REQUIRE(track->keyFrames_.size() == 3);
            CHECK(track->channelMask_ == CHANNEL_POSITION);
            CHECK(track->keyFrames_[2].time_ == 2.0f);
            CHECK(track->keyFrames_[2].position_.Equals(Vector3::ONE * 2.0f));
        }

        {
            auto track = secondAnimation->GetTrack(ea::string{ "Track 2" });
            REQUIRE(track);
            REQUIRE(track->keyFrames_.size() == 3);
            CHECK(track->channelMask_ == (CHANNEL_POSITION | CHANNEL_ROTATION | CHANNEL_SCALE));
            CHECK(track->keyFrames_[1].time_ == 1.0f);
            CHECK(track->keyFrames_[1].position_.Equals(Vector3::ONE * 1.5f));
            CHECK(track->keyFrames_[1].rotation_.Equals(Quaternion{ 60.0f, Vector3::UP }));
            CHECK(track->keyFrames_[1].scale_.Equals(Vector3::ONE * 0.5f));
        }

        {
            auto track = secondAnimation->GetVariantTrack(ea::string{ "Track 3" });
            REQUIRE(track);
            REQUIRE(track->keyFrames_.size() == 3);
            CHECK(track->keyFrames_[1].time_ == 1.0f);
            CHECK(track->keyFrames_[1].value_ == Variant("B"));
        }
    }
}
