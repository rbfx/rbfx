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
/// Definition of the Diligent::AsyncPipelineState class

#include <memory>

#include "PipelineState.h"
#include "RenderStateCache.h"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "ProxyPipelineState.hpp"
#include "UniqueIdentifier.hpp"

namespace Diligent
{

class RenderStateCacheImpl;

/// Async pipeline state waits until all shaders are loaded before initializing the internal pipeline state.
class AsyncPipelineState final : public ProxyPipelineState<ObjectBase<IPipelineState>>
{
public:
    using TBase = ProxyPipelineState<ObjectBase<IPipelineState>>;

    // {B6EFB3C0-0716-4997-86F1-E3DE8F7E0179}
    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xb6efb3c0, 0x716, 0x4997, {0x86, 0xf1, 0xe3, 0xde, 0x8f, 0x7e, 0x1, 0x79}};

    AsyncPipelineState(IReferenceCounters*            pRefCounters,
                       RenderStateCacheImpl*          pStateCache,
                       const PipelineStateCreateInfo& CreateInfo);
    ~AsyncPipelineState();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    virtual Int32 DILIGENT_CALL_TYPE GetUniqueID() const override final
    {
        return m_UniqueID.GetID() + 0x10000000;
    }

    virtual PIPELINE_STATE_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override;

    static void Create(RenderStateCacheImpl*          pStateCache,
                       const PipelineStateCreateInfo& CreateInfo,
                       IPipelineState**               ppReloadablePipeline);

private:
    void InitInternalPipeline();

private:
    struct CreateInfoWrapperBase;

    template <typename CreateInfoType>
    struct CreateInfoWrapper;

    RefCntAutoPtr<RenderStateCacheImpl>    m_pStateCache;
    std::unique_ptr<CreateInfoWrapperBase> m_pCreateInfo;
    const PIPELINE_TYPE                    m_Type;
    UniqueIdHelper<AsyncPipelineState>     m_UniqueID;
};

} // namespace Diligent
