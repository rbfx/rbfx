//
// Copyright (c) 2021 the rbfx project.
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
#include "Helpers.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Cone : public AbstractNode<Cone, Vector3>
{
    URHO3D_OBJECT(Cone, ParticleGraphNode)
public:
    /// Construct.
    explicit Cone(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

public:

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        const Cone * cone = instance->GetGraphNodeInstace();
        const Matrix3x4 m = cone->GetShapeTransform();
        auto& out = ea::get<0>(spans);

        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = m * cone->Generate();
        }
    }

public:
    /// Get cone base radius.
    float GetRadius() const { return radius_; }
    /// Set cone base radius.
    void SetRadius(float val) { radius_ = val; }
    /// Get cone angle in degrees.
    float GetAngle() const { return angle_; }
    /// Set cone angle in degrees.
    void SetAngle(float val) { angle_ = val; }
    /// Get cone rotation.
    const Quaternion& GetRotation() const { return rotation_; }
    /// Set cone rotation.
    void SetRotation(const Quaternion& val) { rotation_ = val; }
    /// Get cone offset.
    const Vector3& GetTranslation() const { return translation_; }
    /// Set cone offset.
    void SetTranslation(const Vector3& val) { translation_ = val; }

    /// Generate value in the cone.
    Vector3 Generate() const;

    /// Get shape transform.
    Matrix3x4 GetShapeTransform() const;

protected:
    /// Cone base radius.
    float radius_;
    /// Cone angle in degrees.
    float angle_;
    /// Cone orientation.
    Quaternion rotation_;
    /// Cone offset.
    Vector3 translation_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
