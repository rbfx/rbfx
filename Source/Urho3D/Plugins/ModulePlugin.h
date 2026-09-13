// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Plugins/DynamicModule.h"
#include "Urho3D/Plugins/Plugin.h"

namespace Urho3D
{

/// Plugin that is loaded from a native or managed dynamic library.
class URHO3D_API ModulePlugin : public Plugin
{
    URHO3D_OBJECT(ModulePlugin, Plugin);

public:
    explicit ModulePlugin(Context* context) : Plugin(context) { }

    /// Implement Plugin
    /// @{
    bool Load() override;
    bool IsLoaded() const override;
    bool IsOutOfDate() const override;
    bool IsReadyToReload() const override;
    bool PerformUnload() override;
    /// @}

    static ea::string GetTemporaryPdbName(const ea::string& fileName);

protected:
    ea::string GetAbsoluteFileName(const ea::string& name, bool original) const;
    void PatchTemporaryBinary(const ea::string& fileName);

    /// Absolute file name of original plugin.
    ea::string originalFileName_;
    /// Absolute file name of temporary copy.
    ea::string temporaryFileName_;

    /// Native module of this plugin.
    DynamicModule module_{context_};
    /// Last modification time.
    unsigned lastModificationTime_{};
    /// Last loaded module type.
    ModuleType lastModuleType_{};
};


}
