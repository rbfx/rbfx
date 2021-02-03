//
// Copyright (c) 2008-2017 the Urho3D project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Audio/SoundListener.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/Archive.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/BinaryArchive.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLArchive.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include "Serialization.h"

#include <Urho3D/DebugNew.h>

/// Plain struct for serialization.
struct PlainStruct
{
    /// Boolean value.
    bool bool_{};

    /// 8-bit integer.
    unsigned char byte_{};
    /// 16-bit integer.
    short short_{};
    /// 32-bit integer.
    int int_{};
    /// 64-bit integer.
    long long long_{};

    /// 32-bit floating point value.
    float float_{};
    /// 64-bit floating point value.
    float double_{};

    /// 2D vector of floats.
    Vector2 vec2_{};
    /// 3D vector of floats.
    Vector3 vec3_{};
    /// 4D vector of floats.
    Vector4 vec4_{};
    /// 2D vector of integers.
    IntVector2 intVec2_{};
    /// 3D vector of integers.
    IntVector3 intVec3_{};

    /// Rectangle of floats.
    Rect rect_{};
    /// Rectangle of integers.
    IntRect intRect_{};

    /// 3x3 matrix of floats.
    Matrix3 mat3_{};
    /// 3x4 matrix of floats.
    Matrix3x4 mat3x4_{};
    /// 4x4 matrix of floats.
    Matrix4 mat4_{};

    /// Quaternion (4 floats).
    Quaternion quat_{};
    /// Color (4 floats).
    Color color_{};

    /// Compare.
    bool operator ==(const PlainStruct& other) const
    {
        return bool_ == other.bool_

            && byte_ == other.byte_
            && short_ == other.short_
            && int_ == other.int_
            && long_ == other.long_

            && Equals(float_, other.float_)
            && Equals(double_, other.double_)

            && vec2_.Equals(other.vec2_)
            && vec3_.Equals(other.vec3_)
            && vec4_.Equals(other.vec4_)
            && intVec2_ == other.intVec2_
            && intVec3_ == other.intVec3_

            && rect_.Equals(other.rect_)
            && intRect_ == other.intRect_

            && mat3_.Equals(other.mat3_)
            && mat3x4_.Equals(other.mat3x4_)
            && mat4_.Equals(other.mat4_)

            && quat_.Equals(other.quat_)
            && color_.Equals(other.color_);
    }
};

/// Serialize plain struct.
bool SerializeValue(Archive& archive, const char* name, PlainStruct& value)
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

/// Struct of containers.
struct ContainerStruct
{
    /// String.
    ea::string string_{};
    /// Vector of floats.
    ea::vector<float> vectorOfFloats_{};
    /// Vector of floats stored as bytes.
    ea::vector<float> byteFloatVector_{};
    /// String-to-float map.
    ea::hash_map<ea::string, float> mapOfFloats_{};

    /// Variant map stored as Variant.
    Variant variantMap_;
    /// Variant vector stored as Variant.
    Variant variantVector_;
    /// Variant buffer stored as Variant.
    Variant variantBuffer_;

    /// Empty serializable.
    SharedPtr<Serializable> emptySerializable_;
    /// Serializable (sound listener).
    SharedPtr<SoundListener> soundListener_;

    /// Create tuple for comparison.
    auto Tie() const
    {
        return std::tie(
            string_,
            vectorOfFloats_,
            byteFloatVector_,
            mapOfFloats_,
            variantMap_,
            variantVector_,
            variantBuffer_,
            emptySerializable_
        );
    }
    /// Compare.
    bool operator ==(const ContainerStruct& other) const
    {
        if (Tie() != other.Tie())
            return false;

        if (!!soundListener_ != !!other.soundListener_)
            return false;

        if (soundListener_ && soundListener_->GetType() != other.soundListener_->GetType())
            return false;

        if (!!soundListener_ != !!other.soundListener_)
            return false;

        if (soundListener_ && soundListener_->IsEnabled() != other.soundListener_->IsEnabled())
            return false;

        return true;
    }
};

/// Serialize struct of containers.
bool SerializeValue(Archive& archive, const char* name, ContainerStruct& value)
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
        SerializeValue(archive, "soundListener_", value.soundListener_);
        return true;
    }
    return false;
}

/// Struct for serialization test.
struct TestStruct
{
    /// Plain struct.
    PlainStruct plain_;
    /// Struct of containers.
    ContainerStruct container_;
    /// Custom Variant with ContainerStruct.
    Variant containerVariant_ = MakeCustomValue(ContainerStruct{});

    /// Create tuple for comparison.
    auto Tie() const { return std::tie(plain_, container_, containerVariant_); }
    /// Compare.
    bool operator ==(const TestStruct& other) const { return Tie() == other.Tie(); }
};

/// Serialize struct for serialization test.
bool SerializeValue(Archive& archive, const char* name, TestStruct& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "plain_", value.plain_);
        SerializeValue(archive, "container_", value.container_);
        SerializeValue(archive, "containerVariant_", value.containerVariant_);
        return true;
    }
    return false;
}

/// Create test struct.
TestStruct CreateTestStruct(Context* context)
{
    const Quaternion rot = Quaternion(30.0f, Vector3::UP);

    TestStruct result;
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

    auto soundListener = MakeShared<SoundListener>(context);
    soundListener->SetEnabled(false);
    result.container_.soundListener_ = soundListener;

    result.containerVariant_ = MakeCustomValue(result.container_);

    return result;
}

/// Save test struct.
template <class ArchiveType, class ResourceType>
SharedPtr<ResourceType> SaveTestStruct(Context* context, const TestStruct& data)
{
    SharedPtr<ResourceType> resource = MakeShared<ResourceType>(context);
    ArchiveType archive{ resource };
    SerializeValue(archive, "TestStruct", const_cast<TestStruct&>(data));

    if (archive.HasError())
        return nullptr;
    return resource;
}

/// Save test struct to binary archive.
template <class ArchiveType>
VectorBuffer SaveTestStructBinary(Context* context, const TestStruct& data)
{
    VectorBuffer buffer;
    ArchiveType archive{ context, buffer };
    SerializeValue(archive, "TestStruct", const_cast<TestStruct&>(data));

    if (archive.HasError())
        return {};
    return buffer;
}

/// Load test struct.
template <class ArchiveType, class ResourceType>
ea::unique_ptr<TestStruct> LoadTestStruct(Context* context, ResourceType& resource)
{
    ArchiveType archive{ &resource };

    TestStruct data;
    SerializeValue(archive, "TestStruct", data);

    if (archive.HasError())
        return nullptr;
    return ea::make_unique<TestStruct>(data);
}

/// Load test struct from binary archive.
template <class ArchiveType>
ea::unique_ptr<TestStruct> LoadTestStructBinary(Context* context, VectorBuffer& buffer)
{
    buffer.Seek(0);
    ArchiveType archive{ context, buffer };

    TestStruct data;
    SerializeValue(archive, "TestStruct", data);

    if (archive.HasError())
        return nullptr;
    return ea::make_unique<TestStruct>(data);
}

/// Create test scene.
SharedPtr<Scene> CreateTestScene(Context* context, int numObjects)
{
    auto scene = MakeShared<Scene>(context);

    auto cache = context->GetSubsystem<ResourceCache>();
    scene->CreateComponent<Octree>();

    for (int i = 0; i < numObjects; ++i)
    {
        Node* node = scene->CreateChild("Object");
        node->SetPosition(Vector3(i * 3.0f, 0.0f, 0.0f));
        node->SetRotation(Quaternion(i * 15.0f, Vector3::UP));
        node->SetScale(1.5f);

        auto model = node->CreateComponent<StaticModel>();
        model->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        model->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

        auto scaleAnimation = MakeShared<ValueAnimation>(context);
        scaleAnimation->SetKeyFrame(0.0f, Vector3::ONE * 1.0f);
        scaleAnimation->SetKeyFrame(1.0f, Vector3::ONE * 1.5f);
        scaleAnimation->SetKeyFrame(2.0f, Vector3::ONE * 1.0f);

        auto textAnimation = MakeShared<ValueAnimation>(context);
        textAnimation->SetKeyFrame(0.0f, "Object");
        textAnimation->SetKeyFrame(1.0f, "Box");
        textAnimation->SetKeyFrame(2.0f, "Object");

        auto objectAnimation = MakeShared<ObjectAnimation>(context);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation);
        node->SetObjectAnimation(objectAnimation);

        node->SetAttributeAnimation("Name", textAnimation);
    }

    return scene;
}

/// Compare value animations.
bool CompareValueAnimations(const ValueAnimation* lhs, const ValueAnimation* rhs)
{
    const auto& lhsFrames = lhs->GetKeyFrames();
    const auto& rhsFrames = rhs->GetKeyFrames();

    const bool metaEqual =
        lhsFrames.size() == rhsFrames.size()
        && lhs->GetInterpolationMethod() == rhs->GetInterpolationMethod();

    if (!metaEqual)
        return false;

    for (unsigned i = 0; i < lhsFrames.size(); ++i)
    {
        if (!Equals(lhsFrames[i].time_, rhsFrames[i].time_) || lhsFrames[i].value_ != rhsFrames[i].value_)
            return false;
    }

    return true;
}

/// Compare value animation infos.
bool CompareValueAnimationInfos(const ValueAnimationInfo* lhs, const ValueAnimationInfo* rhs)
{
    return lhs->GetSpeed() == rhs->GetSpeed()
        && lhs->GetWrapMode() == rhs->GetWrapMode()
        && CompareValueAnimations(lhs->GetAnimation(), rhs->GetAnimation());
}

/// Compare nodes (recursive).
bool CompareNodes(Node* lhs, Node* rhs)
{
    // Compare contents
    if (!lhs->GetPosition().Equals(rhs->GetPosition()))
        return false;

    if (!lhs->GetRotation().Equals(rhs->GetRotation()))
        return false;

    if (!lhs->GetScale().Equals(rhs->GetScale()))
        return false;

    if (lhs->GetNumChildren() != rhs->GetNumChildren())
        return false;

    if (lhs->GetNumComponents() != rhs->GetNumComponents())
        return false;

    if (lhs->GetName() != rhs->GetName())
        return false;

    ObjectAnimation* lhsObjectAnimation = lhs->GetObjectAnimation();
    ObjectAnimation* rhsObjectAnimation = rhs->GetObjectAnimation();
    if (!!lhsObjectAnimation != !!rhsObjectAnimation)
        return false;

    ValueAnimation* lhsAttributeAnimation = lhs->GetAttributeAnimation("Name");
    ValueAnimation* rhsAttributeAnimation = rhs->GetAttributeAnimation("Name");
    if (!!lhsAttributeAnimation != !!rhsAttributeAnimation)
        return false;

    // Compare animations
    if (lhsObjectAnimation)
    {
        const auto& lhsInfos = lhsObjectAnimation->GetAttributeAnimationInfos();
        const auto& rhsInfos = rhsObjectAnimation->GetAttributeAnimationInfos();

        if (lhsInfos.size() != rhsInfos.size())
            return false;

        auto lhsIter = lhsInfos.begin();
        auto rhsIter = rhsInfos.begin();

        for (unsigned i = 0; i < lhsInfos.size(); ++i)
        {
            if (!CompareValueAnimationInfos(lhsIter->second, rhsIter->second))
                return false;
        }
    }

    if (lhsAttributeAnimation)
    {
        if (!CompareValueAnimations(lhsAttributeAnimation, rhsAttributeAnimation))
            return false;
    }

    // Compare components
    for (unsigned i = 0; i < lhs->GetNumComponents(); ++i)
    {
        if (lhs->GetComponents()[i]->GetType() != rhs->GetComponents()[i]->GetType())
            return false;
    }

    // Compare StaticModel component
    auto lhsStaticModel = lhs->GetComponent<StaticModel>();
    auto rhsStaticModel = rhs->GetComponent<StaticModel>();

    if (!!lhsStaticModel != !!rhsStaticModel)
        return false;

    if (lhsStaticModel)
    {
        Model* lhsModel = lhsStaticModel->GetModel();
        Model* rhsModel = rhsStaticModel->GetModel();

        if (!!lhsModel != !!rhsModel)
            return false;

        if (lhsModel)
        {
            if (lhsModel->GetName() != rhsModel->GetName())
                return false;
        }

        if (lhsStaticModel->GetNumGeometries() != rhsStaticModel->GetNumGeometries())
            return false;

        Material* lhsMaterial = lhsStaticModel->GetMaterial();
        Material* rhsMaterial = rhsStaticModel->GetMaterial();
        if (!!lhsMaterial != !!rhsMaterial)
            return false;

        if (lhsMaterial)
        {
            if (lhsMaterial->GetName() != rhsMaterial->GetName())
                return false;
        }
    }

    // Compare children
    for (unsigned i = 0; i < lhs->GetNumChildren(); ++i)
    {
        if (!CompareNodes(lhs->GetChild(i), rhs->GetChild(i)))
            return false;
    }

    return true;
}

/// Assert and call ErrorExit.
#define URHO3D_ASSERT(expr) (!!(expr) || (ErrorDialog("Assertion failed!", Format("File: {}\nLine: {}\nAssertion failed!", __FILE__, __LINE__)), 0))

Serialization::Serialization(Context* context) :
    Sample(context)
{
}

void Serialization::Start()
{
    // Execute base class startup
    Sample::Start();

    // Test serialization.
    TestStructSerialization();
    TestSceneSerialization();
    TestPartialSerialization();
    TestSerializationPerformance();

    // Close sample
    CloseSample();
}

void Serialization::TestStructSerialization()
{
    const TestStruct sourceObject = CreateTestStruct(context_);

    // Save and load binary
    {
        auto binaryData = SaveTestStructBinary<BinaryOutputArchive>(context_, sourceObject);
        URHO3D_ASSERT(binaryData.GetSize() != 0);
        const auto objectFromBinary = LoadTestStructBinary<BinaryInputArchive>(context_, binaryData);
        URHO3D_ASSERT(objectFromBinary);
        URHO3D_ASSERT(sourceObject == *objectFromBinary);
    }

    // Save and load XML
    {
        auto xmlData = SaveTestStruct<XMLOutputArchive, XMLFile>(context_, sourceObject);
        URHO3D_ASSERT(xmlData);
        auto objectFromXML = LoadTestStruct<XMLInputArchive, XMLFile>(context_, *xmlData);
        URHO3D_ASSERT(objectFromXML);
        URHO3D_ASSERT(sourceObject == *objectFromXML);
    }

    // Save and load JSON
    {
        auto jsonData = SaveTestStruct<JSONOutputArchive, JSONFile>(context_, sourceObject);
        URHO3D_ASSERT(jsonData);
        auto objectFromJSON = LoadTestStruct<JSONInputArchive, JSONFile>(context_, *jsonData);
        URHO3D_ASSERT(objectFromJSON);
        URHO3D_ASSERT(sourceObject == *objectFromJSON);
    }
}

void Serialization::TestPartialSerialization()
{
    bool success = true;
    TestStruct sourceObject = CreateTestStruct(context_);

    {
        auto xmlFile = MakeShared<XMLFile>(context_);
        XMLElement root = xmlFile->CreateRoot("root");

        XMLOutputArchive xmlOutputArchive{ context_, root.CreateChild("child") };
        success &= SerializeValue(xmlOutputArchive, "TestStruct", sourceObject);

        XMLInputArchive xmlInputArchive{ context_, root.GetChild("child") };
        TestStruct objectFromXML;
        success &= SerializeValue(xmlInputArchive, "TestStruct", objectFromXML);

        URHO3D_ASSERT(sourceObject == objectFromXML);
    }

    {
        auto jsonFile = MakeShared<JSONFile>(context_);
        JSONValue& root = jsonFile->GetRoot();

        JSONValue child;
        JSONOutputArchive jsonOutputArchive{ context_, child };
        success &= SerializeValue(jsonOutputArchive, "TestStruct", sourceObject);
        root.Set("child", child);

        JSONInputArchive jsonInputArchive{ context_, root.Get("child") };
        TestStruct objectFromJSON;
        success &= SerializeValue(jsonInputArchive, "TestStruct", objectFromJSON);

        URHO3D_ASSERT(sourceObject == objectFromJSON);
    }

    URHO3D_ASSERT(success);
}

void Serialization::TestSceneSerialization()
{
    bool success = true;

    auto sourceScene = CreateTestScene(context_, 10);
    URHO3D_ASSERT(CompareNodes(sourceScene, sourceScene));

    // Save and load binary
    {
        auto sceneFromBinary = MakeShared<Scene>(context_);

        VectorBuffer binarySceneData;
        BinaryOutputArchive binaryOutputArchive{ context_, binarySceneData };
        success &= sourceScene->Serialize(binaryOutputArchive);
        success &= !binaryOutputArchive.HasError();

        binarySceneData.Seek(0);
        BinaryInputArchive binaryInputArchive{ context_, binarySceneData };
        success &= sceneFromBinary->Serialize(binaryInputArchive);
        success &= !binaryInputArchive.HasError();

        URHO3D_ASSERT(CompareNodes(sourceScene, sceneFromBinary));
    }

    // Save and load XML
    {
        auto sceneFromXML = MakeShared<Scene>(context_);

        XMLFile xmlSceneData{ context_ };
        XMLOutputArchive xmlOutputArchive{ &xmlSceneData };
        success &= sourceScene->Serialize(xmlOutputArchive);
        success &= !xmlOutputArchive.HasError();

        XMLInputArchive xmlInputArchive{ &xmlSceneData };
        success &= sceneFromXML->Serialize(xmlInputArchive);
        success &= !xmlInputArchive.HasError();

        URHO3D_ASSERT(CompareNodes(sourceScene, sceneFromXML));
    }

    // Save and load JSON
    {
        auto sceneFromJSON = MakeShared<Scene>(context_);

        JSONFile jsonSceneData{ context_ };
        JSONOutputArchive jsonOutputArchive{ &jsonSceneData };
        success &= sourceScene->Serialize(jsonOutputArchive);
        success &= !jsonOutputArchive.HasError();

        JSONInputArchive jsonInputArchive{ &jsonSceneData };
        success &= sceneFromJSON->Serialize(jsonInputArchive);
        success &= !jsonInputArchive.HasError();

        URHO3D_ASSERT(CompareNodes(sourceScene, sceneFromJSON));
    }

    // Save legacy JSON and load JSON archive
    {
        auto sceneFromLegacyJSON = MakeShared<Scene>(context_);

        JSONFile jsonLegacySceneData{ context_ };
        success &= sourceScene->SaveJSON(jsonLegacySceneData.GetRoot());

        JSONInputArchive jsonLegacyInputArchive{ &jsonLegacySceneData };
        success &= sceneFromLegacyJSON->Serialize(jsonLegacyInputArchive);
        success &= !jsonLegacyInputArchive.HasError();

        URHO3D_ASSERT(CompareNodes(sourceScene, sceneFromLegacyJSON));
    }

    // Compare scenes
    URHO3D_ASSERT(success);
}

void Serialization::TestSerializationPerformance()
{
    static const unsigned N = 8 * 1024 * 1024;

    // Fill random buffer
    ea::vector<unsigned char> buffer(N, 0);
    for (unsigned i = 0; i < N; ++i)
        buffer[i] = static_cast<unsigned char>(Random(0, 255));

    // Prepare output buffers
    ea::vector<unsigned char> firstBufferData(N + 128, 0);
    ea::vector<unsigned char> secondBufferData(N + 128, 0);
    MemoryBuffer firstBuffer(firstBufferData.data(), firstBufferData.size());
    MemoryBuffer secondBuffer(secondBufferData.data(), secondBufferData.size());
    BinaryOutputArchive secondArchive{ context_, secondBuffer };

    // Serialize array as bytes
    HiresTimer timer;
    firstBuffer.WriteVLE(buffer.size());
    for (unsigned i = 0; i < N; ++i)
        firstBuffer.WriteUByte(buffer[i]);
    const long long firstDuration = timer.GetUSec(true);

    SerializeVectorAsObjects(secondArchive, "buffer", "element", buffer);
    const long long secondDuration = timer.GetUSec(true);

    // Log result
    const double ratio = static_cast<double>(secondDuration) / static_cast<double>(firstDuration);
    const ea::string message = Format("Archive output is {:.1f} times slower than native serialization", ratio);
    URHO3D_LOGINFO(message);
    URHO3D_ASSERT(firstBufferData == secondBufferData);
    ErrorDialog("Serialization Performance", message);
}
