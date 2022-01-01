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

#include "Emitter.h"
#include "Helpers.h"
#include "ParticleGraphNode.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Hemisphere : public AbstractNode<Hemisphere, Vector3, Vector3>
{
    URHO3D_OBJECT(Hemisphere, ParticleGraphNode)
public:
    /// Construct.
    explicit Hemisphere(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

public:
    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(Hemisphere* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }

        template <typename Pos, typename Vel>
        void operator()(UpdateContext& context, unsigned numParticles, Pos pos, Vel vel)
        {
            const Hemisphere* cone = GetGraphNodeInstace();
            const Matrix3x4 m = cone->GetShapeTransform();
            const Matrix3 md = m.RotationMatrix();

            for (unsigned i = 0; i < numParticles; ++i)
            {
                Vector3 p, v;
                cone->Generate(p, v);
                pos[i] = m * p;
                vel[i] = md * v;
            }
        }
    };

public:

    /// Get cone base radius.
    float GetRadius() const { return radius_; }
    /// Set cone base radius.
    void SetRadius(float val) { radius_ = val; }

    /// Get cone base radius thickness.
    float GetRadiusThickness() const { return radiusThickness_; }
    /// Set cone base radius thickness.
    void SetRadiusThickness(float val) { radiusThickness_ = val; }

    /// Get cone rotation.
    const Quaternion& GetRotation() const { return rotation_; }
    /// Set cone rotation.
    void SetRotation(const Quaternion& val) { rotation_ = val; }
    /// Get cone offset.
    const Vector3& GetPosition() const { return position_; }
    /// Set cone offset.
    void SetPosition(const Vector3& val) { position_ = val; }
    /// Get cone scale.
    const Vector3& GetScale() const { return scale_; }
    /// Set cone scale.
    void SetScale(const Vector3& val) { scale_ = val; }

    /// Generate value in the cone.
    void Generate(Vector3& pos, Vector3& vel) const;

    /// Get shape transform.
    Matrix3x4 GetShapeTransform() const;

protected:
    /// Cone base radius.
    float radius_{0.0f};
    /// Cone radius thickness.
    float radiusThickness_{1.0f};
    /// Cone orientation.
    Quaternion rotation_;
    /// Cone offset.
    Vector3 position_;
    /// Cone scale.
    Vector3 scale_;
    /// Emitting area.
    EmitFrom emitFrom_{EmitFrom::Volume};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
