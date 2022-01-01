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
#include "ParticleGraphNodeInstance.h"
#include "Helpers.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Slerp operator.
class URHO3D_API Slerp : public AbstractNode<Slerp, Quaternion, Quaternion, float, Quaternion>
{
    URHO3D_OBJECT(Slerp, ParticleGraphNode);

public:
    class Instance : public AbstractNodeType::Instance
    {
    public:
        /// Construct instance.
        Instance(Slerp* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }
        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& t = ea::get<2>(spans);
            auto& out = ea::get<3>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = x[i].Slerp(y[i], t[i]);
            }
        }
    };


public:
    /// Construct.
    explicit Slerp(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

/// Make Vector2.
class URHO3D_API MakeVec2 : public AbstractNode<MakeVec2, float, float, Vector2>
{
    URHO3D_OBJECT(MakeVec2, ParticleGraphNode);

public:
    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(MakeVec2* node, ParticleGraphLayerInstance* layer)
            : AbstractNode<MakeVec2, float, float, Vector2>::Instance(node, layer)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = {x[i], y[i]};
            }
        }
    };

public:
    /// Construct.
    explicit MakeVec2(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

/// Make Vector3.
class URHO3D_API MakeVec3 : public AbstractNode<MakeVec3, float, float, float, Vector3>
{
    URHO3D_OBJECT(MakeVec3, ParticleGraphNode);

public:
    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(MakeVec3* node, ParticleGraphLayerInstance* layer)
            : AbstractNode<MakeVec3, float, float, float, Vector3>::Instance(node, layer)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& z = ea::get<2>(spans);
            auto& out = ea::get<3>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = {x[i], y[i], z[i]};
            }
        }
    };

public:
    /// Construct.
    explicit MakeVec3(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};


/// Make Matrix3x4.
class URHO3D_API MakeMatrix3x4 : public AbstractNode<MakeMatrix3x4, Vector3, Quaternion, Vector3, Matrix3x4>
{
    URHO3D_OBJECT(MakeMatrix3x4, ParticleGraphNode);

public:
    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(MakeMatrix3x4* node, ParticleGraphLayerInstance* layer)
            : AbstractNode<MakeMatrix3x4, Vector3, Quaternion, Vector3, Matrix3x4>::Instance(node, layer)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& z = ea::get<2>(spans);
            auto& out = ea::get<3>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = Matrix3x4(x[i], y[i], z[i]);
            }
        }
    };


public:
    /// Construct.
    explicit MakeMatrix3x4(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

}

} // namespace Urho3D
