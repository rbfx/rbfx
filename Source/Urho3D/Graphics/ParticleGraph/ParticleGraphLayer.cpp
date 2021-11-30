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

#include "../../Precompiled.h"

#include "ParticleGraphLayer.h"
#include "ParticleGraph.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodePin.h"

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
    ParticleGraphAttributeBuilder(ea::map<StringHash, ParticleGraphSpan>& attributes,
                                  ParticleGraphLayer::AttributeBufferLayout* layout, unsigned capacity,
                                  unsigned& tempSize)
        : attributes_(attributes)
        , layout_(layout)
        , capacity_(capacity)
        , tempSize_(tempSize)
    {
    }

    unsigned GetSize(VariantType variant)
    {
        switch (variant)
        {
        case VAR_INT:
            return sizeof(int);
        case VAR_INT64:
            return sizeof(long long);
        case VAR_BOOL:
            return sizeof(bool);
        case VAR_FLOAT:
            return sizeof(float);
        case VAR_DOUBLE:
            return sizeof(double);
        case VAR_VECTOR2:
            return sizeof(Vector2);
        case VAR_VECTOR3:
            return sizeof(Vector3);
        case VAR_VECTOR4:
            return sizeof(Vector4);
        case VAR_QUATERNION:
            return sizeof(Quaternion);
        case VAR_COLOR:
            return sizeof(Color);
        case VAR_STRING:
            return sizeof(ea::string);
        case VAR_BUFFER:
            return sizeof(VariantBuffer);
        case VAR_RESOURCEREF:
            return sizeof(ResourceRef);
        case VAR_RESOURCEREFLIST:
            return sizeof(ResourceRefList);
        case VAR_VARIANTVECTOR:
            return sizeof(VariantVector);
        case VAR_STRINGVECTOR:
            return sizeof(StringVector);
        case VAR_VARIANTMAP:
            return sizeof(VariantMap);
        case VAR_RECT:
            return sizeof(Rect);
        case VAR_INTRECT:
            return sizeof(IntRect);
        case VAR_INTVECTOR2:
            return sizeof(IntVector2);
        case VAR_INTVECTOR3:
            return sizeof(IntVector3);
        case VAR_MATRIX3:
            return sizeof(Matrix3);
        case VAR_MATRIX3X4:
            return sizeof(Matrix3x4);
        case VAR_MATRIX4:
            return sizeof(Matrix4);
        }
        assert(!"Unsupported value type");
        return 0;
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
                    node->EvaluateOutputPinType(pin);
                    if (pin.valueType_ == VAR_NONE)
                    {
                        URHO3D_LOGERROR("Can't detect output pin type");
                        return false;
                    }
                }
                // Allocate attribute buffer if this is a new attribute
                if (pin.containerType_ == ParticleGraphContainerType::Sparse)
                {
                    const auto nameHash = StringHash(pin.name_);
                    auto& span = attributes_[nameHash];
                    if (!span.size_)
                    {
                        span = Append(layout_, GetSize(pin.requestedValueType_) * capacity_);
                    }
                    pin.outputSpan_ = span;
                }
                // Allocate temp buffer for an output pin
                else if (!pin.GetIsInput())
                {
                    // Allocate temp buffer
                    const auto elementSize = GetSize(pin.requestedValueType_);
                    pin.outputSpan_.offset_ = tempSize_;
                    pin.outputSpan_.size_ =
                        elementSize * ((pin.containerType_ == ParticleGraphContainerType::Scalar) ? 1 : capacity_);
                    pin.sourceSpan_ = pin.outputSpan_;
                    tempSize_ += pin.outputSpan_.size_;
                }
                // Connect input pin
                if (pin.GetIsInput())
                {
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
                    pin.sourceSpan_ = sourcePin.outputSpan_;
                    pin.sourceContainerType_ = sourcePin.GetContainerType();
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
    ea::map<StringHash, ParticleGraphSpan>& attributes_;
    ParticleGraphLayer::AttributeBufferLayout* layout_;
    unsigned capacity_;
    unsigned& tempSize_;
};

/// Construct ParticleGraphLayer.
ParticleGraphLayer::ParticleGraphLayer(Context* context)
    : Object(context)
    , capacity_(16)
    , emit_(context)
    , update_(context)
    , isPrepared_(false)
    , tempBufferSize_(0)
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
    isPrepared_ = false;
    memset(&attributeBufferLayout_, 0, sizeof(AttributeBufferLayout));
    tempBufferSize_ = 0;
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
        instanceSize += node->EvalueInstanceSize();
    }
    for (unsigned i = 0; i < updateGraphNodes; ++i)
    {
        auto node = layer.update_.GetNode(i);
        instanceSize += node->EvalueInstanceSize();
    }
    nodeInstances_ = Append(this, instanceSize);
    indices_ = Append<unsigned>(this, layer.capacity_);
    values_ = Append(this, 0);
}

bool ParticleGraphLayer::Prepare()
{
    Invalidate();
    // Evaluate attribute buffer layout except attributes size.
    attributeBufferLayout_.Apply(*this);

    // TODO: attribute key should be name + type!
    ea::map<StringHash, ParticleGraphSpan> attributes;
    // Allocate memory for each pin.
    {
        ParticleGraphAttributeBuilder builder(attributes, &attributeBufferLayout_, capacity_, tempBufferSize_);
        if (!builder.Build(emit_))
            return false;
    }
    {
        ParticleGraphAttributeBuilder builder(attributes, &attributeBufferLayout_, capacity_, tempBufferSize_);
        if (!builder.Build(update_))
            return false;
    }
    isPrepared_ = true;
    return true;
}

const ParticleGraphLayer::AttributeBufferLayout& ParticleGraphLayer::GetAttributeBufferLayout() const
{
    return attributeBufferLayout_;
}

unsigned ParticleGraphLayer::GetTempBufferSize() const { return tempBufferSize_; }

/// Serialize from/to archive. Return true if successful.
bool ParticleGraphLayer::Serialize(Archive& archive)
{
    if (!emit_.Serialize(archive, "emit"))
    {
        return false;
    }
    if (!update_.Serialize(archive, "update"))
    {
        return false;
    }
    return true;
}
} // namespace Urho3D
