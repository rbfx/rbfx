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

#include "../ResourceBrowser/ResourceBrowserTab.h"
#include "../Project/ProjectEditor.h"

namespace Urho3D
{

ResourceBrowserTab::ResourceBrowserTab(Context* context)
    : EditorTab(context, "Resource Browser", "96c69b8e-ee83-43de-885c-8a51cef65d59",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockBottom)
{
    auto project = GetProject();
    coreDataReflection_ = MakeShared<FileSystemReflection>(context_, StringVector{project->GetCoreDataPath()});
    dataReflection_ = MakeShared<FileSystemReflection>(context_, StringVector{project->GetDataPath(), project->GetCachePath()});
}

ResourceBrowserTab::~ResourceBrowserTab()
{
}

void ResourceBrowserTab::RenderContentUI()
{
    coreDataReflection_->Update();
    dataReflection_->Update();

    ui::Columns(2);
    if (ui::BeginChild("##DirectoryTree", ui::GetContentRegionAvail()))
    {
        /*URHO3D_PROFILE("ResourceTab::DirectoryTree");
        if (ui::TreeNodeEx("Root", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth | (currentDir_.empty() ? ImGuiTreeNodeFlags_Selected : 0)))
        {
            if (ui::IsItemClicked(MOUSEB_LEFT))
            {
                currentDir_.clear();
                selectedItem_.clear();
                rescan_ = true;
            }
            ui::TreePop();
        }
        RenderDirectoryTree();*/
    }
    ui::EndChild();
    ui::SameLine();
    ui::NextColumn();
    if (ui::BeginChild("##DirectoryContent", ui::GetContentRegionAvail()))
    {
    }
    ui::EndChild();

}

}
