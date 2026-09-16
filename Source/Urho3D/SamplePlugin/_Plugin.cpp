#include "_Plugin.h"

#include "SampleComponent.h"

#include <Urho3D/Plugins/PluginApplication.h>

using namespace Urho3D;

namespace
{

void RegisterPluginObjects(PluginApplication& plugin)
{
    plugin.RegisterObject<SampleComponent>();
}

void UnregisterPluginObjects(PluginApplication& plugin)
{
}

} // namespace

URHO3D_DEFINE_PLUGIN_MAIN_SIMPLE(RegisterPluginObjects, UnregisterPluginObjects);
