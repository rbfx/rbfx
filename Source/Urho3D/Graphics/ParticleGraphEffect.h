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

#include "../Resource/Resource.h"
#include "EASTL/span.h"

namespace Urho3D
{
class JSONFile;
class XMLFile;

enum class ParticleGraphContainerType
{
    Span,
    Sparse,
    Scalar,
    Auto
};

/// Memory layout definition.
struct ParticleGraphSpan
{
    /// Construct.
    ParticleGraphSpan();

    /// Offset in the buffer.
    unsigned offset_;
    /// Size in bytes.
    unsigned size_;

    /// Make a span of type T from memory buffer.
    template <typename T> ea::span<T> MakeSpan(ea::span<uint8_t> buffer) const
    {
        const auto slice = buffer.subspan(offset_, size_);
        return ea::span<T>(reinterpret_cast<T*>(slice.begin()), reinterpret_cast<T*>(slice.end()));
    }
};

class ParticleGraphNodePin
{
public:
    /// Construct default pin.
    ParticleGraphNodePin();
    /// Construct pin.
    ParticleGraphNodePin(
        bool isInput,
        const ea::string name,
        VariantType type = VariantType::VAR_NONE,
        ParticleGraphContainerType container = ParticleGraphContainerType::Auto);

    /// Container type: span, sparse or scalar.
    ParticleGraphContainerType containerType_{ParticleGraphContainerType::Auto};

    /// Source node.
    unsigned sourceNode_{};
    /// Source node pin index.
    unsigned sourcePin_{};

    /// Get input pin flag.
    /// @property 
    bool GetIsInput() const { return isInput_; }

    /// Name of the pin for visual editor.
    /// @property 
    const ea::string& GetName() const { return name_; }

    /// Name hash of the pin.
    /// @property
    StringHash GetNameHash() const { return nameHash_; }

    /// Value type of the pin. VAR_NONE for autodetected value type.
    /// @property
    VariantType GetValueType() const { return valueType_; }

    template <typename T> ea::span<T> MakeSpan(ea::span<uint8_t> buffer) const
    {
        return (isInput_ ? sourceSpan_ : outputSpan_).MakeSpan<T>(buffer);
    }
protected:
    /// Set pin name and hash.
    void SetName(const ea::string& name);
    /// Set pin value type.
    void SetValueType(VariantType valueType);

    /// Get input pin flag.
    /// @property
    void SetIsInput(bool isInput);

private:
    /// Source pin container type: span, sparse or scalar.
    ParticleGraphContainerType sourceContainerType_{ParticleGraphContainerType::Auto};

    /// Source node pin memory layout.
    ParticleGraphSpan sourceSpan_;

    /// Memory layout if the pin belongs to attribute or if it is an output pin.
    ParticleGraphSpan outputSpan_;

    /// Value type at runtime.
    VariantType runtimeValueType_{VAR_NONE};

    /// Name of the pin for visual editor.
    ea::string name_;
    /// Pin name hash.
    StringHash nameHash_;

    /// Is input pin.
    bool isInput_{true};
    /// Value type (float, vector3, etc).
    VariantType valueType_{VariantType::VAR_NONE};

    friend class ParticleGraphAttributeBuilder;
    friend class ParticleGraphNode;
};

class ParticleGraphNodeInstance;

class URHO3D_API ParticleGraphNode : public Object
{
    URHO3D_OBJECT(ParticleGraphNode, Object)
    
public:
    /// Construct.
    explicit ParticleGraphNode(Context* context);

    /// Destruct.
    ~ParticleGraphNode() override;

    /// Get number of pins.
    virtual unsigned NumPins() const = 0;

    /// Get pin by index.
    virtual ParticleGraphNodePin& GetPin(unsigned index) = 0;

    /// Evaluate size required to place new node instance.
    virtual unsigned EvalueInstanceSize() = 0;

    /// Place new instance at the provided address.
    virtual ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) = 0;

    /// Save to an XML element. Return true if successful.
    virtual bool Save(XMLElement& dest) const;

protected:
    /// Evaluate runtime output pin type.
    virtual bool EvaluateOutputPinType(ParticleGraphNodePin& pin);

    /// Set pin name.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    void SetPinName(unsigned pinIndex, const ea::string& name);

    /// Set pin type.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    void SetPinValueType(unsigned pinIndex, VariantType type);

    friend class ParticleGraphAttributeBuilder;
};

class URHO3D_API ParticleGraph: public Object
{
    URHO3D_OBJECT(ParticleGraph, Object);
public:
    /// Construct.
    explicit ParticleGraph(Context* context);
    /// Destruct.
    virtual ~ParticleGraph();

    /// Add node to the graph.
    /// Returns node index;
    unsigned Add(const SharedPtr<ParticleGraphNode> node);

    /// Get number of nodes.
    /// @property
    unsigned GetNumNodes() const;

    /// Get node by index.
    SharedPtr<ParticleGraphNode> GetNode(unsigned index) const;

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive, const char* blockName);

private:
    /// Nodes in the graph;
    ea::vector<SharedPtr<ParticleGraphNode>> nodes_;
};

class URHO3D_API ParticleGraphLayer: public Object
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
        /// Particle attribute values.
        ParticleGraphSpan values_;

        void Apply(ParticleGraphLayer& layer);
    };

    /// Construct.
    explicit ParticleGraphLayer(Context* context);
    /// Destruct.
    virtual ~ParticleGraphLayer();

    /// Get emit graph.
    ParticleGraph& GetEmitGraph();

    /// Get update graph.
    ParticleGraph& GetUpdateGraph();

    /// Invalidate graph layer state.
    /// Call this method when something is changed in the layer graphs and it requires new preparation.
    void Invalidate();

    /// Prepare layer for execution. Returns false if graph is invalid. See output logs for errors.
    bool Prepare();

    /// Return attribute buffer layout.
    /// @property
    const AttributeBufferLayout& GetAttributeBufferLayout() const;

    /// Return size of temp buffer in bytes.
    /// @property
    unsigned GetTempBufferSize() const;

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive);

private:
    /// Maximum number of particles.
    unsigned capacity_;
    ParticleGraph emit_;
    ParticleGraph update_;
    /// Is prepared.
    bool isPrepared_;
    /// Attribure buffer layout.
    AttributeBufferLayout attributeBufferLayout_;
    /// Required temp buffer size.
    unsigned tempBufferSize_;
};

/// %Particle graph effect definition.
class URHO3D_API ParticleGraphEffect : public Resource
{
    URHO3D_OBJECT(ParticleGraphEffect, Resource);

public:
    /// Construct.
    explicit ParticleGraphEffect(Context* context);
    /// Destruct.
    ~ParticleGraphEffect() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set number of layers.
    void SetNumLayers(unsigned numLayers);
    /// Get number of layers.
    unsigned GetNumLayers() const;

    /// Get layer by index.
    SharedPtr<ParticleGraphLayer> GetLayer(unsigned layerIndex) const;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;
    
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive) override;


private:

    /// Reset to defaults.
    void ResetToDefaults();

private:
    ea::vector<SharedPtr<ParticleGraphLayer>> layers_;

    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;
};

}
