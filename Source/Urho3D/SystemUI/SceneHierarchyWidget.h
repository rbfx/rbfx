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
    explicit SceneHierarchyWidget(Context* context);

    void SetSettings(const SceneHierarchySettings& settings);
    void RenderContent(Scene* scene, SceneSelection& selection);

    const SceneHierarchySettings& GetSettings() const { return settings_; }

private:
    void RenderNode(SceneSelection& selection, Node* node);
    void RenderComponent(SceneSelection& selection, Component* component);
    void ProcessObjectSelected(SceneSelection& selection, Object* object, bool toggle, bool range);
    void ProcessItemIfActive(SceneSelection& selection, Object* currentObject);

    void ProcessActiveObject(Object* activeObject);

    void BeginRangeSelection();
    void ProcessRangeSelection(Object* currentObject, bool open);
    void EndRangeSelection(SceneSelection& selection);

    void UpdateSearchResults(Scene* scene);

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
    /// @}
};

}
