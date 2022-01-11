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

#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"
#include "TemplateNode.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class RenderQuadInstance;

class URHO3D_API RenderQuad : public TemplateNode<RenderQuadInstance, Matrix3x4>
{
    URHO3D_OBJECT(RenderQuad, ParticleGraphNode)
public:
    /// Construct RenderQuad.
    explicit RenderQuad(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Material.
    /// @property
    void SetMaterial(ResourceRef value);
    /// Get Material.
    /// @property
    ResourceRef GetMaterial() const;

    /// Set Is Worldspace.
    /// @property
    void SetIsWorldspace(bool value);
    /// Get Is Worldspace.
    /// @property
    bool GetIsWorldspace() const;

protected:
    ResourceRef material_{};
    bool isWorldspace_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
