/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::ShaderVariableManagerD3D12 and Diligent::ShaderVariableD3D12Impl classes

//
//  * ShaderVariableManagerD3D12 keeps the list of variables of specific types (static or mutable/dynamic)
//  * Every ShaderVariableD3D12Impl references ResourceAttribs by index from PipelineResourceSignatureD3D12Impl
//  * ShaderVariableManagerD3D12 keeps reference to ShaderResourceCacheD3D12
//  * ShaderVariableManagerD3D12 is used by PipelineResourceSignatureD3D12Impl to manage static resources and by
//    ShaderResourceBindingD3D12Impl to manage mutable and dynamic resources
//
//            _____________________________                   ________________________________________________________________________________
//           |                             |                 |                              |                               |                 |
//      .----|  ShaderVariableManagerD3D12 |---------------->|  ShaderVariableD3D12Impl[0]  |   ShaderVariableD3D12Impl[1]  |     ...         |
//      |    |_____________________________|                 |______________________________|_______________________________|_________________|
//      |                |                                                    |                               |
//      |          m_pSignature                                          m_ResIndex                       m_ResIndex
//      |                |                                                    |                               |
//      |   _____________V____________________                      __________V_______________________________V_________________________________
//      |  |                                  | m_pResourceAttribs |                  |                  |             |                        |
//      |  |PipelineResourceSignatureD3D12Impl|------------------->|    Resource[0]   |    Resource[1]   |     ...     |   Resource[s+m+d-1]    |
//      |  |__________________________________|                    |__________________|__________________|_____________|________________________|
//      |                                                                |                                                    |
// m_ResourceCache                                                       |                                                    |
//      |                                                                | (RootTable, Offset)                               / (RootTable, Offset)
//      |                                                                \                                                  /
//      |     __________________________                   _______________V________________________________________________V_______
//      |    |                          |                 |                                                                        |
//      '--->| ShaderResourceCacheD3D12 |---------------->|                                   Resources                            |
//           |__________________________|                 |________________________________________________________________________|
//

#include "EngineD3D12ImplTraits.hpp"
#include "ShaderResourceVariableD3D.h"
#include "ShaderResourceVariableBase.hpp"
#include "PipelineResourceAttribsD3D12.hpp"

namespace Diligent
{

class ShaderVariableD3D12Impl;
class ShaderResourceCacheD3D12;
class PipelineResourceSignatureD3D12Impl;

// sizeof(ShaderVariableManagerD3D12) == 40 (x64, msvc, Release)
class ShaderVariableManagerD3D12 : public ShaderVariableManagerBase<EngineD3D12ImplTraits, ShaderVariableD3D12Impl>
{
public:
    using TBase = ShaderVariableManagerBase<EngineD3D12ImplTraits, ShaderVariableD3D12Impl>;
    ShaderVariableManagerD3D12(IObject&                  Owner,
                               ShaderResourceCacheD3D12& ResourceCache) noexcept :
        TBase{Owner, ResourceCache}
    {}

    // clang-format off
    ShaderVariableManagerD3D12            (const ShaderVariableManagerD3D12&)  = delete;
    ShaderVariableManagerD3D12            (      ShaderVariableManagerD3D12&&) = delete;
    ShaderVariableManagerD3D12& operator= (const ShaderVariableManagerD3D12&)  = delete;
    ShaderVariableManagerD3D12& operator= (      ShaderVariableManagerD3D12&&) = delete;
    // clang-format on

    void Initialize(const PipelineResourceSignatureD3D12Impl& Signature,
                    IMemoryAllocator&                         Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                    Uint32                                    NumAllowedTypes,
                    SHADER_TYPE                               ShaderStages);

    void Destroy(IMemoryAllocator& Allocator);

    ShaderVariableD3D12Impl* GetVariable(const Char* Name) const;
    ShaderVariableD3D12Impl* GetVariable(Uint32 Index) const;

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

    static size_t GetRequiredMemorySize(const PipelineResourceSignatureD3D12Impl& Signature,
                                        const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                                        Uint32                                    NumAllowedTypes,
                                        SHADER_TYPE                               ShaderStages,
                                        Uint32*                                   pNumVariables = nullptr);

    Uint32 GetVariableCount() const { return m_NumVariables; }

    IObject& GetOwner() { return m_Owner; }

private:
    friend TBase;
    friend ShaderVariableD3D12Impl;
    friend ShaderVariableBase<ShaderVariableD3D12Impl, ShaderVariableManagerD3D12, IShaderResourceVariableD3D>;

    using ResourceAttribs = PipelineResourceAttribsD3D12;

    Uint32 GetVariableIndex(const ShaderVariableD3D12Impl& Variable);

    // These methods can't be defined in the header due to dependency on PipelineResourceSignatureD3D12Impl
    const PipelineResourceDesc& GetResourceDesc(Uint32 Index) const;
    const ResourceAttribs&      GetResourceAttribs(Uint32 Index) const;

private:
    Uint32 m_NumVariables = 0;
};

// sizeof(ShaderVariableD3D12Impl) == 24 (x64)
class ShaderVariableD3D12Impl final : public ShaderVariableBase<ShaderVariableD3D12Impl, ShaderVariableManagerD3D12, IShaderResourceVariableD3D>
{
public:
    using TBase = ShaderVariableBase<ShaderVariableD3D12Impl, ShaderVariableManagerD3D12, IShaderResourceVariableD3D>;
    ShaderVariableD3D12Impl(ShaderVariableManagerD3D12& ParentManager,
                            Uint32                      ResIndex) :
        TBase{ParentManager, ResIndex}
    {}

    // clang-format off
    ShaderVariableD3D12Impl            (const ShaderVariableD3D12Impl&)  = delete;
    ShaderVariableD3D12Impl            (      ShaderVariableD3D12Impl&&) = delete;
    ShaderVariableD3D12Impl& operator= (const ShaderVariableD3D12Impl&)  = delete;
    ShaderVariableD3D12Impl& operator= (      ShaderVariableD3D12Impl&&) = delete;
    // clang-format on

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (IID == IID_ShaderResourceVariableD3D || IID == IID_ShaderResourceVariable || IID == IID_Unknown)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
    {
        return m_ParentManager.Get(ArrayIndex, m_ResIndex);
    }

    virtual void DILIGENT_CALL_TYPE GetHLSLResourceDesc(HLSLShaderResourceDesc& HLSLResDesc) const override final
    {
        GetResourceDesc(HLSLResDesc);
        HLSLResDesc.ShaderRegister = GetAttribs().Register;
    }

    void BindResource(const BindResourceInfo& BindInfo) const
    {
        m_ParentManager.BindResource(m_ResIndex, BindInfo);
    }

    void SetDynamicOffset(Uint32 ArrayIndex,
                          Uint32 BufferRangeOffset)
    {
        m_ParentManager.SetBufferDynamicOffset(m_ResIndex, ArrayIndex, BufferRangeOffset);
    }

private:
    using ResourceAttribs = PipelineResourceAttribsD3D12;
    const ResourceAttribs& GetAttribs() const
    {
        return m_ParentManager.GetResourceAttribs(m_ResIndex);
    }
};

} // namespace Diligent
