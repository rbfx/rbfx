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
#include "../Core/PluginModule.h"


namespace Urho3D
{

/// Base class for creating plugins for Editor.
class URHO3D_API PluginApplication : public Object
{
    URHO3D_OBJECT(PluginApplication, Object);
public:
    /// Construct.
    explicit PluginApplication(Context* context) : Object(context) { }
    /// Destruct.
    ~PluginApplication() override = default;
    /// Internal, optional. Finalize object initialization. Depends on calling virtual methods and thus can not be done from constructor. Should be called after creating this object. May not be called if module reloading is not required.
    void InitializeReloadablePlugin();
    /// Internal, optional. Unregister plugin and it's types from the engine. Should be called before freeing this object. May not be called if module reloading is not required.
    void UninitializeReloadablePlugin();
    /// Called when plugin is being loaded. Register all custom components and subscribe to events here.
    virtual void Load() { }
    /// Called when application is started. May be called multiple times but no earlier than before next Stop() call.
    virtual void Start() { }
    /// Called when application is stopped.
    virtual void Stop() { }
    /// Called when plugin is being unloaded. Unregister all custom components and unsubscribe from events here.
    virtual void Unload() { }
    /// Register a factory for an object type.
    template<typename T> void RegisterFactory();
    /// Register a factory for an object type and specify the object category.
    template<typename T> void RegisterFactory(const char* category);

protected:
    /// Record type factory that will be unregistered on plugin unload.
    void RecordPluginFactory(StringHash type, const char* category);

private:
    /// Types registered with the engine. They will be unloaded when plugin is reloaded.
    ea::vector<ea::pair<StringHash, ea::string>> registeredTypes_{};
};

template<typename T>
void PluginApplication::RegisterFactory()
{
    context_->RegisterFactory<T>();
    RecordPluginFactory(T::GetTypeStatic(), nullptr);
}

template<typename T>
void PluginApplication::RegisterFactory(const char* category)
{
    context_->RegisterFactory<T>(category);
    RecordPluginFactory(T::GetTypeStatic(), category);
}

}

/// Macro for defining entry point of editor plugin.
#if !defined(URHO3D_PLUGINS)
#   define URHO3D_DEFINE_PLUGIN_MAIN(Class)
#   define URHO3D_DEFINE_PLUGIN_STATIC(Class)
#elif defined(URHO3D_STATIC)
    /// Noop in static builds.
#   define URHO3D_DEFINE_PLUGIN_MAIN(Class)
    /// Creates an instance of plugin application and calls its Load() method. Use this macro in Application::Start() method.
#   define URHO3D_DEFINE_PLUGIN_STATIC(Class) RegisterPlugin(new Class(context_))
#else
     /// Noop in shared builds.
#    define URHO3D_DEFINE_PLUGIN_STATIC(Class)
     /// Defines a main entry point of native plugin. Use this macro in a global scope.
#    define URHO3D_DEFINE_PLUGIN_MAIN(Class)                                                                    \
        extern "C" URHO3D_EXPORT_API Urho3D::PluginApplication* PluginApplicationMain(Urho3D::Context* context) \
        {                                                                                                       \
            context->RegisterFactory<Class>();                                                                  \
            return static_cast<Class*>(context->CreateObject<Class>().Detach());                                \
        }
#endif
