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


#include "ToolboxAPI.h"
#include <Urho3D/Core/Object.h>


namespace Urho3D
{

class Camera;
class Context;
class Scene;
class Texture2D;
class Viewport;
class Node;

class URHO3D_TOOLBOX_API SceneView
{
public:
    /// Construct.
    explicit SceneView(Context* context, const IntRect& rect);
    /// Destruct.
    ~SceneView() = default;
    /// Set screen rectangle where scene is being rendered.
    virtual void SetSize(const IntRect& rect);
    /// Return scene debug camera component.
    Camera* GetCamera() const;
    /// Return scene rendered in this tab.
    Scene* GetScene() const { return scene_; }
    /// Return scene viewport instance.
    Viewport* GetViewport() const { return viewport_; }
    /// Return texture to which view is rendered to.
    Texture2D* GetTexture() const { return texture_; }
    /// Creates scene camera and other objects required by editor.
    virtual void CreateObjects();

protected:
    /// Rectangle dimensions that are rendered by this view.
    IntRect rect_;
    /// Scene which is rendered by this view.
    SharedPtr<Scene> scene_;
    /// Texture to which scene is rendered.
    SharedPtr<Texture2D> texture_;
    /// Viewport which defines rendering area.
    SharedPtr<Viewport> viewport_;
};

}
