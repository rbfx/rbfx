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

#include "EngineFactoryBase.hpp"

#include <array>

#include "PrivateConstants.h"
#include "Errors.hpp"
#include "DebugUtilities.hpp"
#include "Buffer.h"
#include "Texture.h"

namespace Diligent
{

void VerifyEngineCreateInfo(const EngineCreateInfo& EngineCI, const GraphicsAdapterInfo& AdapterInfo) noexcept(false)
{
    if (EngineCI.EngineAPIVersion != DILIGENT_API_VERSION)
    {
        LOG_ERROR_AND_THROW("Diligent Engine runtime (", DILIGENT_API_VERSION, ") is not compatible with the client API version (", EngineCI.EngineAPIVersion, ")");
    }

    if ((EngineCI.NumImmediateContexts > 0) != (EngineCI.pImmediateContextInfo != nullptr))
    {
        LOG_ERROR_AND_THROW("If NumImmediateContexts is not zero, pContextInfo must not be null");
    }

    constexpr auto MaxImmediateContexts = 8 * std::min(sizeof(decltype(BufferDesc::ImmediateContextMask)), sizeof(decltype(TextureDesc::ImmediateContextMask)));
    static_assert(MAX_COMMAND_QUEUES == MaxImmediateContexts, "Number of bits in MaxImmediateContexts must be equal to MAX_COMMAND_QUEUES");

    if (EngineCI.NumImmediateContexts >= MaxImmediateContexts)
    {
        LOG_ERROR_AND_THROW("NumImmediateContexts (", EngineCI.NumImmediateContexts, ") must be less than ", MaxImmediateContexts, "");
    }

    std::array<Uint32, DILIGENT_MAX_ADAPTER_QUEUES> QueueCount = {};
    for (Uint32 CtxInd = 0; CtxInd < EngineCI.NumImmediateContexts; ++CtxInd)
    {
        const auto& ContextInfo = EngineCI.pImmediateContextInfo[CtxInd];

        if (ContextInfo.QueueId >= AdapterInfo.NumQueues)
        {
            LOG_ERROR_AND_THROW("pContextInfo[", CtxInd, "].QueueId (", ContextInfo.QueueId, ") must be less than AdapterInfo.NumQueues (",
                                AdapterInfo.NumQueues, ").");
        }
        QueueCount[ContextInfo.QueueId] += 1;
        if (QueueCount[ContextInfo.QueueId] > AdapterInfo.Queues[ContextInfo.QueueId].MaxDeviceContexts)
        {
            LOG_ERROR_AND_THROW("pContextInfo[", CtxInd, "]: the number of contexts with QueueId ", ContextInfo.QueueId,
                                " exceeds the maximum available number ", AdapterInfo.Queues[ContextInfo.QueueId].MaxDeviceContexts, ".");
        }

        switch (ContextInfo.Priority)
        {
            case QUEUE_PRIORITY_LOW:
            case QUEUE_PRIORITY_MEDIUM:
            case QUEUE_PRIORITY_HIGH:
            case QUEUE_PRIORITY_REALTIME:
                break;
            default:
                LOG_ERROR_AND_THROW("Unknown queue priority");
        }
    }
}

} // namespace Diligent
