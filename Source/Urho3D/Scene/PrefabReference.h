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

#include "../Scene/Component.h"
#include "../Resource/XMLFile.h"

namespace Urho3D
{
/// %Prefab resource.
class URHO3D_API PrefabReference : public Component
{
    URHO3D_OBJECT(PrefabReference, Component)

public:
    /// Construct empty.
    explicit PrefabReference(Context* context);
    /// Destruct.
    ~PrefabReference() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set prefab resource.
    void SetPrefab(XMLFile* prefab);
    /// Get prefab resource.
    XMLFile* GetPrefab() const;

    /// Set flag to preserve prefab root node transform.
    void SetPreserveTransfrom(bool preserve);
    /// Get preserve prefab root node transform flag state.
    bool GetPreserveTransfrom() const { return _preserveTransform; }

    /// Set reference to prefab resource.
    void SetPrefabAttr(ResourceRef prefab);
    /// Get reference to prefab resource.
    ResourceRef GetPrefabAttr() const;

    /// Make all prefab nodes not temporary and remove component.
    void Inline();

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Get prefab instance root node if created.
    /// The node may not be yet created if the component is disabled or is not attached to a Node.
    Node* GetRootNode() const;

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* node) override;
private:
    /// Handle prefab reloaded.
    void HandlePrefabReloaded(StringHash eventType, VariantMap& map);
    /// Create or remove prefab based on current environment.
    void ToggleNode(bool forceReload = false);
    /// Create prefab instance.
    Node* CreateInstance() const;

    /// Prefab root node.
    SharedPtr<Node> node_;
    /// Prefab resource.
    SharedPtr<XMLFile> prefab_;
    /// Reference to prefab resource.
    ResourceRef prefabRef_;
    /// Preserve prefab root node transform.
    bool _preserveTransform{false};
};

} // namespace Urho3D
