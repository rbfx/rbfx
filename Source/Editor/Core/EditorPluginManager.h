// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/EditorPlugin.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class EditorPluginManager : public Object
{
    URHO3D_OBJECT(EditorPluginManager, Object);

public:
    EditorPluginManager(Context* context);
    ~EditorPluginManager() override;

    /// Add new editor plugin. Should be called before any plugin user is initialized.
    void AddPlugin(SharedPtr<EditorPlugin> plugin);
    template <class T> void AddPlugin(const ea::string& name, EditorPluginFunction<T> function);
    /// Apply all plugins to the target.
    void Apply(Object* target);

    /// Return all plugins.
    const ea::vector<SharedPtr<EditorPlugin>>& GetPlugins() const { return plugins_; }

private:
    ea::vector<SharedPtr<EditorPlugin>> plugins_;
};

template <class T> void EditorPluginManager::AddPlugin(const ea::string& name, EditorPluginFunction<T> function)
{
    AddPlugin(MakeShared<EditorPluginT<T>>(context_, name, function));
}

}
