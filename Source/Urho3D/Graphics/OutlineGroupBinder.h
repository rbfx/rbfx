//
// Copyright (c) 2022-2024 the rbfx project.
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

namespace Urho3D
{

class Drawable;
class OutlineGroup;

/// Add this to a node with a drawable to bind it to a non-debug OutlineGroup in the same scene.
class URHO3D_API OutlineGroupBinder : public Component
{
    URHO3D_OBJECT(OutlineGroupBinder, Component);

public:
    OutlineGroupBinder(Context* context);
    ~OutlineGroupBinder() override;

    static void RegisterObject(Context* context);

protected:
    void OnSetEnabled() override;
    void OnSceneSet(Scene* scene) override;

private:
    void Bind(Scene* scene);
    void Unbind();

    WeakPtr<Drawable> drawable_;
    WeakPtr<OutlineGroup> outlineGroup_;
};

}
