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

#include "pch.h"
#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceObjectArchiveD3D12.hpp"
#include "PipelineResourceSignatureD3D12Impl.hpp"

namespace Diligent
{

template <SerializerMode Mode>
bool PRSSerializerD3D12<Mode>::SerializeInternalData(
    Serializer<Mode>&                                      Ser,
    ConstQual<PipelineResourceSignatureInternalDataD3D12>& InternalData,
    DynamicLinearAllocator*                                Allocator)
{
    if (!PRSSerializer<Mode>::SerializeInternalData(Ser, InternalData, Allocator))
        return false;

    if (!Ser.SerializeArrayRaw(Allocator, InternalData.pResourceAttribs, InternalData.NumResources))
        return false;
    if (!Ser.SerializeArrayRaw(Allocator, InternalData.pImmutableSamplers, InternalData.NumImmutableSamplers))
        return false;

    ASSERT_SIZEOF64(InternalData, 48, "Did you add a new member to PipelineResourceSignatureInternalDataD3D12? Please add serialization here.");

    return true;
}

template struct PRSSerializerD3D12<SerializerMode::Read>;
template struct PRSSerializerD3D12<SerializerMode::Write>;
template struct PRSSerializerD3D12<SerializerMode::Measure>;

} // namespace Diligent
