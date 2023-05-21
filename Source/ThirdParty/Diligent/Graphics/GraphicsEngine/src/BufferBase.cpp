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

#include "BufferBase.hpp"
#include "DeviceContext.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

#define LOG_BUFFER_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of buffer '", (Desc.Name != nullptr ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)
#define VERIFY_BUFFER(Expr, ...)                     \
    do                                               \
    {                                                \
        if (!(Expr))                                 \
        {                                            \
            LOG_BUFFER_ERROR_AND_THROW(__VA_ARGS__); \
        }                                            \
    } while (false)


void ValidateBufferDesc(const BufferDesc& Desc, const IRenderDevice* pDevice) noexcept(false)
{
    const auto& MemoryInfo = pDevice->GetAdapterInfo().Memory;
    const auto& Features   = pDevice->GetDeviceInfo().Features;

    static_assert(BIND_FLAG_LAST == 0x800L, "Please update this function to handle the new bind flags");

    constexpr Uint32 AllowedBindFlags =
        BIND_VERTEX_BUFFER |
        BIND_INDEX_BUFFER |
        BIND_UNIFORM_BUFFER |
        BIND_SHADER_RESOURCE |
        BIND_STREAM_OUTPUT |
        BIND_UNORDERED_ACCESS |
        BIND_INDIRECT_DRAW_ARGS |
        BIND_RAY_TRACING;

    VERIFY_BUFFER((Desc.BindFlags & ~AllowedBindFlags) == 0, "the following bind flags are not allowed for a buffer: ", GetBindFlagsString(Desc.BindFlags & ~AllowedBindFlags, ", "), '.');

    if ((Desc.BindFlags & BIND_UNORDERED_ACCESS) ||
        (Desc.BindFlags & BIND_SHADER_RESOURCE))
    {
        VERIFY_BUFFER(Desc.Mode > BUFFER_MODE_UNDEFINED && Desc.Mode < BUFFER_MODE_NUM_MODES, GetBufferModeString(Desc.Mode), " is not a valid mode for a buffer created with BIND_SHADER_RESOURCE or BIND_UNORDERED_ACCESS flags.");
        if (Desc.Mode == BUFFER_MODE_STRUCTURED || Desc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY_BUFFER(Desc.ElementByteStride != 0, "element stride must not be zero for structured and formatted buffers.");
        }
        else if (Desc.Mode == BUFFER_MODE_RAW)
        {
            // Nothing needs to be done
        }
    }

    if ((Desc.BindFlags & BIND_RAY_TRACING) != 0)
        VERIFY_BUFFER(Features.RayTracing, "BIND_RAY_TRACING flag can't be used when RayTracing feature is not enabled.");

    if ((Desc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0)
    {
        VERIFY_BUFFER((pDevice->GetAdapterInfo().DrawCommand.CapFlags & DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT) != 0,
                      "BIND_INDIRECT_DRAW_ARGS flag can't be used when DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT capability is not supported");
    }

    switch (Desc.Usage)
    {
        case USAGE_IMMUTABLE:
        case USAGE_DEFAULT:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_NONE, "static and default buffers can't have any CPU access flags set.");
            break;

        case USAGE_DYNAMIC:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_WRITE, "dynamic buffers require CPU_ACCESS_WRITE flag.");
            break;

        case USAGE_STAGING:
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_WRITE || Desc.CPUAccessFlags == CPU_ACCESS_READ,
                          "exactly one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags must be specified for a staging buffer.");
            VERIFY_BUFFER(Desc.BindFlags == BIND_NONE,
                          "staging buffers cannot be bound to any part of the graphics pipeline and can't have any bind flags set.");
            break;

        case USAGE_UNIFIED:
            VERIFY_BUFFER(MemoryInfo.UnifiedMemory != 0,
                          "Unified memory is not present on this device. Check the amount of available unified memory "
                          "in the device caps before creating unified buffers.");
            VERIFY_BUFFER(Desc.CPUAccessFlags != CPU_ACCESS_NONE,
                          "at least one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags must be specified for a unified buffer.");
            if (Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
            {
                VERIFY_BUFFER(MemoryInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_WRITE,
                              "Unified memory on this device does not support write access. Check the available access flags "
                              "in the device caps before creating unified buffers.");
            }
            if (Desc.CPUAccessFlags & CPU_ACCESS_READ)
            {
                VERIFY_BUFFER(MemoryInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_READ,
                              "Unified memory on this device does not support read access. Check the available access flags "
                              "in the device caps before creating unified buffers.");
            }
            break;

        case USAGE_SPARSE:
        {
            const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;
            VERIFY_BUFFER(Features.SparseResources, "sparse buffer requires SparseResources feature");
            VERIFY_BUFFER(Desc.CPUAccessFlags == CPU_ACCESS_NONE, "sparse buffers can't have any CPU access flags set.");
            VERIFY_BUFFER(Desc.Size <= SparseRes.ResourceSpaceSize, "sparse buffer size (", Desc.Size, ") must not exceed the ResourceSpaceSize (", SparseRes.ResourceSpaceSize, ")");
            VERIFY_BUFFER((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) != 0, "sparse buffer requires SPARSE_RESOURCE_CAP_FLAG_BUFFER capability");
            if ((Desc.MiscFlags & MISC_BUFFER_FLAG_SPARSE_ALIASING) != 0)
                VERIFY_BUFFER(SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_ALIASED, "MISC_BUFFER_FLAG_SPARSE_ALIASING flag requires SPARSE_RESOURCE_CAP_FLAG_ALIASED capability");
            VERIFY_BUFFER((Desc.BindFlags & ~SparseRes.BufferBindFlags) == 0,
                          "the following bind flags are not allowed for sparse buffers: ", GetBindFlagsString(Desc.BindFlags & ~SparseRes.BufferBindFlags, ", "), '.');
            break;
        }

        default:
            UNEXPECTED("Unknown usage");
    }


    if (Desc.Usage == USAGE_DYNAMIC && PlatformMisc::CountOneBits(Desc.ImmediateContextMask) > 1)
    {
        bool NeedsBackingResource = (Desc.BindFlags & BIND_UNORDERED_ACCESS) != 0 || Desc.Mode == BUFFER_MODE_FORMATTED;
        if (NeedsBackingResource)
        {
            LOG_BUFFER_ERROR_AND_THROW("USAGE_DYNAMIC buffers that use UAV flag or FORMATTED mode require internal backing resource. "
                                       "This resource is implicitly transitioned by the device context and thus can't be safely used in "
                                       "multiple contexts. Create DYNAMIC buffer without UAV flag and use UNDEFINED mode and copy the contents to USAGE_DEFAULT buffer "
                                       "with required flags, which can be shared between contexts.");
        }
    }

    if (Desc.Usage != USAGE_SPARSE)
    {
        VERIFY_BUFFER(MemoryInfo.MaxMemoryAllocation == 0 || Desc.Size <= MemoryInfo.MaxMemoryAllocation,
                      "non-sparse buffer size (", Desc.Size, ") must not exceed the maximum allocation size (", MemoryInfo.MaxMemoryAllocation, ")");
        VERIFY_BUFFER((Desc.MiscFlags & MISC_BUFFER_FLAG_SPARSE_ALIASING) == 0,
                      "MiscFlags must not have MISC_BUFFER_FLAG_SPARSE_ALIASING if usage is not USAGE_SPARSE");
    }
}

void ValidateBufferInitData(const BufferDesc& Desc, const BufferData* pBuffData) noexcept(false)
{
    const bool HasInitialData = (pBuffData != nullptr && pBuffData->pData != nullptr);

    if (Desc.Usage == USAGE_IMMUTABLE && !HasInitialData)
        LOG_BUFFER_ERROR_AND_THROW("initial data must not be null as immutable buffers must be initialized at creation time.");

    if (Desc.Usage == USAGE_DYNAMIC && HasInitialData)
        LOG_BUFFER_ERROR_AND_THROW("initial data must be null for dynamic buffers.");

    if (Desc.Usage == USAGE_SPARSE && HasInitialData)
        LOG_BUFFER_ERROR_AND_THROW("initial data must be null for sparse buffers.");

    if (Desc.Usage == USAGE_STAGING)
    {
        if (Desc.CPUAccessFlags == CPU_ACCESS_WRITE)
        {
            VERIFY_BUFFER(!HasInitialData, "CPU-writable staging buffers must be updated via map.");
        }
    }
    else if (Desc.Usage == USAGE_UNIFIED)
    {
        if (HasInitialData && (Desc.CPUAccessFlags & CPU_ACCESS_WRITE) == 0)
        {
            LOG_BUFFER_ERROR_AND_THROW("CPU_ACCESS_WRITE flag is required to initialize a unified buffer.");
        }
    }

    if (pBuffData != nullptr && pBuffData->pContext != nullptr)
    {
        const auto& CtxDesc = pBuffData->pContext->GetDesc();
        if (CtxDesc.IsDeferred)
            LOG_BUFFER_ERROR_AND_THROW("Deferred contexts can't be used to initialize resources");
        if ((Desc.ImmediateContextMask & (Uint64{1} << CtxDesc.ContextId)) == 0)
        {
            LOG_BUFFER_ERROR_AND_THROW("Can not initialize the buffer in device context '", CtxDesc.Name,
                                       "' as ImmediateContextMask (", std::hex, Desc.ImmediateContextMask, ") does not contain ",
                                       std::hex, (Uint64{1} << CtxDesc.ContextId), " bit.");
        }
    }

    if (HasInitialData)
    {
        VERIFY_BUFFER(pBuffData->DataSize >= Desc.Size,
                      "Buffer initial DataSize (", pBuffData->DataSize, ") must be larger than the buffer size (", Desc.Size, ")");
    }
}

#undef VERIFY_BUFFER
#undef LOG_BUFFER_ERROR_AND_THROW

void ValidateAndCorrectBufferViewDesc(const BufferDesc& BuffDesc,
                                      BufferViewDesc&   ViewDesc,
                                      Uint32            StructuredBufferOffsetAlignment) noexcept(false)
{
    if (ViewDesc.ByteWidth == 0)
    {
        DEV_CHECK_ERR(BuffDesc.Size > ViewDesc.ByteOffset, "Byte offset (", ViewDesc.ByteOffset, ") exceeds buffer size (", BuffDesc.Size, ")");
        ViewDesc.ByteWidth = BuffDesc.Size - ViewDesc.ByteOffset;
    }

    if (ViewDesc.ByteOffset + ViewDesc.ByteWidth > BuffDesc.Size)
        LOG_ERROR_AND_THROW("Buffer view range [", ViewDesc.ByteOffset, ", ", ViewDesc.ByteOffset + ViewDesc.ByteWidth, ") is out of the buffer boundaries [0, ", BuffDesc.Size, ").");

    if ((BuffDesc.BindFlags & BIND_UNORDERED_ACCESS) ||
        (BuffDesc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED || BuffDesc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY(BuffDesc.ElementByteStride != 0, "Element byte stride is zero");
            if ((ViewDesc.ByteOffset % BuffDesc.ElementByteStride) != 0)
                LOG_ERROR_AND_THROW("Buffer view byte offset (", ViewDesc.ByteOffset, ") is not a multiple of element byte stride (", BuffDesc.ElementByteStride, ").");
            if ((ViewDesc.ByteWidth % BuffDesc.ElementByteStride) != 0)
                LOG_ERROR_AND_THROW("Buffer view byte width (", ViewDesc.ByteWidth, ") is not a multiple of element byte stride (", BuffDesc.ElementByteStride, ").");
        }

        if (BuffDesc.Mode == BUFFER_MODE_FORMATTED && ViewDesc.Format.ValueType == VT_UNDEFINED)
            LOG_ERROR_AND_THROW("Format must be specified when creating a view of a formatted buffer");

        if (BuffDesc.Mode == BUFFER_MODE_FORMATTED || (BuffDesc.Mode == BUFFER_MODE_RAW && ViewDesc.Format.ValueType != VT_UNDEFINED))
        {
            if (ViewDesc.Format.NumComponents <= 0 || ViewDesc.Format.NumComponents > 4)
                LOG_ERROR_AND_THROW("Incorrect number of components (", Uint32{ViewDesc.Format.NumComponents}, "). 1, 2, 3, or 4 are allowed values");
            if (ViewDesc.Format.ValueType == VT_FLOAT32 || ViewDesc.Format.ValueType == VT_FLOAT16)
                ViewDesc.Format.IsNormalized = false;
            auto ViewElementStride = GetValueSize(ViewDesc.Format.ValueType) * Uint32{ViewDesc.Format.NumComponents};
            if (BuffDesc.Mode == BUFFER_MODE_RAW && BuffDesc.ElementByteStride == 0)
                LOG_ERROR_AND_THROW("To enable formatted views of a raw buffer, element byte must be specified during buffer initialization");
            if (ViewElementStride != BuffDesc.ElementByteStride)
            {
                LOG_ERROR_AND_THROW("Buffer element byte stride (", BuffDesc.ElementByteStride,
                                    ") is not consistent with the size (", ViewElementStride,
                                    ") defined by the format of the view (", GetBufferFormatString(ViewDesc.Format), ')');
            }
        }

        if (BuffDesc.Mode == BUFFER_MODE_RAW && ViewDesc.Format.ValueType == VT_UNDEFINED)
        {
            if ((ViewDesc.ByteOffset % 16) != 0)
            {
                LOG_ERROR_AND_THROW("When creating a RAW view, the offset of the first element from the start of the buffer (",
                                    ViewDesc.ByteOffset, ") must be a multiple of 16 bytes");
            }
        }

        if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED)
        {
            VERIFY_EXPR(StructuredBufferOffsetAlignment != 0);
            if ((ViewDesc.ByteOffset % StructuredBufferOffsetAlignment) != 0)
            {
                LOG_ERROR_AND_THROW("Structured buffer view byte offset (", ViewDesc.ByteOffset,
                                    ") is not a multiple of the required structured buffer offset alignment (",
                                    StructuredBufferOffsetAlignment, ").");
            }
        }
    }
}

} // namespace Diligent
