//
// Copyright (c) 2021-2023 the rbfx project.
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

#include "Urho3D/Actions/BaseAction.h"
#include "../Resource/Resource.h"
#include "../IO/Archive.h"

namespace Urho3D
{

class Graph;

/// Action as resource
class URHO3D_API ActionSet : public SimpleResource
{
    URHO3D_OBJECT(ActionSet, SimpleResource)

public:
    /// Construct.
    explicit ActionSet(Context* context);
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

    /// Get action
    Actions::BaseAction* GetAction() const { return action_; }
    /// Set action
    void SetAction(Actions::BaseAction* action);

    /// Create GraphNode from the action. Required for action editor.
    SharedPtr<Graph> ToGraph() const;
    /// Initialize action from GraphNode. Required for action editor.
    bool FromGraph(const Graph* graph);

protected:
    /// Root block name. Used for XML serialization only.
    const char* GetRootBlockName() const override { return "actionset"; }

private:
    /// Root action.
    SharedPtr<Actions::BaseAction> action_;
};

} // namespace Urho3D
