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

#include "../Core/Context.h"
#include "../Graphics/ParticleGraphEffect.h"

#include "Graphics.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Resource/XMLArchive.h"

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
                    if(sourcePin.GetIsInput())
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

/// Construct ParticleGraphSpan.
ParticleGraphSpan::ParticleGraphSpan()
    : offset_(0)
    , size_(0)
{
}
ParticleGraphNodePin::ParticleGraphNodePin()
    : isInput_(true)
{
    
}
ParticleGraphNodePin::ParticleGraphNodePin(
    bool isInput,
    const ea::string name,
    VariantType type,
    ParticleGraphContainerType container)
    : isInput_(isInput)
    , requestedValueType_(type)
    , containerType_(container)
{
    SetName(name);
}

bool ParticleGraphNodePin::Serialize(Archive& archive) const
{
    return true;
}

void ParticleGraphNodePin::SetName(const ea::string& name)
{
    name_ = name;
    nameHash_ = StringHash(name_);
}

void ParticleGraphNodePin::SetValueType(VariantType valueType)
{
    requestedValueType_ = valueType;
}

void ParticleGraphNodePin::SetIsInput(bool isInput)
{
    isInput_ = isInput;
}

bool SerializeValue(Archive& archive, const char* name, ParticleGraphNodePin& value)
{
    if (auto block = archive.OpenSequentialBlock(name))
    {
        return value.Serialize(archive);
    }
    return false;
}

/// Construct ParticleGraphLayer.
ParticleGraph::ParticleGraph(Context* context)
    : Object(context)
{
}

/// Destruct ParticleGraphLayer.
ParticleGraph::~ParticleGraph() = default;

/// Add node to the graph.
/// Returns node index;
unsigned ParticleGraph::Add(const SharedPtr<ParticleGraphNode> node)
{
    if (!node)
    {
        URHO3D_LOGERROR("Can't add empty node");
        return 0;
    }
    const auto index = static_cast<unsigned>(nodes_.size());
    nodes_.push_back(node);
    return index;
}

unsigned ParticleGraph::GetNumNodes() const
{
    return static_cast<unsigned>(nodes_.size());
}

SharedPtr<ParticleGraphNode> ParticleGraph::GetNode(unsigned index) const
{
    if (index >= nodes_.size())
    {
        URHO3D_LOGERROR("Node index out of bounds");
        return {};
    }
    return nodes_[index];
}

/// Serialize from/to archive. Return true if successful.
bool ParticleGraph::Serialize(Archive& archive, const char* blockName)
{
    return SerializeVectorAsObjects(archive, blockName, "node", nodes_);
}

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
ParticleGraph& ParticleGraphLayer::GetEmitGraph()
{
    return emit_;
}

/// Get update graph.
ParticleGraph& ParticleGraphLayer::GetUpdateGraph()
{
    return update_;
}

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
    for (unsigned i=0; i<emitGraphNodes; ++i)
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

unsigned ParticleGraphLayer::GetTempBufferSize() const {
    return tempBufferSize_;
}

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

ParticleGraphNode::ParticleGraphNode(Context* context)
    : Object(context)
{
}

void ParticleGraphNode::SetPinName(unsigned pinIndex, const ea::string& name)
{
    if (pinIndex >= NumPins())
        return;
    auto& pin = GetPin(pinIndex);
    pin.SetName(name);
}

void ParticleGraphNode::SetPinValueType(unsigned pinIndex, VariantType type)
{
    if (pinIndex >= NumPins())
        return;
    auto& pin = GetPin(pinIndex);
    pin.SetValueType(type);
}

ParticleGraphNode::~ParticleGraphNode() = default;

namespace 
{
    struct PinArrayAdapter
    {
        
    };
}

bool ParticleGraphNode::Serialize(Archive& archive) const
{
    ea::array<ParticleGraphNodePin,2> pins {};
    return SerializeArrayAsObjects<>(archive, "pins", "pin", pins);

    //dest.SetAttribute("type", GetTypeName());
    //for (unsigned i = 0; i < NumPins(); ++i)
    //{
    //    auto& pin = const_cast<ParticleGraphNode*>(this)->GetPin(i);
    //    XMLElement pinElem = dest.CreateChild("pin");
    //    pinElem.SetAttribute("name", pin.name_);
    //    //pinElem.SetAttribute("type", Variant::GetTypeNameList()[pin.valueType_]);
    //    //switch (pin.containerType_)
    //    //{
    //    //case ParticleGraphContainerType::Span:
    //    //    pinElem.SetAttribute("container", "span");
    //    //    break;
    //    //case ParticleGraphContainerType::Sparse:
    //    //    pinElem.SetAttribute("container", "sparse");
    //    //    break;
    //    //case ParticleGraphContainerType::Scalar:
    //    //    pinElem.SetAttribute("container", "scalar");
    //    //    break;
    //    //case ParticleGraphContainerType::Auto:
    //    //    break;
    //    //default: ;
    //    //}
    //    if (pin.GetIsInput())
    //    {
    //        pinElem.SetAttribute("sourceNode", ea::to_string(pin.sourceNode_));
    //        if (pin.sourcePin_ != 0)
    //        {
    //            pinElem.SetAttribute("sourcePin", ea::to_string(pin.sourcePin_));
    //        }
    //    }

    //}
    return true;
}

bool ParticleGraphNode::EvaluateOutputPinType(ParticleGraphNodePin& pin)
{
    return false;
}

ParticleGraphEffect::ParticleGraphEffect(Context* context)
    : Resource(context)
{
}

ParticleGraphEffect::~ParticleGraphEffect() = default;

void ParticleGraphEffect::RegisterObject(Context* context)
{
    context->RegisterFactory<ParticleGraphEffect>();
}

/// Set number of layers.
void ParticleGraphEffect::SetNumLayers(unsigned numLayers)
{
    while (numLayers < layers_.size())
    {
        layers_.pop_back();
    }
    layers_.reserve(numLayers);
    while (numLayers > layers_.size())
    {
        layers_.push_back(MakeShared<ParticleGraphLayer>(context_));
    }
}

/// Get number of layers.
unsigned ParticleGraphEffect::GetNumLayers() const
{
    return static_cast<unsigned>(layers_.size());
}

/// Get layer by index.
SharedPtr<ParticleGraphLayer> ParticleGraphEffect::GetLayer(unsigned layerIndex) const
{
    return layers_[layerIndex];
}



bool ParticleGraphEffect::BeginLoad(Deserializer& source)
{
    // In headless mode, do not actually load the material, just return success
    //auto* graphics = GetSubsystem<Graphics>();
    //if (!graphics)
    //    return true;

    ea::string extension = GetExtension(source.GetName());

    bool success = false;

    ResetToDefaults();

    loadXMLFile_ = context_->CreateObject<XMLFile>();
    if (!loadXMLFile_->Load(source))
        return false;

    return true;

    //// All loading failed
    //ResetToDefaults();
    //loadJSONFile_.Reset();
    //return false;
}

void ParticleGraphEffect::ResetToDefaults()
{
    // Needs to be a no-op when async loading, as this does a GetResource() which is not allowed from worker threads
    if (!Thread::IsMainThread())
        return;

    layers_.clear();
}

bool ParticleGraphEffect::EndLoad()
{
    // In headless mode, do not actually load the material, just return success
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return true;

    XMLInputArchive archive(loadXMLFile_);
    return Serialize(archive);
}

bool ParticleGraphEffect::Save(Serializer& dest) const
{
    SharedPtr<XMLFile> xml(context_->CreateObject<XMLFile>());
    XMLElement graphElem = xml->CreateRoot("particleGraph");
    XMLOutputArchive archive(xml);
    if (!const_cast<ParticleGraphEffect*>(this)->Serialize(archive))
        return false;
    return xml->Save(dest);
}

bool ParticleGraphEffect::Serialize(Archive& archive)
{
    return SerializeVectorAsObjects(archive, "particleGraphEffect", "layer", layers_);
}


}
