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

#pragma once

/// \file
/// Definition of the Diligent::ReloadableShader class

#include "Shader.h"
#include "ShaderBase.hpp"

namespace Diligent
{

class RenderStateCacheImpl;

/// Reloadable shader implements the IShader interface and delegates all
/// calls to the internal shader object, which can be replaced at run-time.
class ReloadableShader final : public ObjectBase<IShader>
{
public:
    using TBase = ObjectBase<IShader>;

    // {6BFAAABD-FE55-4420-B0C8-5C4B4F5F8D65}
    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x6bfaaabd, 0xfe55, 0x4420, {0xb0, 0xc8, 0x5c, 0x4b, 0x4f, 0x5f, 0x8d, 0x65}};

    ReloadableShader(IReferenceCounters*     pRefCounters,
                     RenderStateCacheImpl*   pStateCache,
                     IShader*                pShader,
                     const ShaderCreateInfo& CreateInfo);

    ~ReloadableShader();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    // Delegate all calls to the internal shader object

    virtual const ShaderDesc& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        return m_pShader->GetDesc();
    }

    virtual Int32 DILIGENT_CALL_TYPE GetUniqueID() const override final
    {
        return m_pShader->GetUniqueID();
    }

    virtual void DILIGENT_CALL_TYPE SetUserData(IObject* pUserData) override final
    {
        m_pShader->SetUserData(pUserData);
    }

    virtual IObject* DILIGENT_CALL_TYPE GetUserData() const override final
    {
        return m_pShader->GetUserData();
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        return m_pShader->GetResourceCount();
    }

    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final
    {
        m_pShader->GetResourceDesc(Index, ResourceDesc);
    }

    virtual const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final
    {
        return m_pShader->GetConstantBufferDesc(Index);
    }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const void** ppBytecode, Uint64& Size) const override final
    {
        m_pShader->GetBytecode(ppBytecode, Size);
    }

    virtual SHADER_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override final
    {
        return m_pShader->GetStatus(WaitForCompletion);
    }

    static void Create(RenderStateCacheImpl*   pStateCache,
                       IShader*                pShader,
                       const ShaderCreateInfo& CreateInfo,
                       IShader**               ppReloadableShader);

    bool Reload();

private:
    RefCntAutoPtr<RenderStateCacheImpl> m_pStateCache;
    RefCntAutoPtr<IShader>              m_pShader;
    ShaderCreateInfoWrapper             m_CreateInfo;
};

} // namespace Diligent
