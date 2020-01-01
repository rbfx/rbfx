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

#include <Urho3D/IO/Log.h>

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
    // Render main tab inspectors
    if (!provider_.first.Expired())
        provider_.second->RenderHierarchy();
    return true;
}

void HierarchyTab::SetProvider(IHierarchyProvider* provider)
{
    if (auto* ptr = dynamic_cast<RefCounted*>(provider))
    {
        provider_.first = ptr;
        provider_.second = provider;
    }
    else
        URHO3D_LOGERROR("Classes that inherit IHierarchyProvider must also inherit RefCounted.");
}

}
