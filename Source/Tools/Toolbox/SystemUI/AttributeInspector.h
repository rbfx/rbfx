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

#pragma once


#include <array>

#include "ToolboxAPI.h"
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/SystemUI/SystemUI.h>


namespace Urho3D
{

class Viewport;

URHO3D_EVENT(E_INSPECTORLOCATERESOURCE, InspectorLocateResource)
{
    URHO3D_PARAM(P_NAME, ResourceName);                                         // String
}

URHO3D_EVENT(E_INSPECTORRENDERSTART, InspectorRenderStart)
{
    URHO3D_PARAM(P_SERIALIZABLE, Serializable);                                 // Serializable*
}

URHO3D_EVENT(E_INSPECTORRENDEREND, InspectorRenderEnd)
{
}

URHO3D_EVENT(E_INSPECTORRENDERATTRIBUTE, InspectorRenderAttribute)
{
    URHO3D_PARAM(P_ATTRIBUTEINFO, AttributeInfo);                               // void*
    URHO3D_PARAM(P_SERIALIZABLE, Serializable);                                 // RefCounted*
    URHO3D_PARAM(P_HANDLED, Handled);                                           // bool
    URHO3D_PARAM(P_MODIFIED, Modified);                                         // bool
}

/// Automate tracking of initial values that are modified by ImGui widget.
template<typename TValue>
struct ModifiedStateTracker
{
    bool TrackModification(bool modified, std::function<TValue()> getInitial)
    {
        if (modified)
        {
            if (!lastFrameModified_)
            {
                initial_ = getInitial();
                lastFrameModified_ = true;
            }
            return false;
        }
        else if (!ui::IsAnyItemActive() && lastFrameModified_)
        {
            lastFrameModified_ = false;
            return true;
        }

        return false;
    }

    bool TrackModification(bool modified, const TValue& initialValue)
    {
        std::function<TValue()> fn = [&]() { return initialValue; };
        return TrackModification(modified, fn);
    }

    const TValue& GetInitialValue() { return initial_; }

protected:
    /// Initial value.
    TValue initial_{};
    /// Flag indicating if value was modified on previous frame.
    bool lastFrameModified_ = false;
};

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
URHO3D_TOOLBOX_API bool RenderSingleAttribute(Variant& value);

}

namespace ImGui
{

URHO3D_TOOLBOX_API void SameLine(Urho3D::VariantType type);

}
