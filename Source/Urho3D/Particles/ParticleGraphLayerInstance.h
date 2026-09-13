// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphLayer.h"
#include "ParticleGraphNodeInstance.h"
#include <EASTL/sort.h>

namespace Urho3D
{

/// Instance of particle graph layer in emitter.
class URHO3D_API ParticleGraphLayerInstance
{
public:
    /// Construct.
    ParticleGraphLayerInstance();

    /// Destruct.
    ~ParticleGraphLayerInstance();

    /// Apply layer settings to the layer instance
    void Apply(const SharedPtr<ParticleGraphLayer>& layer);

    /// Return number of active particles.
    unsigned GetNumActiveParticles() const { return activeParticles_; }

    /// Remove all current particles.
    void RemoveAllParticles();

    /// Create a new particles. Return true if there was room.
    bool EmitNewParticles(float numParticles = 1.0f);

    /// Run update step.
    void Update(float timeStep, bool emitting);

    /// Get number of attributes.
    unsigned GetNumAttributes() const;

    /// Get attribute values.
    template <typename T>
    SparseSpan<T> GetAttributeValues(unsigned attributeIndex);

    template <typename ValueType>
    SparseSpan<ValueType> GetSparse(unsigned attributeIndex, const ea::span<unsigned>& indices);
    template <typename ValueType> SparseSpan<ValueType> GetScalar(unsigned pinIndex);
    template <typename ValueType> SparseSpan<ValueType> GetSpan(unsigned pinIndex);

    /// Get emitter.
    ParticleGraphEmitter* GetEmitter() const { return emitter_; }

    /// Mark a particle for deletion.
    void MarkForDeletion(unsigned particleIndex);

    /// Get uniform index. Creates new uniform slot on demand.
    unsigned GetUniformIndex(const StringHash& string_hash, VariantType variant);

    /// Get uniform variant by index.
    Variant& GetUniform(unsigned index);

    /// Reset the particle emitter layer completely. Removes current particles, sets emitting state on, and resets the
    /// emission timer.
    void Reset();

    /// Update all drawable attributes. Executed by ParticleGraphEmitter.
    void UpdateDrawables();

    /// Get effect layer.
    ParticleGraphLayer* GetLayer() const { return layer_; }

protected:
    /// Handle scene change in instance.
    void OnSceneSet(Scene* scene);

    /// Set emitter reference.
    void SetEmitter(ParticleGraphEmitter* emitter);

    /// Initialize update context.
    UpdateContext MakeUpdateContext(float timeStep);

    /// Run graph.
    void RunGraph(ea::span<ParticleGraphNodeInstance*>& nodes, UpdateContext& updateContext);

    /// Destroy particles.
    void DestroyParticles();

    ea::span<uint8_t> InitNodeInstances(ea::span<uint8_t> nodeInstanceBuffer,
        ea::span<ParticleGraphNodeInstance*>& nodeInstances, const ParticleGraph& particle_graph);

private:
    /// Emit counter reminder. When reminder value get over 1 the layer emits particle.
    float emitCounterReminder_{};
    /// Memory used to store all layer related arrays: nodes, indices, attributes.
    ea::vector<uint8_t> attributes_;
    /// Temp memory needed for graph calculation.
    /// TODO: Should be replaced with memory pool as it could be shared between multiple emitter instances.
    ea::vector<uint8_t> temp_;
    /// Node instances for emit graph
    ea::span<ParticleGraphNodeInstance*> emitNodeInstances_;
    /// Node instances for initialization graph
    ea::span<ParticleGraphNodeInstance*> initNodeInstances_;
    /// Node instances for update graph
    ea::span<ParticleGraphNodeInstance*> updateNodeInstances_;
    /// All indices of the particle system.
    ea::span<unsigned> indices_;
    /// All indices set to 0.
    ea::span<unsigned> scalarIndices_;
    /// All indices going in natural order (0, 1, ...).
    ea::span<unsigned> naturalIndices_;
    /// Particle indices to be removed.
    ea::span<unsigned> destructionQueue_;
    /// Number of particles to destroy at end of the frame.
    unsigned destructionQueueSize_;
    /// Number of active particles.
    unsigned activeParticles_;
    /// Reference to layer.
    SharedPtr<ParticleGraphLayer> layer_;
    /// Emitter that owns the layer instance.
    ParticleGraphEmitter* emitter_{};
    /// Time since emitter start.
    float time_{};

    friend class ParticleGraphEmitter;
};

inline void ParticleGraphLayerInstance::DestroyParticles()
{
    if (!destructionQueueSize_)
        return;
    auto queue = destructionQueue_.subspan(0, destructionQueueSize_);
    ea::sort(queue.begin(), queue.end(), ea::greater<unsigned>());
    //TODO: Eliminate duplicates.
    for (unsigned index: queue)
    {
        ea::swap(indices_[index], indices_[activeParticles_ - 1]);
        --activeParticles_;
    }
    destructionQueueSize_ = 0;
}

/// Get attribute values.
template <typename T> inline SparseSpan<T> ParticleGraphLayerInstance::GetAttributeValues(unsigned attributeIndex)
{
    if (activeParticles_ == 0)
        return {};
    return GetSparse<T>(attributeIndex, indices_.subspan(0, activeParticles_));
}

template <typename ValueType> SparseSpan<ValueType> ParticleGraphLayerInstance::GetSparse(unsigned attributeIndex, const ea::span<unsigned>& indices)
{
    const auto& attr = layer_->GetAttributeLayout().GetSpan(attributeIndex);
    const auto values = attr.MakeSpan<ValueType>(attributes_);
    return SparseSpan<ValueType>(values, indices);
}

template <typename ValueType> SparseSpan<ValueType> ParticleGraphLayerInstance::GetScalar(unsigned pinIndex)
{
    const auto& attr = layer_->GetIntermediateValues()[pinIndex];
    const auto values = attr.MakeSpan<ValueType>(temp_);
    return SparseSpan<ValueType>(values, scalarIndices_);
}

template <typename ValueType> SparseSpan<ValueType> ParticleGraphLayerInstance::GetSpan(unsigned pinIndex)
{
    const auto& attr = layer_->GetIntermediateValues()[pinIndex];
    const auto values = attr.MakeSpan<ValueType>(temp_);
    return SparseSpan<ValueType>(values, naturalIndices_);
}

} // namespace Urho3D
