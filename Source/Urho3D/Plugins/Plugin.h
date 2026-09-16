// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/Object.h"

namespace Urho3D
{

class Plugin;
using PluginFactory = SharedPtr<Plugin>(*)(Context* context);

/// Base class for creating dynamically linked plugins.
class URHO3D_API Plugin : public Object
{
    URHO3D_OBJECT(Plugin, Object);

public:
    /// Register plugin class to be visible in all future instances of PluginManager.
    static void RegisterPlugin(const ea::string& name, PluginFactory factory);
    template <class T> static void RegisterPlugin(const ea::string& name);
    template <class T> static void RegisterPlugin();

    explicit Plugin(Context* context);
    ~Plugin() override;
    void SetPluginName(const ea::string& name) { pluginName_ = name; }

    /// Return whether the plugin can be executed as main entry point.
    virtual bool IsExecutable() const { return false; }
    /// Return whether suspend/resume is supported by this plugin.
    virtual bool IsSuspendSupported() const { return true; }
    /// Return default object category for the plugin.
    virtual ea::string GetDefaultCategory() const { return Category_User; }

    /// Prepare object for destruction.
    void Dispose();

    /// Load plugin into the context and the engine subsystems.
    void LoadPlugin();
    /// Unload plugin from the context and the engine subsystems.
    void UnloadPlugin();

    /// Start application.
    void StartApplication(bool isMain);
    /// Stop application.
    void StopApplication();

    /// Suspend application. It's highly recommended to release all plugin-related objects here.
    void SuspendApplication(Archive& output, unsigned version);
    /// Resume application. Archive may be null if it wasn't serialized before.
    void ResumeApplication(Archive* input, unsigned version);

    /// Return plugin name. Should be the same as dynamic library name when plugin is linked dynamically.
    const ea::string& GetPluginName() { return pluginName_; }
    /// Return whether the plugin is loaded.
    bool IsLoaded() const { return isLoaded_; }
    /// Return whether the application is started.
    bool IsStarted() const { return isStarted_; }

    /// Register a factory for an object type that would be automatically unregistered on unload.
    template<typename T> ObjectReflection* AddFactoryReflection();
    template<typename T> ObjectReflection* AddFactoryReflection(ea::string_view category);
    /// Register an object that would be automatically unregistered on unload.
    template<typename T> void RegisterObject();

protected:
    /// Called on LoadPlugin().
    virtual void Load() {}
    /// Called on UnloadPlugin().
    virtual void Unload() {}
    /// Called on StartApplication().
    virtual void Start(bool isMain) {}
    /// Called on StopApplication().
    virtual void Stop() {}
    /// Called on SuspendApplication().
    virtual void Suspend(Archive& output) {}
    /// Called on ResumeApplication().
    virtual void Resume(Archive* input, bool differentVersion) {}

private:
    ea::string pluginName_;
    ea::vector<StringHash> reflectedTypes_;

    bool isLoaded_{};
    bool isStarted_{};
};

/// Similar to Plugin, but can act as executable entry point.
class URHO3D_API ExecutablePlugin : public Plugin
{
    URHO3D_OBJECT(ExecutablePlugin, Plugin);

public:
    explicit ExecutablePlugin(Context* context);
    ~ExecutablePlugin() override;

    /// Implement Plugin.
    /// @{
    bool IsExecutable() const final { return true; }
    bool IsSuspendSupported() const override { return false; }
    /// @}
};

/// API for interacting with linked plugins.
/// Implementation of those functions should be provided by the executable via target_link_plugins in CMake.
namespace LinkedPlugins
{

/// Return names of all linked plugins.
const StringVector& GetLinkedPlugins();
/// Register all statically linked plugins.
void RegisterStaticPlugins();

} // namespace Plugins

template<typename T>
ObjectReflection* Plugin::AddFactoryReflection()
{
    auto reflection = context_->AddFactoryReflection<T>(GetDefaultCategory());
    if (reflection)
        reflectedTypes_.push_back(T::GetTypeStatic());
    return reflection;
}

template<typename T>
ObjectReflection* Plugin::AddFactoryReflection(ea::string_view category)
{
    auto reflection = context_->AddFactoryReflection<T>(category);
    if (reflection)
        reflectedTypes_.push_back(T::GetTypeStatic());
    return reflection;
}

template<typename T>
void Plugin::RegisterObject()
{
    T::RegisterObject(context_);
    reflectedTypes_.push_back(T::GetTypeStatic());
}

template <class T>
void Plugin::RegisterPlugin(const ea::string& name)
{
    const auto factory = +[](Context* context) -> SharedPtr<Plugin>
    {
        return MakeShared<T>(context);
    };
    RegisterPlugin(name, factory);
}

template <class T>
void Plugin::RegisterPlugin()
{
    RegisterPlugin<T>(T::GetStaticPluginName());
}

} // namespace Urho3D

/// Macro for marking a class as manual plugin without automatic registration. Use after `URHO3D_OBJECT`.
#define URHO3D_MANUAL_PLUGIN(pluginName) \
    static const ea::string& GetStaticPluginName() { static const ea::string name{pluginName}; return name; } \
    static void RegisterObject() { Plugin::RegisterPlugin<ClassName>(); } \

/// Macro for exporting a plugin. Should be called once in global namespace in source file.
#if !defined(URHO3D_PLUGINS) || defined(URHO3D_STATIC)
    #define URHO3D_EXPORT_PLUGIN(type) \
        extern "C" void CONCATENATE(RegisterPlugin_, URHO3D_CURRENT_PLUGIN_NAME_SANITIZED)() \
        { \
            Urho3D::Plugin::RegisterPlugin<type>(TO_STRING(URHO3D_CURRENT_PLUGIN_NAME)); \
        }
#else
    #define URHO3D_EXPORT_PLUGIN(type) \
        extern "C" URHO3D_EXPORT_API Urho3D::Plugin* PluginDynamicLibraryMain(Urho3D::Context* context) \
        { \
            Urho3D::Plugin* application = new type(context); \
            application->SetPluginName(TO_STRING(URHO3D_CURRENT_PLUGIN_NAME)); \
            return application; \
        }
#endif

/// Macro for exporting a simple plugin. Should be called once in global namespace in source file.
#define URHO3D_EXPORT_PLUGIN_SIMPLE(onLoad, onUnload) \
    namespace \
    { \
    class SimplePlugin : public Urho3D::Plugin \
    { \
    public: \
        using Plugin::Plugin; \
        void Load() override \
        { \
            onLoad(*this); \
        } \
        void Unload() override \
        { \
            onUnload(*this); \
        } \
    }; \
    } \
    URHO3D_EXPORT_PLUGIN(SimplePlugin)
