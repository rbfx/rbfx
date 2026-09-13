// Copyright (c) 2021-2022 the rbfx project.
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
class BoxInstance;

class URHO3D_API Box : public TemplateNode<BoxInstance, Vector3, Vector3>
{
    URHO3D_OBJECT(Box, ParticleGraphNode)
public:
    /// Construct Box.
    explicit Box(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Box Thickness.
    void SetBoxThickness(Vector3 value);
    /// Get Box Thickness.
    Vector3 GetBoxThickness() const;

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

    /// Set From.
    void SetFrom(int value);
    /// Get From.
    int GetFrom() const;

protected:
    Vector3 boxThickness_{};
    Vector3 translation_{};
    Quaternion rotation_{};
    Vector3 scale_{};
    int from_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
