// Copyright (c) 2017 the Urho3D project.
// Copyright (c) 2008-2015 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "SystemUI.h"

namespace Urho3D
{

/// Message box dialog.
class URHO3D_API SystemMessageBox : public Object
{
    URHO3D_OBJECT(SystemMessageBox, Object);

public:
    /// Construct. If layout file is not given, use the default message box layout. If style file is not given, use the default style file from root UI element.
    SystemMessageBox(Context* context, const ea::string& messageString = EMPTY_STRING, const ea::string& titleString = EMPTY_STRING);
    /// Destruct.
    virtual ~SystemMessageBox();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set title text. No-ops if there is no title text element.
    void SetTitle(const ea::string& text);
    /// Set message text. No-ops if there is no message text element.
    void SetMessage(const ea::string& text);

    /// Return title text. Return empty string if there is no title text element.
    const ea::string& GetTitle() const;
    /// Return message text. Return empty string if there is no message text element.
    const ea::string& GetMessage() const;
    /// Returns true if message box is open.
    bool IsOpen() const { return isOpen_; }

private:
    /// Render message box ui.
    void RenderFrame(StringHash eventType, VariantMap& eventData);

    /// Title text element.
    ea::string titleText_;
    /// Message text element.
    ea::string messageText_;
    /// Is message box window open.
    bool isOpen_;
    /// Initial message box window position.
    ImVec2 windowPosition_;
    /// Initial message box window size.
    ImVec2 windowSize_;
};

}
