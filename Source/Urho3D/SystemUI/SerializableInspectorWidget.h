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

struct SerializableHookContext
{
    const WeakSerializableVector* objects_{};
    const AttributeInfo* info_{};
    bool isUndefined_{};
    bool isDefaultValue_{};
};

using SerializableHookKey = ea::pair<ea::string /*class*/, ea::string /*attribute*/>;
using SerializableHookFunction = ea::function<bool(const SerializableHookContext& ctx, Variant& boxedValue)>;

/// SystemUI widget used to edit materials.
class URHO3D_API SerializableInspectorWidget : public Object
{
    URHO3D_OBJECT(SerializableInspectorWidget, Object);

public:
    static void RegisterHook(const SerializableHookKey& key, const SerializableHookFunction& function);
    static void UnregisterHook(const SerializableHookKey& key);
    static const SerializableHookFunction& GetHook(const SerializableHookKey& key);

    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditAttributeBegin;
    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditAttributeEnd;
    Signal<void(const WeakSerializableVector& objects)> OnActionBegin;
    Signal<void(const WeakSerializableVector& objects)> OnActionEnd;

    SerializableInspectorWidget(Context* context, const WeakSerializableVector& objects);
    ~SerializableInspectorWidget() override;

    void RenderTitle();
    void RenderContent();

    ea::string GetTitle();
    const WeakSerializableVector& GetObjects() const { return objects_; }

private:
    void PruneObjects();
    void RenderAttribute(const AttributeInfo& info);
    void RenderAction(const AttributeInfo& info);

    WeakSerializableVector objects_;
    ea::vector<ea::pair<const AttributeInfo*, Variant>> pendingSetAttributes_;
    ea::vector<const AttributeInfo*> pendingActions_;
};

}
