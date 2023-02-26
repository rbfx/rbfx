#pragma once
#include <Diligent/Graphics/GraphicsEngine/interface/ResourceMapping.h>
#include "DiligentGraphicsImpl.h"

namespace Urho3D
{
    class DiligentResourceMappingCache {
    public:
        DiligentResourceMappingCache(Graphics* graphics);
        ~DiligentResourceMappingCache();
        Diligent::IResourceMapping* CreateOrGetResourceMap(const ea::vector<Diligent::ResourceMappingEntry>& entries);
    private:
        Graphics* graphics_;
        ea::unordered_map<unsigned, Diligent::IResourceMapping*> resourceMaps_;
    };
}
