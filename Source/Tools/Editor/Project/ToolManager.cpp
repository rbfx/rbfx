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

#include "../Project/ToolManager.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

ToolManager::ToolManager(Context* context)
    : SettingsPage(context)
{
    ForceScan();

    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap&)
    {
        Update();
    });
}

ToolManager::~ToolManager()
{
}

ea::string ToolManager::GetBlender() const
{
    return blender_.path_.empty() ? "blender" : blender_.path_;
}

ea::string ToolManager::GetFBX2glTF() const
{
    return fbx2gltf_.path_.empty() ? "FBX2glTF" : fbx2gltf_.path_;
}

void ToolManager::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "BlenderPath", blender_.path_);
    SerializeOptionalValue(archive, "FBX2glTFPath", fbx2gltf_.path_);

    if (archive.IsInput())
        ForceScan();
}

void ToolManager::RenderSettings()
{
    ui::Text("Path to Blender executable (use system PATH if empty):");
    RenderStatus(blender_.found_, blender_.path_);
    if (ui::InputText("##BlenderPath", &blender_.path_))
        ScanBlender();

    ui::Separator();

    ui::Text("Path to FBX2glTF executable (use system PATH if empty):");
    RenderStatus(fbx2gltf_.found_, fbx2gltf_.path_);
    if (ui::InputText("##FBX2glTFPath", &fbx2gltf_.path_))
        ScanFBX2glTF();
}

void ToolManager::RenderStatus(bool found, const ea::string& path)
{
    if (found)
    {
        const ColorScopeGuard guard{ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f}};
        ui::Text(ICON_FA_SQUARE_CHECK " Tool is found and available");
    }
    else
    {
        const ColorScopeGuard guard{ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f}};
        if (!path.empty())
            ui::Text(ICON_FA_TRIANGLE_EXCLAMATION " Tool is not found by the path '%s'", path.c_str());
        else
            ui::Text(ICON_FA_TRIANGLE_EXCLAMATION " Tool is not found in system PATH");
    }
}

void ToolManager::ForceScan()
{
    ScanBlender(true);
    ScanFBX2glTF(true);
}

void ToolManager::Update()
{
    if (blender_.scanPending_)
        ScanBlender();
    if (fbx2gltf_.scanPending_)
        ScanFBX2glTF();
}

void ToolManager::ScanBlender(bool force)
{
    blender_.scanPending_ = true;
    if (!force && blender_.scanTimer_.GetMSec(false) < scanCooldownMs_)
        return;

    ea::string dummy;
    auto fs = GetSubsystem<FileSystem>();
    blender_.found_ = fs->SystemRun(GetBlender(), {"-b", "-noaudio", "--python-expr", "import bpy; bpy.ops.wm.quit_blender()"}, dummy) >= 0;

    blender_.scanTimer_.Reset();
    blender_.scanPending_ = false;
}

void ToolManager::ScanFBX2glTF(bool force)
{
    fbx2gltf_.scanPending_ = true;
    if (!force && fbx2gltf_.scanTimer_.GetMSec(false) < scanCooldownMs_)
        return;

    ea::string dummy;
    auto fs = GetSubsystem<FileSystem>();
    fbx2gltf_.found_ = fs->SystemRun(GetFBX2glTF(), {"-h"}, dummy) >= 0;

    fbx2gltf_.scanTimer_.Reset();
    fbx2gltf_.scanPending_ = false;
}

}
