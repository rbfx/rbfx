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
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"

#include <Urho3D/PatternMatching/CharacterConfigurator.h>
#include <Urho3D/PatternMatching/PatternCollection.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLArchive.h>

#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("CharacterConfigurator serilization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto cache = context->GetSubsystem<ResourceCache>();

    auto conf = MakeShared<CharacterConfiguration>(context);
    conf->SetModelAttr(ResourceRef(Model::GetTypeStatic(),("Models/Mutant/Mutant.mdl")));
    conf->SetMaterialAttr(ResourceRefList(Material::GetTypeStatic(), StringVector{"Models/Mutant/Materials/mutant_M.xml" }));
    conf->SetRotation(Quaternion(180, Vector3(0, 1, 0)));
    conf->SetNumBodyParts(2);
    auto& bodyParts = conf->GetModifiableBodyParts();
    bodyParts[0].attachmentBone_ = "mixamorig:RightHand";
    bodyParts[1].attachmentBone_ = "mixamorig:LeftHand";
    auto* states = conf->GetStates();
    {
        states->BeginPattern();
        StringVariantMap idleArgs;
        idleArgs["existing"] = true;
        idleArgs["exclusive"] = true;
        idleArgs["looped"] = true;
        idleArgs["animation"] = ResourceRef(Animation::GetTypeStatic(), "Models/Mutant/Mutant_Idle.ani");
        idleArgs["fadeInTime"] = 0.2f;
        states->AddEvent("PlayAnimation", idleArgs);
        states->CommitPattern();
    }
    {
        states->BeginPattern();
        states->AddKey("Run");
        states->AddKeyGreaterOrEqual("OnGround", 0.5f);
        StringVariantMap runArgs;
        runArgs["existing"] = true;
        runArgs["exclusive"] = true;
        runArgs["looped"] = true;
        runArgs["animation"] = ResourceRef(Animation::GetTypeStatic(), "Models/Mutant/Mutant_Run.ani");
        runArgs["fadeInTime"] = 0.2f;
        states->AddEvent("PlayAnimation", runArgs);
        states->CommitPattern();
    }
    {
        states->BeginPattern();
        states->AddKeyLessOrEqual("OnGround", 0.5f);
        StringVariantMap jumpArgs;
        jumpArgs["existing"] = false;
        jumpArgs["exclusive"] = true;
        jumpArgs["removeOnCompletion"] = false;
        jumpArgs["animation"] = ResourceRef(Animation::GetTypeStatic(), "Models/Mutant/Mutant_Jump.ani");
        jumpArgs["fadeInTime"] = 0.2f;
        states->AddEvent("PlayAnimation", jumpArgs);
        states->CommitPattern();
    }
    conf->Commit();

    conf->AddMetadata("Key0", "Value0");
    conf->AddMetadata("Key1", 42);

    conf->SaveFile("Char.xml");

    auto scene =  MakeShared<Scene>(context);
    auto configurator = scene->CreateComponent<CharacterConfigurator>();
    configurator->SetConfiguration(conf);
    
    VectorBuffer buf;
    conf->Save(buf);
    //ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());

    auto conf2 = MakeShared<CharacterConfiguration>(context);

    buf.Seek(0);
    conf2->Load(buf);

    CHECK(conf2->GetMetadata("Key0").GetString() == "Value0");
    CHECK(conf2->GetMetadata("Key1").GetInt() == 42);
}
