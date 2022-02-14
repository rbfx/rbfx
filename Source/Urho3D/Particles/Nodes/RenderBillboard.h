//
// Copyright (c) 2021-2022 the rbfx project.
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

#pragma once

#include "../TemplateNode.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

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
