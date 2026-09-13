// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../TemplateNode.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class CircleInstance;

class URHO3D_API Circle : public TemplateNode<CircleInstance, Vector3, Vector3>
{
    URHO3D_OBJECT(Circle, ParticleGraphNode)
public:
    /// Construct Circle.
    explicit Circle(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Radius.
    void SetRadius(float value);
    /// Get Radius.
    float GetRadius() const;

    /// Set Radius Thickness.
    void SetRadiusThickness(float value);
    /// Get Radius Thickness.
    float GetRadiusThickness() const;

    /// Set Translation.
    void SetTranslation(Vector3 value);
    /// Get Translation.
    Vector3 GetTranslation() const;

    /// Set Rotation.
    void SetRotation(Quaternion value);
    /// Get Rotation.
    Quaternion GetRotation() const;

    /// Set Scale.
    void SetScale(Vector3 value);
    /// Get Scale.
    Vector3 GetScale() const;

protected:
    float radius_{};
    float radiusThickness_{};
    Vector3 translation_{};
    Quaternion rotation_{};
    Vector3 scale_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
