// SPDX-License-Identifier: MIT

#include "BehaviorTree.h"

#include <algorithm>

namespace Urho3D
{

unsigned BehaviorTree::AddNode(const ea::string& name, BehaviorNodeType type, BehaviorTask task)
{
    if (name.empty())
        return 0;
    BehaviorTreeNode node;
    node.id = nodes_.size() + 1;
    node.name = name;
    node.type = type;
    node.task = ea::move(task);
    nodes_.push_back(ea::move(node));
    if (rootId_ == 0)
        rootId_ = nodes_.back().id;
    return nodes_.back().id;
}

bool BehaviorTree::RemoveNode(unsigned nodeId)
{
    BehaviorTreeNode* node = GetNode(nodeId);
    if (!node)
        return false;
    for (BehaviorTreeNode& parent : nodes_)
    {
        parent.children.erase(std::remove(parent.children.begin(), parent.children.end(), nodeId), parent.children.end());
    }
    if (rootId_ == nodeId)
        rootId_ = 0;
    node->children.clear();
    node->task = {};
    node->name.clear();
    node->type = BehaviorNodeType::Task;
    node->id = 0;
    Reset();
    return true;
}

bool BehaviorTree::AddChild(unsigned parentId, unsigned childId)
{
    BehaviorTreeNode* parent = GetNode(parentId);
    if (!parent || !GetNode(childId) || parentId == childId)
        return false;
    if (ContainsPath(childId, parentId))
        return false;
    if (std::find(parent->children.begin(), parent->children.end(), childId) != parent->children.end())
        return false;
    parent->children.push_back(childId);
    return true;
}

bool BehaviorTree::SetRoot(unsigned nodeId)
{
    if (!GetNode(nodeId))
        return false;
    rootId_ = nodeId;
    return true;
}

BehaviorTreeNode* BehaviorTree::GetNode(unsigned nodeId)
{
    if (nodeId == 0 || nodeId > nodes_.size() || nodes_[nodeId - 1].id != nodeId)
        return nullptr;
    return &nodes_[nodeId - 1];
}

const BehaviorTreeNode* BehaviorTree::GetNode(unsigned nodeId) const
{
    if (nodeId == 0 || nodeId > nodes_.size() || nodes_[nodeId - 1].id != nodeId)
        return nullptr;
    return &nodes_[nodeId - 1];
}

void BehaviorTree::Reset()
{
    for (BehaviorTreeNode& node : nodes_)
    {
        node.activeChild = 0;
        node.repeatCount = 0;
    }
    lastStatus_ = BehaviorStatus::Invalid;
}

BehaviorStatus BehaviorTree::Tick(BehaviorTreeTickContext& context)
{
    if (rootId_ == 0)
    {
        lastStatus_ = BehaviorStatus::Invalid;
        return lastStatus_;
    }
    lastStatus_ = TickNode(rootId_, context);
    if (lastStatus_ != BehaviorStatus::Running)
        ResetNode(rootId_);
    return lastStatus_;
}

BehaviorStatus BehaviorTree::TickNode(unsigned nodeId, BehaviorTreeTickContext& context)
{
    BehaviorTreeNode* node = GetNode(nodeId);
    if (!node || node->id == 0)
        return BehaviorStatus::Failure;

    switch (node->type)
    {
    case BehaviorNodeType::Task:
        return node->task ? node->task(context) : BehaviorStatus::Failure;

    case BehaviorNodeType::Sequence:
        while (node->activeChild < node->children.size())
        {
            const BehaviorStatus status = TickNode(node->children[node->activeChild], context);
            if (status == BehaviorStatus::Running)
                return status;
            if (status == BehaviorStatus::Failure || status == BehaviorStatus::Invalid)
            {
                node->activeChild = 0;
                return status;
            }
            ++node->activeChild;
        }
        node->activeChild = 0;
        return BehaviorStatus::Success;

    case BehaviorNodeType::Selector:
        while (node->activeChild < node->children.size())
        {
            const BehaviorStatus status = TickNode(node->children[node->activeChild], context);
            if (status == BehaviorStatus::Running)
                return status;
            if (status == BehaviorStatus::Success)
            {
                node->activeChild = 0;
                return status;
            }
            ++node->activeChild;
        }
        node->activeChild = 0;
        return BehaviorStatus::Failure;

    case BehaviorNodeType::Parallel:
    {
        if (node->children.empty())
            return BehaviorStatus::Success;
        bool allSucceeded = true;
        for (const unsigned childId : node->children)
        {
            const BehaviorStatus status = TickNode(childId, context);
            if (status == BehaviorStatus::Failure || status == BehaviorStatus::Invalid)
                return status;
            if (status == BehaviorStatus::Running)
                allSucceeded = false;
        }
        return allSucceeded ? BehaviorStatus::Success : BehaviorStatus::Running;
    }

    case BehaviorNodeType::Inverter:
        if (node->children.size() != 1)
            return BehaviorStatus::Failure;
        switch (TickNode(node->children.front(), context))
        {
        case BehaviorStatus::Success: return BehaviorStatus::Failure;
        case BehaviorStatus::Failure: return BehaviorStatus::Success;
        default: return BehaviorStatus::Running;
        }

    case BehaviorNodeType::Repeater:
        if (node->children.size() != 1)
            return BehaviorStatus::Failure;
        if (node->maxRepeats != 0 && node->repeatCount >= node->maxRepeats)
        {
            node->repeatCount = 0;
            return BehaviorStatus::Success;
        }
        {
            const BehaviorStatus status = TickNode(node->children.front(), context);
            if (status != BehaviorStatus::Success)
                return status;
            ++node->repeatCount;
            ResetNode(node->children.front());
            if (node->maxRepeats != 0 && node->repeatCount >= node->maxRepeats)
            {
                node->repeatCount = 0;
                return BehaviorStatus::Success;
            }
            return BehaviorStatus::Running;
        }
    }
    return BehaviorStatus::Invalid;
}

void BehaviorTree::ResetNode(unsigned nodeId)
{
    BehaviorTreeNode* node = GetNode(nodeId);
    if (!node)
        return;
    node->activeChild = 0;
    node->repeatCount = 0;
    for (const unsigned childId : node->children)
        ResetNode(childId);
}

bool BehaviorTree::ContainsPath(unsigned fromNode, unsigned targetNode) const
{
    if (fromNode == targetNode)
        return true;
    const BehaviorTreeNode* node = GetNode(fromNode);
    if (!node)
        return false;
    for (const unsigned childId : node->children)
    {
        if (ContainsPath(childId, targetNode))
            return true;
    }
    return false;
}

} // namespace Urho3D
