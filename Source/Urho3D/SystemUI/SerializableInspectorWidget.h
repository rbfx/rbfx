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

struct AttributeHookContext
{
    const WeakSerializableVector* objects_{};
    const AttributeInfo* info_{};
    bool isUndefined_{};
    bool isDefaultValue_{};
};

enum class ObjectHookType
{
    /// Rendered before default attributes rendering.
    Prepend,
    /// Rendered after default attributes rendering.
    Append,
    /// Rendered instead of default attributes rendering.
    Replace,
};

using AttributeHookKey = ea::pair<ea::string /*class*/, ea::string /*attribute*/>;
using AttributeHookFunction = ea::function<bool(const AttributeHookContext& ctx, Variant& boxedValue)>;
using ObjectHookKey = ea::pair<ea::string /*class*/, ObjectHookType /*type*/>;
using ObjectHookFunction = ea::function<void(const WeakSerializableVector& objects)>;

/// SystemUI widget used to edit materials.
class URHO3D_API SerializableInspectorWidget : public Object
{
    URHO3D_OBJECT(SerializableInspectorWidget, Object);

public:
    /// Hooks used to customize attribute rendering.
    /// @{
    static void RegisterAttributeHook(const AttributeHookKey& key, const AttributeHookFunction& function);
    static void UnregisterAttributeHook(const AttributeHookKey& key);
    static const AttributeHookFunction& GetAttributeHook(const AttributeHookKey& key);
    static void CopyAttributeHook(const AttributeHookKey& from, const AttributeHookKey& to);
    /// @}

    /// Hooks used to customize object rendering.
    /// TODO: Object hooks cannot change object's attributes (yet)
    /// @{
    static void RegisterObjectHook(const ObjectHookKey& key, const ObjectHookFunction& function);
    static void UnregisterObjectHook(const ObjectHookKey& key);
    static const ObjectHookFunction& GetObjectHook(const ObjectHookKey& key);
    static void CopyObjectHook(const ObjectHookKey& from, const ObjectHookKey& to);
    /// @}

    /// Template syntax sugar for hooks.
    /// @{
    template <class T> static void RegisterAttributeHook(const ea::string& name, const AttributeHookFunction& function);
    template <class T> static void UnregisterAttributeHook(const ea::string& name);
    template <class T, class U> static void CopyAttributeHook(const ea::string& name);

    template <class T> static void RegisterObjectHook(ObjectHookType type, const ObjectHookFunction& function);
    template <class T> static void UnregisterObjectHook();
    template <class T, class U> static void CopyObjectHook();
    /// @}

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
    void RenderObjects();

    WeakSerializableVector objects_;
    ea::vector<ea::pair<const AttributeInfo*, Variant>> pendingSetAttributes_;
    ea::vector<const AttributeInfo*> pendingActions_;
};

template <class T>
void SerializableInspectorWidget::RegisterAttributeHook(const ea::string& name, const AttributeHookFunction& function)
{
    SerializableInspectorWidget::RegisterAttributeHook({T::GetTypeNameStatic(), name}, function);
}

template <class T> void SerializableInspectorWidget::UnregisterAttributeHook(const ea::string& name)
{
    SerializableInspectorWidget::UnregisterAttributeHook({T::GetTypeNameStatic(), name});
}

template <class T, class U> void SerializableInspectorWidget::CopyAttributeHook(const ea::string& name)
{
    SerializableInspectorWidget::CopyAttributeHook({T::GetTypeNameStatic(), name}, {U::GetTypeNameStatic(), name});
}

template <class T>
void SerializableInspectorWidget::RegisterObjectHook(ObjectHookType type, const ObjectHookFunction& function)
{
    SerializableInspectorWidget::RegisterObjectHook({T::GetTypeNameStatic(), type}, function);
}

template <class T> void SerializableInspectorWidget::UnregisterObjectHook()
{
    for (const auto type : {ObjectHookType::Prepend, ObjectHookType::Append, ObjectHookType::Replace})
        SerializableInspectorWidget::UnregisterObjectHook({T::GetTypeNameStatic(), type});
}

template <class T, class U> void SerializableInspectorWidget::CopyObjectHook()
{
    for (const auto type : {ObjectHookType::Prepend, ObjectHookType::Append, ObjectHookType::Replace})
        SerializableInspectorWidget::CopyObjectHook({T::GetTypeNameStatic(), type}, {U::GetTypeNameStatic(), type});
}

} // namespace Urho3D
