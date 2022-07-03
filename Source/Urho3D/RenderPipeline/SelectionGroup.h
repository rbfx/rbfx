//
// Copyright (c) 2022-2022 the rbfx project.
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
#include "../Scene/Node.h"
#include "../Graphics/Drawable.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Group of selected drawables.
class URHO3D_API SelectionGroup : public Component
{
    URHO3D_OBJECT(SelectionGroup, Component);

public:
    SelectionGroup(Context* context);
    ~SelectionGroup() override;

    static void RegisterObject(Context* context);

    void SetSelectionColor(const Color& color) { color_ = color; }
    const Color& GetSelectionColor() const { return color_; }

    bool Empty() const { return selected_.empty(); }
    void Clear();
    void Add(Drawable* drawable);
    void Remove(Drawable* drawable);

    const ea::unordered_set<WeakPtr<Drawable>>& GetDrawables() const { return selected_; }

private:
    /// Set of selected nodes.
    ea::unordered_set<WeakPtr<Drawable>> selected_;

    /// Selection color.
    Color color_{Color::GREEN};
};

}
