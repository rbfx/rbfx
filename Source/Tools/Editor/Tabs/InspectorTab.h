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

struct InspectArgs
{
    /// In. Attribute filter string.
    ea::string_view filter_;
    /// In. Object that is to be inspected.
    WeakPtr<Object> object_;
    /// In. Object that will be sending events on attribute modification. If null, object_ will be used for event sending.
    WeakPtr<Object> eventSender_;
    /// In/Out. Number of times object object inspection was handled this frame.
    unsigned handledTimes_ = 0;
};

class InspectorTab : public Tab
{
    URHO3D_OBJECT(InspectorTab, Tab)
public:
    explicit InspectorTab(Context* context);
    ///
    bool RenderWindowContent() override;
    /// Remove all items from inspector.
    void Clear();
    /// Request editor to inspect specified object. Reference to this object will not be held.
    void Inspect(Object* object, Object* eventSender=nullptr);
    /// Returns true when specified object is currently inspected.
    bool IsInspected(Object* object) const;

protected:
    /// Inspector attribute filter string.
    ea::string filter_;
    /// All currently inspected objects.
    ea::vector<ea::pair<WeakPtr<Object>, WeakPtr<Object>>> inspected_;
};

}
