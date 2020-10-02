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

#include "../Scene/Component.h"

namespace Rml { class ElementDocument; }

namespace Urho3D
{

struct RmlCanvasResizedArgs;
struct RmlDocumentReloadedArgs;

/// Adds a single window to game screen.
class RmlUIComponent : public Component
{
    URHO3D_OBJECT(RmlUIComponent, Component);
public:
    /// Construct.
    explicit RmlUIComponent(Context* context);
    /// Destruct.
    ~RmlUIComponent() override;

    /// Registers object with the engine.
    static void RegisterObject(Context* context);

    /// Set resource path of rml file defining a window.
    void SetResource(const ResourceRef& resource);
    /// Returns a path to a rml file defining a window.
    const ResourceRef& GetResource() const { return resource_; }
    /// Returns true if window is open, false otherwise. May return true when component is detached from a node and no window is open.
    bool IsOpen() const { return open_; }
    /// Set whether window opens as soon as component ias added to an object.
    void SetOpen(bool open);
    /// Return true if component is using normalized coordinates for window position and size.
    bool GetUseNormalizedCoordinates() const { return useNormalized_; }
    /// Enable or disable use of normalized coordinates for window position and size.
    void SetUseNormalizedCoordinates(bool enable) { useNormalized_ = enable; }
    /// Returns window position in pixels or normalized coordinates.
    Vector2 GetPosition() const;
    /// Sets window position in pixels or normalized coordinates.
    void SetPosition(Vector2 pos);
    /// Returns window size in pixels or normalized coordinates.
    Vector2 GetSize() const;
    /// Sets window size in pixels or normalized coordinates.
    void SetSize(Vector2 size);

protected:
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* node) override;
    /// Open a window document if it was not already open.
    void OpenInternal();
    /// Close a window document if it was open.
    void CloseInternal();
    /// Resets document_ pointer when window is closed.
    void OnDocumentClosed(Rml::ElementDocument*& document);
    /// Reposition UI elements on UI canvas resize.
    void OnUICanvasResized(RmlCanvasResizedArgs& size);
    /// Handle document pointer changes on resource reload.
    void OnDocumentReloaded(RmlDocumentReloadedArgs& args);
    /// Update position and size when toggling use of normalized coordinates.
    void OnUseNormalizedCoordinates();

    /// A rml file resource.
    ResourceRef resource_;
    /// Flag indicating that window will open as soon as component is added to an object.
    bool open_ = false;
    /// Currently open document. Null if document was closed.
    Rml::ElementDocument* document_ = nullptr;
    /// Flag indicating that component will save normalized coordiantes. When not set, component will save pixel coordinates.
    bool useNormalized_ = false;
    /// Used to store size when document is not available.
    Vector2 size_;
    /// Used to store position when document is not available.
    Vector2 position_;
};

}
