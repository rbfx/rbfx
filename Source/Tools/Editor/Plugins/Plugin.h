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

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/PluginModule.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Container/FlagSet.h>

namespace Urho3D
{

/// A base class for plugins of all kinds. It only provides a common plugin interface.
class Plugin : public Object
{
    URHO3D_OBJECT(Plugin, Object);
public:
    /// Construct.
    explicit Plugin(Context* context) : Object(context) { }
    /// Destruct.
    ~Plugin() override = default;

    /// Returns a name of the plugin. Name is usually a base name of plugin file.
    const ea::string& GetName() const { return name_; }
    /// Name must be set right after creating a plugin object.
    void SetName(const ea::string& name) { name_ = name; }
    /// Returns true when plugin is private (meant for developer tools only).
    bool IsPrivate() const { return isPrivate_; }
    /// Sets plugin privacy status. Private plugins are meant for developer tools and not shipped with a final product.
    void SetPrivate(bool isPrivate) { isPrivate_ = isPrivate; }
    /// Mark plugin for unloading. Plugin will be unloaded at the end of current frame.
    void Unload() { unloading_ = true; }
    /// Loads plugin into application memory space and initializes it.
    virtual bool Load() { return true; }
    /// Returns true if plugin is loaded and functional.
    virtual bool IsLoaded() const { return application_.NotNull(); }
    /// Returns true if plugin was modified on the disk and should be reloaded.
    virtual bool IsOutOfDate() const { return false; }
    /// This function will block until plugin file is complete and ready to be loaded. Returns false if timeout exceeded, but file is still incomplete.
    virtual bool WaitForCompleteFile(unsigned timeoutMs) const { return true; }
    /// Returns true if user may configure loading or unloading plugin.
    bool IsManagedManually() const { return isManagedManually_; }

protected:
    /// Actually unloads the module. Called by %PluginManager at the end of frame when unloading_ flag is set.
    virtual bool PerformUnload() { return true; }

    /// Base plugin file name.
    ea::string name_{};
    /// Flag indicating that plugin should unload on the end of the frame.
    bool unloading_ = false;
    /// Current plugin version.
    unsigned version_ = 0;
    /// Plugin flags.
    bool isPrivate_ = false;
    /// Instance to the plugin application. This should be a single owning reference to the plugin. Managed plugins are
    /// an exception as managed object holds reference to native object and must be disposed in order to free this object.
    SharedPtr<PluginApplication> application_;
    /// Flag indicating that user may configure loading or unloading plugin.
    bool isManagedManually_ = true;

    friend class PluginManager;
};


}
