/*
 *  Copyright 2023-2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

/// \file
/// Declaration of Diligent::ShaderVariableManagerWebGPU class

#include "EngineWebGPUImplTraits.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "ShaderResourceCacheWebGPU.hpp"
#include "PipelineResourceAttribsWebGPU.hpp"

namespace Diligent
{

class ShaderVariableWebGPUImpl;

class ShaderVariableManagerWebGPU : ShaderVariableManagerBase<EngineWebGPUImplTraits, ShaderVariableWebGPUImpl>
{
public:
    using TBase = ShaderVariableManagerBase<EngineWebGPUImplTraits, ShaderVariableWebGPUImpl>;
    ShaderVariableManagerWebGPU(IObject&                   Owner,
                                ShaderResourceCacheWebGPU& ResourceCache) noexcept :
        TBase{Owner, ResourceCache}
    {}

    void Initialize(const PipelineResourceSignatureWebGPUImpl& Signature,
                    IMemoryAllocator&                          Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE*       AllowedVarTypes,
                    Uint32                                     NumAllowedTypes,
                    SHADER_TYPE                                ShaderType);

    void Destroy(IMemoryAllocator& Allocator);

    ShaderVariableWebGPUImpl* GetVariable(const Char* Name) const;
    ShaderVariableWebGPUImpl* GetVariable(Uint32 Index) const;

    void BindResource(Uint32 ResIndex, const BindResourceInfo& BindInfo);

    void SetBufferDynamicOffset(Uint32 ResIndex,
                                Uint32 ArrayIndex,
                                Uint32 BufferDynamicOffset);

    IDeviceObject* Get(Uint32 ArrayIndex,
                       Uint32 ResIndex) const;

    void BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags);

    void CheckResources(IResourceMapping*                    pResourceMapping,
                        BIND_SHADER_RESOURCES_FLAGS          Flags,
                        SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const;

    static size_t GetRequiredMemorySize(const PipelineResourceSignatureWebGPUImpl& Signature,
                                        const SHADER_RESOURCE_VARIABLE_TYPE*       AllowedVarTypes,
                                        Uint32                                     NumAllowedTypes,
                                        SHADER_TYPE                                ShaderStages,
                                        Uint32*                                    pNumVariables = nullptr);

    Uint32 GetVariableCount() const { return m_NumVariables; }

    IObject& GetOwner() { return m_Owner; }

private:
    friend TBase;
    friend ShaderVariableWebGPUImpl;
    friend ShaderVariableBase<ShaderVariableWebGPUImpl, ShaderVariableManagerWebGPU, IShaderResourceVariable>;

    using ResourceAttribs = PipelineResourceAttribsWebGPU;

    Uint32 GetVariableIndex(const ShaderVariableWebGPUImpl& Variable);

    // These two methods can't be implemented in the header because they depend on PipelineResourceSignatureWebGPUImpl
    const PipelineResourceDesc& GetResourceDesc(Uint32 Index) const;
    const ResourceAttribs&      GetResourceAttribs(Uint32 Index) const;

private:
    Uint32 m_NumVariables = 0;
};

class ShaderVariableWebGPUImpl final : public ShaderVariableBase<ShaderVariableWebGPUImpl, ShaderVariableManagerWebGPU, IShaderResourceVariable>
{
public:
    using TBase = ShaderVariableBase<ShaderVariableWebGPUImpl, ShaderVariableManagerWebGPU, IShaderResourceVariable>;

    ShaderVariableWebGPUImpl(ShaderVariableManagerWebGPU& ParentManager,
                             Uint32                       ResIndex) :
        TBase{ParentManager, ResIndex}
    {}

    // clang-format off
    ShaderVariableWebGPUImpl           (const ShaderVariableWebGPUImpl&)  = delete;
    ShaderVariableWebGPUImpl           (      ShaderVariableWebGPUImpl&&) = delete;
    ShaderVariableWebGPUImpl& operator=(const ShaderVariableWebGPUImpl&)  = delete;
    ShaderVariableWebGPUImpl& operator=(      ShaderVariableWebGPUImpl&&) = delete;
    // clang-format on

    virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
    {
        return m_ParentManager.Get(ArrayIndex, m_ResIndex);
    }

    void BindResource(const BindResourceInfo& BindInfo) const
    {
        m_ParentManager.BindResource(m_ResIndex, BindInfo);
    }

    void SetDynamicOffset(Uint32 ArrayIndex,
                          Uint32 BufferDynamicOffset) const
    {
        m_ParentManager.SetBufferDynamicOffset(m_ResIndex, ArrayIndex, BufferDynamicOffset);
    }
};

} // namespace Diligent
