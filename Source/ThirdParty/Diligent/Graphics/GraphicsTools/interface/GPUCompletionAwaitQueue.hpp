/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <vector>
#include <queue>

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Fence.h"
#include "../../../Common/interface/RefCntAutoPtr.hpp"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// Helper class that facilitates asynchronous waiting for the GPU completion.
template <typename ObjectType>
class GPUCompletionAwaitQueue
{
public:
    explicit GPUCompletionAwaitQueue(IRenderDevice* pDevice)
    {
        FenceDesc Desc;
        Desc.Name = "GPUCompletionAwaitQueue fence";
        Desc.Type = FENCE_TYPE_CPU_WAIT_ONLY;
        pDevice->CreateFence(Desc, &m_pFence);

        DEV_CHECK_ERR(m_pFence, "Failed to create fence");
    }

    // clang-format off
    GPUCompletionAwaitQueue           (const GPUCompletionAwaitQueue&) = delete;
    GPUCompletionAwaitQueue& operator=(const GPUCompletionAwaitQueue&) = delete;
    GPUCompletionAwaitQueue           (GPUCompletionAwaitQueue&&)      = default;
    GPUCompletionAwaitQueue& operator=(GPUCompletionAwaitQueue&&)      = delete;
    // clang-format on

    ObjectType GetRecycled()
    {
        ObjectType Obj{};
        if (!m_RecycledObjects.empty())
        {
            Obj = std::move(m_RecycledObjects.back());
            m_RecycledObjects.pop_back();
        }

        return Obj;
    }

    void Recycle(ObjectType&& Obj)
    {
        m_RecycledObjects.emplace_back(std::move(Obj));
    }

    ObjectType GetFirstCompleted()
    {
        const auto CompletedFenceValue = m_pFence->GetCompletedValue();

        ObjectType Obj{};
        if (!m_PendingObjects.empty() && m_PendingObjects.front().FenceValue <= CompletedFenceValue)
        {
            Obj = std::move(m_PendingObjects.front().Object);
            m_PendingObjects.pop();
        }

        return Obj;
    }

    void Enqueue(IDeviceContext* pCtx, ObjectType&& Obj)
    {
        m_PendingObjects.emplace(std::move(Obj), m_NextFenceValue);
        pCtx->EnqueueSignal(m_pFence, m_NextFenceValue++);
    }

private:
    struct PendingObject
    {
        PendingObject(ObjectType&& _Object, Uint64 _FenceValue) :
            Object{std::move(_Object)},
            FenceValue{_FenceValue}
        {}

        ObjectType   Object;
        const Uint64 FenceValue;
    };

    RefCntAutoPtr<IFence> m_pFence;
    Uint64                m_NextFenceValue = 1;

    std::queue<PendingObject> m_PendingObjects;
    std::vector<ObjectType>   m_RecycledObjects;
};

} // namespace Diligent
