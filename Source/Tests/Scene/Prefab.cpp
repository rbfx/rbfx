//
// Copyright (c) 2022-2022 the rbfx project.
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
#include "../SceneUtils.h"
#include "Urho3D/Graphics/StaticModel.h"

#include <Urho3D/IO/BinaryArchive.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RigidBody.h>

#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Scene/PrefabReader.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/PrefabWriter.h>

namespace
{

enum class TestEnum
{
    Red,
    Green,
    Blue,
};

const StringVector testEnumNames{"Red", "Green", "Blue"};

class TestComponent : public Component
{
    URHO3D_OBJECT(TestComponent, Component);

public:
    explicit TestComponent(Context* context) : Component(context) {}

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<TestComponent>();

        URHO3D_ATTRIBUTE("Vector", IntVector2, vector_, IntVector2::ZERO, AM_DEFAULT);
        URHO3D_ENUM_ATTRIBUTE("Enum", enum_, testEnumNames, TestEnum::Red, AM_DEFAULT);
        URHO3D_ATTRIBUTE("VectorString", StringVector, vectorString_, StringVector{}, AM_DEFAULT);
        URHO3D_ATTRIBUTE("UnchangedString", ea::string, unchangedString_, "default", AM_DEFAULT);
    }

    IntVector2 vector_{};
    TestEnum enum_{};
    StringVector vectorString_{};
    ea::string unchangedString_{"default"};
};

NodePrefab MakeTestPrefab()
{
    NodePrefab source;

    {
        auto& node = source.GetMutableNode();
        node.SetId(SerializableId{101});

        auto& nodeAttributes = node.GetMutableAttributes();
        nodeAttributes.emplace_back("Name").SetValue("Apple");
        nodeAttributes.emplace_back("Position").SetValue(Vector3{1, 2, 3});
    }

    auto& childNodes = source.GetMutableChildren();
    for (unsigned i = 0; i < 3; ++i)
    {
        auto& childNode = childNodes.emplace_back().GetMutableNode();
        childNode.SetId(SerializableId{201 + i});

        auto& childNodeAttributes = childNode.GetMutableAttributes();
        childNodeAttributes.emplace_back("Name").SetValue("Worm");
        childNodeAttributes.emplace_back("Position").SetValue(Vector3{1, 1, 1});
    }

    unsigned componentIndex = 301;
    for (NodePrefab* parentNode : {&source, &source.GetMutableChildren()[0], &source.GetMutableChildren()[2]})
    {
        auto& components = parentNode->GetMutableComponents();
        for (unsigned i = 0; i < 2; ++i)
        {
            auto& component = components.emplace_back();
            component.SetId(SerializableId{componentIndex++});
            component.SetType(TestComponent::GetTypeNameStatic());

            auto& componentAttributes = component.GetMutableAttributes();
            componentAttributes.emplace_back("Enum").SetValue("Blue");
        }
    }

    {
        auto& child = childNodes.emplace_back();
        child.GetMutableNode().SetId(SerializableId{401});

        auto& child2 = child.GetMutableChildren().emplace_back();
        child2.GetMutableNode().SetId(SerializableId{402});

        auto& child3 = child2.GetMutableChildren().emplace_back();
        child3.GetMutableNode().SetId(SerializableId{403});
    }

    return source;
}

} // namespace

TEST_CASE("Attribute prefab is serialized as binary")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Save id
    for (const AttributeId id : {AttributeId{11}, AttributeId{212}, AttributeId{20340}, AttributeId{3123456}})
    {
        BinaryFile file(context);

        AttributePrefab source{id};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetName() == ea::string{});
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetNameHash() == StringHash{});
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        const unsigned char vleSize =
            1 + (id >= AttributeId{128}) + (id >= AttributeId{16384}) + (id >= AttributeId{2097152});
        CHECK(file.GetData().size() == vleSize + sizeof(VariantType) + sizeof(IntVector2));
    }

    // Save name hash
    for (const StringHash hash : {"foo", "b", "1111111111111111111111111111111111111"})
    {
        BinaryFile file(context);

        AttributePrefab source{hash};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetId() == AttributeId::None);
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetName() == ea::string{});
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        CHECK(file.GetData().size() == sizeof(StringHash) + sizeof(VariantType) + sizeof(IntVector2));
    }

    // Save name
    for (const ea::string name : {"foo", "bar", "1111111111111111111111111111111111111"})
    {
        BinaryFile file(context);

        AttributePrefab source{name};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetId() == AttributeId::None);
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        CHECK(file.GetData().size() == 1 /*name length*/ + name.length() + sizeof(VariantType) + sizeof(IntVector2));
    }

    // Save name (compact mode)
    for (const ea::string name : {"foo", "bar", "1111111111111111111111111111111111111"})
    {
        BinaryFile file(context);

        AttributePrefab source{name};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source, true));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetId() == AttributeId::None);
        CHECK(!source.GetName().empty());
        CHECK(dest.GetName().empty());
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        CHECK(file.GetData().size() == sizeof(StringHash) + sizeof(VariantType) + sizeof(IntVector2));
    }
}

TEST_CASE("Attribute prefab is serialized as JSON")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Save id
    {
        JSONFile file(context);

        AttributePrefab source{AttributeId{11}};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetName() == ea::string{});
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetNameHash() == StringHash{});
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 3);
        CHECK(object.find("id") != object.end());
        CHECK(object.find("type") != object.end());
        CHECK(object.find("value") != object.end());
    }

    // Save name hash
    {
        JSONFile file(context);

        AttributePrefab source{StringHash{"foo"}};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetId() == AttributeId::None);
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetName() == ea::string{});
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 3);
        CHECK(object.find("nameHash") != object.end());
        CHECK(object.find("type") != object.end());
        CHECK(object.find("value") != object.end());
    }

    // Save name
    {
        JSONFile file(context);

        AttributePrefab source{"bar"};
        source.SetValue(IntVector2{2, 3});

        CHECK(file.SaveObject("attribute", source));

        AttributePrefab dest;
        CHECK(file.LoadObject("attribute", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetId() == AttributeId::None);
        CHECK(source.GetName() == dest.GetName());
        CHECK(source.GetNameHash() == dest.GetNameHash());
        CHECK(source.GetType() == dest.GetType());
        CHECK(source.GetValue() == dest.GetValue());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 3);
        CHECK(object.find("name") != object.end());
        CHECK(object.find("type") != object.end());
        CHECK(object.find("value") != object.end());
    }
}

TEST_CASE("Serializable is loaded from and to prefab")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    const auto testFlags = {PrefabSaveFlag::None, PrefabSaveFlag::CompactAttributeNames,
        PrefabSaveFlag::EnumsAsStrings, PrefabSaveFlag::SaveDefaultValues};

    SerializablePrefab prefab;
    prefab.SetId(SerializableId{11});
    for (PrefabSaveFlags saveFlags : testFlags)
    {
        const bool saveDefaults = saveFlags.Test(PrefabSaveFlag::SaveDefaultValues);
        const bool enumsAsString = saveFlags.Test(PrefabSaveFlag::EnumsAsStrings);

        TestComponent source(context);
        source.vector_ = {10, 12};
        source.enum_ = TestEnum::Blue;
        source.vectorString_ = {"foo", "bar", "1234567890123456789012345678901234567890"};

        prefab.Import(&source, saveFlags);

        const auto& attributes = prefab.GetAttributes();
        CHECK(prefab.GetId() == SerializableId{11});
        REQUIRE(attributes.size() == (saveDefaults ? 4 : 3));
        CHECK(attributes[0].GetNameHash() == StringHash{"Vector"});
        CHECK(attributes[0].GetValue() == Variant{source.vector_});
        CHECK(attributes[1].GetNameHash() == StringHash{"Enum"});
        if (enumsAsString)
            CHECK(attributes[1].GetValue() == Variant{"Blue"});
        else
            CHECK(attributes[1].GetValue() == Variant{static_cast<int>(source.enum_)});
        CHECK(attributes[2].GetNameHash() == StringHash{"VectorString"});
        CHECK(attributes[2].GetValue() == Variant{source.vectorString_});
        if (saveDefaults)
        {
            CHECK(attributes[3].GetNameHash() == StringHash{"UnchangedString"});
            CHECK(attributes[3].GetValue() == Variant{"default"});
        }

        TestComponent dest(context);

        prefab.Export(&dest, PrefabLoadFlag::None);

        CHECK(source.vector_ == dest.vector_);
        CHECK(source.enum_ == dest.enum_);
        CHECK(source.vectorString_ == dest.vectorString_);
        CHECK(source.unchangedString_ == dest.unchangedString_);
    }
}

TEST_CASE("Serializable prefab is serialized as binary")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Save full
    {
        BinaryFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetTypeName() == dest.GetTypeName());
        CHECK(source.GetTypeNameHash() == dest.GetTypeNameHash());

        CHECK(file.GetData().size()
            == 1 /*id*/ + 1 /*typeName length*/ + 1 /*attribute count*/ + dest.GetTypeName().size());
    }

    // Save nothing
    {
        const auto saveLoadFlags =
            PrefabArchiveFlag::IgnoreSerializableId | PrefabArchiveFlag::IgnoreSerializableType;
        BinaryFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source, saveLoadFlags));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest, saveLoadFlags));

        CHECK(dest.GetId() == SerializableId{});
        CHECK(dest.GetTypeName() == EMPTY_STRING);
        CHECK(dest.GetTypeNameHash() == StringHash::Empty);

        CHECK(file.GetData().size() == 1 /*attribute count*/);
    }

    // Save type name as hash
    {
        const auto saveLoadFlags = PrefabArchiveFlag::CompactTypeNames;
        BinaryFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source, saveLoadFlags));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest, saveLoadFlags));

        CHECK(dest.GetId() == SerializableId{11});
        CHECK(dest.GetTypeName() == EMPTY_STRING);
        CHECK(dest.GetTypeNameHash() == TestComponent::GetTypeStatic());

        CHECK(file.GetData().size()
            == 1 /*id*/ + 1 /*attribute count*/ + sizeof(StringHash));
    }
}

TEST_CASE("Serializable prefab is serialized as JSON")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Save full
    {
        JSONFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest));

        CHECK(source.GetId() == dest.GetId());
        CHECK(source.GetTypeName() == dest.GetTypeName());
        CHECK(source.GetTypeNameHash() == dest.GetTypeNameHash());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 2);
        CHECK(object.find("_id") != object.end());
        CHECK(object.find("_typeName") != object.end());
    }

    // Save nothing
    {
        const auto saveLoadFlags =
            PrefabArchiveFlag::IgnoreSerializableId | PrefabArchiveFlag::IgnoreSerializableType;
        JSONFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source, saveLoadFlags));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest, saveLoadFlags));

        CHECK(dest.GetId() == SerializableId{});
        CHECK(dest.GetTypeName() == EMPTY_STRING);
        CHECK(dest.GetTypeNameHash() == StringHash::Empty);

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.empty());
    }

    // Save type name as hash
    {
        const auto saveLoadFlags = PrefabArchiveFlag::CompactTypeNames;
        JSONFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeStatic());

        CHECK(file.SaveObject("serializable", source, saveLoadFlags));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest, saveLoadFlags));

        CHECK(dest.GetId() == SerializableId{11});
        CHECK(dest.GetTypeName() == EMPTY_STRING);
        CHECK(dest.GetTypeNameHash() == TestComponent::GetTypeStatic());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 2);
        CHECK(object.find("_id") != object.end());
        CHECK(object.find("_typeHash") != object.end());
    }

    // Save type name as name and load as hash
    {
        JSONFile file(context);

        SerializablePrefab source;
        source.SetId(SerializableId{11});
        source.SetType(TestComponent::GetTypeNameStatic());

        CHECK(file.SaveObject("serializable", source));

        SerializablePrefab dest;
        CHECK(file.LoadObject("serializable", dest, PrefabArchiveFlag::CompactTypeNames));

        CHECK(dest.GetId() == SerializableId{11});
        CHECK(dest.GetTypeName() == TestComponent::GetTypeNameStatic());
        CHECK(dest.GetTypeNameHash() == TestComponent::GetTypeStatic());

        const auto& object = file.GetRoot().GetObject();
        CHECK(object.size() == 2);
        CHECK(object.find("_id") != object.end());
        CHECK(object.find("_typeName") != object.end());
    }
}

TEST_CASE("Scene prefab is serialized")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    JSONFile file(context);

    const NodePrefab source = MakeTestPrefab();
    CHECK(file.SaveObject("scene", source));

    NodePrefab dest;
    CHECK(file.LoadObject("scene", dest));

    CHECK(dest == source);
}

TEST_CASE("PrefabReader iterates over nodes and components")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const NodePrefab source = MakeTestPrefab();

    BinaryFile binaryFile(context);
    binaryFile.SaveObject("scene", source);
    MemoryBuffer binaryFileBuffer{binaryFile.GetData()};
    BinaryInputArchive binaryArchive{context, binaryFileBuffer};

    JSONFile jsonFile(context);
    jsonFile.SaveObject("scene", source);
    JSONInputArchive jsonArchive{context, jsonFile.GetRoot()};

    PrefabReaderFromMemory memoryReader{source};
    PrefabReaderFromArchive binaryArchiveReader{binaryArchive, "scene"};
    PrefabReaderFromArchive jsonArchiveReader{jsonArchive, "scene"};

    PrefabReader* readers[] = {&memoryReader, &binaryArchiveReader, &jsonArchiveReader};
    for (PrefabReader* reader : readers)
    {
        CHECK(*reader->ReadNode() == source.GetNode());
        REQUIRE(reader->ReadNumComponents() == 2);
        {
            CHECK(*reader->ReadComponent() == source.GetComponents()[0]);
            CHECK(*reader->ReadComponent() == source.GetComponents()[1]);
        }
        REQUIRE(reader->ReadNumChildren() == 4);
        {
            {
                reader->BeginChild();
                CHECK(*reader->ReadNode() == source.GetChildren()[0].GetNode());
                REQUIRE(reader->ReadNumComponents() == 2);
                {
                    CHECK(*reader->ReadComponent() == source.GetChildren()[0].GetComponents()[0]);
                    CHECK(*reader->ReadComponent() == source.GetChildren()[0].GetComponents()[1]);
                }
                CHECK(reader->ReadNumChildren() == 0);
                reader->EndChild();
            }
            {
                reader->BeginChild();
                CHECK(*reader->ReadNode() == source.GetChildren()[1].GetNode());
                CHECK(reader->ReadNumComponents() == 0);
                CHECK(reader->ReadNumChildren() == 0);
                reader->EndChild();
            }
            {
                reader->BeginChild();
                CHECK(*reader->ReadNode() == source.GetChildren()[2].GetNode());
                REQUIRE(reader->ReadNumComponents() == 2);
                {
                    CHECK(*reader->ReadComponent() == source.GetChildren()[2].GetComponents()[0]);
                    CHECK(*reader->ReadComponent() == source.GetChildren()[2].GetComponents()[1]);
                }
                CHECK(reader->ReadNumChildren() == 0);
                reader->EndChild();
            }
            {
                reader->BeginChild();
                CHECK(*reader->ReadNode() == source.GetChildren()[3].GetNode());
                CHECK(reader->ReadNumComponents() == 0);
                REQUIRE(reader->ReadNumChildren() == 1);
                {
                    reader->BeginChild();
                    CHECK(*reader->ReadNode() == source.GetChildren()[3].GetChildren()[0].GetNode());
                    CHECK(reader->ReadNumComponents() == 0);
                    CHECK(reader->ReadNumChildren() == 1);
                    {
                        reader->BeginChild();
                        CHECK(*reader->ReadNode()
                            == source.GetChildren()[3].GetChildren()[0].GetChildren()[0].GetNode());
                        CHECK(reader->ReadNumComponents() == 0);
                        CHECK(reader->ReadNumChildren() == 0);
                        reader->EndChild();
                    }
                    reader->EndChild();
                }
                CHECK(!reader->IsEOF());
                reader->EndChild();
            }
        }
        CHECK(reader->IsEOF());
    }
}

TEST_CASE("Prefab is loaded to Node")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    const NodePrefab source = MakeTestPrefab();

    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild();

    {
        PrefabReaderFromMemory reader{source};
        REQUIRE(node->Load(reader));

        CHECK(node->GetName() == "Apple");
        CHECK(node->GetPosition() == Vector3{1, 2, 3});
        REQUIRE(node->GetNumComponents() == 2);
        {
            REQUIRE(node->GetComponent<TestComponent>() == node->GetComponents()[0]);
            CHECK(node->GetComponent<TestComponent>()->enum_ == TestEnum::Blue);
        }
        REQUIRE(node->GetNumChildren() == 4);
        {
            CHECK(node->GetChildren()[0]->GetName() == "Worm");
            CHECK(node->GetChildren()[0]->GetNumComponents() == 2);
            CHECK(node->GetChildren()[0]->GetNumChildren() == 0);

            CHECK(node->GetChildren()[1]->GetName() == "Worm");
            CHECK(node->GetChildren()[1]->GetNumComponents() == 0);
            CHECK(node->GetChildren()[1]->GetNumChildren() == 0);

            CHECK(node->GetChildren()[2]->GetName() == "Worm");
            CHECK(node->GetChildren()[2]->GetNumComponents() == 2);
            CHECK(node->GetChildren()[2]->GetNumChildren() == 0);

            CHECK(node->GetChildren()[3]->GetName() == "");
            REQUIRE(node->GetChildren()[3]->GetNumChildren() == 1);
            REQUIRE(node->GetChildren()[3]->GetChildren()[0]->GetNumChildren() == 1);
            CHECK(node->GetChildren()[3]->GetChildren()[0]->GetChildren()[0]->GetNumChildren() == 0);
        }
    }
}

TEST_CASE("PrefabWriter iterates over nodes and components")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    const NodePrefab source = MakeTestPrefab();

    auto scene = MakeShared<Scene>(context);
    auto node = scene->CreateChild(EMPTY_STRING, static_cast<unsigned>(source.GetNode().GetId()));

    {
        PrefabReaderFromMemory reader{source};
        REQUIRE(node->Load(reader));
    }

    NodePrefab destPrefab;

    BinaryFile destBinaryFile(context);
    BinaryOutputArchive destBinaryArchive{context, destBinaryFile.AsSerializer()};

    JSONFile destJsonFile(context);
    JSONOutputArchive destJsonArchive{context, destJsonFile.GetRoot()};

    PrefabWriterToMemory memoryWriter{destPrefab, PrefabSaveFlag::EnumsAsStrings};
    PrefabWriterToArchive binaryWriter{destBinaryArchive, "scene", PrefabSaveFlag::EnumsAsStrings};
    PrefabWriterToArchive jsonWriter{destJsonArchive, "scene", PrefabSaveFlag::EnumsAsStrings};

    PrefabWriter* writers[] = {&memoryWriter, &binaryWriter, &jsonWriter};
    for (PrefabWriter* writer : writers)
    {
        writer->WriteNode(node->GetID(), node);
        writer->WriteNumComponents(2);
        {
            writer->WriteComponent(node->GetComponents()[0]->GetID(), node->GetComponents()[0]);
            writer->WriteComponent(node->GetComponents()[1]->GetID(), node->GetComponents()[1]);
        }
        writer->WriteNumChildren(4);
        {
            {
                Node* child = node->GetChildren()[0];
                writer->BeginChild();
                writer->WriteNode(child->GetID(), child);
                writer->WriteNumComponents(2);
                {
                    writer->WriteComponent(child->GetComponents()[0]->GetID(), child->GetComponents()[0]);
                    writer->WriteComponent(child->GetComponents()[1]->GetID(), child->GetComponents()[1]);
                }
                writer->WriteNumChildren(0);
                writer->EndChild();
            }
            {
                Node* child = node->GetChildren()[1];
                writer->BeginChild();
                writer->WriteNode(child->GetID(), child);
                writer->WriteNumComponents(0);
                writer->WriteNumChildren(0);
                writer->EndChild();
            }
            {
                Node* child = node->GetChildren()[2];
                writer->BeginChild();
                writer->WriteNode(child->GetID(), child);
                writer->WriteNumComponents(2);
                {
                    writer->WriteComponent(child->GetComponents()[0]->GetID(), child->GetComponents()[0]);
                    writer->WriteComponent(child->GetComponents()[1]->GetID(), child->GetComponents()[1]);
                }
                writer->WriteNumChildren(0);
                writer->EndChild();
            }
            {
                Node* child = node->GetChildren()[3];
                writer->BeginChild();
                writer->WriteNode(child->GetID(), child);
                writer->WriteNumComponents(0);
                writer->WriteNumChildren(1);
                {
                    Node* grandChild = child->GetChildren()[0];
                    writer->BeginChild();
                    writer->WriteNode(grandChild->GetID(), grandChild);
                    writer->WriteNumComponents(0);
                    writer->WriteNumChildren(1);
                    {
                        Node* grandGrandChild = grandChild->GetChildren()[0];
                        writer->BeginChild();
                        writer->WriteNode(grandGrandChild->GetID(), grandGrandChild);
                        writer->WriteNumComponents(0);
                        writer->WriteNumChildren(0);
                        writer->EndChild();
                    }
                    writer->EndChild();
                }
                writer->EndChild();
            }
        }
        CHECK(writer->IsEOF());
    }

    CHECK(destPrefab == source);

    {
        destBinaryFile.LoadObject("scene", destPrefab);
        CHECK(destPrefab == source);
    }

    {
        destJsonFile.LoadObject("scene", destPrefab);
        CHECK(destPrefab == source);
    }
}

TEST_CASE("PrefabResource is serialized")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    auto resource = MakeShared<PrefabResource>(context);
    resource->GetMutableScenePrefab() = MakeTestPrefab();

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Binary));
        CHECK(buffer.GetData()[0] == '\0');

        buffer.Seek(0);

        auto loadedResource = MakeShared<PrefabResource>(context);
        CHECK(loadedResource->Load(buffer));
    }

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Json));
        CHECK(buffer.GetData()[0] == '{');

        buffer.Seek(0);

        auto loadedResource = MakeShared<PrefabResource>(context);
        CHECK(loadedResource->Load(buffer));
    }

    {
        VectorBuffer buffer;
        CHECK(resource->Save(buffer, InternalResourceFormat::Xml));
        CHECK(buffer.GetData()[0] == '<');

        buffer.Seek(0);

        auto loadedResource = MakeShared<PrefabResource>(context);
        CHECK(loadedResource->Load(buffer));
    }
}

TEST_CASE("Prefab reference is instantiated")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    auto prefabResource = MakeShared<PrefabResource>(context);
    prefabResource->GetMutableScenePrefab().GetMutableChildren().push_back(MakeTestPrefab());

    auto scene = MakeShared<Scene>(context);

    auto node = scene->CreateChild();

    auto prefabRef = node->CreateComponent<PrefabReference>();
    prefabRef->SetPrefab(prefabResource);

    REQUIRE(node->GetNumComponents() == 3);
    {
        CHECK_FALSE(node->GetComponents()[0]->IsTemporary());
        CHECK(node->GetComponents()[0] == prefabRef);
        CHECK(node->GetComponents()[1]->IsTemporary());
        CHECK(node->GetComponents()[2]->IsTemporary());

        REQUIRE(node->GetComponent<TestComponent>() == node->GetComponents()[1]);
        CHECK(node->GetComponent<TestComponent>()->enum_ == TestEnum::Blue);
    }
    REQUIRE(node->GetNumChildren() == 4);
    {
        CHECK(node->GetChildren()[0]->IsTemporary());
        CHECK(node->GetChildren()[0]->GetName() == "Worm");
        CHECK(node->GetChildren()[0]->GetNumComponents() == 2);
        CHECK(node->GetChildren()[0]->GetNumChildren() == 0);
    }

    node->SetEnabled(false);

    REQUIRE(node->GetNumComponents() == 1);
    CHECK_FALSE(node->GetComponents()[0]->IsTemporary());
    CHECK(node->GetComponents()[0] == prefabRef);
    REQUIRE(node->GetNumChildren() == 0);

    node->SetEnabled(true);

    REQUIRE(node->GetNumComponents() == 3);
    REQUIRE(node->GetNumChildren() == 4);

    prefabRef->SetPrefab(nullptr);

    REQUIRE(node->GetNumComponents() == 1);
    REQUIRE(node->GetNumChildren() == 0);

    prefabRef->SetPrefab(prefabResource);
    prefabRef->Remove();

    REQUIRE(node->GetNumComponents() == 0);
    REQUIRE(node->GetNumChildren() == 0);
}

TEST_CASE("Prefab reference is reloaded")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<Tests::RegisterObject<TestComponent>>(context);

    auto prefabResource = MakeShared<PrefabResource>(context);
    prefabResource->GetMutableScenePrefab().GetMutableChildren().push_back(MakeTestPrefab());
    auto& prefabData = prefabResource->GetMutableScenePrefab().GetMutableChildren()[0];

    auto scene = MakeShared<Scene>(context);

    auto node = scene->CreateChild();

    auto prefabRef = node->CreateComponent<PrefabReference>();
    prefabRef->SetPrefab(prefabResource);

    REQUIRE(node->GetNumComponents() == 3);
    CHECK_FALSE(node->GetComponents()[0]->IsTemporary());
    CHECK(node->GetComponents()[1]->IsTemporary());
    CHECK(node->GetComponents()[2]->IsTemporary());
    REQUIRE(node->GetNumChildren() == 4);
    REQUIRE(node->GetChild(0u)->GetNumComponents() == 2);
    REQUIRE(node->GetChild(0u)->GetNumChildren() == 0);
    REQUIRE(node->GetChild(1u)->GetNumComponents() == 0);
    REQUIRE(node->GetChild(1u)->GetNumChildren() == 0);
    REQUIRE(node->GetChild(2u)->GetNumComponents() == 2);
    REQUIRE(node->GetChild(2u)->GetNumChildren() == 0);
    REQUIRE(node->GetChild(3u)->GetNumComponents() == 0);
    REQUIRE(node->GetChild(3u)->GetNumChildren() == 1);

    prefabData.GetMutableChildren().erase_at(1);
    prefabData.GetMutableComponents().clear();
    prefabResource->SendEvent(E_RELOADFINISHED);

    REQUIRE(node->GetNumComponents() == 1);
    CHECK_FALSE(node->GetComponents()[0]->IsTemporary());
    REQUIRE(node->GetNumChildren() == 3);
    REQUIRE(node->GetChild(0u)->GetNumComponents() == 2);
    REQUIRE(node->GetChild(0u)->GetNumChildren() == 0);
    REQUIRE(node->GetChild(1u)->GetNumComponents() == 2);
    REQUIRE(node->GetChild(1u)->GetNumChildren() == 0);
    REQUIRE(node->GetChild(2u)->GetNumComponents() == 0);
    REQUIRE(node->GetChild(2u)->GetNumChildren() == 1);
}
