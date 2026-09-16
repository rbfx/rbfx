#pragma once

#include "_Plugin.h"

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class PLUGIN_CORE_SAMPLEPLUGIN_API SampleComponent : public LogicComponent
{
    URHO3D_OBJECT(SampleComponent, LogicComponent);

public:
    SampleComponent(Context* context);
    static void RegisterObject(Context* context);

    void Update(float timeStep) override;

    /// Attributes.
    /// @{
    void SetAxis(const Vector3& axis) { axis_ = axis; }
    const Vector3& GetAxis() const { return axis_; }
    void SetRotationSpeed(float rotationSpeed) { rotationSpeed_ = rotationSpeed; }
    float GetRotationSpeed() const { return rotationSpeed_; }
    /// @}

private:
    Vector3 axis_;
    float rotationSpeed_{};
};

} // namespace Urho3D
