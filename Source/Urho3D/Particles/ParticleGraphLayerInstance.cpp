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

#include "../Precompiled.h"

#include "ParticleGraphLayerInstance.h"

#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"
#include "Span.h"
#include "UpdateContext.h"

namespace Urho3D
{

ParticleGraphLayerInstance::ParticleGraphLayerInstance()
    : activeParticles_(0)
    , destructionQueueSize_(0)
{
}

ParticleGraphLayerInstance::~ParticleGraphLayerInstance()
{
    for (unsigned i = 0; i < emitNodeInstances_.size(); ++i)
    {
        emitNodeInstances_[i]->~ParticleGraphNodeInstance();
    }
    for (unsigned i = 0; i < initNodeInstances_.size(); ++i)
    {
        initNodeInstances_[i]->~ParticleGraphNodeInstance();
    }
    for (unsigned i = 0; i < updateNodeInstances_.size(); ++i)
    {
        updateNodeInstances_[i]->~ParticleGraphNodeInstance();
    }
}

void ParticleGraphLayerInstance::Apply(const SharedPtr<ParticleGraphLayer>& layer)
{
    if (!layer)
        return;
    if (!layer->Commit())
        return;
    layer_ = layer;
    const auto& layout = layer_->GetAttributeBufferLayout();
    if (layout.attributeBufferSize_ > 0)
        attributes_.resize(layout.attributeBufferSize_);
    if (layer_->GetTempBufferSize() > 0)
        temp_.resize(layer_->GetTempBufferSize());

    auto nodeInstances = layout.nodeInstances_.MakeSpan<uint8_t>(attributes_);
    // Initialize indices
    emitNodeInstances_ = layout.emitNodePointers_.MakeSpan<ParticleGraphNodeInstance*>(attributes_);
    nodeInstances = InitNodeInstances(nodeInstances, emitNodeInstances_, layer_->GetEmitGraph());
    initNodeInstances_ = layout.initNodePointers_.MakeSpan<ParticleGraphNodeInstance*>(attributes_);
    nodeInstances = InitNodeInstances(nodeInstances, initNodeInstances_, layer_->GetInitGraph());
    updateNodeInstances_ = layout.updateNodePointers_.MakeSpan<ParticleGraphNodeInstance*>(attributes_);
    nodeInstances = InitNodeInstances(nodeInstances, updateNodeInstances_, layer_->GetUpdateGraph());

    // Initialize indices
    indices_ = layout.indices_.MakeSpan<unsigned>(attributes_);
    for (unsigned i = 0; i < indices_.size(); ++i)
    {
        indices_[i] = i;
    }
    destructionQueue_ = layout.destructionQueue_.MakeSpan<unsigned>(attributes_);
    Reset();
}

/// Remove all current particles.
void ParticleGraphLayerInstance::RemoveAllParticles() { activeParticles_ = 0; }

bool ParticleGraphLayerInstance::EmitNewParticles(float numParticles)
{
    emitCounterReminder_ += numParticles;

    if (emitCounterReminder_ < 1.0f)
        return true;

    unsigned particlesToEmit = static_cast<unsigned>(emitCounterReminder_);
    emitCounterReminder_ -= static_cast<float>(particlesToEmit);

    particlesToEmit = Urho3D::Min(particlesToEmit, indices_.size() - activeParticles_);
    if (!particlesToEmit)
        return false;

    const auto startIndex = activeParticles_;
    activeParticles_ += particlesToEmit;

    auto autoContext = MakeUpdateContext(0.0f);
    autoContext.indices_ = autoContext.indices_.subspan(startIndex, particlesToEmit);
    RunGraph(initNodeInstances_, autoContext);

    return true;
}

void ParticleGraphLayerInstance::Update(float timeStep, bool emitting)
{
    timeStep *= layer_->GetTimeScale();
    auto emitContext = MakeUpdateContext(timeStep);
    if (indices_.empty())
        return;
    if (emitting)
    {
        emitContext.indices_ = indices_.subspan(0, 1);
        RunGraph(emitNodeInstances_, emitContext);
    }

    auto updateContext = MakeUpdateContext(timeStep);
    RunGraph(updateNodeInstances_, updateContext);
    DestroyParticles();
    time_ += timeStep;
}

unsigned ParticleGraphLayerInstance::GetNumAttributes() const
{
    return layer_->GetAttributeLayout().GetNumAttributes();
}

void ParticleGraphLayerInstance::MarkForDeletion(unsigned particleIndex)
{
    if (particleIndex >= activeParticles_)
        return;

    //TODO: if buffer is full sort and eliminate duplicates.
    if (destructionQueueSize_ < destructionQueue_.size())
    {
        destructionQueue_[destructionQueueSize_] = particleIndex;
        ++destructionQueueSize_;
    }
}

/// Get uniform index. Creates new uniform slot on demand.
unsigned ParticleGraphLayerInstance::GetUniformIndex(const StringHash& string_hash, VariantType variant)
{
    // TODO: Make a collection of uniforms.
    return 0;
}

/// Get uniform variant by index.
Variant& ParticleGraphLayerInstance::GetUniform(unsigned index)
{
    //TODO: Make a collection of uniforms.
    thread_local Variant v;
    return v;
}

void ParticleGraphLayerInstance::Reset()
{
    for (ParticleGraphNodeInstance* node : emitNodeInstances_)
        node->Reset();
    for (ParticleGraphNodeInstance* node : initNodeInstances_)
        node->Reset();
    for (ParticleGraphNodeInstance* node : updateNodeInstances_)
        node->Reset();
    activeParticles_ = 0;
    time_ = 0.0f;
}

void ParticleGraphLayerInstance::SetEmitter(ParticleGraphEmitter* emitter)
{
    emitter_ = emitter;
}

/// Handle scene change in instance.
void ParticleGraphLayerInstance::OnSceneSet(Scene* scene)
{
    for (ParticleGraphNodeInstance* node : initNodeInstances_)
    {
        node->OnSceneSet(scene);
    }
    for (ParticleGraphNodeInstance* node : emitNodeInstances_)
    {
        node->OnSceneSet(scene);
    }
    for (ParticleGraphNodeInstance* node : updateNodeInstances_)
    {
        node->OnSceneSet(scene);
    }
}


UpdateContext ParticleGraphLayerInstance::MakeUpdateContext(float timeStep)
{
    UpdateContext context;
    if (activeParticles_ > 0)
        context.indices_ = indices_.subspan(0, activeParticles_);
    context.attributes_ = attributes_;
    context.tempBuffer_ = temp_;
    context.timeStep_ = timeStep;
    context.time_ = time_;
    context.layer_ = this;
    return context;
}

void ParticleGraphLayerInstance::RunGraph(ea::span<ParticleGraphNodeInstance*>& nodes, UpdateContext& updateContext)
{
    for (ParticleGraphNodeInstance* node : nodes)
    {
        node->Update(updateContext);
    }
}

ea::span<uint8_t> ParticleGraphLayerInstance::InitNodeInstances(ea::span<uint8_t> nodeInstanceBuffer,
    ea::span<ParticleGraphNodeInstance*>& nodeInstances, const ParticleGraph& graph)
{
    unsigned instanceOffset = 0;
    for (unsigned i = 0; i < graph.GetNumNodes(); ++i)
    {
        const auto node = graph.GetNode(i);
        const auto size = node->EvaluateInstanceSize();
        assert(instanceOffset + size <= nodeInstanceBuffer.size());
        uint8_t* ptr = nodeInstanceBuffer.begin() + instanceOffset;
        nodeInstances[i] = node->CreateInstanceAt(ptr, this);
        assert(nodeInstances[i] == reinterpret_cast<ParticleGraphNodeInstance*>(ptr));
        nodeInstances[i]->Reset();
        instanceOffset += size;
    }
    if (instanceOffset == nodeInstanceBuffer.size())
        return {};
    return nodeInstanceBuffer.subspan(instanceOffset);
}

#define InstantiateSpan(Span, FuncName)                                                                                \
    template Span<int> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                        \
    template Span<long long> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                  \
    template Span<bool> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                       \
    template Span<float> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                      \
    template Span<double> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                     \
    template Span<Vector2> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<Vector3> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<Vector4> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<Quaternion> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                 \
    template Span<Color> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                      \
    template Span<ea::string> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                 \
    template Span<VariantBuffer> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                              \
    template Span<ResourceRef> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                \
    template Span<ResourceRefList> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                            \
    template Span<VariantVector> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                              \
    template Span<StringVector> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                               \
    template Span<VariantMap> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                 \
    template Span<Rect> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                       \
    template Span<IntRect> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<IntVector2> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                 \
    template Span<IntVector3> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                 \
    template Span<Matrix3> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<Matrix3x4> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                  \
    template Span<Matrix4> UpdateContext::FuncName(const ParticleGraphPinRef& pin);                                    \
    template Span<const VariantCurve*> UpdateContext::FuncName(const ParticleGraphPinRef& pin);

InstantiateSpan(ea::span, GetSpan);
InstantiateSpan(ScalarSpan, GetScalar);
InstantiateSpan(SparseSpan, GetSparse);

} // namespace Urho3D
