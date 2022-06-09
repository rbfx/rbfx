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

/// \file

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
    bool textureDirty_{true};
    bool isActive_{};
    IntVector2 textureSize_;
    SharedPtr<Texture2D> texture_;
};

/// Renders scene to texture with its own camera.
// TODO(editor): Remove inheritance
class URHO3D_API SceneRendererToTexture : public CustomBackbufferTexture
{
    URHO3D_OBJECT(SceneRendererToTexture, CustomBackbufferTexture);

public:
    explicit SceneRendererToTexture(Scene* scene);
    ~SceneRendererToTexture() override;

    /// Return properties
    /// @{
    Camera* GetCamera() const { return camera_; }
    Node* GetCameraNode() const { return cameraNode_; }
    Vector3 GetCameraPosition() const;
    Quaternion GetCameraRotation() const;
    /// @}

private:
    void SetupViewport(RenderSurface* renderSurface);

    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    Camera* camera_{};

    SharedPtr<Viewport> viewport_;
};

}
