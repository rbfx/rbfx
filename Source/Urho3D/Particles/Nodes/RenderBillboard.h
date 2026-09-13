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
class RenderBillboardInstance;

class URHO3D_API RenderBillboard : public TemplateNode<RenderBillboardInstance, Vector3, Vector2, float, Color, float, Vector3>
{
    URHO3D_OBJECT(RenderBillboard, ParticleGraphNode)
public:
    /// Construct RenderBillboard.
    explicit RenderBillboard(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Material.
    void SetMaterial(ResourceRef value);
    /// Get Material.
    ResourceRef GetMaterial() const;

    /// Set Rows.
    void SetRows(int value);
    /// Get Rows.
    int GetRows() const;

    /// Set Columns.
    void SetColumns(int value);
    /// Get Columns.
    int GetColumns() const;

    /// Set Face Camera Mode.
    void SetFaceCameraMode(int value);
    /// Get Face Camera Mode.
    int GetFaceCameraMode() const;

    /// Set Sort By Distance.
    void SetSortByDistance(bool value);
    /// Get Sort By Distance.
    bool GetSortByDistance() const;

    /// Set Is Worldspace.
    void SetIsWorldspace(bool value);
    /// Get Is Worldspace.
    bool GetIsWorldspace() const;

    /// Set Crop.
    void SetCrop(Rect value);
    /// Get Crop.
    Rect GetCrop() const;

protected:
    ResourceRef material_{};
    int rows_{};
    int columns_{};
    int faceCameraMode_{};
    bool sortByDistance_{};
    bool isWorldspace_{};
    Rect crop_{Rect::POSITIVE};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
