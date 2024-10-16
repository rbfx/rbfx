/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "RenderPass.h"
#include "RenderPassBase.hpp"
#include "SerializationEngineImplTraits.hpp"
#include "Serializer.hpp"

namespace Diligent
{

// {6A00F074-BA7B-47B7-B9C1-16A705C76C47}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SerializedRenderPass =
    {0x6a00f074, 0xba7b, 0x47b7, {0xb9, 0xc1, 0x16, 0xa7, 0x5, 0xc7, 0x6c, 0x47}};

class SerializedRenderPassImpl final : public RenderPassBase<SerializationEngineImplTraits>
{
public:
    using TBase = RenderPassBase<SerializationEngineImplTraits>;

    SerializedRenderPassImpl(IReferenceCounters*      pRefCounters,
                             SerializationDeviceImpl* pDevice,
                             const RenderPassDesc&    Desc);

    ~SerializedRenderPassImpl() override;

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_SerializedRenderPass, IID_RenderPass, TBase)

    const SerializedData& GetCommonData() const { return m_CommonData; }

    bool operator==(const SerializedRenderPassImpl& Rhs) const noexcept;
    bool operator!=(const SerializedRenderPassImpl& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }

private:
    SerializedData m_CommonData;
};

} // namespace Diligent
