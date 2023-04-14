//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
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

#include "VFSSample.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#if URHO3D_SYSTEMUI
    #include <Urho3D/SystemUI/SystemUI.h>
#endif

#include <Urho3D/DebugNew.h>

VFSSample::VFSSample(Context* context)
    : Sample(context)
{
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void VFSSample::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create "Hello World" Text
    CreateText();

    // Subscribe to Update event for rendering UI
    SubscribeToEvent(E_UPDATE, &VFSSample::RenderUi);
}

void VFSSample::CreateText()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* vfs = GetSubsystem<VirtualFileSystem>();

    // Construct new Text object
    SharedPtr<Text> helloText(new Text(context_));
    ea::string message;

    message += "  MountPoints:\n";
    for (unsigned i = 0; i < vfs->NumMountPoints(); ++i)
    {
        auto mountPoint = vfs->GetMountPoint(i);
        message += mountPoint->GetName();
        message += "\n";
    }

    // Set String to display
    helloText->SetText(message);

    // Set font and text color
    helloText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 16);
    helloText->SetColor(Color(0.0f, 1.0f, 0.0f));

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_CENTER);

    // Add Text instance to the UI root element
    GetUIRoot()->AddChild(helloText);
}

void VFSSample::RenderUi()
{
#if URHO3D_SYSTEMUI
    auto vfs = GetSubsystem<VirtualFileSystem>();

    ui::SetNextWindowSize(ImVec2(550, 500), ImGuiCond_FirstUseEver);
    ui::SetNextWindowPos(ImVec2(350, 50), ImGuiCond_FirstUseEver);
    if (ui::Begin("VFS Query Interface", 0, ImGuiWindowFlags_NoSavedSettings))
    {
        ui::Text("URI: ");
        ui::SameLine();
        if (ui::InputText("##uri", &uri_) || ui::IsWindowAppearing())
        {
            fileIdentifier_ = FileIdentifier::FromUri(uri_);
            canonicalForm_ = vfs->GetCanonicalIdentifier(fileIdentifier_);
            exists_ = vfs->Exists(fileIdentifier_);
            absoluteFileName_ = vfs->GetAbsoluteNameFromIdentifier(fileIdentifier_);
            readOnlyFile_ = vfs->OpenFile(fileIdentifier_, FILE_READ);
            modificationTime_ = vfs->GetLastModifiedTime(fileIdentifier_, true);
            reversedUri_ = vfs->GetIdentifierFromAbsoluteName(absoluteFileName_).ToUri();
        }

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("scheme: ");
            ui::SameLine();
        }
        ui::Text("%s", fileIdentifier_.scheme_.c_str());

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("path: ");
            ui::SameLine();
        }
        ui::Text("%s", fileIdentifier_.fileName_.c_str());

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("canonical: ");
            ui::SameLine();
        }
        ui::Text("%s", canonicalForm_.ToUri().c_str());

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("exists: ");
            ui::SameLine();
        }
        ui::Text("%s", exists_ ? "yes" : "no");

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("absolute path: ");
            ui::SameLine();
            if (absoluteFileName_.empty())
                ui::Text("[not found]");
        }
        if (!absoluteFileName_.empty())
            ui::Text("%s", absoluteFileName_.c_str());

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("file size: ");
            ui::SameLine();
            if (!readOnlyFile_)
                ui::Text("[not found]");
        }
        if (readOnlyFile_)
            ui::Text("%d", readOnlyFile_->GetSize());

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("modification time: ");
            ui::SameLine();
            if (modificationTime_ == 0)
                ui::Text("[unknown]");
        }
        if (modificationTime_ != 0)
        {
            // TODO: move to common place?
            const time_t time = static_cast<time_t>(modificationTime_);
            ui::Text("%s", Time::GetTimeStamp(time).c_str());
        }

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("reversed URI: ");
            ui::SameLine();
            if (reversedUri_.empty())
                ui::Text("[not found]");
        }
        if (!reversedUri_.empty())
            ui::Text("%s", reversedUri_.c_str());

        ui::Separator();

        bool needScan = ui::IsWindowAppearing();

        ui::Text("scheme:");
        ui::SameLine();
        if (ui::InputText("##scanscheme", &scanPath_.scheme_))
            needScan = true;

        ui::Text("path:");
        ui::SameLine();
        if (ui::InputText("##scanpath", &scanPath_.fileName_))
            needScan = true;

        ui::Text("filter:");
        ui::SameLine();
        if (ui::InputText("##scanfilter", &scanFilter_))
            needScan = true;

        if (ui::Checkbox("Recursive", &scanRecursive_))
            needScan = true;
        ui::SameLine();
        if (ui::Checkbox("Files", &scanFiles_))
            needScan = true;
        ui::SameLine();
        if (ui::Checkbox("Directories", &scanDirectories_))
            needScan = true;

        if (needScan)
        {
            ScanFlags scanFlags{};
            if (scanRecursive_)
                scanFlags |= SCAN_RECURSIVE;
            if (scanFiles_)
                scanFlags |= SCAN_FILES;
            if (scanDirectories_)
                scanFlags |= SCAN_DIRS;
            vfs->Scan(scanResults_, scanPath_, scanFilter_, scanFlags);
        }

        {
            ColorScopeGuard colorScopeGuard{ImGuiCol_Text, Color::YELLOW};
            ui::Text("scan results:");
        }
        if (ui::BeginListBox("##results"))
        {
            for (unsigned i = 0; i < scanResults_.size(); i++)
            {
                const auto result = scanPath_ + scanResults_[i];
                ui::Selectable(result.fileName_.c_str(), false);
            }
            ui::EndListBox();
        }
    }
    ui::End();
#endif
}
