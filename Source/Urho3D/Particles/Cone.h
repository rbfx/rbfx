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

#include "TemplateNode.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class ConeInstance;

class URHO3D_API Cone : public TemplateNode<ConeInstance, Vector3, Vector3>
{
    URHO3D_OBJECT(Cone, ParticleGraphNode)
public:
    /// Construct Cone.
    explicit Cone(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set dampen.
    /// @property
    void SetRadius(float value);
    /// Get dampen.
    /// @property
    float GetRadius() const;

    /// Set dampen.
    /// @property
    void SetRadiusThickness(float value);
    /// Get dampen.
    /// @property
    float GetRadiusThickness() const;

    /// Set dampen.
    /// @property
    void SetAngle(float value);
    /// Get dampen.
    /// @property
    float GetAngle() const;

    /// Set dampen.
    /// @property
    void SetLength(float value);
    /// Get dampen.
    /// @property
    float GetLength() const;

    /// Set dampen.
    /// @property
    void SetPosition(Vector3 value);
    /// Get dampen.
    /// @property
    Vector3 GetPosition() const;

    /// Set dampen.
    /// @property
    void SetRotation(Quaternion value);
    /// Get dampen.
    /// @property
    Quaternion GetRotation() const;

    /// Set dampen.
    /// @property
    void SetScale(Vector3 value);
    /// Get dampen.
    /// @property
    Vector3 GetScale() const;

    /// Set dampen.
    /// @property
    void SetFrom(int value);
    /// Get dampen.
    /// @property
    int GetFrom() const;

protected:
    float radius_{};
    float radiusThickness_{};
    float angle_{};
    float length_{};
    Vector3 position_{};
    Quaternion rotation_{};
    Vector3 scale_{};
    int from_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
