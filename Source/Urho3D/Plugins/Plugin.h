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

#include "../Plugins/PluginApplication.h"

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
    /// This function will block until plugin file is complete and ready to be loaded.
    /// Returns false if timeout exceeded, but file is still incomplete.
    virtual bool WaitForCompleteFile(unsigned timeoutMs) const { return true; }

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
