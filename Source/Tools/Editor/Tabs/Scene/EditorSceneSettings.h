//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Resource/XMLElement.h>


namespace Urho3D
{

/// Class handling common scene settings
class EditorSceneSettings : public Component
{
    URHO3D_OBJECT(EditorSceneSettings, Component);
public:
    /// Construct
    explicit EditorSceneSettings(Context* context);
    /// Register object with engine.
    static void RegisterObject(Context* context);

    /// Returns configured editor viewport renderpath.
    ResourceRef GetEditorViewportRenderPath() const { return editorViewportRenderPath_; }
    /// Sets current editor scene view renderpath.
    void SetEditorViewportRenderPath(const ResourceRef& renderPath);

    ///
    Vector3 GetCameraPosition() const;
    ///
    void SetCameraPosition(const Vector3& position);
    ///
    float GetCameraOrthoSize() const;
    ///
    void SetCameraOrthoSize(float size);
    ///
    float GetCameraZoom() const;
    ///
    void SetCameraZoom(float zoom);
    ///
    bool GetCamera2D() const;
    ///
    void SetCamera2D(bool is2D);
    ///
    Quaternion GetCameraRotation() const;
    ///
    void SetCameraRotation(const Quaternion& rotation);

protected:
    ///
    void OnSceneSet(Scene* scene) override;
    ///
    Node* GetCameraNode();
    ///
    Node* GetCameraNode() const;

    /// Flag indicating that editor scene viewport should use PBR renderpath.
    ResourceRef editorViewportRenderPath_;
    ///
    bool is2D_ = false;
};

}

