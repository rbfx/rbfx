#include "CharacterConfigurator.h"
#include "CharacterConfiguration.h"
#include "Material.h"
#include "AnimatedModel.h"
#include "Animation.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"

namespace Urho3D
{
namespace 
{
template <typename  T> T GetOptional(StringHash key, const VariantMap& map, const T& defaultValue)
{
    auto it = map.find(key);
    if (it == map.end())
        return defaultValue;
    return it->second.Get<T>();
}
}

extern const char* GEOMETRY_CATEGORY;

CharacterConfigurator::CharacterConfigurator(Context* context)
    : Component(context)
{
}

Urho3D::CharacterConfigurator::~CharacterConfigurator() { }

void CharacterConfigurator::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterConfigurator>(GEOMETRY_CATEGORY);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Configuration", GetConfigurationAttr, SetConfigurationAttr, ResourceRef,
        ResourceRef(CharacterConfiguration::GetTypeStatic()), AM_DEFAULT);
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
        SubscribeToEvent(configuration_, E_RELOADFINISHED,
            URHO3D_HANDLER(CharacterConfigurator, HandleConfigurationReloadFinished));

        RecreateBodyStructure();
    }
}

void CharacterConfigurator::RecreateBodyStructure()
{
    if (!configuration_)
    {
        characterNode_->Remove();
        masterModel.Reset();
        characterNode_.Reset();
        return;
    }

    auto* cache = context_->GetSubsystem<ResourceCache>();

    if (!characterNode_)
    {
        characterNode_ = node_->CreateChild("CharacterRoot", LOCAL, 0, true);
        characterNode_->SetPosition(configuration_->GetPosition());
        characterNode_->SetRotation(configuration_->GetRotation());
        characterNode_->SetScale(configuration_->GetScale());
    }

    // Create and setup master animated model
    if (!masterModel)
        masterModel = characterNode_->GetOrCreateComponent<AnimatedModel>(LOCAL);

    const auto rootModel = configuration_->GetModelAttr();
    if (!rootModel.name_.empty())
    {
        masterModel->SetModel(cache->GetResource<Model>(configuration_->GetModelAttr().name_));
        unsigned materialIndex = 0;
        for (auto& material : configuration_->GetMaterialAttr().names_)
        {
            masterModel->SetMaterial(materialIndex, cache->GetResource<Material>(material));
            ++materialIndex;
        }
        masterModel->SetCastShadows(configuration_->GetCastShadows());
    }
    else
    {
        masterModel->SetModel(nullptr);
    }
    // Create and setup body parts
    const unsigned numBodyParts = configuration_->GetNumBodyParts();
    bodyPartNodes_.resize(numBodyParts);
    PatternQuery q;
    for (auto& savedKey: savedState_)
    {
        q.SetKey(savedKey.first, savedKey.second.GetFloat());
    }
    for (unsigned bodyPartIndex = 0; bodyPartIndex < numBodyParts; ++bodyPartIndex)
    {
        auto& bodyPart = bodyPartNodes_[bodyPartIndex];
        if (!bodyPart.modelComponent_)
        {
            bodyPart.modelComponent_ = configuration_->CreateBodyPartModelComponent(bodyPartIndex, characterNode_);
            bodyPart.lastMatch_ = configuration_->UpdateBodyPart(bodyPartIndex, bodyPart.modelComponent_, q, -1);
        }
    }

    animationController_ = characterNode_->GetOrCreateComponent<AnimationController>(LOCAL);
}

void CharacterConfigurator::Update(const PatternQuery& query)
{
    RecreateBodyStructure();

    savedState_.clear();
    for (unsigned i = 0; i < query.GetNumKeys(); ++i)
    {
        savedState_[query.GetKeyHash(i)] = query.GetValue(i);
    }

    const unsigned numBodyParts = configuration_->GetNumBodyParts();
    bodyPartNodes_.resize(numBodyParts);

    for (unsigned bodyPartIndex = 0; bodyPartIndex < numBodyParts; ++bodyPartIndex)
    {
        auto& bodyPart = bodyPartNodes_[bodyPartIndex];
        if (bodyPart.modelComponent_)
        {
            bodyPart.lastMatch_ = configuration_->UpdateBodyPart(
                bodyPartIndex, bodyPart.modelComponent_, query, bodyPart.lastMatch_);
        }
    }

    const auto* states = configuration_->GetStates();
    if (states)
    {
        const auto stateMatch = states->Query(query);
        if (stateIndex_ != stateMatch)
        {
            stateIndex_ = stateMatch;
            if (stateIndex_ >= 0)
            {
                const auto numEvents = configuration_->GetStates()->GetNumEvents(stateIndex_);
                for (unsigned i = 0; i < numEvents; ++i)
                {
                    auto eventId = configuration_->GetStates()->GetEventId(stateIndex_, i);
                    if (eventId == "PlayAnimation")
                    {
                        PlayAnimation(eventId, configuration_->GetStates()->GetEventArgs(stateIndex_, i));
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
        animation = context_->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationIt->second.GetResourceRef().name_);
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

    velocity_ = configuration_->LocalToWorld() * (animation->GetMetadata("LinearVelocity").GetVector3() * params.speed_);

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

void CharacterConfigurator::HandleConfigurationReloadFinished(StringHash eventType, VariantMap& eventData)
{
    CharacterConfiguration* currentModel = configuration_;
    configuration_.Reset(); // Set null to allow to be re-set
    SetConfiguration(currentModel);
}

} // namespace Urho3D
