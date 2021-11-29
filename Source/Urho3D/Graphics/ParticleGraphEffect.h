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
    ParticleGraphNodePin();
    ParticleGraphNodePin(bool isInput, const ea::string name);

    /// Container type: span, sparse or scalar.
    ParticleGraphContainerType containerType_{ParticleGraphContainerType::Auto};
    /// Value type (float, vector3, etc).
    VariantType valueType_{ VariantType::VAR_NONE };

    /// Source node.
    unsigned sourceNode_{};
    /// Source node pin index.
    unsigned sourcePin_{};

    /// Is input pin.
    /// @property 
    bool IsInput() const { return isInput_; }

    /// Name of the pin for visual editor.
    const ea::string& GetName() const { return name_; };

    /// Is input pin.
    bool isInput_{true};
    /// Name of the pin for visual editor.
    ea::string name_;

    template <typename T> ea::span<T> MakeSpan(ea::span<uint8_t> buffer) const
    {
        return (isInput_ ? sourceSpan_ : outputSpan_).MakeSpan<T>(buffer);
    }

private:
    /// Source pin container type: span, sparse or scalar.
    ParticleGraphContainerType sourceContainerType_{ParticleGraphContainerType::Auto};

    /// Source node pin memory layout.
    ParticleGraphSpan sourceSpan_;

    /// Memory layout if the pin belongs to attribute or if it is an output pin.
    ParticleGraphSpan outputSpan_;

    friend class ParticleGraphAttributeBuilder;
};

class ParticleGraphNodeInstance;

class URHO3D_API ParticleGraphNode : public RefCounted
{
protected:
    virtual const ea::string& GetTypeName() const
    {
        static const ea::string typeName{"ParticleGraphNode"};
        return typeName;
    }
    
public:
    /// Construct.
    explicit ParticleGraphNode();

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
};

class URHO3D_API ParticleGraph
{
public:
    /// Construct.
    explicit ParticleGraph();
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

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);
    /// Save to an XML element. Return true if successful.
    bool Save(XMLElement& dest) const;

    /// Load from a JSON value. Return true if successful.
    bool Load(const JSONValue& source);
    /// Save to a JSON value. Return true if successful.
    bool Save(JSONValue& dest) const;

private:
    /// Nodes in the graph;
    ea::vector<SharedPtr<ParticleGraphNode>> nodes_;
};

class URHO3D_API ParticleGraphLayer
{
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
    explicit ParticleGraphLayer();
    /// Destruct.
    virtual ~ParticleGraphLayer();

    /// Get emit graph.
    ParticleGraph& GetEmitGraph();

    /// Get update graph.
    ParticleGraph& GetUpdateGraph();

    /// Invalidate graph layer state.
    /// Call this method when something is changed in the layer graphs and it requires new preparation.
    void Invalidate();

    /// Prepare layer for execution.
    void Prepare();

    /// Return attribute buffer layout.
    /// @property
    const AttributeBufferLayout& GetAttributeBufferLayout() const;

    /// Return size of temp buffer in bytes.
    /// @property
    unsigned GetTempBufferSize() const;

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);
    /// Save to an XML element. Return true if successful.
    bool Save(XMLElement& dest) const;

    /// Load from a JSON value. Return true if successful.
    bool Load(const JSONValue& source);
    /// Save to a JSON value. Return true if successful.
    bool Save(JSONValue& dest) const;

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
    ParticleGraphLayer& GetLayer(unsigned layerIndex);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;

    using Resource::Load;

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);
    /// Save to an XML element. Return true if successful.
    bool Save(XMLElement& dest) const;

    /// Load from a JSON value. Return true if successful.
    bool Load(const JSONValue& source);
    /// Save to a JSON value. Return true if successful.
    bool Save(JSONValue& dest) const;

private:
    /// Helper function for loading JSON files.
    bool BeginLoadJSON(Deserializer& source);
    /// Helper function for loading XML files.
    bool BeginLoadXML(Deserializer& source);

    /// Reset to defaults.
    void ResetToDefaults();

private:
    ea::vector<ParticleGraphLayer> layers_;

    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;
    /// JSON file used while loading.
    SharedPtr<JSONFile> loadJSONFile_;
};

}
