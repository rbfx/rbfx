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
Serialization::Serialization(Context* context) :
    Sample(context)
{
}

void Serialization::Start()
{
    // Execute base class startup
    Sample::Start();

    // Test serialization.
    TestSceneSerialization();
    TestSerializationPerformance();

    // Close sample
    CloseSample();
}

void Serialization::TestSceneSerialization()
{
    // TODO: Move to unit tests
#if 0
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
#endif
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

    SerializeVectorAsObjects(secondArchive, "buffer", buffer, "element");
    const long long secondDuration = timer.GetUSec(true);

    // Log result
    const double ratio = static_cast<double>(secondDuration) / static_cast<double>(firstDuration);
    const ea::string message = Format("Archive output is {:.1f} times slower than native serialization", ratio);
    URHO3D_LOGINFO(message);
    URHO3D_ASSERT(firstBufferData == secondBufferData);
    ErrorDialog("Serialization Performance", message);
}
