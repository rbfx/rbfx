// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Plugins/Plugin.h"

namespace Urho3D
{

// Deprecated. Use Plugin instead.
using PluginApplication = Plugin;
// Deprecated. Use PluginFactory instead.
using PluginApplicationFactory = PluginFactory;
// Deprecated. Use ExecutablePlugin instead.
using MainPluginApplication = ExecutablePlugin;

} // namespace Urho3D

/// Deprecated. Use URHO3D_EXPORT_PLUGIN instead.
#define URHO3D_DEFINE_PLUGIN_MAIN URHO3D_EXPORT_PLUGIN
/// Deprecated. Use URHO3D_EXPORT_PLUGIN_SIMPLE instead.
#define URHO3D_DEFINE_PLUGIN_MAIN_SIMPLE URHO3D_EXPORT_PLUGIN_SIMPLE
