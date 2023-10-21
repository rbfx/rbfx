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
/// Implementation of the Diligent::CommandListBase template class

#include "CommandList.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"

namespace Diligent
{

struct CommandListDesc : public DeviceObjectAttribs
{
};

/// Template class implementing base functionality of the command list object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class CommandListBase : public DeviceObjectBase<typename EngineImplTraits::CommandListInterface, typename EngineImplTraits::RenderDeviceImplType, CommandListDesc>
{
public:
    // Base interface that this class inherits (ICommandList).
    using BaseInterface = typename EngineImplTraits::CommandListInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using DeviceContextImplType = typename EngineImplTraits::DeviceContextImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, CommandListDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this command list.
    /// \param pDevice           - Pointer to the device.
    /// \param pDeferredCtx      - Deferred context that recorded this command list.
    /// \param bIsDeviceInternal - Flag indicating if the CommandList is an internal device object and
    ///							   must not keep a strong reference to the device.
    CommandListBase(IReferenceCounters*    pRefCounters,
                    RenderDeviceImplType*  pDevice,
                    DeviceContextImplType* pDeferredCtx,
                    bool                   bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, CommandListDesc{}, bIsDeviceInternal},
        m_QueueId{pDeferredCtx->GetDesc().QueueId}
    {
        VERIFY_EXPR(pDeferredCtx->GetDesc().IsDeferred);
    }

    ~CommandListBase()
    {
    }

    Uint32 GetQueueId() const
    {
        return m_QueueId;
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_CommandList, TDeviceObjectBase)

private:
    const Uint8 m_QueueId;
};

} // namespace Diligent
