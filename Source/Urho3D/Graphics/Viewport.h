//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Core/Object.h"
#include "../Container/Ptr.h"
#include "../Math/Ray.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"

namespace Urho3D
{

class Camera;
class RenderPath;
class Scene;
class XMLFile;
class View;
class RenderPipelineView;
class RenderSurface;
class RenderPipeline;

/// %Viewport definition either for a render surface or the backbuffer.
class URHO3D_API Viewport : public Object
{
    URHO3D_OBJECT(Viewport, Object);

public:
    /// Construct with defaults.
    explicit Viewport(Context* context);
    /// Construct with a full rectangle.
    Viewport(Context* context, Scene* scene, Camera* camera, RenderPath* renderPath = nullptr);
    /// Construct with a specified rectangle.
    Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath = nullptr);
    /// Construct with a specified rectangle and render pipeline.
    Viewport(Context* context, Scene* scene, Camera* camera, const IntRect& rect, RenderPipeline* renderPipeline);
    /// Destruct.
    ~Viewport() override;

    /// Register object with the engine.
    static void RegisterObject(Context* context);

    /// Set scene.
    /// @property
    void SetScene(Scene* scene);
    /// Set viewport camera.
    /// @property
    void SetCamera(Camera* camera);
    /// Set view rectangle. A zero rectangle (0 0 0 0) means to use the rendertarget's full dimensions.
    /// @property
    void SetRect(const IntRect& rect);
    /// Set rendering path.
    /// @property
    void SetRenderPath(RenderPath* renderPath);
    /// Set rendering path from an XML file.
    bool SetRenderPath(XMLFile* file);
    /// Set whether to render debug geometry. Default true.
    /// @property
    void SetDrawDebug(bool enable);
    /// Set separate camera to use for culling. Sharing a culling camera between several viewports allows to prepare the view only once, saving in CPU use. The culling camera's frustum should cover all the viewport cameras' frusta or else objects may be missing from the rendered view.
    /// @property
    void SetCullCamera(Camera* camera);

    /// Return scene.
    /// @property
    Scene* GetScene() const;
    /// Return viewport camera.
    /// @property
    Camera* GetCamera() const;
    /// Return the internal rendering structure. May be null if the viewport has not been rendered yet.
    View* GetView() const;
    /// Return render pipeline.
    RenderPipelineView* GetRenderPipelineView() const;

    /// Return view rectangle. A zero rectangle (0 0 0 0) means to use the rendertarget's full dimensions. In this case you could fetch the actual view rectangle from View object, though it will be valid only after the first frame.
    /// @property
    const IntRect& GetRect() const { return rect_; }

    /// Return effective view rectangle.
    IntRect GetEffectiveRect(RenderSurface* renderTarget) const;

    /// Return rendering path.
    /// @property
    RenderPath* GetRenderPath() const;

    /// Return whether to draw debug geometry.
    /// @property
    bool GetDrawDebug() const { return drawDebug_; }

    /// Return the culling camera. If null, the viewport camera will be used for culling (normal case).
    /// @property
    Camera* GetCullCamera() const;

    /// Return ray corresponding to normalized screen coordinates.
    Ray GetScreenRay(int x, int y) const;
    /// Convert a world space point to normalized screen coordinates.
    IntVector2 WorldToScreenPoint(const Vector3& worldPos) const;
    /// Convert screen coordinates and depth to a world space point.
    Vector3 ScreenToWorldPoint(int x, int y, float depth) const;

    /// Allocate the view structure. Called by Renderer.
    void AllocateView();

private:
    /// Scene pointer.
    WeakPtr<Scene> scene_;
    /// Camera pointer.
    WeakPtr<Camera> camera_;
    /// Culling camera pointer.
    WeakPtr<Camera> cullCamera_;
    /// Viewport rectangle.
    IntRect rect_;
    /// Rendering path.
    SharedPtr<RenderPath> renderPath_;
    /// Internal rendering structure.
    SharedPtr<View> view_;
    /// Debug draw flag.
    bool drawDebug_;

    /// Whether to search for render pipeline automatically.
    bool autoRenderPipeline_{true};
    /// Render pipeline component from scene_.
    WeakPtr<RenderPipeline> renderPipeline_;
    /// Instance of render pipeline connected to renderPipeline_.
    SharedPtr<RenderPipelineView> renderPipelineView_;
};

}
