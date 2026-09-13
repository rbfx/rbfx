// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
