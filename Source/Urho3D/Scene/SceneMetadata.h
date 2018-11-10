//
// Copyright (c) 2018 Rokas Kupstys
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
#include "../Scene/CameraViewport.h"


namespace Urho3D
{

class RenderSurface;

/// Listens on various engine events and gathers useful information about scene. This component is used by internal tools.
class URHO3D_API SceneMetadata : public Component
{
    URHO3D_OBJECT(SceneMetadata, Component);
public:
    /// Construct.
    explicit SceneMetadata(Context* context);
    /// Store component.
    void RegisterComponent(Component* component);
    /// Remove stored component.
    void UnregisterComponent(Component* component);

    /// Returns a list of existing CameraViewport components.
    const Vector<WeakPtr<CameraViewport>>& GetCameraViewportComponents() const { return viewportComponents_; }

    /// Register object with the engine.
    static void RegisterObject(Context* context);

protected:
    /// A list of components.
    Vector<WeakPtr<CameraViewport>> viewportComponents_;
};

}
