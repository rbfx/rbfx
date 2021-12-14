//
// Copyright (c) 2021 the Urho3D project.
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

enum GraphPinDirection
{
    PINDIR_INPUT,
    PINDIR_OUTPUT,
    PINDIR_ENTER,
    PINDIR_EXIT,
};

/// Abstract graph node pin.
class URHO3D_API GraphPin
{
protected:
    /// Construct.
    GraphPin(GraphNode* node, GraphPinDirection direction);

public:
    /// Get pin type.
    /// @property 
    VariantType GetType() const { return type_; }

    /// Set pin type.
    /// @property
    void SetType(VariantType type);

    /// Get name of the pin.
    /// @property
    const ea::string& GetName() { return name_; }

    /// Get pin direction.
    GraphPinDirection GetDirection() const { return direction_; }

    /// Get pin node.
    GraphNode* GetNode() const { return node_; }

protected:
    /// Serialize from/to archive. Return true if successful.
    virtual bool Serialize(Archive& archive);

    /// Set name of the pin. Executed by GraphNode.
    /// @property
    void SetName(const ea::string_view name);

    /// Pin name.
    ea::string name_;

    /// Pin type.
    VariantType type_;


    GraphNode* node_;
    GraphPinDirection direction_;

    friend class GraphNode;
    template <typename T, size_t nodeCount> friend class GraphNodeMapHelper;
};

/// Graph node pin that connects to other pins.
class URHO3D_API GraphOutPin : public GraphPin
{
protected:
    /// Construct.
    GraphOutPin(GraphNode* node, GraphPinDirection direction);

    friend class GraphNode;
};

/// Graph node pin with connection.
class URHO3D_API GraphInPin : public GraphPin
{
protected:
    /// Construct.
    GraphInPin(GraphNode* node, GraphPinDirection direction);

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive) override;

public:
    /// Connect to other pin.
    bool ConnectTo(GraphOutPin& pin);

    /// Get pin connected to the current pin.
    GraphOutPin* GetConnectedPin() const;

    /// Get true if pin is connected.
    bool GetConnected() const { return targetNode_; }

private:
    /// Target node.
    unsigned targetNode_;

    /// Target pin name.
    ea::string targetPin_;

    friend class GraphNode;
};

/// Graph data flow node pin with connection and default value.
class URHO3D_API GraphDataInPin : public GraphInPin
{
protected:
    /// Construct.
    GraphDataInPin(GraphNode* node, GraphPinDirection direction);

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive) override;

public:
    /// Get default value.
    /// @property
    const Variant& GetDefaultValue();

    /// Set default value.
    /// @property 
    void SetDefaultValue(const Variant& variant);

private:
    /// Target node.
    Variant defaultValue_;

    friend class GraphNode;
};
} // namespace Urho3D
