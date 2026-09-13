// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/span.h>
#include "Urho3D/Resource/Resource.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/Resource/GraphNode.h"
#include "Urho3D/Scene/Serializable.h"
#include "Urho3D/Particles/ParticleGraphPin.h"

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
