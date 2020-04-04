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


#include <Urho3D/Core/Object.h>
#include "EditorEventsPrivate.h"


namespace Urho3D
{

/// Event sent during construction of toolbar buttons. Subscribe to it to add new buttons.
URHO3D_EVENT(E_EDITORTOOLBARBUTTONS, EditorToolbarButtons)
{
    URHO3D_PARAM(P_SCENE, Scene);                     // Scene pointer.
}

/// Event sent when selection in any tab changes.
URHO3D_EVENT(E_EDITORSELECTIONCHANGED, EditorSelectionChanged)
{
    URHO3D_PARAM(P_TAB, Tab);                         // Tab pointer.
}

/// Event sent when rendering top menu bar of editor.
URHO3D_EVENT(E_EDITORAPPLICATIONMENU, EditorApplicationMenu)
{
}

/// Event sent when editor is about to save a Project.json file.
URHO3D_EVENT(E_EDITORPROJECTSERIALIZE, EditorProjectSerialize)
{
    URHO3D_PARAM(P_ARCHIVE, Root);                      // Raw pointer to Archive.
}

/// Event sent when editor is about to load a Project.json file.
URHO3D_EVENT(E_EDITORPROJECTCLOSING, EditorProjectClosing)
{
}

/// Notify subsystems about closed editor tab.
URHO3D_EVENT(E_EDITORTABCLOSED, EditorTabClosed)
{
    URHO3D_PARAM(P_TAB, Tab);                         // RefCounted pointer.
}

/// Sent when scene is played. Not sent when scene is resumed from paused state.
URHO3D_EVENT(E_SIMULATIONSTART, SimulationStart)
{
}

/// Sent when scene is stopped. Not sent when scene is paused.
URHO3D_EVENT(E_SIMULATIONSTOP, SimulationStop)
{
}

/// Sent when user selects a resource in resource browser.
URHO3D_EVENT(E_EDITORRESOURCESELECTED, EditorResourceSelected)
{
    URHO3D_PARAM(P_CTYPE, CType);                       // ContentType
    URHO3D_PARAM(P_RESOURCENAME, ResourceName);         // String
}

/// Sent when user adds a new flavor.
URHO3D_EVENT(E_EDITORFLAVORADDED, EditorFlavorAdded)
{
    URHO3D_PARAM(P_FLAVOR, Flavor);                     // Ptr
}

/// Sent when user adds a new flavor.
URHO3D_EVENT(E_EDITORFLAVORREMOVED, EditorFlavorRemoved)
{
    URHO3D_PARAM(P_FLAVOR, Flavor);                     // Ptr
}

/// Sent when user adds a new flavor.
URHO3D_EVENT(E_EDITORFLAVORRENAMED, EditorFlavorRenamed)
{
    URHO3D_PARAM(P_FLAVOR, Flavor);                     // Ptr
    URHO3D_PARAM(P_OLDNAME, OldName);                   // String
    URHO3D_PARAM(P_NEWNAME, NewName);                   // String
}

/// Sent when user adds a new flavor.
URHO3D_EVENT(E_EDITORIMPORTERATTRIBUTEMODIFIED, EditorImporterAttributeModified)
{
    URHO3D_PARAM(P_ASSET, Asset);                       // Ptr
    URHO3D_PARAM(P_IMPORTER, Importer);                 // Ptr
    URHO3D_PARAM(P_ATTRINFO, AttrInfo);                 // Void Ptr
    URHO3D_PARAM(P_NEWVALUE, NewValue);                 // Variant
}

}
