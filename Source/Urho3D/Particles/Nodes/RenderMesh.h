// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/TemplateNode.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class RenderMeshInstance;

class URHO3D_API RenderMesh : public TemplateNode<RenderMeshInstance, Matrix3x4>
{
    URHO3D_OBJECT(RenderMesh, ParticleGraphNode)
public:
    /// Construct RenderMesh.
    explicit RenderMesh(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Model.
    void SetModel(ResourceRef value);
    /// Get Model.
    ResourceRef GetModel() const;

    /// Set Material.
    void SetMaterial(ResourceRefList value);
    /// Get Material.
    ResourceRefList GetMaterial() const;

    /// Set Is Worldspace.
    void SetIsWorldspace(bool value);
    /// Get Is Worldspace.
    bool GetIsWorldspace() const;

protected:
    ResourceRef model_{};
    ResourceRefList material_{};
    bool isWorldspace_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
