#include "DiligentResourceMappingCache.h"

namespace Urho3D
{
    DiligentResourceMappingCache::DiligentResourceMappingCache(Graphics* graphics) : graphics_(graphics) {}
    DiligentResourceMappingCache::~DiligentResourceMappingCache() {
        for (auto it = resourceMaps_.begin(); it != resourceMaps_.end(); it++) {
            it->second->Release();
        }
        resourceMaps_.clear();
    }
    Diligent::IResourceMapping* DiligentResourceMappingCache::CreateOrGetResourceMap(const ea::vector<Diligent::ResourceMappingEntry>& entries) {
        unsigned hash = 0;
        for (auto it = entries.begin(); it != entries.end(); it++) {
            CombineHash(hash, MakeHash(it->Name));
            CombineHash(hash, MakeHash(it->pObject));
        }

        auto res = resourceMaps_.find(hash);
        if (res != resourceMaps_.end())
            return res->second;
        Diligent::IResourceMapping* resourceMapping = nullptr;
        Diligent::ResourceMappingDesc desc;
        desc.pEntries = nullptr;

        graphics_->GetImpl()->GetDevice()->CreateResourceMapping(desc, &resourceMapping);
        assert(resourceMapping);

        for (auto it = entries.begin(); it != entries.end(); it++) {
            resourceMapping->AddResource(it->Name, it->pObject, true);
        }

        resourceMaps_.insert(ea::make_pair(hash, resourceMapping));
        return resourceMapping;
    }
}
