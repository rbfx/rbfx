#include "SampleComponent.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

SampleComponent::SampleComponent(Context* context)
    : LogicComponent(context)
{
}

void SampleComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SampleComponent>(Category_User);

    URHO3D_ATTRIBUTE("Axis", Vector3, axis_, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Speed", float, rotationSpeed_, 0.0f, AM_DEFAULT);
}

void SampleComponent::Update(float timeStep)
{
    node_->Rotate(Quaternion{rotationSpeed_ * timeStep, axis_}, TS_WORLD);
}

} // namespace Urho3D
