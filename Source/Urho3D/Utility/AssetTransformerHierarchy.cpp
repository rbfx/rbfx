//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Utility/AssetTransformerHierarchy.h"

#include "../IO/Log.h"

#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

namespace
{

struct TransformerInfo
{
    AssetTransformer* transformer_{};
    unsigned typeOrder_{};
    unsigned pathOrder_{};
    unsigned flavorOrder_{};

    bool operator <(const TransformerInfo& rhs) const
    {
        return ea::tie(typeOrder_, pathOrder_, flavorOrder_) < ea::tie(rhs.typeOrder_, rhs.pathOrder_, rhs.flavorOrder_);
    }
};

using TransformerInfoVector = ea::vector<TransformerInfo>;

void FilterDuplicates(TransformerInfoVector& result)
{
    ea::unordered_set<StringHash> usedTypes;
    ea::erase_if(result, [&](const TransformerInfo& info)
    {
        if (info.transformer_->IsSingleInstanced() && usedTypes.contains(info.transformer_->GetType()))
            return true;
        usedTypes.insert(info.transformer_->GetType());
        return false;
    });
}

AssetTransformerVector Reduce(const TransformerInfoVector& source)
{
    AssetTransformerVector result;
    for (const TransformerInfo& info : source)
        result.push_back(info.transformer_);
    return result;
}

}

bool AssetTransformerHierarchy::TreeNode::ByName::operator()(const ea::unique_ptr<TreeNode>& lhs, const ea::unique_ptr<TreeNode>& rhs) const
{
    return lhs->name_ < rhs->name_;
}

bool AssetTransformerHierarchy::TreeNode::ByName::operator()(const ea::string& lhs, const ea::unique_ptr<TreeNode>& rhs) const
{
    return lhs < rhs->name_;
}

bool AssetTransformerHierarchy::TreeNode::ByName::operator()(const ea::unique_ptr<TreeNode>& lhs, const ea::string& rhs) const
{
    return lhs->name_ < rhs;
}

AssetTransformerHierarchy::AssetTransformerHierarchy(Context* context)
    : Object(context)
{
    Clear();
}

void AssetTransformerHierarchy::Clear()
{
    root_ = {};
    dependencyRoot_ = ea::make_shared<DependencyGraphNode>();
    dependencyNodes_ = {{"", dependencyRoot_}};
}

void AssetTransformerHierarchy::AddTransformer(const ea::string& path, AssetTransformer* transformer)
{
    const StringVector pathComponents = path.split('/');
    TreeNode& node = root_.GetOrCreateChild(pathComponents);
    node.transformers_.push_back(SharedPtr<AssetTransformer>(transformer));
}

bool AssetTransformerHierarchy::RemoveTransformers(const TypeInfo* typeInfo)
{
    bool anyRemoved = false;
    root_.Iterate([&](TreeNode& node)
    {
        const unsigned oldSize = node.transformers_.size();
        ea::erase_if(node.transformers_, [&](AssetTransformer* transformer) { return transformer->GetTypeInfo() == typeInfo; });
        anyRemoved = anyRemoved || (oldSize != node.transformers_.size());
    });
    return anyRemoved;
}

bool AssetTransformerHierarchy::AddDependency(const ea::string& transformerClass, const ea::string& dependsOn)
{
    const auto rootNode = GetOrCreateDependencyNode(transformerClass);
    const auto dependencyNode = GetOrCreateDependencyNode(dependsOn);
    if (DependsOn(*dependencyNode, *rootNode))
    {
        URHO3D_LOGERROR("Cannot add dependency '{} before {}' because it would create circular dependency",
            dependsOn, transformerClass);
        return false;
    }

    rootNode->dependencies_.push_back(dependencyNode);
    return true;
}

AssetTransformerHierarchy::DependencyGraphNodePtr AssetTransformerHierarchy::GetOrCreateDependencyNode(const ea::string& name)
{
    const auto iter = dependencyNodes_.find(name);
    if (iter != dependencyNodes_.end())
        return iter->second;

    const auto newNode = ea::make_shared<DependencyGraphNode>();
    newNode->name_ = name;

    dependencyNodes_.emplace(name, newNode);
    dependencyRoot_->dependencies_.push_back(newNode);
    return newNode;
}

bool AssetTransformerHierarchy::DependsOn(const DependencyGraphNode& queryNode, const DependencyGraphNode& dependencyNode) const
{
    for (const auto& dependency : queryNode.dependencies_)
    {
        if (dependency.get() == &dependencyNode)
            return true;
    }

    for (const auto& dependency : queryNode.dependencies_)
    {
        if (DependsOn(*dependency, dependencyNode))
            return true;
    }

    return false;
}

unsigned AssetTransformerHierarchy::GetTypeOrder(StringHash type) const
{
    const auto iter = dependencyOrder_.find(type);
    return iter != dependencyOrder_.end() ? iter->second : 0;
}

void AssetTransformerHierarchy::CommitDependencies()
{
    for (auto& [name, node] : dependencyNodes_)
        node->depth_ = ea::nullopt;
    dependencyRoot_->depth_ = 0;

    // Grows during iteration!
    ea::vector<DependencyGraphNodePtr> processQueue{dependencyRoot_};
    for (unsigned i = 0; i < processQueue.size(); ++i)
    {
        const DependencyGraphNode& parentNode = *processQueue[i];
        for (const auto& childNode : parentNode.dependencies_)
        {
            childNode->depth_ = ea::max(*parentNode.depth_ + 1, childNode->depth_.value_or(0u));
            processQueue.push_back(childNode);
        }
    }

    ea::vector<DependencyGraphNodePtr> orderedNodes = dependencyNodes_.values();
    ea::sort(orderedNodes.begin(), orderedNodes.end(), [](const DependencyGraphNodePtr& lhs, const DependencyGraphNodePtr& rhs)
    {
        URHO3D_ASSERT(lhs->depth_ && rhs->depth_);
        return *lhs->depth_ > *rhs->depth_;
    });

    dependencyOrder_.clear();
    for (unsigned i = 0; i < orderedNodes.size(); ++i)
    {
        const DependencyGraphNode& node = *orderedNodes[i];
        if (!node.name_.empty())
            dependencyOrder_.emplace(StringHash{node.name_}, i + 1);
    }
}

AssetTransformerVector AssetTransformerHierarchy::GetTransformerCandidates(const ea::string& resourcePath) const
{
    return GetTransformerCandidates(resourcePath, ApplicationFlavor::Universal);
}

AssetTransformerVector AssetTransformerHierarchy::GetTransformerCandidates(
    const ea::string& resourcePath, const ApplicationFlavor& flavor) const
{
    TransformerInfoVector result;
    root_.IterateNodesChildFirst(resourcePath.split('/'), [&](unsigned inverseDepth, const TreeNode& node)
    {
        for (AssetTransformer* transformer : node.transformers_)
        {
            if (auto flavorMatchPenalty = flavor.Matches(transformer->GetFlavor()))
                result.push_back(TransformerInfo{transformer, GetTypeOrder(transformer->GetType()), inverseDepth, *flavorMatchPenalty});
        }
    });

    ea::stable_sort(result.begin(), result.end());
    FilterDuplicates(result);
    return Reduce(result);
}

AssetTransformerHierarchy::TreeNode& AssetTransformerHierarchy::TreeNode::GetOrCreateChild(const ea::string& name)
{
    const auto iter = children_.find_as(name, TreeNode::ByName{});
    if (iter != children_.end())
        return **iter;

    return **children_.insert(ea::make_unique<TreeNode>(name));
}

AssetTransformerHierarchy::TreeNode& AssetTransformerHierarchy::TreeNode::GetOrCreateChild(ea::span<const ea::string> path)
{
    if (path.empty())
        return *this;

    return GetOrCreateChild(path[0]).GetOrCreateChild(path.subspan(1));
}

const AssetTransformerHierarchy::TreeNode* AssetTransformerHierarchy::TreeNode::GetChild(const ea::string& name) const
{
    const auto iter = children_.find_as(name, TreeNode::ByName{});
    if (iter != children_.end())
        return iter->get();
    return nullptr;
}

template <class T>
void AssetTransformerHierarchy::TreeNode::Iterate(const T& callback)
{
    callback(*this);
    for (auto& child : children_)
        child->Iterate(callback);
}

template <class T>
void AssetTransformerHierarchy::TreeNode::IterateNodesChildFirst(ea::span<const ea::string> path, const T& callback) const
{
    if (!path.empty())
    {
        if (const TreeNode* child = GetChild(path[0]))
            child->IterateNodesChildFirst(path.subspan(1), callback);
    }

    callback(path.size(), *this);
}

}
