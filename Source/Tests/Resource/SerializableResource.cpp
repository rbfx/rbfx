// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/SerializableResource.h>


namespace Tests
{

namespace
{

class TestSerializable : public Serializable
{
    URHO3D_OBJECT(TestSerializable, Serializable)

public:
    explicit TestSerializable(Context* context)
        : Serializable(context)
    {
    }

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<TestSerializable>();

        URHO3D_ATTRIBUTE("Vector", IntVector2, vector_, IntVector2::ZERO, AM_DEFAULT);
    }

    IntVector2 vector_{};
};

} // namespace

TEST_CASE("SerializableResource loads resources from memory"){
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestSerializable>>(context);

    auto resource = MakeShared<SerializableResource>(context);
    auto serializable = MakeShared<TestSerializable>(context);
    serializable->vector_ = IntVector2(-1, 42);
    resource->SetValue(serializable);

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Binary));
        CHECK(buffer.GetData()[0] == '\0');

        buffer.Seek(0);

        auto loadedResource = MakeShared<SerializableResource>(context);
        CHECK(loadedResource->Load(buffer));
        auto* loadedValue = dynamic_cast<TestSerializable*>(loadedResource->GetValue());
        REQUIRE(loadedValue);
        CHECK(loadedValue->vector_ == serializable->vector_);
    }

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Json));
        CHECK(buffer.GetData()[0] == '{');

        buffer.Seek(0);

        auto loadedResource = MakeShared<SerializableResource>(context);
        CHECK(loadedResource->Load(buffer));
        auto* loadedValue = dynamic_cast<TestSerializable*>(loadedResource->GetValue());
        REQUIRE(loadedValue);
        CHECK(loadedValue->vector_ == serializable->vector_);
    }

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Xml));
        CHECK(buffer.GetData()[0] == '<');

        buffer.Seek(0);

        auto loadedResource = MakeShared<SerializableResource>(context);
        CHECK(loadedResource->Load(buffer));
        auto* loadedValue = dynamic_cast<TestSerializable*>(loadedResource->GetValue());
        REQUIRE(loadedValue);
        CHECK(loadedValue->vector_ == serializable->vector_);
    }
};

} // namespace Tests
