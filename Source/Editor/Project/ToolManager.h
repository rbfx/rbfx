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

#include "../Core/SettingsManager.h"

#include <Urho3D/Core/Timer.h>

namespace Urho3D
{

class JSONFile;
class Project;

/// Manages third-party tools. Make it SettingsPage for simplicity.
class ToolManager : public SettingsPage
{
    URHO3D_OBJECT(ToolManager, SettingsPage);

public:
    explicit ToolManager(Context* context);
    ~ToolManager() override;

    void Update();

    bool HasBlender() const { return blender_.found_; }
    ea::string GetBlender() const;
    bool HasFBX2glTF() const { return fbx2gltf_.found_; }
    ea::string GetFBX2glTF() const;

    /// Implement SettingsPage
    /// @{
    ea::string GetUniqueName() override { return "Editor.ExternalTools"; }
    bool IsSerializable() override { return true; }

    void SerializeInBlock(Archive& archive) override;
    void RenderSettings() override;
    /// @}

private:
    static const unsigned scanCooldownMs_ = 3000;

    void ScanBlender(bool force = false);
    void ScanFBX2glTF(bool force = false);
    void RenderStatus(bool found, const ea::string& path, const ea::string& hint);

    struct Blender
    {
        bool firstScan_{true};
        bool found_{};
        ea::string path_;

        Timer scanTimer_;
        bool scanPending_{};
    } blender_;

    struct FBX2glTF
    {
        bool firstScan_{true};
        bool found_{};
        ea::string path_;

        Timer scanTimer_;
        bool scanPending_{};
    } fbx2gltf_;
};

}
