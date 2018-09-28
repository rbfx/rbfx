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


#include "Tabs/Tab.h"


namespace Urho3D
{

class PreviewTab : public Tab
{
    URHO3D_OBJECT(PreviewTab, Tab)
public:
    explicit PreviewTab(Context* context);

    bool RenderWindowContent() override;
    ///
    void SetPreviewScene(Scene* scene);
    /// Set color of view texture to black.
    void Clear();

protected:
    ///
    IntRect UpdateViewRect() override;
    ///
    void UpdateViewports();
    ///
    void OnComponentUpdated(Component* component);

    /// Last view rectangle.
    IntRect viewRect_{};
    /// Texture used to display preview.
    SharedPtr<Texture2D> view_{};
    /// Scene that is being previewed.
    WeakPtr<Scene> scene_{};
};

}
