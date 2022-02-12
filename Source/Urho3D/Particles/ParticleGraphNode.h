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

#include <EASTL/span.h>
#include "../Resource/Resource.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"
#include "../Resource/GraphNode.h"
#include "../Scene/Serializable.h"
#include "ParticleGraphPin.h"

namespace Urho3D
{
class ParticleGraphPin;
class ParticleGraphNodeInstance;
class ParticleGraphReader;
class ParticleGraphWriter;
class ParticleGraph;

class URHO3D_API ParticleGraphNode : public Serializable
{
    URHO3D_OBJECT(ParticleGraphNode, Serializable)

public:
    static constexpr unsigned INVALID_PIN = std::numeric_limits<unsigned>::max();

    /// Construct.
    explicit ParticleGraphNode(Context* context);

    /// Destruct.
    ~ParticleGraphNode() override;

    /// Get graph.
    ParticleGraph* GetGraph() { return graph_; }

    /// Get number of pins.
    virtual unsigned GetNumPins() const = 0;

    /// Get pin by index.
    virtual ParticleGraphPin& GetPin(unsigned index) = 0;

    /// Get pin by index.
    const ParticleGraphPin& GetPin(unsigned index) const;

    /// Set pin source node and pin.
    bool SetPinSource(unsigned pinIndex, unsigned nodeIndex, unsigned nodePinIndex = 0);

    /// Get pin by name.
    ParticleGraphPin* GetPin(const ea::string& name);

    /// Get pin index by name.
    unsigned GetPinIndex(const ea::string& name);

    /// Get pin name.
    const ea::string& GetPinName(unsigned pinIndex) const;

    /// Get pin type.
    VariantType GetPinValueType(unsigned pinIndex) const;

    /// Evaluate size required to place new node instance.
    virtual unsigned EvaluateInstanceSize() const = 0;

    /// Place new instance at the provided address.
    virtual ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) = 0;

    /// Load node.
    virtual bool Load(ParticleGraphReader& reader, GraphNode& node);
    /// Save node.
    virtual bool Save(ParticleGraphWriter& writer, GraphNode& node);

protected:
    /// Load input pin.
    virtual ParticleGraphPin* LoadInputPin(ParticleGraphReader& reader, GraphInPin& pin);
    /// Load output pin.
    virtual ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin);
    /// Load pins.
    virtual bool LoadPins(ParticleGraphReader& reader, GraphNode& node);
    /// Load property value.
    virtual bool LoadProperty(GraphNodeProperty& prop);
    /// Load properties.
    virtual bool LoadProperties(ParticleGraphReader& reader, GraphNode& node);
    /// Save pins.
    virtual bool SavePins(ParticleGraphWriter& writer, GraphNode& node);
    /// Save node.
    virtual bool SaveProperties(ParticleGraphWriter& writer, GraphNode& node);

    /// Evaluate runtime output pin type.
    virtual VariantType EvaluateOutputPinType(ParticleGraphPin& pin);

    /// Set graph reference. Called by ParticleGraph.
    void SetGraph(ParticleGraph* graph, unsigned index);

    /// Set pin name.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    bool SetPinName(unsigned pinIndex, const ea::string& name);

    /// Set pin type.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    bool SetPinValueType(unsigned pinIndex, VariantType type);

    /// Parent graph.
    ParticleGraph* graph_;
    /// This node index in the graph.
    unsigned index_;

    friend class ParticleGraphAttributeBuilder;
    friend class ParticleGraph;
};

}
