#include "CharacterConfigurator.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/Material.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "CharacterConfiguration.h"
#include "PatternDatabase.h"

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

extern const char* GEOMETRY_CATEGORY;

CharacterConfigurator::CharacterConfigurator(Context* context)
    : Component(context)
{
}

Urho3D::CharacterConfigurator::~CharacterConfigurator() {}

void CharacterConfigurator::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterConfigurator>(GEOMETRY_CATEGORY);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Configuration", GetConfigurationAttr, SetConfigurationAttr, ResourceRef,
        ResourceRef(CharacterConfiguration::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Query", VariantMap, savedQuery_, Variant::emptyVariantMap, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Secondary Material", GetSecondaryMaterialAttr, SetSecondaryMaterialAttr,
        ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
}

void CharacterConfigurator::SetConfiguration(CharacterConfiguration* configuration)
{
    if (configuration == configuration_)
        return;
    if (!node_)
    {
        URHO3D_LOGERROR("Can not set configuration while configurator component is not attached to a scene node");
        return;
    }
    // Unsubscribe from the reload event of previous model (if any), then subscribe to the new

    if (configuration_)
    {
        UnsubscribeFromEvent(configuration_, E_RELOADFINISHED);
    }

    configuration_ = configuration;

    if (configuration_)
    {
        SubscribeToEvent(
            configuration_, E_RELOADFINISHED, URHO3D_HANDLER(CharacterConfigurator, HandleConfigurationReloadFinished));

        ResetBodyStructure();
    }
}
void CharacterConfigurator::ResetMasterModel()
{
    auto* cache = context_->GetSubsystem<ResourceCache>();

    if (!characterNode_)
    {
        characterNode_ = node_->CreateChild("CharacterRoot", LOCAL, 0, true);
    }
    characterNode_->SetPosition(configuration_->GetPosition());
    characterNode_->SetRotation(configuration_->GetRotation());
    characterNode_->SetScale(configuration_->GetScale());

    CharacterBodyPart masterModelBodyPart;
    masterModelBodyPart.static_ = false;

    // Create and setup master animated model
    if (!masterModel_.primaryModel_)
        masterModel_ = configuration_->CreateBodyPartModelComponent(masterModelBodyPart, characterNode_);

    VariantMap masterModelArgs;
    masterModelArgs["model"] = configuration_->GetActualModelAttr();
    masterModelArgs["material"] = configuration_->GetActualMaterialAttr();
    masterModelArgs["castShadows"] = configuration_->GetCastShadows();
    configuration_->SetBodyPartModel(masterModel_, masterModelArgs, secondaryMaterial_);
}

void CharacterConfigurator::ResetBodyPartModels(
    ea::span<BodyPart> parts, const CharacterConfiguration* configuration, const PatternQuery& query)
{
    if (!configuration)
        return;

    unsigned bodyPartIndex = 0;
    for (; bodyPartIndex < configuration->GetNumBodyParts() && bodyPartIndex < parts.size(); ++bodyPartIndex)
    {
        auto& bodyPart = bodyPartNodes_[bodyPartIndex];
        if (!bodyPart.modelComponent_.primaryModel_)
        {
            bodyPart.configuration_ = configuration;
            bodyPart.index_ = bodyPartIndex;

            const auto& charBodyParts = configuration->GetBodyParts();

            if (bodyPartIndex < charBodyParts.size())
            {
                auto& charBodyPart = charBodyParts[bodyPartIndex];

                bodyPart.modelComponent_ = configuration->CreateBodyPartModelComponent(charBodyPart, characterNode_);
                configuration->UpdateBodyPart(bodyPart.modelComponent_, charBodyPart, query, secondaryMaterial_);
            }
        }
    }
    if (bodyPartIndex < parts.size())
    {
        ResetBodyPartModels(parts.subspan(bodyPartIndex), configuration->GetParent(), query);
    }
}

void CharacterConfigurator::ResetBodyStructure()
{
    if (!configuration_)
    {
        if (characterNode_)
            characterNode_->Remove();
        masterModel_.primaryModel_.Reset();
        masterModel_.secondaryModel_.Reset();
        characterNode_.Reset();
        return;
    }

    // Create and setup body parts
    ResizeBodyParts(configuration_->GetTotalNumBodyParts());

    PatternQuery q;
    for (auto& savedKey : savedQuery_)
    {
        q.SetKey(savedKey.first, savedKey.second.GetFloat());
    }

    ResetMasterModel();

    ResetBodyPartModels(ea::span<BodyPart>(bodyPartNodes_), configuration_, q);

    animationController_ = characterNode_->GetOrCreateComponent<AnimationController>(LOCAL);
}
void CharacterConfigurator::ResizeBodyParts(unsigned numBodyParts)
{
    if (bodyPartNodes_.size() == numBodyParts)
        return;

    for (unsigned partToDelete = numBodyParts; partToDelete < bodyPartNodes_.size(); ++partToDelete)
    {
        auto& model = bodyPartNodes_[partToDelete].modelComponent_.primaryModel_;
        if (model && model->GetNode())
        {
            Node* node = model->GetNode();
            if (node != characterNode_)
                node->Remove();
            else
                model->GetNode()->RemoveComponent(model);
        }
        model.Reset();
    }
    bodyPartNodes_.resize(numBodyParts);
}

/// Handle enabled/disabled state change.
void CharacterConfigurator::OnSetEnabled() { Component::OnSetEnabled(); }

/// Handle scene node being assigned at creation.
void CharacterConfigurator::OnNodeSet(Node* node)
{
    Component::OnNodeSet(node);
    if (node)
    {
        ResetBodyStructure();
    }
    else
    {
        ResizeBodyParts(0);
        if (characterNode_)
        {
            characterNode_->Remove();
            characterNode_.Reset();
        }
    }
}

void CharacterConfigurator::Update(const PatternQuery& query)
{
    ResetBodyStructure();

    savedQuery_.clear();
    for (unsigned i = 0; i < query.GetNumKeys(); ++i)
    {
        savedQuery_[query.GetKeyHash(i)] = query.GetValue(i);
    }

    if (!configuration_)
    {
        return;
    }

    for (unsigned bodyPartIndex = 0; bodyPartIndex < bodyPartNodes_.size(); ++bodyPartIndex)
    {
        auto& bodyPart = bodyPartNodes_[bodyPartIndex];
        if (bodyPart.modelComponent_.primaryModel_ && bodyPart.configuration_)
        {
            bodyPart.configuration_->UpdateBodyPart(bodyPart.modelComponent_,
                bodyPart.configuration_->GetBodyParts()[bodyPart.index_], query, secondaryMaterial_);
        }
    }

    const auto* states = configuration_->GetIndex();
    if (states)
    {
        const auto stateMatch = states->Query(query);
        if (stateIndex_ != stateMatch)
        {
            stateIndex_ = stateMatch;
            if (stateIndex_ >= 0)
            {
                const auto numEvents = states->GetNumEvents(stateIndex_);
                for (unsigned i = 0; i < numEvents; ++i)
                {
                    auto eventId = states->GetEventId(stateIndex_, i);
                    if (eventId == StringHash("PlayAnimation"))
                    {
                        PlayAnimation(eventId, states->GetEventArgs(stateIndex_, i));
                    }
                }
            }
        }
    }
}

void CharacterConfigurator::PlayAnimation(StringHash eventType, const VariantMap& eventData)
{
    auto animationIt = eventData.find("animation");
    if (animationIt == eventData.end())
        return;
    Animation* animation{};
    if (animationIt->second.GetType() == VAR_RESOURCEREF)
    {
        animation =
            context_->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationIt->second.GetResourceRef().name_);
    }
    else if (animationIt->second.GetType() == VAR_RESOURCEREFLIST)
    {
        auto& names = animationIt->second.GetResourceRefList().names_;
        if (names.empty())
            animation = context_->GetSubsystem<ResourceCache>()->GetResource<Animation>(names[Random(0, names.size())]);
    }
    if (!animation)
        return;
    AnimationParameters params(animation);

    const bool exclusive = GetOptional("exclusive", eventData, false);
    const bool existing = GetOptional("existing", eventData, false);
    const float fadeInTime = GetOptional("fadeInTime", eventData, 0.0f);
    params.looped_ = GetOptional<bool>("looped", eventData, params.looped_);
    params.layer_ = GetOptional<unsigned>("layer", eventData, params.layer_);
    params.removeOnZeroWeight_ = GetOptional<bool>("removeOnZeroWeight", eventData, params.removeOnZeroWeight_);
    params.blendMode_ = static_cast<AnimationBlendMode>(GetOptional<int>("blendMode", eventData, params.blendMode_));
    params.autoFadeOutTime_ = GetOptional<float>("autoFadeOutTime", eventData, params.autoFadeOutTime_);
    params.removeOnCompletion_ = GetOptional<bool>("removeOnCompletion", eventData, params.removeOnCompletion_);
    params.speed_ = GetOptional<float>("speed", eventData, params.speed_);
    params.weight_ = GetOptional<float>("weight", eventData, params.weight_);
    params.removeOnZeroWeight_ = GetOptional<bool>("removeOnZeroWeight", eventData, params.removeOnZeroWeight_);

    velocity_ =
        configuration_->LocalToWorld() * (animation->GetMetadata("LinearVelocity").GetVector3() * params.speed_);

    if (exclusive)
    {
        if (existing)
        {
            animationController_->PlayExistingExclusive(params, fadeInTime);
        }
        else
        {
            animationController_->PlayNewExclusive(params, fadeInTime);
        }
    }
    else
    {
        if (existing)
        {
            animationController_->PlayExisting(params, fadeInTime);
        }
        else
        {
            animationController_->PlayNew(params, fadeInTime);
        }
    }
}

void CharacterConfigurator::SetConfigurationAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetConfiguration(cache->GetResource<CharacterConfiguration>(value.name_));
}

ResourceRef CharacterConfigurator::GetConfigurationAttr() const
{
    return GetResourceRef(configuration_, CharacterConfiguration::GetTypeStatic());
}

/// Set secondary material.
void CharacterConfigurator::SetSecondaryMaterial(Material* material)
{
    if (secondaryMaterial_ != material)
    {
        secondaryMaterial_ = material;
        for (auto& part : bodyPartNodes_)
        {
            StaticModel* model = part.modelComponent_.secondaryModel_;
            if (model)
            {
                model->SetEnabled(material != nullptr);
                model->SetMaterial(material);
            }
        }
    }
}

/// Set secondary material attribute.
void CharacterConfigurator::SetSecondaryMaterialAttr(const ResourceRef& value)
{
    SetSecondaryMaterial(context_->GetSubsystem<ResourceCache>()->GetResource<Material>(value.name_));
}

/// Return secondary material attribute.
ResourceRef CharacterConfigurator::GetSecondaryMaterialAttr() const
{
    return GetResourceRef(secondaryMaterial_, Material::GetTypeStatic());
}

void CharacterConfigurator::HandleConfigurationReloadFinished(StringHash eventType, VariantMap& eventData)
{
    CharacterConfiguration* currentModel = configuration_;
    configuration_.Reset(); // Set null to allow to be re-set
    SetConfiguration(currentModel);
}

void RegisterPatternMatchingLibrary(Context* context)
{
    CharacterConfigurator::RegisterObject(context);
    CharacterConfiguration::RegisterObject(context);
    PatternDatabase::RegisterObject(context);
}

} // namespace Urho3D
