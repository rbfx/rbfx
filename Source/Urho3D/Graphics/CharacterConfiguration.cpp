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
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../Particles/ParticleGraphEffect.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Serializable.h"
#include "Material.h"
#include "Model.h"

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

    //URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
    //    "Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    //URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRefList,
    //    ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    //URHO3D_ATTRIBUTE("Position", Vector3, position_, Vector3::ZERO, AM_DEFAULT);
    //URHO3D_ATTRIBUTE("Rotatoion", Quaternion, rotation_, Quaternion::IDENTITY, AM_DEFAULT);
    //URHO3D_ATTRIBUTE("Scale", Vector3, scale_, Vector3::ONE, AM_DEFAULT);
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

    model_ = ResourceRef{};
    material_ = ResourceRefList{};
    bodyParts_.clear();
    stateMachine_.Clear();
}

void CharacterConfiguration::UpdateMatrices()
{
    localToWorld_ = Matrix3x4(position_, rotation_, scale_);
    worldToLocal_ = localToWorld_.Inverse();
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
    SerializeOptionalValue(archive, "model", model_);
    SerializeOptionalValue(archive, "material", material_);
    SerializeOptionalValue(archive, "position", position_, Vector3::ZERO);
    SerializeOptionalValue(archive, "rotation", rotation_, Quaternion::IDENTITY);
    SerializeOptionalValue(archive, "scale", scale_, Vector3::ONE);
    SerializeOptionalValue(archive, "castShadows", castShadows_, true);

    SerializeOptionalValue(archive, "bodyParts", bodyParts_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "part");
    });
    stateMachine_.SerializeInBlock(archive);
    //SerializeValue(archive, "states", stateMachine_);
}

/// Resize body parts vector.
void CharacterConfiguration::SetNumBodyParts(unsigned num) { bodyParts_.resize(num); }
/// Get number of body parts.
unsigned CharacterConfiguration::GetNumBodyParts() const { return bodyParts_.size(); }

void CharacterConfiguration::SetModel(Model* model)
{
    if (model)
        SetModelAttr(ResourceRef(model->GetType(), model->GetName()));
    else
        SetModelAttr(ResourceRef{});
}

void CharacterConfiguration::SetModelAttr(ResourceRef model) { model_ = model; }

void CharacterConfiguration::SetMaterialAttr(const ResourceRefList& materials) { material_ = materials; }
void CharacterConfiguration::SetMaterial(Material* material)
{
    if (material)
        SetMaterialAttr(ResourceRefList{material->GetType(), {material->GetName()}});
    else
        SetMaterialAttr(ResourceRefList{});
}
void CharacterConfiguration::SetCastShadows(bool enable) { castShadows_ = enable; }

void CharacterConfiguration::SetPosition(const Vector3& position)
{
    position_ = position;
    UpdateMatrices();
}
void CharacterConfiguration::SetRotation(const Quaternion& rotation)
{
    rotation_ = rotation;
    UpdateMatrices();
}
void CharacterConfiguration::SetScale(float scale)
{
    scale_ = Vector3(scale, scale, scale);
    UpdateMatrices();
}
void CharacterConfiguration::SetScale(const Vector3& scale)
{
    scale_ = scale;
    UpdateMatrices();
}

} // namespace Urho3D
