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

#pragma once

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Scene/NodePrefab.h>

namespace Urho3D
{

class Node;

/// Prefab resource.
/// Constains representation of nodes and components with attributes, ready to be instantiated.
class URHO3D_API PrefabResource : public SimpleResource
{
    URHO3D_OBJECT(PrefabResource, SimpleResource)

public:
    explicit PrefabResource(Context* context);
    ~PrefabResource() override;

    static void RegisterObject(Context* context);

    /// Instantiate prefab into a scene or node as PrefabReference.
    /// If inplace, the prefab will be instantiated into the parent node directly, otherwise a new node will be created.
    Node* InstantiateReference(Node* parentNode, bool inplace = false);

    void NormalizeIds();

    void SerializeInBlock(Archive& archive) override;

    const NodePrefab& GetScenePrefab() const { return prefab_; }
    NodePrefab& GetMutableScenePrefab() { return prefab_; }

    const NodePrefab& GetNodePrefab() const;
    NodePrefab& GetMutableNodePrefab();

    const NodePrefab& GetNodePrefabSlice(ea::string_view path) const;

private:
    bool LoadLegacyXML(const XMLElement& source) override;

    NodePrefab prefab_;
};

} // namespace Urho3D
