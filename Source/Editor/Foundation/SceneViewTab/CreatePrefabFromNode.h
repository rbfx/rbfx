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

#include "../../Foundation/SceneViewTab.h"
#include "../../Project/ResourceFactory.h"

#include <Urho3D/Scene/PrefabResource.h>

namespace Urho3D
{

void Foundation_CreatePrefabFromNode(Context* context, SceneViewTab* sceneViewTab);

/// Intermediate class for request processing.
class PrefabFromNodeFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(PrefabFromNodeFactory, BaseResourceFactory);

public:
    using WeakNodeVector = ea::vector<WeakPtr<Node>>;

    explicit PrefabFromNodeFactory(Context* context);
    void Setup(SceneViewTab* tab, const WeakNodeVector& nodes);

    /// Implement ResourceFactory.
    /// @{
    ea::string GetDefaultFileName() const override;
    bool IsFileNameEditable() const override;
    void Render(const FileNameChecker& checker, bool& canCommit, bool& shouldCommit) override;
    void RenderAuxilary() override;
    void CommitAndClose() override;
    /// @}

private:
    NodePrefab CreatePrefabBase() const;
    NodePrefab CreatePrefabFromNode(Node* node) const;

    ea::string FindBestFileName(Node* node, const ea::string& filePath) const;
    void SaveNodeAsPrefab(Node* node, const ea::string& resourceName, const ea::string& fileName);

    WeakPtr<SceneViewTab> tab_;
    WeakNodeVector nodes_;
    SharedPtr<PrefabResource> prefab_;
    bool replaceWithReference_{true};
};

/// Addon to manage scene selection with mouse and render debug geometry.
class CreatePrefabFromNode : public SceneViewAddon
{
    URHO3D_OBJECT(CreatePrefabFromNode, SceneViewAddon);

public:
    explicit CreatePrefabFromNode(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "CreatePrefabFrom"; }
    /// @}

private:
    void RenderMenu(SceneViewPage& page, Scene* scene, SceneSelection& selection);
    void CreatePrefabs(const SceneSelection& selection);

    SharedPtr<PrefabFromNodeFactory> factory_;
};

}
