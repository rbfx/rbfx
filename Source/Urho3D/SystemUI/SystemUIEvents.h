//
// Copyright (c) 2008-2015 the Urho3D project.
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

#include "../Core/Object.h"
#include "../UI/UIEvents.h"


namespace Urho3D
{

URHO3D_EVENT(E_ENDRENDERINGSYSTEMUI, EndRenderingSystemUI)
{
}

URHO3D_EVENT(E_CONSOLECLOSED, ConsoleClosed)
{
}

URHO3D_EVENT(E_ATTRIBUTEINSPECTORMENU, AttributeInspectorMenu)
{
    URHO3D_PARAM(P_SERIALIZABLE, Serializable);                  // Serializable pointer
    URHO3D_PARAM(P_ATTRIBUTEINFO, AttributeInfo);                // AttributeInfo pointer
}

URHO3D_EVENT(E_ATTRIBUTEINSPECTVALUEMODIFIED, AttributeInspectorValueModified)
{
    URHO3D_PARAM(P_SERIALIZABLE, Serializable);                  // Serializable pointer
    URHO3D_PARAM(P_ATTRIBUTEINFO, AttributeInfo);                // AttributeInfo pointer
    URHO3D_PARAM(P_OLDVALUE, OldValue);                          // Variant
    URHO3D_PARAM(P_NEWVALUE, NewValue);                          // Variant
    URHO3D_PARAM(P_REASON, Reason);                              // unsigned
}

URHO3D_EVENT(E_ATTRIBUTEINSPECTOATTRIBUTE, AttributeInspectorAttribute)
{
    URHO3D_PARAM(P_SERIALIZABLE, Serializable);                  // Serializable pointer
    URHO3D_PARAM(P_ATTRIBUTEINFO, AttributeInfo);                // AttributeInfo pointer
    URHO3D_PARAM(P_COLOR, Color);                                // Color
    URHO3D_PARAM(P_HIDDEN, Hidden);                              // Boolean
    URHO3D_PARAM(P_TOOLTIP, Tooltip);                            // String
    URHO3D_PARAM(P_VALUE_KIND, ValueKind);                       // int
}

URHO3D_EVENT(E_GIZMONODEMODIFIED, GizmoNodeModified)
{
    URHO3D_PARAM(P_NODE, Node);                                  // Node pointer
    URHO3D_PARAM(P_OLDTRANSFORM, OldTransform);                  // Matrix3x4
    URHO3D_PARAM(P_NEWTRANSFORM, NewTransform);                  // Matrix3x4
}

URHO3D_EVENT(E_GIZMOSELECTIONCHANGED, GizmoSelectionChanged)
{
}

}
