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
#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/XMLFile.h"

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
    ParticleGraphAttributeBuilder(ea::map<StringHash, ParticleGraphSpan> attributes,
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

    void Build(ParticleGraph& graph)
    {
        const unsigned graphNodes = graph.GetNumNodes();

        for (unsigned i = 0; i < graphNodes; ++i)
        {
            auto node = graph.GetNode(i);
            for (unsigned pinIndex = 0; pinIndex < node->NumPins(); ++pinIndex)
            {
                auto pin = node->GetPin(pinIndex);

                // Allocate attribute buffer if this is a new attribute
                if (pin.containerType_ == ParticleGraphContainerType::Sparse)
                {
                    const auto nameHash = StringHash(pin.name_);
                    auto& span = attributes_[nameHash];
                    if (!span.size_)
                    {
                        span = Append(layout_, GetSize(pin.valueType_) * capacity_);
                    }
                    pin.outputSpan_ = span;
                }
                // Allocate temp buffer for an output pin
                else if (!pin.IsInput())
                {
                    // Allocate temp buffer
                    const auto elementSize = GetSize(pin.valueType_);
                    pin.outputSpan_.offset_ = tempSize_;
                    pin.outputSpan_.size_ =
                        elementSize * ((pin.containerType_ == ParticleGraphContainerType::Scalar) ? 1 : capacity_);
                    pin.sourceSpan_ = pin.outputSpan_;
                }
                // Connect input pin
                if (pin.IsInput())
                {
                    const auto sourceNode = graph.GetNode(pin.sourceNode_);
                    const auto& sourcePin = sourceNode->GetPin(pin.sourcePin_);
                    assert(!sourcePin.IsInput());
                    pin.sourceSpan_ = sourcePin.outputSpan_;
                    pin.sourceContainerType_ = sourcePin.sourceContainerType_;
                }
            }
        }
    }
    ea::map<StringHash, ParticleGraphSpan> attributes_;
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
ParticleGraphNodePin::ParticleGraphNodePin(bool isInput, const ea::string name)
    : isInput_(isInput)
    , name_(name)
{
}

/// Construct ParticleGraphLayer.
ParticleGraph::ParticleGraph() = default;

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


/// Load from an XML element. Return true if successful.
bool ParticleGraph::Load(const XMLElement& source)
{
    return true;
}

/// Save to an XML element. Return true if successful.
bool ParticleGraph::Save(XMLElement& dest) const
{
    for (unsigned i=0; i<nodes_.size(); ++i)
    {
        ParticleGraphNode* node = nodes_[i];
        XMLElement emitElem = dest.CreateChild("node");
        if (!node->Save(emitElem))
        {
            return false;
        }
    }

    return true;
}

/// Load from a JSON value. Return true if successful.
bool ParticleGraph::Load(const JSONValue& source)
{
    return true;
}

/// Save to a JSON value. Return true if successful.
bool ParticleGraph::Save(JSONValue& dest) const
{
    return true;
}

/// Construct ParticleGraphLayer.
ParticleGraphLayer::ParticleGraphLayer()
    : isPrepared_(false)
    , tempBufferSize_(0)
    , capacity_(16)
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

void ParticleGraphLayer::Prepare()
{
    Invalidate();
    isPrepared_ = true;
    // Evaluate attribute buffer layout except attributes size.
    attributeBufferLayout_.Apply(*this);

    // TODO: attribute key should be name + type!
    ea::map<StringHash, ParticleGraphSpan> attributes;
    // Allocate memory for each pin.
    {
        ParticleGraphAttributeBuilder builder(attributes, &attributeBufferLayout_, capacity_, tempBufferSize_);
        builder.Build(emit_);
    }
    {
        ParticleGraphAttributeBuilder builder(attributes, &attributeBufferLayout_, capacity_, tempBufferSize_);
        builder.Build(update_);
    }
}

const ParticleGraphLayer::AttributeBufferLayout& ParticleGraphLayer::GetAttributeBufferLayout() const
{
    return attributeBufferLayout_;
}

unsigned ParticleGraphLayer::GetTempBufferSize() const {
    return tempBufferSize_;
}

/// Load from an XML element. Return true if successful.
bool ParticleGraphLayer::Load(const XMLElement& source)
{
    return true;
}

/// Save to an XML element. Return true if successful.
bool ParticleGraphLayer::Save(XMLElement& dest) const
{
    XMLElement emitElem = dest.CreateChild("emit");
    if (!emit_.Save(emitElem))
    {
        return false;
    }
    XMLElement updateElem = dest.CreateChild("update");
    if (!update_.Save(updateElem))
    {
        return false;
    }

    return true;
}

/// Load from a JSON value. Return true if successful.
bool ParticleGraphLayer::Load(const JSONValue& source)
{
    return true;
}

/// Save to a JSON value. Return true if successful.
bool ParticleGraphLayer::Save(JSONValue& dest) const
{
    return true;
}

ParticleGraphNode::ParticleGraphNode() = default;

ParticleGraphNode::~ParticleGraphNode() = default;

bool ParticleGraphNode::Save(XMLElement& dest) const
{
    dest.SetAttribute("type", GetTypeName());
    for (unsigned i = 0; i < NumPins(); ++i)
    {
        auto& pin = const_cast<ParticleGraphNode*>(this)->GetPin(i);
        XMLElement pinElem = dest.CreateChild("pin");
        pinElem.SetAttribute("name", pin.name_);
        pinElem.SetAttribute("type", Variant::GetTypeNameList()[pin.valueType_]);
        switch (pin.containerType_)
        {
        case ParticleGraphContainerType::Span:
            pinElem.SetAttribute("container", "span");
            break;
        case ParticleGraphContainerType::Sparse:
            pinElem.SetAttribute("container", "sparse");
            break;
        case ParticleGraphContainerType::Scalar:
            pinElem.SetAttribute("container", "scalar");
            break;
        case ParticleGraphContainerType::Auto:
            break;
        default: ;
        }
        if (pin.IsInput())
        {
            pinElem.SetAttribute("sourceNode", ea::to_string(pin.sourceNode_));
            if (pin.sourcePin_ != 0)
            {
                pinElem.SetAttribute("sourcePin", ea::to_string(pin.sourcePin_));
            }
        }

    }
    return true;
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
    layers_.resize(numLayers);
}

/// Get number of layers.
unsigned ParticleGraphEffect::GetNumLayers() const
{
    return static_cast<unsigned>(layers_.size());
}

/// Get layer by index.
ParticleGraphLayer& ParticleGraphEffect::GetLayer(unsigned layerIndex)
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
    if (extension == ".xml")
    {
        success = BeginLoadXML(source);
        if (!success)
            success = BeginLoadJSON(source);

        if (success)
            return true;
    }
    else // Load JSON file
    {
        success = BeginLoadJSON(source);
        if (!success)
            success = BeginLoadXML(source);

        if (success)
            return true;
    }

    // All loading failed
    ResetToDefaults();
    loadJSONFile_.Reset();
    return false;
}


bool ParticleGraphEffect::BeginLoadXML(Deserializer& source)
{
    ResetToDefaults();
    loadXMLFile_ = context_->CreateObject<XMLFile>();
    if (loadXMLFile_->Load(source))
    {
        // If async loading, scan the XML content beforehand for technique & texture resources
        // and request them to also be loaded. Can not do anything else at this point
        if (GetAsyncLoadState() == ASYNC_LOADING)
        {
            // TODO
        }

        return true;
    }

    return false;
}

bool ParticleGraphEffect::BeginLoadJSON(Deserializer& source)
{
    // Attempt to load a JSON file
    ResetToDefaults();
    loadXMLFile_.Reset();

    // Attempt to load from JSON file instead
    loadJSONFile_ = context_->CreateObject<JSONFile>();
    if (loadJSONFile_->Load(source))
    {
        // If async loading, scan the XML content beforehand for technique & texture resources
        // and request them to also be loaded. Can not do anything else at this point
        if (GetAsyncLoadState() == ASYNC_LOADING)
        {
            // TODO
        }

        // JSON material was successfully loaded
        return true;
    }

    return false;
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

    bool success = false;
    if (loadXMLFile_)
    {
        // If async loading, get the techniques / textures which should be ready now
        XMLElement rootElem = loadXMLFile_->GetRoot();
        success = Load(rootElem);
    }

    if (loadJSONFile_)
    {
        JSONValue rootVal = loadJSONFile_->GetRoot();
        success = Load(rootVal);
    }

    loadXMLFile_.Reset();
    loadJSONFile_.Reset();
    return success;
}

bool ParticleGraphEffect::Save(Serializer& dest) const
{
    SharedPtr<XMLFile> xml(context_->CreateObject<XMLFile>());
    XMLElement graphElem = xml->CreateRoot("particleGraph");

    Save(graphElem);
    return xml->Save(dest);
}

bool ParticleGraphEffect::Load(const XMLElement& source)
{
    ResetToDefaults();

    return true;
}

bool ParticleGraphEffect::Load(const JSONValue& source)
{
    ResetToDefaults();

    return true;
}

bool ParticleGraphEffect::Save(XMLElement& dest) const
{
    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Can not save material to null XML element");
        return false;
    }

    // Write layers
    for (unsigned i = 0; i < layers_.size(); ++i)
    {
        const auto& entry = layers_[i];

        XMLElement layerElem = dest.CreateChild("layer");
        if (!entry.Save(layerElem))
        {
            return false;
        }
    }

    return true;
}

bool ParticleGraphEffect::Save(JSONValue& dest) const
{
    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Can not save material to null XML element");
        return false;
    }

    return true;
}

}
