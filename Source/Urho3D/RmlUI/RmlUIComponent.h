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

    /// Returns a path to a rml file defining a window.
    const ResourceRef& GetWindowResource() const { return windowResource_; }
    /// Returns true if window is open, false otherwise. May return true when component is detached from a node and no window is open.
    bool IsOpen() const { return open_; }
    /// Set whether window opens as soon as component ias added to an object.
    void SetOpen(bool open);

protected:
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* node) override;
    /// Open a window document if it was not already open.
    void OpenInternal();
    /// Close a window document if it was open.
    void CloseInternal();
    /// Resets document_ pointer when window is closed.
    void OnDocumentClosed(StringHash, VariantMap& args);
    /// Handle UI opening and closing when component state changes.
    void UpdateWindowState();

    /// A rml file resource.
    ResourceRef windowResource_;
    /// Flag indicating that window will open as soon as component is added to an object.
    bool open_ = false;
    /// Currently open document. Null if document was closed.
    Rml::ElementDocument* document_ = nullptr;
};

}
