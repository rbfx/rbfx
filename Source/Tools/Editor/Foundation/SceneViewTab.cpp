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

#include "../Foundation/SceneViewTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<SceneViewTab>(context));
}

SceneViewTab::SceneViewTab(Context* context)
    : ResourceEditorTab(context, "Scene View", "9f4f7432-dd60-4c83-aecd-2f6cf69d3549",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
}

SceneViewTab::~SceneViewTab()
{
}

bool SceneViewTab::CanOpenResource(const OpenResourceRequest& request)
{
    return request.xmlFile_ && request.xmlFile_->GetRoot("scene");
}

void SceneViewTab::OnResourceLoaded(const ea::string& resourceName)
{

}

void SceneViewTab::OnResourceUnloaded(const ea::string& resourceName)
{

}

void SceneViewTab::OnActiveResourceChanged(const ea::string& resourceName)
{

}

void SceneViewTab::UpdateAndRenderContent()
{
    if (GetResourceNames().empty())
        ui::Text("No Scene");
    for (const ea::string& resourceName : GetResourceNames())
    {
        const char* mark = GetActiveResourceName() == resourceName ? "> " : "";
        ui::Text("%s%s", mark, resourceName.c_str());
    }
}

}
