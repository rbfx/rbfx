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

// clang-format off

/// \file
/// Definition of the Diligent::IPipelineStateCache interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/DataBlob.h"
#include "GraphicsTypes.h"
#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Pipeline state cache mode.
DILIGENT_TYPED_ENUM(PSO_CACHE_MODE, Uint8)
{
    /// PSO cache will be used to load PSOs from it.
    PSO_CACHE_MODE_LOAD  = 1u << 0u,

    /// PSO cache will be used to store PSOs.
    PSO_CACHE_MODE_STORE = 1u << 1u,

    /// PSO cache will be used to load and store PSOs.
    PSO_CACHE_MODE_LOAD_STORE = PSO_CACHE_MODE_LOAD | PSO_CACHE_MODE_STORE
};
DEFINE_FLAG_ENUM_OPERATORS(PSO_CACHE_MODE);

/// Pipeline state cache flags.
DILIGENT_TYPED_ENUM(PSO_CACHE_FLAGS, Uint8)
{
    /// No flags.
    PSO_CACHE_FLAG_NONE = 0u,

    /// Print diagnostic messages e.g. when PSO is not found in the cache.
    PSO_CACHE_FLAG_VERBOSE = 1u << 0u
};
DEFINE_FLAG_ENUM_OPERATORS(PSO_CACHE_FLAGS);

/// Pipeline state cache description
struct PipelineStateCacheDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Cache mode, see Diligent::PSO_CACHE_MODE.

    /// \note Metal backend allows generating the cache on one device
    ///       and loading PSOs from it on another.
    ///       Vulkan PSO cache depends on the GPU device, driver version and other parameters,
    ///       so the cache must be generated and used on the same device.
    PSO_CACHE_MODE Mode DEFAULT_INITIALIZER(PSO_CACHE_MODE_LOAD_STORE);

    /// PSO cache flags, see Diligent::PSO_CACHE_FLAGS.
    PSO_CACHE_FLAGS Flags DEFAULT_INITIALIZER(PSO_CACHE_FLAG_NONE);

    // ImmediateContextMask ?
};
typedef struct PipelineStateCacheDesc PipelineStateCacheDesc;

/// Pipeline state pbject cache create info
struct PipelineStateCacheCreateInfo
{
    PipelineStateCacheDesc Desc;

    /// All fields can be null to create an empty cache
    const void* pCacheData    DEFAULT_INITIALIZER(nullptr);

    /// The size of data pointed to by pCacheData
    Uint32      CacheDataSize DEFAULT_INITIALIZER(0);
};
typedef struct PipelineStateCacheCreateInfo PipelineStateCacheCreateInfo;

// clang-format on

// {6AC86F22-FFF4-493C-8C1F-C539D934F4BC}
static const INTERFACE_ID IID_PipelineStateCache =
    {0x6ac86f22, 0xfff4, 0x493c, {0x8c, 0x1f, 0xc5, 0x39, 0xd9, 0x34, 0xf4, 0xbc}};


#define DILIGENT_INTERFACE_NAME IPipelineStateCache
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateCacheInclusiveMethods \
    IDeviceObjectInclusiveMethods;          \
    IPipelineStateCacheMethods PipelineStateCache

// clang-format off

/// Pipeline state cache interface
DILIGENT_BEGIN_INTERFACE(IPipelineStateCache, IDeviceObject)
{
    /// Creates a blob with pipeline state cache data
    VIRTUAL void METHOD(GetData)(THIS_
                                 IDataBlob** ppBlob) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IPipelineStateCache_GetData(This, ...)  CALL_IFACE_METHOD(PipelineStateCache, GetData, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE
