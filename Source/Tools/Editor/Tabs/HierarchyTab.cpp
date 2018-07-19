//
// Copyright (c) 2018 Rokas Kupstys
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

namespace Urho3D
{

HierarchyTab::HierarchyTab(Context* context)
    : Tab(context)
{
    SetTitle("Hierarchy");
    isUtility_ = true;
    SubscribeToEvent(E_EDITORRENDERHIERARCHY, [&](StringHash, VariantMap& args) {
        instance_ = (Tab*)args[EditorRenderHierarchy::P_INSPECTABLE].GetPtr();
        hierarchyProvider_ = dynamic_cast<IHierarchyProvider*>(instance_.Get());
    });
}

bool HierarchyTab::RenderWindowContent()
{
    if (!instance_.Expired())
        hierarchyProvider_->RenderHierarchy();
    return true;
}

}
