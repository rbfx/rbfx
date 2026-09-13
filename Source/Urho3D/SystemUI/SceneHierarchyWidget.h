// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Signal.h"
#include "../Utility/SceneSelection.h"

namespace Urho3D
{

struct SceneHierarchySettings
{
    bool showTemporary_{};
    bool showComponents_{true};
    ea::string filterByName_;
};

/// Widget to render scene hierarchy.
class URHO3D_API SceneHierarchyWidget : public Object
{
    URHO3D_OBJECT(SceneHierarchyWidget, Object)

public:
    Signal<void(Scene* scene, SceneSelection& selection)> OnContextMenu;
    Signal<void(Node* node, unsigned oldIndex, unsigned newIndex)> OnNodeReordered;
    Signal<void(Component* component, unsigned oldIndex, unsigned newIndex)> OnComponentReordered;
    Signal<void(Node* parentNode, Node* childNode)> OnNodeReparented;

    explicit SceneHierarchyWidget(Context* context);

    void SetSettings(const SceneHierarchySettings& settings);
    void RenderContent(Scene* scene, SceneSelection& selection);

    const SceneHierarchySettings& GetSettings() const { return settings_; }

private:
    void RenderNode(SceneSelection& selection, Node* node);
    void RenderComponent(SceneSelection& selection, Component* component);
    void ApplyPendingUpdates(Scene* scene);

    void ProcessObjectSelected(SceneSelection& selection, Object* object, bool toggle, bool range);
    void ProcessItemIfActive(SceneSelection& selection, Object* currentObject);

    void ProcessActiveObject(Object* activeObject);

    void BeginRangeSelection();
    void ProcessRangeSelection(Object* currentObject, bool open);
    void EndRangeSelection(SceneSelection& selection);

    void UpdateSearchResults(Scene* scene);

    void OpenSelectionContextMenu();
    void RenderContextMenu(Scene* scene, SceneSelection& selection);

    void BeginSelectionDrag(Scene* scene, SceneSelection& selection);
    void DropPayloadToNode(Node* parentNode);

    struct ReorderInfo
    {
        unsigned id_{};
        unsigned oldIndex_{};
        unsigned newIndex_{};
        ea::optional<float> decrementMaxY_;
        ea::optional<float> incrementMinY_;
    };
    using OptionalReorderInfo = ea::optional<ReorderInfo>;

    struct ReparentInfo
    {
        unsigned parentId_{};
        unsigned childId_{};
    };

    template <class T>
    OptionalReorderInfo RenderObjectReorder(OptionalReorderInfo& info, T* object, Node* parentNode, const char* hint);

    struct RangeSelectionRequest
    {
        WeakPtr<Object> from_;
        WeakPtr<Object> to_;

        bool IsBorder(Object* object) const { return object == from_ || object == to_; }
    };

    SceneHierarchySettings settings_;

    /// UI state
    /// @{
    bool isActiveObjectVisible_{};
    bool wasActiveObjectVisible_{};

    bool ignoreNextMouseRelease_{};

    bool scrollToActiveObject_{};
    Object* lastActiveObject_{};
    ea::vector<Node*> pathToActiveObject_;

    struct RangeSelection
    {
        ea::optional<RangeSelectionRequest> pendingRequest_;

        ea::optional<RangeSelectionRequest> currentRequest_;
        bool isActive_{};
        ea::vector<WeakPtr<Object>> result_;
    } rangeSelection_;

    struct NodeSearch
    {
        ea::string currentQuery_;
        WeakPtr<Scene> lastScene_;
        ea::string lastQuery_;
        ea::vector<WeakPtr<Node>> lastResults_;
    } search_;

    OptionalReorderInfo nodeReorder_;
    OptionalReorderInfo pendingNodeReorder_;
    OptionalReorderInfo componentReorder_;
    OptionalReorderInfo pendingComponentReorder_;

    ea::vector<ReparentInfo> pendingNodeReparents_;

    bool openContextMenu_{};
    /// @}
};

}
