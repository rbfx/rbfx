#include "../ShaderResourceBinding.h"
#include "DiligentGraphicsImpl.h"
#include "./DiligentLookupSettings.h"
#include "./DiligentResourceMappingCache.h"

namespace Urho3D
{
    using namespace Diligent;
    void ShaderResourceBinding::ReleaseResources() {
        shaderResBindingObj_ = nullptr;
    }
    void ShaderResourceBinding::UpdateInternalBindings() {
        hash_ = 0u;
        IShaderResourceBinding* shaderResBinding = static_cast<IShaderResourceBinding*>(shaderResBindingObj_);
        DiligentResourceMappingCache* resMappingCache = graphics_->GetImpl()->GetResourceMappingCache();
        ea::vector<ResourceMappingEntry> resourceEntries;

        // Add Constant Buffers to Resource Mapping Entries
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i) {
            if (constantBuffers_[i] == nullptr)
                continue;
            ResourceMappingEntry resMap;
            resMap.Name = shaderParameterGroupNames[i];
            resMap.pObject = static_cast<IDeviceObject*>(constantBuffers_[i]->GetGPUObject());

            resourceEntries.push_back(resMap);

            CombineHash(hash_, constantBuffers_[i]->ToHash());
        }

        const SHADER_TYPE shaderTypes[] = {
            SHADER_TYPE_VERTEX,
            SHADER_TYPE_PIXEL,
            SHADER_TYPE_GEOMETRY,
            SHADER_TYPE_DOMAIN,
            SHADER_TYPE_HULL
        };
        ea::vector<ea::shared_ptr<ea::string>> strList;
        for (unsigned i = 0; i < _countof(shaderTypes); ++i) {
            SHADER_TYPE shaderType = shaderTypes[i];
            // Extract Shader Resource Textures used on all stages
            unsigned varCount = shaderResBinding->GetVariableCount(shaderType);
            for (unsigned j = 0; j < varCount; ++j) {
                IShaderResourceVariable* shaderResVar = shaderResBinding->GetVariableByIndex(shaderType, j);
                ShaderResourceDesc shaderResDesc;
                shaderResVar->GetResourceDesc(shaderResDesc);

                if (shaderResDesc.Type == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER || shaderResDesc.Type == SHADER_RESOURCE_TYPE_SAMPLER)
                    continue;
                ea::string shaderResName = shaderResDesc.Name;

                if (shaderResName.starts_with("s"))
                    shaderResName = shaderResName.substr(1, shaderResName.size());

                auto texUnitIt = DiligentTextureUnitLookup.find(shaderResName);
                if (texUnitIt == DiligentTextureUnitLookup.end())
                    continue;

                assert(textures_[texUnitIt->second] != nullptr);

                ea::shared_ptr<ea::string> resSamplerName = ea::make_shared<ea::string>(Format("_{}_sampler",shaderResDesc.Name));
                strList.push_back(resSamplerName);

                Texture* tex = textures_[texUnitIt->second];
                if (tex->GetLevelsDirty())
                    tex->RegenerateLevels();
                if (tex->GetParametersDirty())
                    tex->UpdateParameters();

                {   // Add Texture Resource
                    ResourceMappingEntry resMap;
                    resMap.Name = shaderResDesc.Name;
                    resMap.pObject = static_cast<IDeviceObject*>(tex->GetShaderResourceView());
                    resourceEntries.push_back(resMap);
                }
                {   // Add Texture Sampler
                    ResourceMappingEntry resMap;
                    resMap.Name = resSamplerName->c_str();
                    resMap.pObject = static_cast<IDeviceObject*>(tex->GetSampler());
                    assert(resMap.pObject);
                    resourceEntries.push_back(resMap);
                }

                CombineHash(hash_, (unsigned long long)textures_[texUnitIt->second].Get());
            }
        }

        Diligent::IResourceMapping* resMapping = resMappingCache->CreateOrGetResourceMap(resourceEntries);
        shaderResBinding->BindResources(SHADER_TYPE_ALL, resMapping, BIND_SHADER_RESOURCES_UPDATE_ALL);

        strList.clear();
        dirty_ = false;
    }
}
