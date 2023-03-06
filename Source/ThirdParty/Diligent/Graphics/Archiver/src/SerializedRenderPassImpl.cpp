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

#include "SerializedRenderPassImpl.hpp"
#include "DeviceObjectArchive.hpp"
#include "PSOSerializer.hpp"
#include "SerializationDeviceImpl.hpp"

namespace Diligent
{

SerializedRenderPassImpl::SerializedRenderPassImpl(IReferenceCounters*      pRefCounters,
                                                   SerializationDeviceImpl* pDevice,
                                                   const RenderPassDesc&    Desc) :
    TBase{pRefCounters, pDevice, Desc, true}
{
    if (Desc.Name == nullptr || Desc.Name[0] == '\0')
        LOG_ERROR_AND_THROW("Serialized render pass name can't be null or empty");

    Serializer<SerializerMode::Measure> MeasureSer;
    RPSerializer<SerializerMode::Measure>::SerializeDesc(MeasureSer, m_Desc, nullptr);

    m_CommonData = MeasureSer.AllocateData(GetRawAllocator());

    Serializer<SerializerMode::Write> Ser{m_CommonData};
    RPSerializer<SerializerMode::Write>::SerializeDesc(Ser, m_Desc, nullptr);
    VERIFY_EXPR(Ser.IsEnded());
}

SerializedRenderPassImpl::~SerializedRenderPassImpl()
{}

bool SerializedRenderPassImpl::operator==(const SerializedRenderPassImpl& Rhs) const noexcept
{
    return GetCommonData() == Rhs.GetCommonData();
}

} // namespace Diligent
