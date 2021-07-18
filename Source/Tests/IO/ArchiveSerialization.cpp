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

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/BinaryArchive.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/XMLArchive.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

namespace
{

class SerializableObject : public Object
{
    URHO3D_OBJECT(SerializableObject, Object);

public:
    explicit SerializableObject(Context* context) : Object(context) {}

    bool Serialize(Archive& archive)
    {
        if (auto block = archive.OpenSafeUnorderedBlock("SerializableObject"))
            return Serialize(archive, block);
        return false;
    }

    bool Serialize(Archive& archive, ArchiveBlock& block)
    {
        SerializeValue(archive, "member_", member_);
        return true;
    }

    bool operator ==(const SerializableObject& rhs) const
    {
        return member_ == rhs.member_;
    }

    int member_{};
};

struct PlainTypesAggregate
{
    bool bool_{};

    unsigned char byte_{};
    short short_{};
    int int_{};
    long long long_{};

    float float_{};
    float double_{};

    Vector2 vec2_{};
    Vector3 vec3_{};
    Vector4 vec4_{};
    IntVector2 intVec2_{};
    IntVector3 intVec3_{};

    Rect rect_{};
    IntRect intRect_{};

    Matrix3 mat3_{};
    Matrix3x4 mat3x4_{};
    Matrix4 mat4_{};

    Quaternion quat_{};
    Color color_{};

    bool operator ==(const PlainTypesAggregate& rhs) const
    {
        return bool_ == rhs.bool_

            && byte_ == rhs.byte_
            && short_ == rhs.short_
            && int_ == rhs.int_
            && long_ == rhs.long_

            && Equals(float_, rhs.float_)
            && Equals(double_, rhs.double_)

            && vec2_.Equals(rhs.vec2_)
            && vec3_.Equals(rhs.vec3_)
            && vec4_.Equals(rhs.vec4_)
            && intVec2_ == rhs.intVec2_
            && intVec3_ == rhs.intVec3_

            && rect_.Equals(rhs.rect_)
            && intRect_ == rhs.intRect_

            && mat3_.Equals(rhs.mat3_)
            && mat3x4_.Equals(rhs.mat3x4_)
            && mat4_.Equals(rhs.mat4_)

            && quat_.Equals(rhs.quat_)
            && color_.Equals(rhs.color_);
    }
};

struct ContainerTypesAggregate
{
    ea::string string_{};
    ea::vector<float> vectorOfFloats_{};
    ea::vector<float> byteFloatVector_{};
    ea::hash_map<ea::string, float> mapOfFloats_{};

    Variant variantMap_;
    Variant variantVector_;
    Variant variantBuffer_;

    SharedPtr<Object> emptySerializable_;
    SharedPtr<SerializableObject> serializableObject_;

    auto Tie() const
    {
        return std::tie(string_, vectorOfFloats_, byteFloatVector_, mapOfFloats_, variantMap_, variantVector_, variantBuffer_);
    }

    bool operator ==(const ContainerTypesAggregate& rhs) const
    {
        if (Tie() != rhs.Tie())
            return false;

        if (!!emptySerializable_ != !!rhs.emptySerializable_)
            return false;

        if (!!serializableObject_ != !!rhs.serializableObject_)
            return false;

        if (serializableObject_ && rhs.serializableObject_)
        {
            if (serializableObject_->GetType() != rhs.serializableObject_->GetType())
                return false;
            if (serializableObject_->member_ != rhs.serializableObject_->member_)
                return false;
        }

        return true;
    }
};

struct SerializationTestStruct
{
    PlainTypesAggregate plain_;
    ContainerTypesAggregate container_;
    // Initialize variant value so it can be deserialized from archive
    Variant variant_ = MakeCustomValue(ContainerTypesAggregate{});

    auto Tie() const { return std::tie(plain_, container_, variant_); }
    bool operator ==(const SerializationTestStruct& rhs) const { return Tie() == rhs.Tie(); }
};

bool SerializeValue(Archive& archive, const char* name, PlainTypesAggregate& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "bool_", value.bool_);

        SerializeValue(archive, "byte_", value.byte_);
        SerializeValue(archive, "short_", value.short_);
        SerializeValue(archive, "int_", value.int_);
        SerializeValue(archive, "long_", value.long_);

        SerializeValue(archive, "float_", value.float_);
        SerializeValue(archive, "double_", value.double_);

        SerializeValue(archive, "vec2_", value.vec2_);
        SerializeValue(archive, "vec3_", value.vec3_);
        SerializeValue(archive, "vec4_", value.vec4_);
        SerializeValue(archive, "intVec2_", value.intVec2_);
        SerializeValue(archive, "intVec3_", value.intVec3_);

        SerializeValue(archive, "rect_", value.rect_);
        SerializeValue(archive, "intRect_", value.intRect_);

        SerializeValue(archive, "mat3_", value.mat3_);
        SerializeValue(archive, "mat3x4_", value.mat3x4_);
        SerializeValue(archive, "mat4_", value.mat4_);

        SerializeValue(archive, "quat_", value.quat_);
        SerializeValue(archive, "color_", value.color_);
        return true;
    }
    return false;
}

bool SerializeValue(Archive& archive, const char* name, ContainerTypesAggregate& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "justString_", value.string_);
        SerializeVectorAsObjects(archive, "vectorOfFloats_", "elem", value.vectorOfFloats_);
        SerializeVectorAsBytes(archive, "byteFloatVector_", value.byteFloatVector_);
        SerializeStringMap(archive, "mapOfFloats_", "elem", value.mapOfFloats_);
        SerializeValue(archive, "variantMap_", value.variantMap_);
        SerializeValue(archive, "variantVector_", value.variantVector_);
        SerializeValue(archive, "variantBuffer_", value.variantBuffer_);
        SerializeValue(archive, "emptySerializable_", value.emptySerializable_);
        SerializeValue(archive, "serializableObject_", value.serializableObject_);
        return true;
    }
    return false;
}

bool SerializeValue(Archive& archive, const char* name, SerializationTestStruct& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "plain_", value.plain_);
        SerializeValue(archive, "container_", value.container_);
        SerializeValue(archive, "variant_", value.variant_);
        return true;
    }
    return false;
}

SharedPtr<Context> CreateTestContext()
{
    auto context = MakeShared<Context>();
    context->RegisterFactory<SerializableObject>();
    return context;
}

SerializationTestStruct CreateTestStruct(Context* context)
{
    const Quaternion rot = Quaternion(30.0f, Vector3::UP);

    SerializationTestStruct result;
    result.plain_.bool_ = true;

    result.plain_.byte_ = 16;
    result.plain_.short_ = 1024;
    result.plain_.int_ = -999999999;
    result.plain_.long_ = -999999999999999;

    result.plain_.float_ = 1.5f;
    result.plain_.double_ = 0.5;

    result.plain_.vec2_ = Vector2(1.0f, 2.0f);
    result.plain_.vec3_ = Vector3(1.0f, 2.0f, 3.0f);
    result.plain_.vec4_ = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
    result.plain_.intVec2_ = IntVector2(1, 2);
    result.plain_.intVec3_ = IntVector3(1, 2, 3);

    result.plain_.rect_ = Rect(1.0f, 2.0f, 3.0f, 4.0f);
    result.plain_.intRect_ = IntRect(1, 2, 3, 4);

    result.plain_.mat3_ = rot.RotationMatrix();
    result.plain_.mat3x4_ = static_cast<Matrix3x4>(rot.RotationMatrix());
    result.plain_.mat4_ = static_cast<Matrix4>(rot.RotationMatrix());

    result.plain_.quat_ = rot;
    result.plain_.color_ = Color(1.0f, 2.0f, 3.0f, 4.0f);

    result.container_.string_ = "\"<tricky&string>\"";
    result.container_.vectorOfFloats_ = { 1.0f, 2.0f, 3.0f };
    result.container_.byteFloatVector_ = { 1.0f, 2.0f, 3.0f };
    result.container_.mapOfFloats_ = { { "first", 1.0f }, { "forth", 4.0f } };

    result.container_.variantMap_ = VariantMap{ ea::make_pair(StringHash("key1"), 1.0f), ea::make_pair(StringHash("key2"), 2.0f) };
    result.container_.variantVector_ = VariantVector{ 1.0f, "string" };
    result.container_.variantBuffer_ = VariantBuffer{ 1, 2, 3, 4, 5 };

    result.container_.serializableObject_ = MakeShared<SerializableObject>(context);
    result.container_.serializableObject_->member_ = 12;

    result.variant_ = MakeCustomValue(result.container_);

    return result;
}

template <class ArchiveType, class ResourceType>
SharedPtr<ResourceType> SaveTestStruct(Context* context, const SerializationTestStruct& data)
{
    SharedPtr<ResourceType> resource = MakeShared<ResourceType>(context);
    ArchiveType archive{ resource };
    SerializeValue(archive, "SerializationTestStruct", const_cast<SerializationTestStruct&>(data));

    if (archive.HasError())
        return nullptr;
    return resource;
}

template <class ArchiveType>
VectorBuffer SaveTestStructBinary(Context* context, const SerializationTestStruct& data)
{
    VectorBuffer buffer;
    ArchiveType archive{ context, buffer };
    SerializeValue(archive, "SerializationTestStruct", const_cast<SerializationTestStruct&>(data));

    if (archive.HasError())
        return {};
    return buffer;
}

template <class ArchiveType, class ResourceType>
ea::unique_ptr<SerializationTestStruct> LoadTestStruct(Context* context, ResourceType& resource)
{
    ArchiveType archive{ &resource };

    SerializationTestStruct data;
    SerializeValue(archive, "SerializationTestStruct", data);

    if (archive.HasError())
        return nullptr;
    return ea::make_unique<SerializationTestStruct>(data);
}

template <class ArchiveType>
ea::unique_ptr<SerializationTestStruct> LoadTestStructBinary(Context* context, VectorBuffer& buffer)
{
    buffer.Seek(0);
    ArchiveType archive{ context, buffer };

    SerializationTestStruct data;
    SerializeValue(archive, "SerializationTestStruct", data);

    if (archive.HasError())
        return nullptr;
    return ea::make_unique<SerializationTestStruct>(data);
}

}

TEST_CASE("Test structure is serialized to archive")
{
    auto context = CreateTestContext();
    const SerializationTestStruct sourceObject = CreateTestStruct(context);

    SECTION("binary archive")
    {
        auto binaryData = SaveTestStructBinary<BinaryOutputArchive>(context, sourceObject);
        REQUIRE(binaryData.GetSize() != 0);

        const auto objectFromBinary = LoadTestStructBinary<BinaryInputArchive>(context, binaryData);
        REQUIRE(objectFromBinary);
        REQUIRE(sourceObject == *objectFromBinary);
    }

    SECTION("XML archive")
    {
        auto xmlData = SaveTestStruct<XMLOutputArchive, XMLFile>(context, sourceObject);
        REQUIRE(xmlData);

        auto objectFromXML = LoadTestStruct<XMLInputArchive, XMLFile>(context, *xmlData);
        REQUIRE(objectFromXML);
        REQUIRE(sourceObject == *objectFromXML);
    }

    SECTION("JSON archive")
    {
        auto jsonData = SaveTestStruct<JSONOutputArchive, JSONFile>(context, sourceObject);
        REQUIRE(jsonData);

        auto objectFromJSON = LoadTestStruct<JSONInputArchive, JSONFile>(context, *jsonData);
        REQUIRE(objectFromJSON);
        REQUIRE(sourceObject == *objectFromJSON);
    }
}

TEST_CASE("Test structure is serialized as part of the file")
{
    auto context = CreateTestContext();
    SerializationTestStruct sourceObject = CreateTestStruct(context);

    SECTION("XML file")
    {
        auto xmlFile = MakeShared<XMLFile>(context);
        XMLElement root = xmlFile->CreateRoot("root");

        XMLOutputArchive xmlOutputArchive{ context, root.CreateChild("child") };
        SerializeValue(xmlOutputArchive, "SerializationTestStruct", sourceObject);
        REQUIRE_FALSE(xmlOutputArchive.HasError());

        XMLInputArchive xmlInputArchive{ context, root.GetChild("child") };
        SerializationTestStruct objectFromXML;
        SerializeValue(xmlInputArchive, "SerializationTestStruct", objectFromXML);
        REQUIRE_FALSE(xmlInputArchive.HasError());

        REQUIRE(sourceObject == objectFromXML);
    }

    SECTION("JSON file")
    {
        auto jsonFile = MakeShared<JSONFile>(context);
        JSONValue& root = jsonFile->GetRoot();

        JSONValue child;
        JSONOutputArchive jsonOutputArchive{ context, child };
        SerializeValue(jsonOutputArchive, "SerializationTestStruct", sourceObject);
        root.Set("child", child);

        JSONInputArchive jsonInputArchive{ context, root.Get("child") };
        SerializationTestStruct objectFromJSON;
        SerializeValue(jsonInputArchive, "SerializationTestStruct", objectFromJSON);

        REQUIRE(sourceObject == objectFromJSON);
    }
}
