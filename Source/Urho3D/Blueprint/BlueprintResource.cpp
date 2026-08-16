// SPDX-License-Identifier: MIT

#include "BlueprintResource.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/Serializer.h>

namespace Urho3D
{

BlueprintResource::BlueprintResource(Context* context)
    : Resource(context)
{
}

void BlueprintResource::RegisterObject(Context* context)
{
    context->RegisterFactory<BlueprintResource>();
}

bool BlueprintResource::BeginLoad(Deserializer& source)
{
    const unsigned size = source.GetSize();
    ea::string json;
    json.resize(size);
    if (size && source.Read(json.data(), size) != size)
        return false;

    BlueprintGraph graph;
    ea::string error;
    if (!graph.FromString(json, &error))
    {
        URHO3D_LOGERROR("Could not load Blueprint resource '{}': {}", source.GetName(), error);
        return false;
    }

    graph_ = ea::move(graph);
    return true;
}

bool BlueprintResource::Save(Serializer& dest) const
{
    const ea::string json = graph_.ToString();
    return dest.Write(json.data(), json.size()) == json.size();
}

} // namespace Urho3D
