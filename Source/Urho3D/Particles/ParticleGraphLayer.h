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

#include "../Resource/Resource.h"
#include "../Scene/Serializable.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class URHO3D_API ParticleGraphLayer : public Serializable
{
    static constexpr float DefaultDuration = 1.0f;
    static constexpr float DefaultTimeScale = 1.0f;
    static constexpr unsigned DefaultCapacity = 16;

    URHO3D_OBJECT(ParticleGraphLayer, Serializable)

public:
    /// Layout of attribute buffer
    struct AttributeBufferLayout
    {
        /// Required attribute buffer size.
        unsigned attributeBufferSize_;
        /// Emit node pointers.
        ParticleGraphSpan emitNodePointers_;
        /// Init node pointers.
        ParticleGraphSpan initNodePointers_;
        /// Update node pointers.
        ParticleGraphSpan updateNodePointers_;
        /// Node instances.
        ParticleGraphSpan nodeInstances_;
        /// Indices.
        ParticleGraphSpan indices_;
        /// Indices.
        ParticleGraphSpan scalarIndices_;
        /// Indices.
        ParticleGraphSpan naturalIndices_;
        /// Indices to destroy.
        ParticleGraphSpan destructionQueue_;
        /// Particle attribute values.
        ParticleGraphSpan values_;

        /// Evaluate attribute buffer size and layout.
        void EvaluateLayout(const ParticleGraphLayer& layer);
    };

    /// Construct.
    explicit ParticleGraphLayer(Context* context);
    /// Destruct.
    virtual ~ParticleGraphLayer();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Get maximum number of particles the layer can hold.
    unsigned GetCapacity() const { return capacity_; }

    /// Set maximum number of particles the layer can hold.
    void SetCapacity(unsigned capacity);

    /// Get time step scale.
    float GetTimeScale() const { return timeScale_; }

    /// Set time step scale.
    void SetTimeScale(float timeScale);

    /// Is effect looped.
    bool IsLoop() const { return loop_; }

    /// Set effect loop flag.
    void SetLoop(bool loop);

    /// Get effect duration in seconds.
    float GetDuration() const { return duration_; }

    /// Set effect duration in seconds.
    void SetDuration(float duration);

    /// Get emit graph.
    ParticleGraph& GetEmitGraph();

    /// Get initialization graph.
    ParticleGraph& GetInitGraph();

    /// Get update graph.
    ParticleGraph& GetUpdateGraph();


    /// Invalidate graph layer state.
    /// Call this method when something is changed in the layer graphs and it requires new preparation.
    void Invalidate();

    /// Prepare layer for execution. Returns false if graph is invalid. See output logs for errors.
    bool Commit();

    /// Return attribute buffer layout.
    const AttributeBufferLayout& GetAttributeBufferLayout() const;

    /// Return attributes memory layout.
    const ParticleGraphAttributeLayout& GetAttributeLayout() const { return attributes_; }

    /// Return intermediate memory layout.
    const ParticleGraphBufferLayout& GetIntermediateValues() const { return tempMemory_; }

    /// Return size of temp buffer in bytes.
    unsigned GetTempBufferSize() const;

    /// Serialize from/to archive.
    void SerializeInBlock(Archive& archive) override;

private:
    /// Is the effect committed.
    ea::optional<bool> committed_;
    /// Maximum number of particles.
    unsigned capacity_{DefaultCapacity};
    /// Time step scale.
    float timeScale_{1.0f};
    /// Effect duration in seconds.
    float duration_{DefaultDuration};
    /// Loop effect.
    bool loop_{};
    /// Emission graph.
    SharedPtr<ParticleGraph> emit_;
    /// Initialization graph.
    SharedPtr<ParticleGraph> init_;
    /// Update graph.
    SharedPtr<ParticleGraph> update_;
    /// Attribute buffer layout.
    AttributeBufferLayout attributeBufferLayout_;
    /// Attributes memory layout.
    ParticleGraphAttributeLayout attributes_;
    /// Intermediate memory layout.
    ParticleGraphBufferLayout tempMemory_;
};

} // namespace Urho3D
