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

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/SystemUI/Texture2DWidget.h>

namespace Urho3D
{

void Foundation_Texture2DViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class Texture2DViewTab : public CustomSceneViewTab
{
    URHO3D_OBJECT(Texture2DViewTab, CustomSceneViewTab)

public:
    explicit Texture2DViewTab(Context* context);
    ~Texture2DViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;

    ea::string GetResourceTitle() override { return "Texture2D"; }
    bool SupportMultipleResources() override { return false; }
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

private:
    SharedPtr<Texture2DWidget> preview_;
};

} // namespace Urho3D
