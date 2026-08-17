// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/AI/Blackboard.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class BehaviorStatus
{
    Invalid,
    Running,
    Success,
    Failure
};

enum class BehaviorNodeType
{
    Task,
    Sequence,
    Selector,
    Parallel,
    Inverter,
    Repeater
};

/// Per-tick services passed to every behavior tree task.
struct URHO3D_API BehaviorTreeTickContext
{
    Blackboard* blackboard{};
    float deltaSeconds{};
};

using BehaviorTask = ea::function<BehaviorStatus(BehaviorTreeTickContext&)>;

/// A composite, decorator or leaf node in a BehaviorTree.
struct URHO3D_API BehaviorTreeNode
{
    unsigned id{};
    ea::string name;
    BehaviorNodeType type{BehaviorNodeType::Task};
    ea::vector<unsigned> children;
    BehaviorTask task;
    unsigned maxRepeats{};
    unsigned repeatCount{};
    unsigned activeChild{};
};

/// Production behavior tree runtime with deterministic composite semantics.
class URHO3D_API BehaviorTree
{
public:
    unsigned AddNode(const ea::string& name, BehaviorNodeType type, BehaviorTask task = {});
    bool RemoveNode(unsigned nodeId);
    bool AddChild(unsigned parentId, unsigned childId);
    bool SetRoot(unsigned nodeId);
    unsigned GetRoot() const { return rootId_; }
    BehaviorTreeNode* GetNode(unsigned nodeId);
    const BehaviorTreeNode* GetNode(unsigned nodeId) const;
    const ea::vector<BehaviorTreeNode>& GetNodes() const { return nodes_; }

    void Reset();
    BehaviorStatus Tick(BehaviorTreeTickContext& context);
    BehaviorStatus GetLastStatus() const { return lastStatus_; }
    bool IsRunning() const { return lastStatus_ == BehaviorStatus::Running; }

private:
    BehaviorStatus TickNode(unsigned nodeId, BehaviorTreeTickContext& context);
    void ResetNode(unsigned nodeId);
    bool ContainsPath(unsigned fromNode, unsigned targetNode) const;

    ea::vector<BehaviorTreeNode> nodes_;
    unsigned rootId_{};
    BehaviorStatus lastStatus_{BehaviorStatus::Invalid};
};

} // namespace Urho3D
