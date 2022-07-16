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

#include "../Core/Signal.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{

#if URHO3D_SYSTEMUI

/// SystemUI widget used to edit materials.
class URHO3D_API SerializableInspectorWidget : public Object
{
    URHO3D_OBJECT(SerializableInspectorWidget, Object);

public:
    Signal<void()> OnEditBegin;
    Signal<void()> OnEditEnd;

    using SerializableVector = ea::vector<WeakPtr<Serializable>>;

    SerializableInspectorWidget(Context* context, const SerializableVector& objects);
    ~SerializableInspectorWidget() override;

    void RenderTitle();
    void RenderContent();

    const SerializableVector& GetObjects() const { return objects_; }

private:
    void RenderAttribute(const AttributeInfo& info);

    SerializableVector objects_;
    ea::vector<ea::pair<const AttributeInfo*, Variant>> pendingSetAttributes_;
};

#endif

}
