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

#include "ParticleGraphLayer.h"
#include "ParticleGraphNodeInstance.h"

namespace Urho3D
{

/// Instance of particle graph layer in emitter.
class ParticleGraphLayerInstance
{
public:
    /// Construct.
    ParticleGraphLayerInstance();

    /// Destruct.
    ~ParticleGraphLayerInstance();

    /// Apply layer settings to the layer instance
    void Apply(const SharedPtr<ParticleGraphLayer>& layer);

    /// Return whether has active particles.
    bool CheckActiveParticles() const;

    /// Create a new particle. Return true if there was room.
    bool EmitNewParticle(unsigned numParticles = 1);

    /// Run update step.
    void Update(float timeStep);

    /// Get attribute memory by attribute index.
    ea::span<uint8_t> GetAttributeMemory(unsigned attributeIndex);

protected:
    /// Initialize update context.
    UpdateContext MakeUpdateContext(float timeStep);

    /// Run graph.
    void RunGraph(ea::span<ParticleGraphNodeInstance*>& nodes, UpdateContext& updateContext);

private:
    /// Memory used to store all layer related arrays: nodes, indices, attributes.
    ea::vector<uint8_t> attributes_;
    /// Temp memory needed for graph calculation.
    /// TODO: Should be replaced with memory pool as it could be shared between multiple emitter instancies.
    ea::vector<uint8_t> temp_;
    /// Node instances for emit graph
    ea::span<ParticleGraphNodeInstance*> emitNodeInstances_;
    /// Node instances for update graph
    ea::span<ParticleGraphNodeInstance*> updateNodeInstances_;
    /// All indices of the particle system.
    ea::span<unsigned> indices_;
    /// Number of active particles.
    unsigned activeParticles_;
    /// Reference to layer.
    SharedPtr<ParticleGraphLayer> layer_;
};

} // namespace Urho3D
