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

#pragma once


#include <array>

#include "ToolboxAPI.h"
#include "AutoColumn.h"
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>


namespace Urho3D
{

class Viewport;

URHO3D_EVENT(E_INSPECTORLOCATERESOURCE, InspectorLocateResource)
{
    URHO3D_PARAM(P_NAME, ResourceName);                                         // String
}

/// A dummy object used as namespace for subscribing to events.
class URHO3D_TOOLBOX_API AttributeInspector : public Object
{
    URHO3D_OBJECT(AttributeInspector, Object);
public:
    explicit AttributeInspector(Context* context) : Object(context) { }
};

/// Render attribute inspector of `item`.
/// If `filter` is not null then only attributes containing this substring will be rendered.
/// If `eventNamespace` is not null then this object will be used to send events.
URHO3D_TOOLBOX_API bool RenderAttributes(Serializable* item, const char* filter=nullptr, Object* eventNamespace=nullptr);

}
