// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Plugins/PluginApplication.h"

namespace Urho3D
{

/// A base class for plugins of all kinds. It only provides a common plugin interface.
class URHO3D_API Plugin : public Object
{
    URHO3D_OBJECT(Plugin, Object);

public:
    explicit Plugin(Context* context) : Object(context) {}
    ~Plugin() override {}

    /// Name must be set right after creating a plugin object.
    void SetName(const ea::string& name) { name_ = name; }

    /// Returns a name of the plugin. Name is usually a base name of plugin file.
    const ea::string& GetName() const { return name_; }
    /// Return current version of the plugin.
    unsigned GetVersion() const { return version_; }
    /// Return plugin application, if available.
    PluginApplication* GetApplication() const { return application_; }

    /// Mark plugin for unloading. Plugin will be unloaded at the end of current frame.
    void Unload() { unloading_ = true; }
    /// Returns whether the plugin is about to be unloaded.
    bool IsUnloading() const { return unloading_; }

    /// Loads plugin into application memory space and initializes it.
    virtual bool Load() { return true; }
    /// Returns true if plugin is loaded and functional.
    virtual bool IsLoaded() const { return application_ != nullptr; }
    /// Returns true if plugin was modified on the disk and should be reloaded.
    virtual bool IsOutOfDate() const { return false; }
    /// Returns true if plugin file is ready to reload.
    virtual bool IsReadyToReload() const { return true; }

    /// Actually unloads the module. Called by %PluginManager at the end of frame when IsUnloading is true.
    virtual bool PerformUnload() { return true; }

protected:
    /// Base plugin file name.
    ea::string name_;
    /// Flag indicating that plugin should unload on the end of the frame.
    bool unloading_{};
    /// Current plugin version.
    unsigned version_{};
    /// Instance to the plugin application. This should be a single owning reference to the plugin. Managed plugins are
    /// an exception as managed object holds reference to native object and must be disposed in order to free this object.
    SharedPtr<PluginApplication> application_;
};


}
