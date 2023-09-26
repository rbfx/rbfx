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

#include "DeviceMemoryBase.hpp"

namespace Diligent
{

#define LOG_DEVMEMORY_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of device memory '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)
#define VERIFY_DEVMEMORY(Expr, ...)                     \
    do                                                  \
    {                                                   \
        if (!(Expr))                                    \
        {                                               \
            LOG_DEVMEMORY_ERROR_AND_THROW(__VA_ARGS__); \
        }                                               \
    } while (false)


void ValidateDeviceMemoryDesc(const DeviceMemoryDesc& Desc, const IRenderDevice* pDevice) noexcept(false)
{
    if (Desc.Type == DEVICE_MEMORY_TYPE_SPARSE)
    {
        const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;

        VERIFY_DEVMEMORY(Desc.PageSize != 0, "page size must not be zero");

        // In a very rare case when the resource has a custom memory alignment that is not a multiple of StandardBlockSize,
        // this might be a false positive.
        VERIFY_DEVMEMORY((Desc.PageSize % SparseRes.StandardBlockSize) == 0,
                         "page size (", Desc.PageSize, ") is not a multiple of sparse block size (", SparseRes.StandardBlockSize, ")");
    }
    else
    {
        LOG_DEVMEMORY_ERROR_AND_THROW("Unexpected device memory type");
    }
}

} // namespace Diligent
