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

#include "pch.h"

#include "FenceGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

FenceGLImpl::FenceGLImpl(IReferenceCounters* pRefCounters,
                         RenderDeviceGLImpl* pDevice,
                         const FenceDesc&    Desc) :
    // clang-format off
    TFenceBase
    {
        pRefCounters,
        pDevice,
        Desc
    }
// clang-format on
{
    if (m_Desc.Type != FENCE_TYPE_CPU_WAIT_ONLY)
        LOG_ERROR_AND_THROW("Description of Fence '", m_Desc.Name, "' is invalid: ", GetFenceTypeString(m_Desc.Type), " is not supported in OpenGL.");
}

FenceGLImpl::~FenceGLImpl()
{
#ifdef DILIGENT_DEVELOPMENT
    if (m_MaxPendingFences > 10)
        LOG_WARNING_MESSAGE("Max queue size of pending fences is too big. This may indicate that none of GetCompletedValue(), HostWait() or DeviceWait() have been used.");
#endif
}

Uint64 FenceGLImpl::GetCompletedValue()
{
    while (!m_PendingFences.empty())
    {
        auto& val_fence = m_PendingFences.front();

        auto res =
            glClientWaitSync(val_fence.second,
                             0, // Can be SYNC_FLUSH_COMMANDS_BIT
                             0  // Timeout in nanoseconds
            );
        if (res == GL_ALREADY_SIGNALED)
        {
            UpdateLastCompletedFenceValue(val_fence.first);
            m_PendingFences.pop_front();
        }
        else
        {
            break;
        }
    }

    return m_LastCompletedFenceValue.load();
}

void FenceGLImpl::HostWait(Uint64 Value, bool FlushCommands)
{
    while (!m_PendingFences.empty())
    {
        auto& val_fence = m_PendingFences.front();
        if (val_fence.first > Value)
            break;

        auto res = glClientWaitSync(val_fence.second, FlushCommands ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, std::numeric_limits<GLuint64>::max());
        VERIFY_EXPR(res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED);
        (void)res;

        UpdateLastCompletedFenceValue(val_fence.first);
        m_PendingFences.pop_front();
    }
}

void FenceGLImpl::DeviceWait(Uint64 Value)
{
    auto Iter = m_PendingFences.begin();
    for (; Iter != m_PendingFences.end(); ++Iter)
    {
        if (Iter->first >= Value)
        {
            glWaitSync(Iter->second, 0, GL_TIMEOUT_IGNORED);
            return;
        }
    }
}

void FenceGLImpl::Signal(Uint64 NewValue)
{
    DEV_CHECK_ERR(m_Desc.Type == FENCE_TYPE_GENERAL, "Fence must have been created with FENCE_TYPE_GENERAL");
    DEV_ERROR("Signal() is not supported in OpenGL backend");
}

void FenceGLImpl::Wait(Uint64 Value)
{
    return HostWait(Value, false);
}

} // namespace Diligent
