// Copyright (c) 2008-2015 the Urho3D project.
// Copyright (c) 2015-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/UI/UIEvents.h"


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
