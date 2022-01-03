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

#include "../Core/Context.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class GraphNode;
template <typename T, size_t nodeCount> struct GraphNodeMapHelper;

/// Abstract graph node pin.
class URHO3D_API GraphPin
{
public:
    GraphPin() = default;
    explicit GraphPin(GraphNode* node);
    virtual ~GraphPin();

    /// Get name of the pin.
    /// @property
    const ea::string& GetName() { return name_; }

    /// Get pin node.
    GraphNode* GetNode() const { return node_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    virtual void SerializeInBlock(Archive& archive);

protected:
    /// Set name of the pin. Executed by GraphNode.
    /// @property
    void SetName(const ea::string_view name) { name_ = name; }

    /// Pin name.
    ea::string name_;

    /// Owner node.
    GraphNode* node_{};

    friend class GraphNode;
    template <typename T, size_t nodeCount> friend struct GraphNodeMapHelper;
};

/// Abstract graph data flow node pin. Has pin type.
class URHO3D_API GraphDataPin : public GraphPin
{
public:
    GraphDataPin() = default;
    explicit GraphDataPin(GraphNode* node);

public:
    /// Get pin type.
    /// @property
    VariantType GetType() const { return type_; }

    /// Set pin type.
    /// @property
    void SetType(VariantType type) { type_ = type; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Pin type.
    VariantType type_{VAR_NONE};
};

/// Graph node pin that connects to other pins.
class URHO3D_API GraphOutPin : public GraphDataPin
{
public:
    GraphOutPin() = default;
    explicit GraphOutPin(GraphNode* node);

    friend class GraphNode;
};

/// Graph node pin with connection.
class URHO3D_API GraphInPin : public GraphDataPin
{
public:
    GraphInPin() = default;
    GraphInPin(GraphNode* node);

    /// Connect to other pin.
    bool ConnectTo(GraphOutPin& pin);

    /// Disconnect pin.
    void Disconnect();

    /// Get pin connected to the current pin.
    GraphOutPin* GetConnectedPin() const;

    /// Get true if pin is connected.
    bool IsConnected() const { return targetNode_; }

    /// Get value.
    /// @property
    const Variant& GetValue() { return value_; }

    /// Set value. Disconnects pin from an "Out" pin.
    /// @property
    void SetValue(const Variant& variant);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

private:
    /// Target node.
    unsigned targetNode_{};

    /// Target pin name.
    ea::string targetPin_;

    /// Target node.
    Variant value_;

    friend class GraphNode;
};

/// Graph node execution flow "enter" pin. May be connected to multiple exit pins.
class URHO3D_API GraphEnterPin : public GraphPin
{
public:
    GraphEnterPin() = default;
    explicit GraphEnterPin(GraphNode* node);

    friend class GraphNode;
};

/// Graph node execution flow "exit" pin. May be connected to one "enter" pins.
class URHO3D_API GraphExitPin : public GraphPin
{
public:
    GraphExitPin() = default;
    explicit GraphExitPin(GraphNode* node);

    /// Connect to other pin.
    bool ConnectTo(GraphEnterPin& pin);

    /// Disconnect pin.
    void Disconnect();

    /// Get pin connected to the current pin.
    GraphEnterPin* GetConnectedPin() const;

    /// Get true if pin is connected.
    bool IsConnected() const { return targetNode_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

private:
    /// Target node.
    unsigned targetNode_{};

    /// Target pin name.
    ea::string targetPin_;

    friend class GraphNode;
};

} // namespace Urho3D
