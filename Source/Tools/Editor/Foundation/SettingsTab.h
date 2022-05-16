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

#include "../Core/SettingsManager.h"
#include "../Project/ProjectEditor.h"
#include "../Project/EditorTab.h"

namespace Urho3D
{

void Foundation_SettingsTab(Context* context, ProjectEditor* projectEditor);

/// Tab that displays project settings.
class SettingsTab : public EditorTab
{
    URHO3D_OBJECT(SettingsTab, EditorTab)

public:
    explicit SettingsTab(Context* context);

    /// Implement EditorTab
    /// @{
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

protected:
    /// Implement EditorTab
    /// @{
    void UpdateAndRenderContent() override;
    /// @}

private:
    void RenderSettingsTree();
    void RenderSettingsSubtree(const SettingTreeNode& treeNode, const ea::string& shortName);

    void RenderCurrentSettingsPage();

    bool selectNextValidPage_{};
    ea::string selectedPage_;
};

}
