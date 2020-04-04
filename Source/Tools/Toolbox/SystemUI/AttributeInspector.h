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


#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include "ToolboxAPI.h"
#include "SystemUI/Widgets.h"


namespace Urho3D
{

class Viewport;

enum class AttributeInspectorModified : unsigned
{
    NO_CHANGE = 0,
    SET_BY_USER = 1,
    SET_DEFAULT = 1u << 1u,
    SET_INHERITED = 1u << 2u,
    RESET = SET_DEFAULT | SET_INHERITED
};
URHO3D_FLAGSET(AttributeInspectorModified, AttributeInspectorModifiedFlags);

enum class AttributeValueKind
{
    ATTRIBUTE_VALUE_DEFAULT,
    ATTRIBUTE_VALUE_INHERITED,
    ATTRIBUTE_VALUE_CUSTOM,
};

URHO3D_EVENT(E_INSPECTORLOCATERESOURCE, InspectorLocateResource)
{
    URHO3D_PARAM(P_NAME, ResourceName);                                         // String
}

/// Automate tracking of initial values that are modified by ImGui widget.
template<typename Value>
struct ValueHistory
{
    /// Copy value type.
    using ValueCopy = typename ea::remove_cvref_t<Value>;
    /// Reference value type.
    using ValueRef = typename std::conditional_t<!std::is_reference_v<std::remove_cv_t<Value>>, const ValueCopy&, ValueCopy&>;
    /// Reference or a copy, depending on whether Value is const.
    using ValueCurrent = std::conditional_t<!std::is_const_v<Value> && std::is_reference_v<Value>, ValueCopy&, ValueCopy>;

    /// Construct. For internal use. Never construct ValueHistory directly.
    ValueHistory(ValueRef current) : current_(current) { }
    /// A helper getter. It should be used to obtain the value.
    static ValueHistory& Get(ValueRef value)
    {
        auto history = ui::GetUIState<ValueHistory>(value);
        history->current_ = value;
        if (history->expired_)
        {
            history->initial_ = value;
            history->expired_ = false;
            history->modified_ = false;
        }
        return *history;
    }
    /// Returns true when value is modified and no continuous modification is happening.
    bool IsModified()
    {
        if (initial_ != current_ && !ui::IsAnyItemActive())
        {
            if (modified_)
            {
                // User changed this value explicitly.
                expired_ = true;
                modified_ = false;
                return true;
            }
            else
            {
                // Change is external.
                expired_ = true;
            }
        }
        return false;
    }
    /// Returns true if value is modified.
    explicit operator bool() { return IsModified(); }
    /// Flag value as modified.
    void SetModified(bool modified) { modified_ = modified; }

    /// Initial value.
    ValueCopy initial_{};
    /// Last value.
    ValueCurrent current_{};
    /// Flag indicating this history entry is expired and initial value can be overwritten.
    bool expired_ = true;
    ///
    bool modified_ = false;
};

/// Render attribute inspector of `item`.
/// If `filter` is not null then only attributes containing this substring will be rendered.
/// If `eventSender` is not null then this object will be used to send events.
URHO3D_TOOLBOX_API bool RenderAttribute(ea::string_view title, Variant& value, const Color& color=Color::WHITE, ea::string_view tooltip="", const AttributeInfo* info=nullptr, Object* eventSender=nullptr, float item_width=0);
URHO3D_TOOLBOX_API bool RenderAttributes(Serializable* item, ea::string_view filter="", Object* eventSender=nullptr);

}
