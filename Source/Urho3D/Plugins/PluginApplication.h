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

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/Object.h"

namespace Urho3D
{

class PluginApplication;
using PluginApplicationFactory = SharedPtr<PluginApplication>(*)(Context* context);

/// Base class for creating dynamically linked plugins.
class URHO3D_API PluginApplication : public Object
{
    URHO3D_OBJECT(PluginApplication, Object);

public:
    /// Register plugin application class to be visible in all future instances of PluginManager.
    static void RegisterPluginApplication(const ea::string& name, PluginApplicationFactory factory);
    template <class T> static void RegisterPluginApplication(const ea::string& name);
    template <class T> static void RegisterPluginApplication();

    explicit PluginApplication(Context* context);
    ~PluginApplication() override;
    void SetPluginName(const ea::string& name) { pluginName_ = name; }

    /// Return whether the plugin can act as main entry point.
    virtual bool IsMain() const { return false; }
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

/// Similar to PluginApplication, but can act as entry point.
class URHO3D_API MainPluginApplication : public PluginApplication
{
    URHO3D_OBJECT(MainPluginApplication, PluginApplication);

public:
    explicit MainPluginApplication(Context* context);
    ~MainPluginApplication() override;

    /// Implement PluginApplication.
    /// @{
    bool IsMain() const final { return true; }
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
ObjectReflection* PluginApplication::AddFactoryReflection()
{
    auto reflection = context_->AddFactoryReflection<T>(GetDefaultCategory());
    if (reflection)
        reflectedTypes_.push_back(T::GetTypeStatic());
    return reflection;
}

template<typename T>
ObjectReflection* PluginApplication::AddFactoryReflection(ea::string_view category)
{
    auto reflection = context_->AddFactoryReflection<T>(category);
    if (reflection)
        reflectedTypes_.push_back(T::GetTypeStatic());
    return reflection;
}

template<typename T>
void PluginApplication::RegisterObject()
{
    T::RegisterObject(context_);
    reflectedTypes_.push_back(T::GetTypeStatic());
}

template <class T>
void PluginApplication::RegisterPluginApplication(const ea::string& name)
{
    const auto factory = +[](Context* context) -> SharedPtr<PluginApplication>
    {
        return MakeShared<T>(context);
    };
    RegisterPluginApplication(name, factory);
}

template <class T>
void PluginApplication::RegisterPluginApplication()
{
    RegisterPluginApplication<T>(T::GetStaticPluginName());
}

}

/// Macro for marking a class as manual plugin application that doesn't need any automatic registration. Use after `URHO3D_OBJECT`.
#define URHO3D_MANUAL_PLUGIN(pluginName) \
    static const ea::string& GetStaticPluginName() { static const ea::string name{pluginName}; return name; } \
    static void RegisterObject() { PluginApplication::RegisterPluginApplication<ClassName>(); } \

/// Macro for defining entry point of dynamically linked plugin.
#if !defined(URHO3D_PLUGINS) || defined(URHO3D_STATIC)
    #define URHO3D_DEFINE_PLUGIN_MAIN(type) \
        extern "C" void CONCATENATE(RegisterPlugin_, URHO3D_CURRENT_PLUGIN_NAME_SANITATED)() \
        { \
            Urho3D::PluginApplication::RegisterPluginApplication<type>(TO_STRING(URHO3D_CURRENT_PLUGIN_NAME)); \
        }
#else
    /// Defines a main entry point of native plugin. Use this macro in a global scope.
    #define URHO3D_DEFINE_PLUGIN_MAIN(type) \
        extern "C" URHO3D_EXPORT_API Urho3D::PluginApplication* PluginApplicationMain(Urho3D::Context* context) \
        { \
            Urho3D::PluginApplication* application = new type(context); \
            application->SetPluginName(TO_STRING(URHO3D_CURRENT_PLUGIN_NAME)); \
            return application; \
        }
#endif

/// Macro for defining entry point of simple plugin.
/// Simple plugin contains only Load function that should register all necessary objects.
#define URHO3D_DEFINE_PLUGIN_MAIN_SIMPLE(onLoad, onUnload) \
    namespace \
    { \
    class PluginApplicationWrapper : public Urho3D::PluginApplication \
    { \
    public: \
        using PluginApplication::PluginApplication; \
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
    URHO3D_DEFINE_PLUGIN_MAIN(PluginApplicationWrapper)
