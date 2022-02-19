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


#include <EASTL/utility.h>
#include <Urho3D/Container/Ptr.h>

#include "Tabs/Tab.h"


namespace Urho3D
{

class HierarchyTab : public Tab
{
    URHO3D_OBJECT(HierarchyTab, Tab)
public:
    ///
    explicit HierarchyTab(Context* context);
    ///
    bool RenderWindowContent() override;
    ///
    void SetProvider(IHierarchyProvider* provider);
    ///
    void OnUpdateFocused() override;

protected:
    ea::pair<WeakPtr<RefCounted>, IHierarchyProvider*> provider_;
};

}
