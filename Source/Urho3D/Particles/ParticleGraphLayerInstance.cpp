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


namespace Urho3D
{
void ParticleGraphLayerInstance::BurstState::Reset(const ParticleGraphLayerBurst& burst)
{
    burst_ = burst;
    timeToBurst_ = burst_.delayInSeconds_;
}

unsigned ParticleGraphLayerInstance::BurstState::Update(float timestep)
{
    if (!burst_.cycles_)
        return 0;
    timeToBurst_ -= timestep;
    if (timeToBurst_ <= 0)
    {
        timeToBurst_ = burst_.cycleIntervalInSeconds_;
        if (burst_.cycles_ != ParticleGraphLayerBurst::InfiniteCycles)
        {
            --burst_.cycles_;
        }
        if (Random() <= burst_.probability_)
            return burst_.count_;
        return 0;
    }
    return 0;
}

ParticleGraphLayerInstance::ParticleGraphLayerInstance()
    : activeParticles_(0)
    , destuctionQueueSize_(0)
{
}

ParticleGraphLayerInstance::~ParticleGraphLayerInstance()
{
    for (unsigned i = 0; i < emitNodeInstances_.size(); ++i)
    {
        emitNodeInstances_[i]->~ParticleGraphNodeInstance();
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
    burstStates_.resize(layer_->GetNumBursts());
    const auto& layout = layer_->GetAttributeBufferLayout();
    if (layout.attributeBufferSize_ > 0)
        attributes_.resize(layout.attributeBufferSize_);
    if (layer_->GetTempBufferSize() > 0)
        temp_.resize(layer_->GetTempBufferSize());

    auto nodeInstances = layout.nodeInstances_.MakeSpan<uint8_t>(attributes_);
    // Initialize indices
    emitNodeInstances_ = layout.emitNodePointers_.MakeSpan<ParticleGraphNodeInstance*>(attributes_);
    auto& emit = layer_->GetEmitGraph();
    unsigned instanceOffset = 0;
    for (unsigned i = 0; i < emit.GetNumNodes(); ++i)
    {
        const auto node = emit.GetNode(i);
        const auto size = node->EvaluateInstanceSize();
        assert(instanceOffset + size <= nodeInstances.size());
        uint8_t* ptr = nodeInstances.begin() + instanceOffset;
        emitNodeInstances_[i] = node->CreateInstanceAt(ptr, this);
        assert(emitNodeInstances_[i] == (ParticleGraphNodeInstance*)ptr);
        instanceOffset += size;
    }
    updateNodeInstances_ = layout.updateNodePointers_.MakeSpan<ParticleGraphNodeInstance*>(attributes_);
    auto& update = layer_->GetUpdateGraph();
    for (unsigned i = 0; i < update.GetNumNodes(); ++i)
    {
        const auto node = update.GetNode(i);
        const auto size = node->EvaluateInstanceSize();
        assert(instanceOffset + size <= nodeInstances.size());
        uint8_t* ptr = nodeInstances.begin() + instanceOffset;
        updateNodeInstances_[i] = node->CreateInstanceAt(ptr, this);
        assert(updateNodeInstances_[i] == (ParticleGraphNodeInstance*)ptr);
        instanceOffset += size;
    }
    // Initialize indices
    indices_ = layout.indices_.MakeSpan<unsigned>(attributes_);
    for (unsigned i = 0; i < indices_.size(); ++i)
    {
        indices_[i] = i;
    }
    destuctionQueue_ = layout.destructionQueue_.MakeSpan<unsigned>(attributes_);
    Reset();
}

bool ParticleGraphLayerInstance::CheckActiveParticles() const { return activeParticles_ != 0; }

bool ParticleGraphLayerInstance::EmitNewParticle(unsigned numParticles)
{
    if (numParticles == 0)
        return true;

    if (activeParticles_ == indices_.size())
        return false;
    if (!numParticles)
        return true;

    const auto startIndex = activeParticles_;
    ++activeParticles_;

    auto autoContext = MakeUpdateContext(0.0f);
    autoContext.indices_ = autoContext.indices_.subspan(startIndex, numParticles);
    RunGraph(emitNodeInstances_, autoContext);

    return true;
}

void ParticleGraphLayerInstance::Update(float timeStep)
{
    unsigned emitCounter = 0;
    for (auto& burstState: burstStates_)
    {
        emitCounter += burstState.Update(timeStep);
    }
    EmitNewParticle(emitCounter);

    auto autoContext = MakeUpdateContext(timeStep);
    RunGraph(updateNodeInstances_, autoContext);
    DestroyParticles();
}

unsigned ParticleGraphLayerInstance::GetNumAttributes() const
{
    return layer_->GetAttributes().GetNumAttributes();
}

void ParticleGraphLayerInstance::MarkForDeletion(unsigned particleIndex)
{
    if (particleIndex >= activeParticles_)
        return;

    //TODO: if buffer is full sort and eliminate duplicates.
    if (destuctionQueueSize_ < destuctionQueue_.size())
    {
        destuctionQueue_[destuctionQueueSize_] = particleIndex;
        ++destuctionQueueSize_;
    }
}

Variant ParticleGraphLayerInstance::GetUniform(const StringHash& string_hash, VariantType variant)
{
    //TODO: Make a collection of uniforms.
    return {};
}

void ParticleGraphLayerInstance::Reset()
{
    for (unsigned i = 0; i < burstStates_.size(); ++i)
    {
        burstStates_[i].Reset(layer_->GetBurst(i));
    }
}

void ParticleGraphLayerInstance::SetEmitter(ParticleGraphEmitter* emitter)
{
    emitter_ = emitter;
}

UpdateContext ParticleGraphLayerInstance::MakeUpdateContext(float timeStep)
{
    UpdateContext context;
    if (activeParticles_ > 0)
        context.indices_ = indices_.subspan(0, activeParticles_);
    context.attributes_ = attributes_;
    context.tempBuffer_ = temp_;
    context.timeStep_ = timeStep;
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
