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

#include "Helpers.h"
#include "ParticleGraphLayerInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Destroy marked particles.
class URHO3D_API Destroy : public AbstractNode<Destroy, bool>
{
    URHO3D_OBJECT(Destroy, ParticleGraphNode)
public:
    /// Construct.
    explicit Destroy(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(Destroy* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }

        template <typename T> void operator()(UpdateContext& context, unsigned numParticles, T pin0)
        {
            // Iterate all particles even if all pins are scalar.
            for (unsigned i = 0; i < context.indices_.size(); ++i)
            {
                if (pin0[i])
                {
                    context.layer_->MarkForDeletion(i);
                }
            }
        }
    };
};

/// Destroy expired particles.
class URHO3D_API Expire : public AbstractNode<Expire, float, float>
{
    URHO3D_OBJECT(Expire, ParticleGraphNode)
public:
    /// Construct.
    explicit Expire(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    class Instance final : public AbstractNodeType::Instance
    {
    public:
        Instance(Expire* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }

        template <typename Time, typename Lifetime>
        void operator()(UpdateContext& context, unsigned numParticles, Time time, Lifetime lifetime)
        {
            // Iterate all particles even if all pins are scalar.
            for (unsigned i = 0; i < context.indices_.size(); ++i)
            {
                if (time[i] >= lifetime[i])
                {
                    context.layer_->MarkForDeletion(i);
                }
            }
        }
    };
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
