#include "CharacterConfigurator.h"
#include "CharacterConfiguration.h"
#include "Material.h"
#include "AnimatedModel.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"

namespace Urho3D
{
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
    auto* cache = context_->GetSubsystem<ResourceCache>();
    if (!skeleton_)
        skeleton_ = node_->CreateComponent<AnimatedModel>();
    skeleton_->SetModel(cache->GetResource<Model>(configuration_->GetModelAttr().name_));
    unsigned materialIndex = 0;
    for (auto& material : configuration_->GetMaterialsAttr().names_)
    {
        skeleton_->SetMaterial(materialIndex, cache->GetResource<Material>(material));
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
