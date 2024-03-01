//
// Copyright (c) 2024-2024 the Urho3D project.
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

#include "../Scene/Component.h"

#include <phonon.h>

namespace Urho3D
{

class SteamAudio;
class Model;

enum class Material {
    generic,
    brick,
    concrete,
    ceramic,
    gravel,
    carpet,
    glass,
    plaster,
    wood,
    metal,
    rock
};

/// %Sound mesh component. Needs to be placed next to a StaticMesh component.
class URHO3D_API SteamSoundMesh : public Component
{
    URHO3D_OBJECT(SteamSoundMesh, Component);

public:
    /// Construct.
    explicit SteamSoundMesh(Context* context);
    /// Destruct. Remove self from the audio subsystem.
    ~SteamSoundMesh() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set model to use.
    void SetModel(const ResourceRef& model);
    /// Set material to use.
    void SetMaterial(Material material);

    /// Returns currently used model.
    ResourceRef GetModel() const;
    /// Returns currently used material.
    Material GetMaterial() const { return materialIndex_; }

private:
    /// Handle node being assigned.
    virtual void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// Handle transform change.
    void OnMarkedDirty(Node *) override;

    /// Reload current model.
    void ReloadModel();
    /// Reset (clear) current model.
    void ResetModel();
    /// Update transform.
    void UpdateTransform();
    /// Return phonon matrix of node
    IPLMatrix4x4 GetPhononMatrix() const;

    /// Currently used model.
    SharedPtr<Model> model_;
    /// Material index.
    Material materialIndex_;
    /// Material.
    IPLMaterial *material_;
    /// Mesh.
    IPLStaticMesh mesh_;
    /// Subscene loaded into main scene.
    IPLScene subScene_;
    /// Instanced mesh for sub scene
    IPLInstancedMesh instancedMesh_;
    /// Steam audio subsystem.
    WeakPtr<SteamAudio> audio_;
};

}
