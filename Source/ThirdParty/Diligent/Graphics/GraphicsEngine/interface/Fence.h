/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Defines Diligent::IFence interface and related data structures

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3B19184D-32AB-4701-84F4-9A0C03AE1672}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_Fence =
    {0x3b19184d, 0x32ab, 0x4701, {0x84, 0xf4, 0x9a, 0xc, 0x3, 0xae, 0x16, 0x72}};

// clang-format off

/// Describes the fence type.

/// This enumeration is used by FenceDesc structure.
DILIGENT_TYPED_ENUM(FENCE_TYPE, Uint8)
{
    /// Basic fence that may be used for:
    ///  - signaling the fence from GPU
    ///  - waiting for the fence on CPU
    FENCE_TYPE_CPU_WAIT_ONLY = 0,

    /// General fence that may be used for:
    ///  - signaling the fence from GPU
    ///  - waiting for the fence on CPU
    ///  - waiting for the fence on GPU
    ///
    /// If NativeFence feature is enabled (see Diligent::DeviceFeatures), the fence may also be used for:
    ///  - signaling the fence on CPU
    ///  - waiting on GPU for a value that will be enqueued for signal later
    FENCE_TYPE_GENERAL = 1,

    FENCE_TYPE_LAST = FENCE_TYPE_GENERAL
};

/// Fence description
struct FenceDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Fence type, see Diligent::FENCE_TYPE.
    FENCE_TYPE Type DEFAULT_INITIALIZER(FENCE_TYPE_CPU_WAIT_ONLY);
};
typedef struct FenceDesc FenceDesc;

// clang-format off

#define DILIGENT_INTERFACE_NAME IFence
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IFenceInclusiveMethods      \
    IDeviceObjectInclusiveMethods;  \
    IFenceMethods Fence

/// Fence interface

/// Defines the methods to manipulate a fence object
///
/// \remarks When a fence that was previously signaled by IDeviceContext::EnqueueSignal() is destroyed,
///          it may block the GPU until all prior commands have completed execution.
///
/// \remarks In Direct3D12 and Vulkan backends, fence is thread-safe.
DILIGENT_BEGIN_INTERFACE(IFence, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the fence description used to create the object
    virtual const FenceDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns the last completed value signaled by the GPU

    /// \remarks   In Direct3D11 backend, this method is not thread-safe (even if the fence
    ///            object is protected by a mutex) and must only be called by the same thread
    ///            that signals the fence via IDeviceContext::EnqueueSignal().
    VIRTUAL Uint64 METHOD(GetCompletedValue)(THIS) PURE;


    /// Sets the fence to the specified value.

    /// \param [in] Value - New value to set the fence to.
    ///                     The value must be greater than the current value of the fence.
    ///
    /// \note  Fence value will be changed immediately on the CPU.
    ///        Use IDeviceContext::EnqueueSignal to enqueue a signal command
    ///        that will change the value on the GPU after all previously submitted commands
    ///        are complete.
    ///
    /// \note  The fence must have been created with type FENCE_TYPE_GENERAL.
    VIRTUAL void METHOD(Signal)(THIS_
                                Uint64 Value) PURE;


    /// Waits until the fence reaches or exceeds the specified value, on the host.

    /// \param [in] Value - The value that the fence is waiting for to reach.
    ///
    /// \note  The method blocks the execution of the calling thread until the wait is complete.
    VIRTUAL void METHOD(Wait)(THIS_
                              Uint64 Value) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IFence_GetDesc(This) (const struct FenceDesc*)IDeviceObject_GetDesc(This)

#    define IFence_GetCompletedValue(This) CALL_IFACE_METHOD(Fence, GetCompletedValue, This)
#    define IFence_Signal(This, ...)       CALL_IFACE_METHOD(Fence, Signal,            This, __VA_ARGS__)
#    define IFence_Wait(This, ...)         CALL_IFACE_METHOD(Fence, Wait,              This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
