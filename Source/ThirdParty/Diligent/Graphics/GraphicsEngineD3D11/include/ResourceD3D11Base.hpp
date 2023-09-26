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
/// Declaration of Diligent::ResourceD3D11Base class

#include "DeviceMemoryD3D11.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Base implementation of a D3D11 resource
class ResourceD3D11Base
{
public:
    RefCntAutoPtr<IDeviceMemoryD3D11> GetSparseResourceMemory()
    {
        return m_pSparseResourceMemory.Lock();
    }

    void SetSparseResourceMemory(IDeviceMemoryD3D11* pMemory)
    {
        m_pSparseResourceMemory = pMemory;
    }

protected:
    // There appears to be a bug on NVidia GPUs: when calling UpdateTileMappings() with
    // null tile pool, all mappings get invalidated including those that are not
    // specified in the call. To workaround the bug, we have to keep the pointer to
    // the last used memory pool.
    RefCntWeakPtr<IDeviceMemoryD3D11> m_pSparseResourceMemory;
};

} // namespace Diligent
