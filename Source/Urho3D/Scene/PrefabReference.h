//
// Copyright (c) 2022-2022 the rbfx project.
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

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/PrefabResource.h>

namespace Urho3D
{

enum class PrefabInlineFlag
{
    None = 0,
    /// Whether to keep *other* components and children temporary.
    /// Components and children that are part of the prefab are always converted to persistent.
    /// This flag controls how to handle temporary components and children that may have been created after prefab
    /// instantiation.
    KeepOtherTemporary = 1 << 0,
};
URHO3D_FLAGSET(PrefabInlineFlag, PrefabInlineFlags);

/// Controls which attributes of the top-level node of the prefab are copied to the scene node
/// containing PrefabReference. By default, none are copied.
enum class PrefabInstanceFlag
{
    None = 0,
    UpdateName = 1 << 0,
    UpdateTags = 1 << 1,
    UpdatePosition = 1 << 2,
    UpdateRotation = 1 << 3,
    UpdateScale = 1 << 4,
    UpdateVariables = 1 << 5,

    UpdateAll = 0x7fffffff
};
URHO3D_FLAGSET(PrefabInstanceFlag, PrefabInstanceFlags);

/// Component that instantiates prefab resource into the parent Node.
class URHO3D_API PrefabReference : public Component
{
    URHO3D_OBJECT(PrefabReference, Component)

public:
    explicit PrefabReference(Context* context);
    ~PrefabReference() override;

    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;

    /// Attributes.
    /// @{
    void SetPrefab(PrefabResource* prefab, ea::string_view path = {}, bool createInstance = true,
        PrefabInstanceFlags instanceFlags = PrefabInstanceFlag::None);
    PrefabResource* GetPrefab() const { return prefab_; }
    void SetPath(ea::string_view path);
    const ea::string& GetPath() const { return path_; }
    void SetPrefabAttr(const ResourceRef& prefab);
    const ResourceRef& GetPrefabAttr() const { return prefabRef_; }
    /// @}

    /// Make all prefab nodes not temporary and remove component.
    void Inline(PrefabInlineFlags flags);
    void InlineConservative() { Inline(PrefabInlineFlag::KeepOtherTemporary); }
    void InlineAggressive() { Inline(PrefabInlineFlag::None); }

    /// Commit prefab changes to the resource.
    void CommitChanges();

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

private:
    const NodePrefab& GetNodePrefab() const;

    bool IsInstanceMatching(const Node* node, const NodePrefab& nodePrefab, bool temporaryOnly) const;
    bool AreComponentsMatching(
        const Node* node, const ea::vector<SerializablePrefab>& componentPrefabs, bool temporaryOnly) const;
    bool AreChildrenMatching(const Node* node, const ea::vector<NodePrefab>& childPrefabs, bool temporaryOnly) const;

    void ExportInstance(Node* node, const NodePrefab& nodePrefab, bool temporaryOnly) const;
    void ExportComponents(Node* node, const ea::vector<SerializablePrefab>& componentPrefabs, bool temporaryOnly) const;
    void ExportChildren(Node* node, const ea::vector<NodePrefab>& childPrefabs, bool temporaryOnly) const;

    void RemoveTemporaryComponents(Node* node) const;
    void RemoveTemporaryChildren(Node* node) const;
    void InstantiatePrefab(const NodePrefab& nodePrefab, PrefabInstanceFlags instanceFlags);

    void MarkPrefabDirty() { prefabDirty_ = true; }

    /// Try to create instance without spawning any new nodes or components.
    /// It may cause some nodes or components to remain if prefab is different.
    /// So, this method is not completely reliable, but it's the best we can do.
    /// It's the Editor only issue anyway.
    bool TryCreateInplace();
    /// Remove prefab instance. Removes all children and components except this PrefabReference.
    void RemoveInstance();
    /// Create prefab instance. Spawns all nodes and components in the prefab.
    /// Removes all existing children and components except this PrefabReference.
    void CreateInstance(bool tryInplace = false, PrefabInstanceFlags instanceFlags = PrefabInstanceFlag::None);

    SharedPtr<PrefabResource> prefab_;
    ResourceRef prefabRef_;
    ea::string path_;

    bool prefabDirty_{};

    /// Node that is used to instance prefab.
    /// It is usually the same as the parent node, but can be different if PrefabReference is moved between nodes.
    WeakPtr<Node> instanceNode_;
    unsigned numInstanceComponents_{};
    unsigned numInstanceChildren_{};
};

} // namespace Urho3D
