//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"
#include "../Foundation/GraphViewTab/GraphViewTab.h"

#include <Urho3D/Actions/ActionSet.h>

namespace Urho3D
{

void Foundation_ActionViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class ActionViewTab : public GraphViewTab
{
    URHO3D_OBJECT(ActionViewTab, GraphViewTab)

public:
    explicit ActionViewTab(Context* context);
    ~ActionViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;

    ea::string GetResourceTitle() { return "Action"; }
    bool SupportMultipleResources() { return false; }
    bool CanOpenResource(const ResourceFileDescriptor& desc) override;
    /// @}

protected:
    /// ResourceEditorTab implementation
    /// @{
    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;
    void OnResourceShallowSaved(const ea::string& resourceName) override;
    /// @}

    void RenderTitle() override;
    SharedPtr<GraphNode> CreateNewNodePopup() const override;

private:
    SharedPtr<ActionSet> actionSet_;
    ea::vector<SharedPtr<ObjectReflection>> actionTypes_;
};

} // namespace Urho3D
