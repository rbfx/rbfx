// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
