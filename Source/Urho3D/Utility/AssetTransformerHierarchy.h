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

#pragma once

#include "../Utility/AssetTransformer.h"

#include <EASTL/optional.h>
#include <EASTL/vector_multiset.h>

namespace Urho3D
{

/// Container for a hierarchy of AssetTransformer instances.
/// Inner (more specific) transformers are preferred to outer (more generic) transformers.
class URHO3D_API AssetTransformerHierarchy : public Object
{
    URHO3D_OBJECT(AssetTransformerHierarchy, Object);

public:
    explicit AssetTransformerHierarchy(Context* context);

    /// Clear all cached transformers and dependencies.
    void Clear();
    /// Add new transformer.
    void AddTransformer(const ea::string& path, AssetTransformer* transformer);
    /// Remove all transformers of specified type.
    bool RemoveTransformers(const TypeInfo* typeInfo);
    /// Add dependency between transformer types.
    bool AddDependency(const ea::string& transformerClass, const ea::string& dependsOn);
    /// Should be called after all dependencies are added.
    void CommitDependencies();

    /// Enumerate all transformers for given path and any flavor.
    AssetTransformerVector GetTransformerCandidates(const ea::string& resourcePath) const;
    /// Enumerate all transformers for given path and flavor.
    AssetTransformerVector GetTransformerCandidates(
        const ea::string& resourcePath, const ApplicationFlavor& flavor) const;

private:
    struct DependencyGraphNode
    {
        ea::optional<unsigned> depth_;
        ea::string name_;
        ea::vector<ea::shared_ptr<DependencyGraphNode>> dependencies_;
    };

    using DependencyGraphNodePtr = ea::shared_ptr<DependencyGraphNode>;

    struct TreeNode
    {
        struct ByName
        {
            bool operator()(const ea::unique_ptr<TreeNode>& lhs, const ea::unique_ptr<TreeNode>& rhs) const;
            bool operator()(const ea::string& lhs, const ea::unique_ptr<TreeNode>& rhs) const;
            bool operator()(const ea::unique_ptr<TreeNode>& lhs, const ea::string& rhs) const;
        };

        TreeNode& GetOrCreateChild(const ea::string& name);
        TreeNode& GetOrCreateChild(ea::span<const ea::string> path);
        const TreeNode* GetChild(const ea::string& name) const;

        template <class T>
        void Iterate(const T& callback);
        template <class T>
        void IterateNodesChildFirst(ea::span<const ea::string> path, const T& callback) const;

        TreeNode() = default;
        explicit TreeNode(const ea::string& name) : name_(name) {}

        ea::string name_;
        ea::vector<SharedPtr<AssetTransformer>> transformers_;
        ea::vector_multiset<ea::unique_ptr<TreeNode>, ByName> children_;
    };

    DependencyGraphNodePtr GetOrCreateDependencyNode(const ea::string& name);
    bool DependsOn(const DependencyGraphNode& queryNode, const DependencyGraphNode& dependencyNode) const;
    unsigned GetTypeOrder(StringHash type) const;

    TreeNode root_;

    DependencyGraphNodePtr dependencyRoot_;
    ea::unordered_map<ea::string, DependencyGraphNodePtr> dependencyNodes_;
    ea::unordered_map<StringHash, unsigned> dependencyOrder_;
};

}
