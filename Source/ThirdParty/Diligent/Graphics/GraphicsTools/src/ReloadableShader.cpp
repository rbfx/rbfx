/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "ReloadableShader.hpp"
#include "RenderStateCacheImpl.hpp"

namespace Diligent
{

constexpr INTERFACE_ID ReloadableShader::IID_InternalImpl;

ReloadableShader::ReloadableShader(IReferenceCounters*     pRefCounters,
                                   RenderStateCacheImpl*   pStateCache,
                                   IShader*                pShader,
                                   const ShaderCreateInfo& CreateInfo) :
    TBase{pRefCounters},
    m_pStateCache{pStateCache},
    m_pShader{pShader},
    m_CreateInfo{CreateInfo, GetRawAllocator()}
{
    if (!m_pShader)
    {
        LOG_ERROR_AND_THROW("Internal shader object must not be null");
    }
}

ReloadableShader::~ReloadableShader()
{
}

void ReloadableShader::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;

    DEV_CHECK_ERR(*ppInterface == nullptr, "Overwriting reference to an existing object may result in memory leaks");
    *ppInterface = nullptr;

    if (IID == IID_InternalImpl || IID == IID_Shader || IID == IID_DeviceObject || IID == IID_Unknown)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        // This will handle implementation-specific interfaces such as ShaderD3D11Impl::IID_InternalImpl,
        // ShaderD3D12Impl::IID_InternalImpl, etc. requested by e.g. pipeline state implementations
        // (PipelineStateD3D11Impl, PipelineStateD3D12Impl, etc.)
        m_pShader->QueryInterface(IID, ppInterface);
    }

    if (*ppInterface == nullptr)
    {
        // This will handle IID_SerializedShader.
        RefCntAutoPtr<IObject> pObject;
        m_pShader->GetReferenceCounters()->QueryObject(&pObject);
        pObject->QueryInterface(IID, ppInterface);
    }
}

bool ReloadableShader::Reload()
{
    RefCntAutoPtr<IShader> pNewShader;

    const bool FoundInCache = m_pStateCache->CreateShaderInternal(m_CreateInfo, &pNewShader);
    if (pNewShader)
    {
        m_pShader = pNewShader;
    }
    else
    {
        const char* Name = m_CreateInfo.Get().Desc.Name;
        LOG_ERROR_MESSAGE("Failed to reload shader '", (Name ? Name : "<unnamed>"), "'.");
    }
    return !FoundInCache;
}


void ReloadableShader::Create(RenderStateCacheImpl*   pStateCache,
                              IShader*                pShader,
                              const ShaderCreateInfo& CreateInfo,
                              IShader**               ppReloadableShader)
{
    try
    {
        RefCntAutoPtr<ReloadableShader> pReloadableShader{MakeNewRCObj<ReloadableShader>()(pStateCache, pShader, CreateInfo)};
        *ppReloadableShader = pReloadableShader.Detach();
    }
    catch (...)
    {
        LOG_ERROR("Failed to create reloadable shader '", (CreateInfo.Desc.Name ? CreateInfo.Desc.Name : "<unnamed>"), "'.");
    }
}

} // namespace Diligent
