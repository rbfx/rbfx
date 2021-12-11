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
#include "ParticleGraph.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphPin.h"
#include "Urho3D/Resource/Graph.h"

#include <Urho3D/IO/Log.h>

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
        , capacity_(capacity)
        , tempBufferLayout_(tempSize)
    {
    }

    bool Build(ParticleGraph& graph)
    {
        const unsigned graphNodes = graph.GetNumNodes();

        for (unsigned i = 0; i < graphNodes; ++i)
        {
            // Configure pin nodes
            auto node = graph.GetNode(i);
            for (unsigned pinIndex = 0; pinIndex < node->NumPins(); ++pinIndex)
            {
                auto& pin = node->GetPin(pinIndex);

                // Evaluate output pin value for output pins.
                pin.valueType_ = pin.requestedValueType_;
                if (pin.valueType_ == VAR_NONE && !pin.GetIsInput())
                {
                    pin.valueType_ = node->EvaluateOutputPinType(pin);
                    if (pin.valueType_ == VAR_NONE)
                    {
                        URHO3D_LOGERROR("Can't detect output pin type");
                        return false;
                    }
                }
                // Allocate attribute buffer if this is a new attribute
                if (pin.containerType_ == PGCONTAINER_SPARSE)
                {
                    pin.attributeIndex_ = attributes_.GetOrAddAttribute(pin.name_, pin.requestedValueType_);

                    // For output sparse pin the memory is the same as the attribute memory
                    if (!pin.GetIsInput())
                    {
                        pin.memory_ = ParticleGraphPinRef(PGCONTAINER_SPARSE, pin.attributeIndex_);
                    }
                }
                // Allocate temp buffer for an output pin
                else if (!pin.GetIsInput())
                {
                    // Allocate temp buffer
                    pin.memory_ = ParticleGraphPinRef(pin.containerType_, tempBufferLayout_.Allocate(pin.containerType_, pin.requestedValueType_));
                }
                // Connect input pin
                if (pin.GetIsInput())
                {
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
                    if (pin.sourcePin_ >= sourceNode->NumPins())
                    {
                        URHO3D_LOGERROR("Reference to a missing pin");
                        return false;
                    }
                    const auto& sourcePin = sourceNode->GetPin(pin.sourcePin_);
                    if (sourcePin.GetIsInput())
                    {
                        URHO3D_LOGERROR("Source pin isn't output pin");
                        return false;
                    }
                    pin.memory_ = sourcePin.memory_;

                    if (pin.requestedValueType_ == VAR_NONE)
                    {
                        pin.valueType_ = sourcePin.valueType_;
                    }
                    else if (pin.requestedValueType_ != sourcePin.valueType_)
                    {
                        URHO3D_LOGERROR("Source pin type doesn't match input pin type");
                        return false;
                    }
                }
            }
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
    context->RegisterFactory<ParticleGraphLayer>();
}
/// Construct ParticleGraphLayer.
ParticleGraphLayer::ParticleGraphLayer(Context* context)
    : Object(context)
    , capacity_(16)
    , emit_(context)
    , update_(context)
{
    Invalidate();
}

/// Destruct ParticleGraphLayer.
ParticleGraphLayer::~ParticleGraphLayer() = default;

/// Get emit graph.
ParticleGraph& ParticleGraphLayer::GetEmitGraph() { return emit_; }

/// Get update graph.
ParticleGraph& ParticleGraphLayer::GetUpdateGraph() { return update_; }

void ParticleGraphLayer::Invalidate()
{
    committed_.reset();
    memset(&attributeBufferLayout_, 0, sizeof(AttributeBufferLayout));
    tempMemory_.Reset(0);
    attributes_.Reset(0, 0);
}

void ParticleGraphLayer::AttributeBufferLayout::Apply(ParticleGraphLayer& layer)
{
    const auto emitGraphNodes = layer.emit_.GetNumNodes();
    const auto updateGraphNodes = layer.update_.GetNumNodes();
    attributeBufferSize_ = 0;
    emitNodePointers_ = Append<ParticleGraphNodeInstance*>(this, emitGraphNodes);
    updateNodePointers_ = Append<ParticleGraphNodeInstance*>(this, updateGraphNodes);
    unsigned instanceSize = 0;
    for (unsigned i = 0; i < emitGraphNodes; ++i)
    {
        auto node = layer.emit_.GetNode(i);
        instanceSize += node->EvaluateInstanceSize();
    }
    for (unsigned i = 0; i < updateGraphNodes; ++i)
    {
        auto node = layer.update_.GetNode(i);
        instanceSize += node->EvaluateInstanceSize();
    }
    nodeInstances_ = Append(this, instanceSize);
    indices_ = Append<unsigned>(this, layer.capacity_);
    values_ = Append(this, 0);
}

bool ParticleGraphLayer::Commit()
{
    if (committed_.has_value())
        return committed_.value();
    committed_ = false;
    // Evaluate attribute buffer layout except attributes size.
    attributeBufferLayout_.Apply(*this);

    attributes_.Reset(attributeBufferLayout_.attributeBufferSize_, capacity_);
    tempMemory_.Reset(capacity_);
    // Allocate memory for each pin.
    {
        ParticleGraphAttributeBuilder builder(attributes_, &attributeBufferLayout_, capacity_, tempMemory_);
        if (!builder.Build(emit_))
            return false;
    }
    {
        ParticleGraphAttributeBuilder builder(attributes_, &attributeBufferLayout_, capacity_, tempMemory_);
        if (!builder.Build(update_))
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

/// Serialize from/to archive. Return true if successful.
bool ParticleGraphLayer::Serialize(Archive& archive)
{
    if (!SerializeValue(archive, "capacity", capacity_))
        return false;
    if (!emit_.Serialize(archive, "emit"))
        return false;
    if (!update_.Serialize(archive, "update"))
        return false;
    if (archive.IsInput())
    {
        if (!Commit())
            return false;
    }
    return true;
}
} // namespace Urho3D
