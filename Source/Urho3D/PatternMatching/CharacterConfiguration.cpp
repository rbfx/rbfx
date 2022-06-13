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
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../Particles/ParticleGraphEffect.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

namespace Urho3D
{
namespace
{
template <typename T> T GetOptional(StringHash key, const VariantMap& map, const T& defaultValue)
{
    auto it = map.find(key);
    if (it == map.end())
        return defaultValue;
    return it->second.Get<T>();
}
} // namespace

void CharacterBodyPartInstance::SetModel(ResourceRef model, const ResourceRefList& materials)
{
    if (primaryModel_)
    {
        primaryModel_->SetModelAttr(model);
        primaryModel_->SetMaterialsAttr(materials);
    }
    if (secondaryModel_)
    {
        auto* material = secondaryModel_->GetMaterial();
        secondaryModel_->SetModelAttr(model);
        secondaryModel_->SetMaterial(material);
    }
}

void CharacterBodyPartInstance::SetSecondaryMaterial(Material* material)
{
    // Early exit if instance is not initialized yet.
    if (primaryModel_ == nullptr)
        return;

    if (secondaryModel_ == nullptr)
    {
        // Early exit if secondary model is missing and no material is set.
        if (material == nullptr)
            return;

        Node* node = primaryModel_->GetNode();
        secondaryModel_.DynamicCast(SharedPtr(node->CreateComponent(primaryModel_->GetType())));
        secondaryModel_->SetModel(primaryModel_->GetModel());
        secondaryModel_->SetCastShadows(false);
    }

    StaticModel* model = secondaryModel_;
    if (model)
    {
        if (model->GetMaterial() != material)
        {
            model->SetEnabled(material != nullptr);
            model->SetMaterial(material);
        }
    }
}

void CharacterBodyPartInstance::Reset()
{
    if (primaryModel_)
    {
        if (!attachedToRoot_)
        {
            Node* node = primaryModel_->GetNode();
            if (node)
            {
                node->Remove();
            }
        }
        primaryModel_->Remove();
        primaryModel_.Reset();
    }
    if (secondaryModel_)
    {
        secondaryModel_->Remove();
        secondaryModel_.Reset();
    }
}

/// Serialize from/to archive. Return true if successful.
void CharacterBodyPart::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "name", name_, {});
    SerializeOptionalValue(archive, "static", static_, {});
    SerializeOptionalValue(archive, "bone", attachmentBone_, {});
    variants_.SerializeInBlock(archive, "variants");
    if (archive.IsInput())
    {
        ResourceRef model;
        SerializeOptionalValue(archive, "model", model, {});
        if (!model.name_.empty())
        {
            Quaternion q(Vector3(90, 0, 0));
            ResourceRefList material;
            SerializeOptionalValue(archive, "material", material, {});
            bool castShadows = true;
            SerializeOptionalValue(archive, "castShadows", castShadows, castShadows);
            Vector3 position{Vector3::ZERO};
            SerializeOptionalValue(archive, "position", position, position);
            Quaternion rotation = Quaternion::IDENTITY;
            SerializeOptionalValue(archive, "rotation", rotation, rotation);
            Vector3 scale = Vector3::ONE;
            SerializeOptionalValue(archive, "scale", scale, scale);

            variants_.BeginPattern();
            StringVariantMap args;
            args["model"] = model;
            args["material"] = material;
            args["castShadows"] = castShadows;
            args["position"] = position;
            args["rotation"] = rotation;
            args["scale"] = scale;

            variants_.AddEvent("SetModel", args);
            variants_.CommitPattern();
            variantIndex_.Build(&variants_);
        }
    }
}

CharacterConfiguration::CharacterConfiguration(Context* context)
    : BaseClassName(context)
{
}

CharacterConfiguration::~CharacterConfiguration() = default;

void CharacterConfiguration::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterConfiguration>();

    // URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
    //     "Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    // URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRefList,
    //     ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    // URHO3D_ATTRIBUTE("Position", Vector3, position_, Vector3::ZERO, AM_DEFAULT);
    // URHO3D_ATTRIBUTE("Rotatoion", Quaternion, rotation_, Quaternion::IDENTITY, AM_DEFAULT);
    // URHO3D_ATTRIBUTE("Scale", Vector3, scale_, Vector3::ONE, AM_DEFAULT);
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
    parentConfiguration_ = ResourceRef{};
    parent_.Reset();
    bodyParts_.clear();
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
    SerializeOptionalValue(archive, "parent", parentConfiguration_);
    SerializeOptionalValue(archive, "model", model_);
    SerializeOptionalValue(archive, "material", material_);
    SerializeOptionalValue(archive, "position", position_, Vector3::ZERO);
    SerializeOptionalValue(archive, "rotation", rotation_, Quaternion::IDENTITY);
    SerializeOptionalValue(archive, "scale", scale_, Vector3::ONE);
    SerializeOptionalValue(archive, "castShadows", castShadows_, true);
    states_.SerializeInBlock(archive, "states");
    SerializeOptionalValue(archive, "bodyParts", bodyParts_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "part"); });
    SerializeOptionalValue(archive, "metadata", metadata_, EmptyObject{});

    if (archive.IsInput())
    {
        if (!parentConfiguration_.name_.empty())
        {
            parent_ = context_->GetSubsystem<ResourceCache>()->GetResource<CharacterConfiguration>(
                parentConfiguration_.name_);
        }
        Commit();
    }
}

void CharacterConfiguration::Commit()
{
    ea::fixed_vector<const PatternCollection*, 2> patterns;
    CharacterConfiguration* conf = this;
    while (conf)
    {
        patterns.push_back(&conf->states_);
        conf = conf->GetParent();
    }
    stateIndex_.Build(patterns.begin(), patterns.end());

    for (auto& bodyPart : bodyParts_)
    {
        bodyPart.variantIndex_.Build(&bodyPart.variants_);
    }
}

/// Resize body parts vector.
void CharacterConfiguration::SetNumBodyParts(unsigned num) { bodyParts_.resize(num); }

/// Get total number of body parts including parent body parts.
unsigned CharacterConfiguration::GetTotalNumBodyParts() const
{
    unsigned num = 0;
    const CharacterConfiguration* conf = this;
    while (conf)
    {
        num += conf->GetNumBodyParts();
        conf = conf->GetParent();
    }
    return num;
}

/// Add new metadata variable or overwrite old value.
void CharacterConfiguration::AddMetadata(const ea::string& name, const Variant& value)
{
    metadata_.insert_or_assign(name, value);
}

/// Remove metadata variable.
void CharacterConfiguration::RemoveMetadata(const ea::string& name) { metadata_.erase(name); }

/// Remove all metadata variables.
void CharacterConfiguration::RemoveAllMetadata() { metadata_.clear(); }

/// Return metadata variable.
const Variant& CharacterConfiguration::GetMetadata(const ea::string& name) const
{
    auto it = metadata_.find(name);
    if (it != metadata_.end())
        return it->second;

    return Variant::EMPTY;
}

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

/// Get model from configuration or from parent configuration.
ResourceRef CharacterConfiguration::GetActualModelAttr() const
{
    if (model_.name_.empty())
    {
        if (parent_)
            return parent_->GetActualModelAttr();
    }
    return model_;
}

void CharacterConfiguration::SetMaterialAttr(const ResourceRefList& materials) { material_ = materials; }

/// Get material list from configuration or from parent configuration.
const ResourceRefList& CharacterConfiguration::GetActualMaterialAttr() const
{
    if (material_.names_.empty())
    {
        if (parent_)
            return parent_->GetActualMaterialAttr();
    }
    return material_;
}

void CharacterConfiguration::SetMaterial(Material* material)
{
    if (material)
        SetMaterialAttr(ResourceRefList{material->GetType(), {material->GetName()}});
    else
        SetMaterialAttr(ResourceRefList{});
}

/// Set parent configuration reference. All states and body parts for it will be appended to the current configuration.
void CharacterConfiguration::SetParentAttr(ResourceRef parent)
{
    if (parentConfiguration_ != parent)
    {
        parentConfiguration_ = parent;
    }
}

/// Set parent configuration. All states and body parts for it will be appended to the current
/// configuration.
void CharacterConfiguration::SetParent(CharacterConfiguration* parent)
{
    CharacterConfiguration* p = parent;
    while (p)
    {
        if (p == this)
        {
            URHO3D_LOGERROR("CharacterConfiguration loop detected");
            return;
        }
        p = p->GetParent();
    }
    parent_ = parent;
    if (parent_)
        parentConfiguration_ = ResourceRef(parent_->GetType(), parent_->GetName());
    else
        parentConfiguration_ = ResourceRef{CharacterConfiguration ::GetTypeStatic()};
}

/// Get parent configuration.
CharacterConfiguration* CharacterConfiguration::GetParent() const { return parent_; }

PatternCollection* CharacterConfiguration::GetStates() { return &states_; }

PatternIndex* CharacterConfiguration::GetIndex() { return &stateIndex_; }

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

CharacterBodyPartInstance CharacterConfiguration::CreateBodyPartModelComponent(
    const CharacterBodyPart& bodyPart, Node* root) const
{
    CharacterBodyPartInstance instance;
    if (!root)
        return instance;

    Node* attachmentBone = root;
    Node* bodyPartNode{};
    if (!bodyPart.attachmentBone_.empty())
    {
        attachmentBone = root->GetChild(bodyPart.attachmentBone_, true);
        if (!attachmentBone)
            attachmentBone = root;
        bodyPartNode = attachmentBone->CreateChild(bodyPart.name_, LOCAL, true);
    }
    else
    {
        bodyPartNode = root;
    }
    instance.attachedToRoot_ = bodyPartNode == root;

    if (bodyPart.static_)
    {
        instance.primaryModel_ = bodyPartNode->CreateComponent<StaticModel>();
    }
    else
    {
        instance.primaryModel_ = bodyPartNode->CreateComponent<AnimatedModel>();
    }
    instance.lastQueryResult = -1;
    return instance;
}

void CharacterConfiguration::UpdateBodyPart(CharacterBodyPartInstance& instance, const CharacterBodyPart& bodyPart,
    const PatternQuery& query, Material* secondaryMaterial) const
{
    if (!instance.primaryModel_)
        return;

    const auto res = bodyPart.variantIndex_.Query(query);
    if (res != instance.lastQueryResult)
    {
        instance.lastQueryResult = res;
        const auto numEvents = bodyPart.variantIndex_.GetNumEvents(res);
        for (unsigned i = 0; i < numEvents; ++i)
        {
            auto eventId = bodyPart.variantIndex_.GetEventId(res, i);
            if (eventId == "SetModel")
            {
                auto& eventArgs = bodyPart.variantIndex_.GetEventArgs(res, i);

                SetBodyPartModel(instance, eventArgs);
            }
        }
    }
    instance.SetSecondaryMaterial(secondaryMaterial);
}

void CharacterConfiguration::SetBodyPartModel(CharacterBodyPartInstance& instance, const VariantMap eventArgs) const
{
    const ResourceRef model = GetOptional<ResourceRef>("model", eventArgs, {});
    const ResourceRefList materials = GetOptional<ResourceRefList>("material", eventArgs, {});
    instance.SetModel(model, materials);
    const auto& primaryModel = instance.primaryModel_;

    primaryModel->SetCastShadows(GetOptional<bool>("castShadows", eventArgs, true));
    if (!instance.attachedToRoot_)
    {
        Node* node = primaryModel->GetNode();
        node->SetPosition(GetOptional<Vector3>("position", eventArgs, Vector3::ZERO));
        node->SetRotation(GetOptional<Quaternion>("rotation", eventArgs, Quaternion::IDENTITY));
        node->SetScale(GetOptional<Vector3>("scale", eventArgs, Vector3::ONE));
    }
}

} // namespace Urho3D
