//
// Copyright (c) 2017-2019 the rbfx project.
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

#include "EditorEvents.h"
#include "HierarchyTab.h"
#include "Editor.h"

namespace Urho3D
{

HierarchyTab::HierarchyTab(Context* context)
    : Tab(context)
{
    SetID("2d753fe8-e3c1-4ccc-afae-ec4f3beb70e4");
    isUtility_ = true;
    SetTitle("Hierarchy");
}

bool HierarchyTab::RenderWindowContent()
{
    // Handle tab switching/closing
    if (Tab* tab = GetSubsystem<Editor>()->GetActiveTab())
        inspector_.Update(tab);

    // Render main tab inspectors
    if (inspector_)
        inspector_->RenderHierarchy();

    return true;
}

}
