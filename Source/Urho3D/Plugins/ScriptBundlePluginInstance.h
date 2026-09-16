// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Plugins/PluginInstance.h"

namespace Urho3D
{

class URHO3D_API ScriptBundlePluginInstance : public PluginInstance
{
    URHO3D_OBJECT(ScriptBundlePluginInstance, PluginInstance);

public:
    explicit ScriptBundlePluginInstance(Context* context);

    /// Implement PluginInstance
    /// @{
    bool Load() override;
    bool IsLoaded() const override { return plugin_ != nullptr; }
    bool IsOutOfDate() const override { return outOfDate_; }
    bool PerformUnload() override;
    /// @}

private:
    void OnFileChanged(const ea::string& name);

    bool outOfDate_{};
};

}
