// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Plugins/Plugin.h"

namespace Urho3D
{

class URHO3D_API ScriptBundlePlugin : public Plugin
{
    URHO3D_OBJECT(ScriptBundlePlugin, Plugin);

public:
    explicit ScriptBundlePlugin(Context* context);

    /// Implement Plugin
    /// @{
    bool Load() override;
    bool IsLoaded() const override { return application_ != nullptr; }
    bool IsOutOfDate() const override { return outOfDate_; }
    bool PerformUnload() override;
    /// @}

private:
    void OnFileChanged(const ea::string& name);

    bool outOfDate_{};
};

}
