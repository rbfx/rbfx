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

#include "ParticleGraphLayer.h"

#include "../IO/Log.h"
#include "../Scene/Serializable.h"
#include "ParticleGraph.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphPin.h"

namespace Urho3D
{

namespace
{

ParticleGraphSpan Append(ParticleGraphLayer::AttributeBufferLayout* layout, unsigned bytes)
{
    ParticleGraphSpan span;
    span.offset_ = layout->attributeBufferSize_;
    span.size_ = bytes;
    layout->attributeBufferSize_ += bytes;
    return span;
}

template <typename T> ParticleGraphSpan Append(ParticleGraphLayer::AttributeBufferLayout* layout, unsigned count)
{
    return Append(layout, sizeof(T) * count);
}

} // namespace

struct ParticleGraphAttributeBuilder
{
    ParticleGraphAttributeBuilder(ParticleGraphAttributeLayout& attributes,
                                  ParticleGraphLayer::AttributeBufferLayout* layout, unsigned capacity,
                                  ParticleGraphBufferLayout& tempSize)
        : attributes_(attributes)
        , layout_(layout)
        , tempBufferLayout_(tempSize)
        , capacity_(capacity)
    {
    }

    bool EvaluateValueType(const SharedPtr<ParticleGraphNode>& node, ParticleGraphPin& pin) const
    {
        // Evaluate output pin value for output pins.
        pin.valueType_ = pin.requestedValueType_;
        if (pin.valueType_ == VAR_NONE && !pin.IsInput())
        {
            pin.valueType_ = node->EvaluateOutputPinType(pin);
            if (pin.valueType_ == VAR_NONE)
            {
                URHO3D_LOGERROR(Format("Can't detect output pin {}.{} type", node->GetTypeName(), pin.GetName()));
                return false;
            }
        }
        return true;
    }

    bool BuildNode(const ParticleGraph& graph, unsigned i) const
    {
        // Configure pin nodes.
        const auto node = graph.GetNode(i);

        ParticleGraphContainerType defaultOutputType = ParticleGraphContainerType::Scalar;

        // Connect input pins.
        for (unsigned pinIndex = 0; pinIndex < node->GetNumPins(); ++pinIndex)
        {
            auto& pin = node->GetPin(pinIndex);

            if (!EvaluateValueType(node, pin))
                return false;

            // Connect input pin.
            if (pin.IsInput())
            {
                if (pin.containerType_ == ParticleGraphContainerType::Sparse)
                {
                    URHO3D_LOGERROR(
                        Format("Sparse input pin {}.{} is not supported", node->GetTypeName(), pin.GetName()));
                    return false;
                }
                if (pin.sourceNode_ == ParticleGraph::INVALID_NODE_INDEX)
                {
                    URHO3D_LOGERROR(Format("Source node is not set for {}.{}", node->GetTypeName(), pin.GetName()));
                    return false;
                }
                if (pin.sourceNode_ >= i)
                {
                    URHO3D_LOGERROR("Graph can't forward reference nodes");
                    return false;
                }
                const auto sourceNode = graph.GetNode(pin.sourceNode_);
                if (pin.sourcePin_ >= sourceNode->GetNumPins())
                {
                    URHO3D_LOGERROR("Reference to a missing pin");
                    return false;
                }
                const auto& sourcePin = sourceNode->GetPin(pin.sourcePin_);
                if (sourcePin.IsInput())
                {
                    URHO3D_LOGERROR("Source pin isn't output pin");
                    return false;
                }
                pin.memory_ = sourcePin.memory_;
                // Detect default output type.
                if (pin.memory_.type_ != ParticleGraphContainerType::Scalar)
                {
                    defaultOutputType = ParticleGraphContainerType::Span;
                }
                // Evaluate input pin type.
                if (pin.requestedValueType_ == VAR_NONE)
                {
                    pin.valueType_ = sourcePin.valueType_;
                }
                else if (pin.requestedValueType_ != sourcePin.valueType_)
                {
                    URHO3D_LOGERROR("Source pin {}.{} type {} doesn't match input pin {}.{} type {}",
                                    sourceNode->GetTypeName(), sourcePin.GetName(), Variant::GetTypeNameList()[sourcePin.GetValueType()],
                                    node->GetTypeName(), pin.GetName(), Variant::GetTypeNameList()[pin.GetRequestedType()]);
                    return false;
                }
            }
        }
        // Allocate memory for output pins.
        for (unsigned pinIndex = 0; pinIndex < node->GetNumPins(); ++pinIndex)
        {
            auto& pin = node->GetPin(pinIndex);

            if (!pin.IsInput())
            {
                // Allocate attribute buffer if this is a new attribute
                if (pin.containerType_ == ParticleGraphContainerType::Sparse)
                {
                    pin.attributeIndex_ = attributes_.GetOrAddAttribute(pin.name_, pin.requestedValueType_);

                    // For output sparse pin the memory is the same as the attribute memory
                    if (!pin.IsInput())
                    {
                        pin.memory_ = ParticleGraphPinRef(ParticleGraphContainerType::Sparse, pin.attributeIndex_);
                    }
                }
                // Allocate temp buffer for an output pin
                else
                {
                    // Allocate temp buffer
                    auto containerType = pin.containerType_;
                    if (containerType == ParticleGraphContainerType::Auto)
                        containerType = defaultOutputType;
                    pin.memory_ = ParticleGraphPinRef(containerType, tempBufferLayout_.Allocate(containerType, pin.valueType_));
                }
            }
        }
        return true;
    }

    bool Build(const ParticleGraph& graph) const
    {
        const unsigned graphNodes = graph.GetNumNodes();

        for (unsigned i = 0; i < graphNodes; ++i)
        {
            if (!BuildNode(graph, i))
                return false;
        }
        return true;
    }
    ParticleGraphAttributeLayout& attributes_;
    ParticleGraphLayer::AttributeBufferLayout* layout_;
    ParticleGraphBufferLayout& tempBufferLayout_;
    unsigned capacity_;
};

void ParticleGraphLayer::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ParticleGraphLayer>();
    URHO3D_ACCESSOR_ATTRIBUTE("Capacity", GetCapacity, SetCapacity, unsigned, DefaultCapacity, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("TimeScale", GetTimeScale, SetTimeScale, float, DefaultTimeScale, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Duration", GetDuration, SetDuration, float, DefaultDuration, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Loop", IsLoop, SetLoop, bool, false, AM_DEFAULT);
}

/// Construct ParticleGraphLayer.
ParticleGraphLayer::ParticleGraphLayer(Context* context)
    : Serializable(context)
    , capacity_(DefaultCapacity)
    , timeScale_(1.0f)
    , duration_(DefaultDuration)
    , loop_(false)
    , emit_(MakeShared<ParticleGraph>(context))
    , init_(MakeShared<ParticleGraph>(context))
    , update_(MakeShared<ParticleGraph>(context))
{
    Invalidate();
}

ParticleGraphLayer::~ParticleGraphLayer() = default;

void ParticleGraphLayer::SetCapacity(unsigned capacity)
{
    capacity_ = capacity;
    Invalidate();
}

void ParticleGraphLayer::SetTimeScale(float timeScale)
{
    timeScale_ = timeScale;
    Invalidate();
}

void ParticleGraphLayer::SetLoop(bool isLoop)
{
    loop_ = isLoop;
    Invalidate();
}

void ParticleGraphLayer::SetDuration(float duration)
{
    duration_ = ea::max(1e-6f, duration);
}

ParticleGraph& ParticleGraphLayer::GetEmitGraph() { return *emit_; }

ParticleGraph& ParticleGraphLayer::GetInitGraph() { return *init_; }

ParticleGraph& ParticleGraphLayer::GetUpdateGraph() { return *update_; }

void ParticleGraphLayer::Invalidate()
{
    committed_.reset();
    memset(&attributeBufferLayout_, 0, sizeof(AttributeBufferLayout));
    tempMemory_.Reset(0);
    attributes_.Reset(0, 0);
}

void ParticleGraphLayer::AttributeBufferLayout::EvaluateLayout(const ParticleGraphLayer& layer)
{
    const auto emitGraphNodes = layer.emit_->GetNumNodes();
    const auto initGraphNodes = layer.init_->GetNumNodes();
    const auto updateGraphNodes = layer.update_->GetNumNodes();
    attributeBufferSize_ = 0;
    emitNodePointers_ = Append<ParticleGraphNodeInstance*>(this, emitGraphNodes);
    initNodePointers_ = Append<ParticleGraphNodeInstance*>(this, initGraphNodes);
    updateNodePointers_ = Append<ParticleGraphNodeInstance*>(this, updateGraphNodes);
    unsigned instanceSize = 0;
    for (unsigned i = 0; i < emitGraphNodes; ++i)
    {
        const auto node = layer.emit_->GetNode(i);
        instanceSize += node->EvaluateInstanceSize();
    }
    for (unsigned i = 0; i < initGraphNodes; ++i)
    {
        const auto node = layer.init_->GetNode(i);
        instanceSize += node->EvaluateInstanceSize();
    }
    for (unsigned i = 0; i < updateGraphNodes; ++i)
    {
        const auto node = layer.update_->GetNode(i);
        instanceSize += node->EvaluateInstanceSize();
    }
    nodeInstances_ = Append(this, instanceSize);
    indices_ = Append<unsigned>(this, layer.capacity_);
    scalarIndices_ = Append<unsigned>(this, layer.capacity_);
    naturalIndices_ = Append<unsigned>(this, layer.capacity_);
    destructionQueue_ = Append<unsigned>(this, layer.capacity_);
    values_ = Append(this, 0);
}

bool ParticleGraphLayer::Commit()
{
    if (committed_.has_value())
        return committed_.value();
    committed_ = false;
    // Evaluate attribute buffer layout except attributes size.
    attributeBufferLayout_.EvaluateLayout(*this);

    attributes_.Reset(attributeBufferLayout_.attributeBufferSize_, capacity_);
    tempMemory_.Reset(capacity_);
    // Allocate memory for each pin.
    {
        ParticleGraphAttributeBuilder builder(attributes_, &attributeBufferLayout_, capacity_, tempMemory_);
        if (!builder.Build(*init_))
            return false;
    }
    {
        ParticleGraphAttributeBuilder builder(attributes_, &attributeBufferLayout_, capacity_, tempMemory_);
        if (!builder.Build(*emit_))
            return false;
    }
    {
        ParticleGraphAttributeBuilder builder(attributes_, &attributeBufferLayout_, capacity_, tempMemory_);
        if (!builder.Build(*update_))
            return false;
    }
    attributeBufferLayout_.attributeBufferSize_ = attributes_.GetRequiredMemory();
    ParticleGraphSpan& values = attributeBufferLayout_.values_;
    values = ParticleGraphSpan(values.offset_, attributeBufferLayout_.attributeBufferSize_ - values.offset_);
    committed_ = true;
    return committed_.value();
}

const ParticleGraphLayer::AttributeBufferLayout& ParticleGraphLayer::GetAttributeBufferLayout() const
{
    return attributeBufferLayout_;
}

unsigned ParticleGraphLayer::GetTempBufferSize() const { return tempMemory_.GetRequiredMemory(); }

void ParticleGraphLayer::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "capacity", capacity_, DefaultCapacity);
    SerializeOptionalValue(archive, "duration", duration_, DefaultDuration);
    SerializeOptionalValue(archive, "timeScale", timeScale_, DefaultTimeScale);
    SerializeOptionalValue(archive, "loop", loop_);

    SerializeOptionalValue(archive, "emit", emit_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeValue(archive, name, *value); });
    SerializeOptionalValue(archive, "init", init_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeValue(archive, name, *value); });
    SerializeOptionalValue(archive, "update", update_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeValue(archive, name, *value); });

    if (archive.IsInput())
    {
        Commit();
    }
}
} // namespace Urho3D
