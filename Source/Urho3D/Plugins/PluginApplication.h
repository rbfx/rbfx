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

#include "../Core/Context.h"
#include "../Core/Object.h"

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
    template <class T> static void RegisterPluginApplication();

    explicit PluginApplication(Context* context);
    ~PluginApplication() override;
    void SetPluginName(const ea::string& name) { pluginName_ = name; }

    /// Prepare object for destruction.
    void Dispose();

    /// Load plugin into the context and the engine subsystems.
    void LoadPlugin();
    /// Unload plugin from the context and the engine subsystems.
    void UnloadPlugin();

    /// Start application.
    void StartApplication();
    /// Stop application.
    void StopApplication();

    /// Suspend application. It's highly recommended to release all plugin-related objects here.
    void SuspendApplication(Archive& output, unsigned version);
    /// Resume application.
    void ResumeApplication(Archive& input, unsigned version);

    /// Return plugin name. Should be the same as dynamic library name when plugin is linked dynamically.
    const ea::string& GetPluginName() { return pluginName_; }
    /// Return whether the plugin is loaded.
    bool IsLoaded() const { return isLoaded_; }
    /// Return whether the application is started.
    bool IsStarted() const { return isStarted_; }

    /// Register a factory for an object type that would be automatically unregistered on unload.
    template<typename T> ObjectReflection* AddFactoryReflection(ea::string_view category = "");
    /// Register an object that would be automatically unregistered on unload.
    template<typename T> void AddObjectReflection();

protected:
    /// Called on LoadPlugin().
    virtual void Load() {}
    /// Called on UnloadPlugin().
    virtual void Unload() {}
    /// Called on StartApplication().
    virtual void Start() {}
    /// Called on StopApplication().
    virtual void Stop() {}
    /// Called on SuspendApplication().
    virtual void Suspend(Archive& output) {}
    /// Called on ResumeApplication().
    virtual void Resume(Archive& input, bool differentVersion) {}

private:
    ea::string pluginName_;
    ea::vector<StringHash> reflectedTypes_;

    bool isLoaded_{};
    bool isStarted_{};
};

template<typename T>
ObjectReflection* PluginApplication::AddFactoryReflection(ea::string_view category)
{
    auto reflection = context_->AddFactoryReflection<T>(category);
    if (reflection)
        reflectedTypes_.push_back(T::GetTypeStatic());
    return reflection;
}

template<typename T>
void PluginApplication::AddObjectReflection()
{
    T::RegisterObject(context_);
    reflectedTypes_.push_back(T::GetTypeStatic());
}

template <class T>
void PluginApplication::RegisterPluginApplication()
{
    const auto factory = +[](Context* context) -> SharedPtr<PluginApplication>
    {
        return MakeShared<T>(context);
    };
    RegisterPluginApplication(T::GetPluginNameStatic(), factory);
}

}

/// Macro for using instead of URHO3D_OBJECT for plugin applications.
#define URHO3D_PLUGIN_OBJECT(typeName, baseTypeName, pluginName) \
    URHO3D_OBJECT(typeName, baseTypeName); \
    static const char* GetPluginNameStatic() { return pluginName; }

/// Macro for defining entry point of editor plugin.
// TODO(editor): Revisit macros
#if !defined(URHO3D_PLUGINS) || defined(URHO3D_STATIC)
    #define URHO3D_DEFINE_PLUGIN_MAIN(type)
#else
     /// Defines a main entry point of native plugin. Use this macro in a global scope.
    #define URHO3D_DEFINE_PLUGIN_MAIN(type) \
        extern "C" URHO3D_EXPORT_API Urho3D::PluginApplication* PluginApplicationMain(Urho3D::Context* context) \
        { \
            Urho3D::PluginApplication* application = new type(context); \
            application->SetPluginName(type::GetPluginNameStatic()); \
            return application; \
        }
#endif
