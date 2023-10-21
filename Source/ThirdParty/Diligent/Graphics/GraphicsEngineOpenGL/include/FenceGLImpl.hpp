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
/// Declaration of Diligent::FenceGLImpl class

#include <deque>
#include <atomic>

#include "EngineGLImplTraits.hpp"
#include "FenceBase.hpp"
#include "GLObjectWrapper.hpp"

namespace Diligent
{

/// Fence object implementation in OpenGL backend.
class FenceGLImpl final : public FenceBase<EngineGLImplTraits>
{
public:
    using TFenceBase = FenceBase<EngineGLImplTraits>;

    FenceGLImpl(IReferenceCounters* pRefCounters,
                RenderDeviceGLImpl* pDevice,
                const FenceDesc&    Desc);
    ~FenceGLImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_FenceGL, TFenceBase)

    /// Implementation of IFence::GetCompletedValue() in OpenGL backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedValue() override final;

    /// Implementation of IFence::Signal() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Signal(Uint64 Value) override final;

    /// Implementation of IFence::Wait() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Wait(Uint64 Value) override final;

    void AddPendingFence(GLObjectWrappers::GLSyncObj&& Fence, Uint64 Value)
    {
        m_PendingFences.emplace_back(Value, std::move(Fence));
        DvpSignal(Value);

#ifdef DILIGENT_DEVELOPMENT
        m_MaxPendingFences = std::max(m_MaxPendingFences, m_PendingFences.size());
#endif
    }

    void HostWait(Uint64 Value, bool FlushCommands);
    void DeviceWait(Uint64 Value);

private:
    std::deque<std::pair<Uint64, GLObjectWrappers::GLSyncObj>> m_PendingFences;

#ifdef DILIGENT_DEVELOPMENT
    size_t m_MaxPendingFences = 0;
#endif
};

} // namespace Diligent
