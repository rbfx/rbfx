// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Core/EditorPluginManager.h"

namespace Urho3D
{

EditorPluginManager::EditorPluginManager(Context* context)
    : Object(context)
{
}

EditorPluginManager::~EditorPluginManager()
{
}

void EditorPluginManager::AddPlugin(SharedPtr<EditorPlugin> plugin)
{
    plugins_.push_back(plugin);
}

void EditorPluginManager::Apply(Object* target)
{
    for (EditorPlugin* plugin : plugins_)
        plugin->Apply(target);
}

}
