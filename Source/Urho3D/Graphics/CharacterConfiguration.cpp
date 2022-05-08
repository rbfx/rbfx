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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "CharacterConfiguration.h"

#include "../Core/Thread.h"
#include "../Particles/ParticleGraphEffect.h"
#include "../Resource/XMLFile.h"
#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/FileSystem.h"

namespace Urho3D
{

/// Serialize from/to archive. Return true if successful.
void CharacterConfiguration::BodyPart::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "bone", attachmentBone_);
    SerializeValue(archive, "model", modelSelector_);
}

CharacterConfiguration::CharacterConfiguration(Context* context)
    : Resource(context)
{
}

CharacterConfiguration::~CharacterConfiguration() = default;

void CharacterConfiguration::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterConfiguration>();
}


bool CharacterConfiguration::BeginLoad(Deserializer& source)
{
    ea::string extension = GetExtension(source.GetName());

    ResetToDefaults();

    const auto xmlFile = MakeShared<XMLFile>(context_);
    if (!xmlFile->Load(source))
        return false;

    return xmlFile->LoadObject("character", *this);
}

void CharacterConfiguration::ResetToDefaults()
{
    // Needs to be a no-op when async loading, as this does a GetResource() which is not allowed from worker threads
    if (!Thread::IsMainThread())
        return;

    skeletonModel_ = ResourceRef{};
    skeletonMaterials_ = ResourceRefList{};
    bodyParts_.clear();
    stateMachine_.Clear();
}

bool CharacterConfiguration::Save(Serializer& dest) const
{
    const auto xmlFile = MakeShared<XMLFile>(context_);
    xmlFile->SaveObject("character", *this);
    xmlFile->Save(dest);
    return true;
}

void CharacterConfiguration::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "model", skeletonModel_);
    SerializeOptionalValue(archive, "materials", skeletonMaterials_);
    SerializeVector(archive, "bodyParts", bodyParts_, "part");
    SerializeValue(archive, "states", stateMachine_);
}

/// Resize body parts vector.
void CharacterConfiguration::SetNumBodyParts(unsigned num)
{
    bodyParts_.resize(num);
}
/// Get number of body parts.
unsigned CharacterConfiguration::GetNumBodyParts() const { return bodyParts_.size(); }

void CharacterConfiguration::SetModelAttr(ResourceRef model) { skeletonModel_ = model; }

void CharacterConfiguration::SetMaterialsAttr(const ResourceRefList& materials) { skeletonMaterials_ = materials; }

}
