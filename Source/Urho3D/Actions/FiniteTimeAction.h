//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "BaseAction.h"

namespace Urho3D
{
namespace Actions
{
/// Finite time action.
class URHO3D_API FiniteTimeAction : public BaseAction
{
    URHO3D_OBJECT(FiniteTimeAction, BaseAction)

public:
    /// Construct.
    FiniteTimeAction(Context* context);

    /// Set action duration.
    void SetDuration(float duration);
    /// Get action duration.
    virtual float GetDuration() const;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Get action from argument or empty action.
    FiniteTimeAction* GetOrDefault(FiniteTimeAction* action) const;

    /// Create reversed action.
    virtual SharedPtr<FiniteTimeAction> Reverse() const;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;
    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;

protected:
    /// Populate fields in reversed action.
    virtual void ReverseImpl(FiniteTimeAction*) const;

private:
    float duration_{ea::numeric_limits<float>::epsilon()};
};

/// Finite time action where duration depends on other actions.
class URHO3D_API DynamicAction : public FiniteTimeAction
{
    URHO3D_OBJECT(DynamicAction, FiniteTimeAction)

public:
    /// Construct.
    DynamicAction(Context* context);

    /// Get action duration override that asserts.
    float GetDuration() const override = 0;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;
    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
};

} // namespace Actions

void SerializeValue(Archive& archive, const char* name, SharedPtr<Actions::FiniteTimeAction>& value);

} // namespace Urho3D
