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

/// \file
/// Declaration of Diligent::DeviceMemoryD3D11Impl class

#include <atlbase.h>

#include "EngineD3D11ImplTraits.hpp"
#include "DeviceMemoryBase.hpp"

namespace Diligent
{

/// Device memory object implementation in Direct3D11 backend.
class DeviceMemoryD3D11Impl final : public DeviceMemoryBase<EngineD3D11ImplTraits>
{
public:
    using TDeviceMemoryBase = DeviceMemoryBase<EngineD3D11ImplTraits>;

    DeviceMemoryD3D11Impl(IReferenceCounters*           pRefCounters,
                          RenderDeviceD3D11Impl*        pDeviceD3D11,
                          const DeviceMemoryCreateInfo& MemCI);

    ~DeviceMemoryD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceMemory::Resize().
    virtual Bool DILIGENT_CALL_TYPE Resize(Uint64 NewSize) override final;

    /// Implementation of IDeviceMemory::GetCapacity().
    virtual Uint64 DILIGENT_CALL_TYPE GetCapacity() const override final;

    /// Implementation of IDeviceMemory::IsCompatible().
    virtual Bool DILIGENT_CALL_TYPE IsCompatible(IDeviceObject* pResource) const override final;

    /// Implementation of IDeviceMemoryD3D11::GetD3D11TilePool().
    virtual ID3D11Buffer* DILIGENT_CALL_TYPE GetD3D11TilePool() const override final
    {
        return m_pd3d11Buffer;
    }

private:
    friend class DeviceContextD3D11Impl;
    CComPtr<ID3D11Buffer> m_pd3d11Buffer; ///< D3D11 buffer object (tile pool)
};

} // namespace Diligent
