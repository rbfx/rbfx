/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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
/// Declaration of Diligent::ShaderVariableManagerVk and Diligent::ShaderVariableVkImpl classes

//
//  * ShaderVariableManagerVk keeps the list of variables of specific types (static or mutable/dynamic)
//  * Every ShaderVariableVkImpl references ResourceAttribs by index from PipelineResourceSignatureVkImpl
//  * ShaderVariableManagerVk keeps reference to ShaderResourceCacheVk
//  * ShaderVariableManagerVk is used by PipelineResourceSignatureVkImpl to manage static resources and by
//    ShaderResourceBindingVkImpl to manage mutable and dynamic resources
//
//          __________________________                             __________________________________________________________________________
//         |                          |                           |                           |                            |                 |
//    .----|  ShaderVariableManagerVk |-------------------------->|  ShaderVariableVkImpl[0]  |   ShaderVariableVkImpl[1]  |     ...         |
//    |    |__________________________|                           |___________________________|____________________________|_________________|
//    |                |                                                              \                          |
//    |           m_pSignature                                                     m_ResIndex               m_ResIndex
//    |                |                                                                \                        |
//    |     ___________V_____________________                      ______________________V_______________________V____________________________
//    |    |                                 | m_pResourceAttribs |                  |                |               |                     |
//    |    | PipelineResourceSignatureVkImpl |------------------->|    Resource[0]   |   Resource[1]  |       ...     |  Resource[s+m+d-1]  |
//    |    |_________________________________|                    |__________________|________________|_______________|_____________________|
//    |                                                                  |                                                        |
//    |                                                                  |                                                        |
//    |                                                                  | (DescriptorSet, CacheOffset)                          / (DescriptorSet, CacheOffset)
//    |                                                                   \                                                     /
//    |     __________________________                             ________V___________________________________________________V_______
//    |    |                          |                           |                                                                    |
//    '--->|   ShaderResourceCacheVk  |-------------------------->|                                   Resources                        |
//         |__________________________|                           |____________________________________________________________________|
//
//

#include <memory>

#include "EngineVkImplTraits.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "ShaderResourceCacheVk.hpp"
#include "PipelineResourceAttribsVk.hpp"

namespace Diligent
{

class ShaderVariableVkImpl;

// sizeof(ShaderVariableManagerVk) == 40 (x64, msvc, Release)
class ShaderVariableManagerVk : ShaderVariableManagerBase<EngineVkImplTraits, ShaderVariableVkImpl>
{
public:
    using TBase = ShaderVariableManagerBase<EngineVkImplTraits, ShaderVariableVkImpl>;
    ShaderVariableManagerVk(IObject&               Owner,
                            ShaderResourceCacheVk& ResourceCache) noexcept :
        TBase{Owner, ResourceCache}
    {}

    void Initialize(const PipelineResourceSignatureVkImpl& Signature,
                    IMemoryAllocator&                      Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                    Uint32                                 NumAllowedTypes,
                    SHADER_TYPE                            ShaderType);

    void Destroy(IMemoryAllocator& Allocator);

    ShaderVariableVkImpl* GetVariable(const Char* Name) const;
    ShaderVariableVkImpl* GetVariable(Uint32 Index) const;

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

    static size_t GetRequiredMemorySize(const PipelineResourceSignatureVkImpl& Signature,
                                        const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                        Uint32                                 NumAllowedTypes,
                                        SHADER_TYPE                            ShaderStages,
                                        Uint32*                                pNumVariables = nullptr);

    Uint32 GetVariableCount() const { return m_NumVariables; }

    IObject& GetOwner() { return m_Owner; }

private:
    friend TBase;
    friend ShaderVariableVkImpl;
    friend ShaderVariableBase<ShaderVariableVkImpl, ShaderVariableManagerVk, IShaderResourceVariable>;

    using ResourceAttribs = PipelineResourceAttribsVk;

    Uint32 GetVariableIndex(const ShaderVariableVkImpl& Variable);

    // These two methods can't be implemented in the header because they depend on PipelineResourceSignatureVkImpl
    const PipelineResourceDesc& GetResourceDesc(Uint32 Index) const;
    const ResourceAttribs&      GetResourceAttribs(Uint32 Index) const;

private:
    Uint32 m_NumVariables = 0;
};

// sizeof(ShaderVariableVkImpl) == 24 (x64)
class ShaderVariableVkImpl final : public ShaderVariableBase<ShaderVariableVkImpl, ShaderVariableManagerVk, IShaderResourceVariable>
{
public:
    using TBase = ShaderVariableBase<ShaderVariableVkImpl, ShaderVariableManagerVk, IShaderResourceVariable>;

    ShaderVariableVkImpl(ShaderVariableManagerVk& ParentManager,
                         Uint32                   ResIndex) :
        TBase{ParentManager, ResIndex}
    {}

    // clang-format off
    ShaderVariableVkImpl            (const ShaderVariableVkImpl&) = delete;
    ShaderVariableVkImpl            (ShaderVariableVkImpl&&)      = delete;
    ShaderVariableVkImpl& operator= (const ShaderVariableVkImpl&) = delete;
    ShaderVariableVkImpl& operator= (ShaderVariableVkImpl&&)      = delete;
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
