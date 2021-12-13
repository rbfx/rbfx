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

#include "ParticleGraph.h"
#include "ParticleGraphPin.h"

#include <EASTL/optional.h>
#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{
struct ParticleGraphLayerBurst
{
    static constexpr unsigned InfiniteCycles = UINT_MAX;

    /// Delay before the burst.
    float delayInSeconds_{0.0f};
    /// How many particles to emit.
    unsigned count_{1};
    /// How many cycles to repeat.
    unsigned cycles_{InfiniteCycles};
    /// Delay between burst cycles.
    float cycleIntervalInSeconds_ {0.01f};
    /// Chance for the burst to happen.
    float probability_ {1.0f};
};

bool SerializeValue(Archive& archive, const char* name, ParticleGraphLayerBurst& burst);

class URHO3D_API ParticleGraphLayer : public Object
{
    URHO3D_OBJECT(ParticleGraphLayer, Object)
public:
    /// Layout of attribute buffer
    struct AttributeBufferLayout
    {
        /// Required attribute buffer size.
        unsigned attributeBufferSize_;
        /// Emit node pointers.
        ParticleGraphSpan emitNodePointers_;
        /// Update node pointers.
        ParticleGraphSpan updateNodePointers_;
        /// Node instances.
        ParticleGraphSpan nodeInstances_;
        /// Indices.
        ParticleGraphSpan indices_;
        /// Indices to destroy.
        ParticleGraphSpan destructionQueue_;
        /// Particle attribute values.
        ParticleGraphSpan values_;

        /// Evaluate attribute buffer size and layout.
        void EvaluateLayout(ParticleGraphLayer& layer);
    };

    /// Construct.
    explicit ParticleGraphLayer(Context* context);
    /// Destruct.
    virtual ~ParticleGraphLayer();
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Get emit graph.
    ParticleGraph& GetEmitGraph();

    /// Get update graph.
    ParticleGraph& GetUpdateGraph();

    /// Get number of bursts.
    /// @property
    unsigned GetNumBursts() const { return bursts_.size(); }
    /// Set number of bursts.
    /// @property 
    void SetNumBursts(unsigned bursts) { bursts_.resize(bursts); }

    /// Get burst.
    const ParticleGraphLayerBurst& GetBurst(unsigned index) const { return  bursts_[index]; }
    /// Set burst.
    void SetBurst(unsigned index, const ParticleGraphLayerBurst& burst) { bursts_[index] = burst; }

    /// Invalidate graph layer state.
    /// Call this method when something is changed in the layer graphs and it requires new preparation.
    void Invalidate();

    /// Prepare layer for execution. Returns false if graph is invalid. See output logs for errors.
    bool Commit();

    /// Return attribute buffer layout.
    /// @property
    const AttributeBufferLayout& GetAttributeBufferLayout() const;

    /// Return attributes memory layout.
    /// @property
    const ParticleGraphAttributeLayout& GetAttributes() const { return attributes_; }

    /// Return intermediate memory layout.
    /// @property
    const ParticleGraphBufferLayout& GetIntermediateValues() const { return tempMemory_; }

    /// Return size of temp buffer in bytes.
    /// @property
    unsigned GetTempBufferSize() const;

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive);

private:
    /// is the effect committed.
    ea::optional<bool> committed_;
    /// Maximum number of particles.
    unsigned capacity_;
    /// Emission graph.
    ParticleGraph emit_;
    /// Update graph.
    ParticleGraph update_;
    /// Attribute buffer layout.
    AttributeBufferLayout attributeBufferLayout_;
    /// Attributes memory layout.
    ParticleGraphAttributeLayout attributes_;
    /// Intermediate memory layout.
    ParticleGraphBufferLayout tempMemory_;
    /// Collection of bursts.
    ea::vector<ParticleGraphLayerBurst> bursts_;
};

} // namespace Urho3D
