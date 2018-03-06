//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../Core/Object.h"


namespace Urho3D
{

/// Base class for clearing plugins for Editor.
class URHO3D_API PluginApplication : public Object
{
    URHO3D_OBJECT(PluginApplication, Object);
public:
    /// Construct.
    explicit PluginApplication(Context* context) : Object(context) { }
    /// Destruct.
    ~PluginApplication() override = default;
    /// Called when plugin is being loaded. Register all custom components and subscribe to events here.
    virtual void OnLoad() = 0;
    /// Called when plugin is being unloaded. Unregister all custom components and unsubscribe from events here.
    virtual void OnUnload() = 0;
    /// Called on every frame during E_UPDATE.
    virtual void OnUpdate() { }
};

/// Main function of the plugin.
int URHO3D_API PluginMain(void* ctx, size_t operation, PluginApplication*(*factory)(Context*),
    void(*destroyer)(PluginApplication*));

}

/// Macro for defining entry point of editor plugin.
#define URHO3D_DEFINE_PLUGIN_MAIN(Class)                                  \
    extern "C" URHO3D_EXPORT_API int cr_main(void* ctx, size_t operation) \
    {                                                                     \
        return Urho3D::PluginMain(ctx, operation,                         \
            [](Context* context) -> PluginApplication* {                  \
                return new Class(context);                                \
            }, [](PluginApplication* plugin) {                            \
                delete plugin;                                            \
            });                                                           \
    }
