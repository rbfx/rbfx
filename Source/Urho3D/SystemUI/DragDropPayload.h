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

#include "../Container/RefCounted.h"
#include "../Container/ConstString.h"

#include <EASTL/functional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class Component;
class Node;
class Scene;

URHO3D_GLOBAL_CONSTANT(ConstString DragDropPayloadType{"DragDropPayload"});
URHO3D_GLOBAL_CONSTANT(ConstString DragDropPayloadVariable{"SystemUI_DragDropPayload"});

/// Base class for drag&drop payload.
class URHO3D_API DragDropPayload : public RefCounted
{
public:
    using CreateCallback = ea::function<SharedPtr<DragDropPayload>()>;

    static void Set(const SharedPtr<DragDropPayload>& payload);
    static DragDropPayload* Get();

    /// Call this function on every frame from drag source.
    static void UpdateSource(const CreateCallback& createPayload);

    /// Format string to display while dragging.
    virtual ea::string GetDisplayString() const { return "Drop me"; }
};

/// Resource file descriptor.
struct URHO3D_API ResourceFileDescriptor
{
    /// Name without path.
    ea::string localName_;
    /// File name relative to resource root.
    ea::string resourceName_;
    /// Absolute file name.
    ea::string fileName_;

    /// Whether the file is a directory.
    bool isDirectory_{};
    /// Whether the file or folder is automatically managed, e.g. file is stored in the generated cache.
    bool isAutomatic_{};

    /// File type tags.
    ea::unordered_set<ea::string> typeNames_;
    ea::unordered_set<StringHash> types_;

    ea::string mostDerivedType_;

    void AddObjectType(const ea::string& typeName);
    bool HasObjectType(const ea::string& typeName) const;
    bool HasObjectType(StringHash type) const;

    template <class T> void AddObjectType() { AddObjectType(T::GetTypeNameStatic()); }
    template <class T> bool HasObjectType() const { return HasObjectType(T::GetTypeNameStatic()); }

    bool HasExtension(ea::string_view extension) const;
    bool HasExtension(std::initializer_list<ea::string_view> extensions) const;
};

/// Drag&drop payload containing reference to a resource or directory.
class URHO3D_API ResourceDragDropPayload : public DragDropPayload
{
public:
    ea::string GetDisplayString() const override;

    ea::vector<ResourceFileDescriptor> resources_;
};

/// Drag&drop payload containing nodes and components.
class URHO3D_API NodeComponentDragDropPayload : public DragDropPayload
{
public:
    ~NodeComponentDragDropPayload() override;
    ea::string GetDisplayString() const override { return !displayString_.empty() ? displayString_ : "???"; }

    WeakPtr<Scene> scene_;
    ea::vector<WeakPtr<Node>> nodes_;
    ea::vector<WeakPtr<Component>> components_;
    ea::string displayString_;
};

}
