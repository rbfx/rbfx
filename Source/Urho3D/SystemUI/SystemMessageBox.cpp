// Copyright (c) 2017 the Urho3D project.
// Copyright (c) 2017-2026 the rbfx project.
// Copyright (c) 2008-2015 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "SystemMessageBox.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Graphics/Graphics.h"
#include "../IO/Log.h"
#include "SystemUI.h"
#include "SystemUIEvents.h"

namespace Urho3D
{

SystemMessageBox::SystemMessageBox(Context* context, const ea::string& messageString, const ea::string& titleString) :
    Object(context),
    messageText_(messageString),
    isOpen_(true)
{
    SetTitle(titleString);
    Graphics* graphics = GetSubsystem<Graphics>();
    windowSize_ = ImVec2(300, 150);
    windowPosition_ = ImVec2(graphics->GetWidth() / 2 - windowSize_.x / 2, graphics->GetHeight() / 2 - windowSize_.y / 2);
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SystemMessageBox, RenderFrame));
}

SystemMessageBox::~SystemMessageBox()
{
}

void SystemMessageBox::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SystemMessageBox>();
}

void SystemMessageBox::SetTitle(const ea::string& text)
{
    titleText_ = ToString("%s##%p", text.c_str(), this);
}

void SystemMessageBox::SetMessage(const ea::string& text)
{
    messageText_ = text;
}

const ea::string& SystemMessageBox::GetTitle() const
{
    return titleText_;
}

const ea::string& SystemMessageBox::GetMessage() const
{
    return messageText_;
}

void SystemMessageBox::RenderFrame(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;
    ui::SetNextWindowPos(windowPosition_, ImGuiCond_FirstUseEver);
    ui::SetNextWindowSize(windowSize_, ImGuiCond_Always);
    if (ui::Begin(titleText_.c_str(), &isOpen_, ImGuiWindowFlags_NoCollapse|
                     ImGuiWindowFlags_NoSavedSettings))
    {
        ui::TextUnformatted(messageText_.c_str());
        auto region = ui::GetContentRegionAvail();
        ui::SetCursorPos(ImVec2(region.x - 100 + 20, region.y + 20));

        bool closeWindow = false;
        bool status = false;
        if (ui::Button("Ok"))
        {
            closeWindow = true;
            status = true;
        }
        ui::SameLine();
        if (ui::Button("Cancel") || !isOpen_)
        {
            closeWindow = true;
            status = false;
        }

        if (closeWindow)
        {
            SendEvent(E_MESSAGEACK, ea::forward_as_tuple(P_OK, status));
            isOpen_ = false;
        }
    }
    ui::End();
}

}
