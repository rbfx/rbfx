// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../Core/Signal.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

class Camera;
class Node;
class RenderSurface;
class Scene;
class Texture2D;
class Viewport;

/// Maintains texture usable as custom backbuffer.
class URHO3D_API CustomBackbufferTexture : public Object
{
    URHO3D_OBJECT(CustomBackbufferTexture, Object);

public:
    Signal<void(RenderSurface*)> OnRenderSurfaceCreated;

    explicit CustomBackbufferTexture(Context* context);
    ~CustomBackbufferTexture() override;

    /// Resize output texture.
    void SetTextureSize(const IntVector2& size);
    /// Set whether to update texture every frame.
    void SetActive(bool active);
    /// Periodical update.
    void Update();

    /// Return properties
    /// @{
    Texture2D* GetTexture() const { return texture_; }
    const IntVector2& GetTextureSize() const { return textureSize_; }
    bool IsActive() const { return isActive_; }
    /// @}

private:
    bool textureDirty_{false};
    bool isActive_{};
    IntVector2 textureSize_;
    SharedPtr<Texture2D> texture_;
};

/// Renders scene to texture with its own camera.
class URHO3D_API SceneRendererToTexture : public CustomBackbufferTexture
{
    URHO3D_OBJECT(SceneRendererToTexture, CustomBackbufferTexture);

public:
    explicit SceneRendererToTexture(Scene* scene);
    ~SceneRendererToTexture() override;

    /// Return properties
    /// @{
    Scene* GetScene() const { return scene_; }
    Camera* GetCamera() const { return camera_; }
    Node* GetCameraNode() const { return cameraNode_; }
    Vector3 GetCameraPosition() const;
    Quaternion GetCameraRotation() const;
    /// @}

private:
    void SetupViewport(RenderSurface* renderSurface);

    WeakPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    Camera* camera_{};

    SharedPtr<Viewport> viewport_;
};

}
