// SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintGraph.h"
#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{

/// Native rbfx Resource containing a serialized Blueprint graph.
class URHO3D_API BlueprintResource : public Resource
{
    URHO3D_OBJECT(BlueprintResource, Resource)

public:
    explicit BlueprintResource(Context* context);

    /// Register the Blueprint resource type in the rbfx object factory.
    static void RegisterObject(Context* context);

    /// Load a .blueprint JSON graph from an rbfx resource stream.
    bool BeginLoad(Deserializer& source) override;
    /// Save the graph to a .blueprint JSON stream.
    bool Save(Serializer& dest) const override;

    BlueprintGraph& GetGraph() { return graph_; }
    const BlueprintGraph& GetGraph() const { return graph_; }
    void SetGraph(const BlueprintGraph& graph) { graph_ = graph; }

private:
    BlueprintGraph graph_;
};

} // namespace Urho3D
