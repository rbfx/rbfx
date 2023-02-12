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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/IO/MountedDirectory.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/VirtualFileSystem.h>

#include "VFSSample.h"

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

    message += "\n  ResourceDirs:\n";
    for (auto& resDir : cache->GetResourceDirs())
    {
        message += resDir + "\n";
    }

    message += "\n  PackageFiles:\n";
    for (auto& pakFile : cache->GetPackageFiles())
    {
        message += pakFile->GetName() + "\n";
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
