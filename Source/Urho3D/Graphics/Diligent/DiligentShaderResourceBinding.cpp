#include "../ShaderResourceBinding.h"
#include "DiligentGraphicsImpl.h"
#include "./DiligentLookupSettings.h"

namespace Urho3D
{
    using namespace Diligent;
    void ShaderResourceBinding::ReleaseResources() {
        shaderResBindingObj_ = nullptr;
    }
    void ShaderResourceBinding::UpdateInternalBindings() {
        hash_ = 0u;
        RefCntAutoPtr<IShaderResourceBinding> shaderResBinding = shaderResBindingObj_;
        
        ResourceMappingDesc resMappingDesc;
        resMappingDesc.pEntries = nullptr;
        RefCntAutoPtr<Diligent::IResourceMapping> resMapping;
        graphics_->GetImpl()->GetDevice()->CreateResourceMapping(resMappingDesc, &resMapping);
        assert(resMapping);

        // Add Constant Buffers to Resource Mapping Entries
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i) {
            if (constantBuffers_[i] == nullptr)
                continue;

            resMapping->AddResource(
                shaderParameterGroupNames[i],
                constantBuffers_[i]->GetGPUObject(),
                true
            );
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

                {   // Add Texture Resource
                    resMapping->AddResource(
                        shaderResDesc.Name,
                        tex->GetShaderResourceView(),
                        true
                    );
                }
                {   // Add Texture Sampler
                    resMapping->AddResource(
                        resSamplerName->c_str(),
                        tex->GetSampler(),
                        true
                    );
                }

                CombineHash(hash_, (unsigned long long)textures_[texUnitIt->second].Get());
            }
        }

        shaderResBinding->BindResources(SHADER_TYPE_ALL, resMapping, BIND_SHADER_RESOURCES_UPDATE_ALL);

        strList.clear();
        dirty_ = false;
    }
}
