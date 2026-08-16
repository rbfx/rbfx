// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintDefs.h"
#include <Urho3D/Resource/JSONValue.h>

namespace Urho3D
{

class JSONFile;

/// Serializable graph asset used by the Blueprint editor and runtime.
class URHO3D_API BlueprintGraph
{
public:
    BlueprintGraph() = default;
    explicit BlueprintGraph(const ea::string& name) : name_(name) {}

    /// Reset the graph and its generated identifiers.
    void Clear();

    /// Return the asset name.
    const ea::string& GetName() const { return name_; }
    /// Set the asset name.
    void SetName(const ea::string& name) { name_ = name; }

    /// Create a node and return its stable identifier.
    BlueprintId AddNode(const ea::string& typeName, const ea::string& title,
        const Vector2& position = Vector2::ZERO,
        BlueprintExecutionMode executionMode = BlueprintExecutionMode::Immediate);
    /// Add a fully initialized node, preserving its identifier when valid.
    BlueprintId AddNode(const BlueprintNode& node);
    /// Remove a node and every link attached to it.
    bool RemoveNode(BlueprintId nodeId);

    /// Find a mutable node by identifier, or nullptr if absent.
    BlueprintNode* GetNode(BlueprintId nodeId);
    /// Find a read-only node by identifier, or nullptr if absent.
    const BlueprintNode* GetNode(BlueprintId nodeId) const;
    /// Return all graph nodes.
    const ea::vector<BlueprintNode>& GetNodes() const { return nodes_; }
    /// Return all graph links.
    const ea::vector<BlueprintLink>& GetLinks() const { return links_; }
    /// Return all graph variables.
    const ea::vector<BlueprintVariable>& GetVariables() const { return variables_; }
    /// Return all graph comments.
    const ea::vector<BlueprintComment>& GetComments() const { return comments_; }
    /// Return all user-defined functions/subgraphs.
    const ea::vector<BlueprintFunction>& GetFunctions() const { return functions_; }
    /// Return all user-defined Blueprint structs.
    const ea::vector<BlueprintStructDef>& GetStructs() const { return structs_; }
    /// Return all user-defined Blueprint enums.
    const ea::vector<BlueprintEnumDef>& GetEnums() const { return enums_; }
    /// Return all user-defined Blueprint delegates and signals.
    const ea::vector<BlueprintDelegate>& GetDelegates() const { return delegates_; }
    /// Return all user-defined timelines.
    const ea::vector<BlueprintTimeline>& GetTimelines() const { return timelines_; }
    /// Return all user-defined macros.
    const ea::vector<BlueprintMacro>& GetMacros() const { return macros_; }

    /// Add or replace a comment box.
    bool AddComment(const BlueprintComment& comment);
    /// Remove a comment box and detach nodes assigned to it.
    bool RemoveComment(BlueprintId commentId);
    /// Find a comment by identifier.
    BlueprintComment* GetComment(BlueprintId commentId);
    const BlueprintComment* GetComment(BlueprintId commentId) const;
    /// Add or replace a user-defined function/subgraph.
    bool AddFunction(const BlueprintFunction& function);
    /// Find a function/subgraph by name.
    BlueprintFunction* GetFunction(const ea::string& name);
    const BlueprintFunction* GetFunction(const ea::string& name) const;
    /// Add or replace a user-defined Blueprint struct.
    bool AddStruct(const BlueprintStructDef& structure);
    /// Remove a user-defined Blueprint struct by name.
    bool RemoveStruct(const ea::string& name);
    /// Find a user-defined Blueprint struct by name.
    BlueprintStructDef* GetStruct(const ea::string& name);
    const BlueprintStructDef* GetStruct(const ea::string& name) const;
    /// Add or replace a user-defined Blueprint enum.
    bool AddEnum(const BlueprintEnumDef& enumeration);
    /// Remove a user-defined Blueprint enum by name.
    bool RemoveEnum(const ea::string& name);
    /// Find a user-defined Blueprint enum by name.
    BlueprintEnumDef* GetEnum(const ea::string& name);
    const BlueprintEnumDef* GetEnum(const ea::string& name) const;
    /// Add or replace a user-defined delegate or signal signature.
    bool AddDelegate(const BlueprintDelegate& delegate);
    /// Remove a delegate or signal by name.
    bool RemoveDelegate(const ea::string& name);
    /// Find a delegate or signal by name.
    BlueprintDelegate* GetDelegate(const ea::string& name);
    const BlueprintDelegate* GetDelegate(const ea::string& name) const;
    /// Add or replace a user-defined timeline.
    bool AddTimeline(const BlueprintTimeline& timeline);
    /// Remove a timeline by name.
    bool RemoveTimeline(const ea::string& name);
    /// Find a timeline by name.
    BlueprintTimeline* GetTimeline(const ea::string& name);
    const BlueprintTimeline* GetTimeline(const ea::string& name) const;
    /// Add or replace a user-defined macro.
    bool AddMacro(const BlueprintMacro& macro);
    /// Remove a macro by name.
    bool RemoveMacro(const ea::string& name);
    /// Find a macro by name.
    BlueprintMacro* GetMacro(const ea::string& name);
    const BlueprintMacro* GetMacro(const ea::string& name) const;
    /// Return nodes whose type, title or category contains the query.
    ea::vector<BlueprintId> SearchNodes(const ea::string& query) const;
    /// Apply a deterministic left-to-right automatic layout to nodes.
    void AutoLayout(float horizontalSpacing = 320.0f, float verticalSpacing = 120.0f);

    /// Add a pin to an existing node.
    bool AddPin(BlueprintId nodeId, const BlueprintPin& pin);
    /// Remove a pin and every link attached to it.
    bool RemovePin(BlueprintId nodeId, const ea::string& pinName);
    /// Find a pin on a node.
    BlueprintPin* GetPin(BlueprintId nodeId, const ea::string& pinName);
    const BlueprintPin* GetPin(BlueprintId nodeId, const ea::string& pinName) const;

    /// Add a connection. Returns zero when either pin is missing or incompatible.
    BlueprintId AddLink(BlueprintId fromNode, const ea::string& fromPin,
        BlueprintId toNode, const ea::string& toPin);
    /// Add a fully initialized connection.
    BlueprintId AddLink(const BlueprintLink& link);
    /// Remove a connection.
    bool RemoveLink(BlueprintId linkId);

    /// Add or replace a graph variable. Returns false for an invalid name.
    bool AddVariable(const BlueprintVariable& variable);
    /// Remove a graph variable by name.
    bool RemoveVariable(const ea::string& name);
    /// Find a graph variable by name.
    BlueprintVariable* GetVariable(const ea::string& name);
    const BlueprintVariable* GetVariable(const ea::string& name) const;

    /// Validate topology, pin types, names and connection directions.
    BlueprintValidationResult Validate() const;

    /// Convert the graph to rbfx's native JSON value representation.
    JSONValue ToJSON() const;
    /// Load the graph from a JSON value. Existing content is replaced only on success.
    bool FromJSON(const JSONValue& value, ea::string* error = nullptr);
    /// Serialize the graph as a JSON document.
    ea::string ToString(const ea::string& indentation = "  ") const;
    /// Parse a JSON document into the graph.
    bool FromString(const ea::string& source, ea::string* error = nullptr);

private:
    BlueprintId AllocateNodeId();
    BlueprintId AllocateLinkId();
    BlueprintId AllocateCommentId();
    bool IsPinCompatible(const BlueprintPin& source, const BlueprintPin& target) const;
    void RemoveLinksForNode(BlueprintId nodeId);
    void RemoveLinksForPin(BlueprintId nodeId, const ea::string& pinName);

    ea::string name_;
    BlueprintId nextNodeId_{1};
    BlueprintId nextLinkId_{1};
    BlueprintId nextCommentId_{1};
    ea::vector<BlueprintNode> nodes_;
    ea::vector<BlueprintLink> links_;
    ea::vector<BlueprintVariable> variables_;
    ea::vector<BlueprintComment> comments_;
    ea::vector<BlueprintFunction> functions_;
    ea::vector<BlueprintStructDef> structs_;
    ea::vector<BlueprintEnumDef> enums_;
    ea::vector<BlueprintDelegate> delegates_;
    ea::vector<BlueprintTimeline> timelines_;
    ea::vector<BlueprintMacro> macros_;
};

}
