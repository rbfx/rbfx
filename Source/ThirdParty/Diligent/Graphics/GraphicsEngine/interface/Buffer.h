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

#pragma once

// clang-format off

/// \file
/// Defines Diligent::IBuffer interface and related data structures

#include "DeviceObject.h"
#include "BufferView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {EC47EAD3-A2C4-44F2-81C5-5248D14F10E4}
static const INTERFACE_ID IID_Buffer =
    {0xec47ead3, 0xa2c4, 0x44f2, {0x81, 0xc5, 0x52, 0x48, 0xd1, 0x4f, 0x10, 0xe4}};

/// Describes the buffer access mode.

/// This enumeration is used by BufferDesc structure.
DILIGENT_TYPED_ENUM(BUFFER_MODE, Uint8)
{
    /// Undefined mode.
    BUFFER_MODE_UNDEFINED = 0,

    /// Formatted buffer. Access to the buffer will use format conversion operations.
    /// In this mode, ElementByteStride member of BufferDesc defines the buffer element size.
    /// Buffer views can use different formats, but the format size must match ElementByteStride.
    BUFFER_MODE_FORMATTED,

    /// Structured buffer.
    /// In this mode, ElementByteStride member of BufferDesc defines the structure stride.
    BUFFER_MODE_STRUCTURED,

    /// Raw buffer.
    /// In this mode, the buffer is accessed as raw bytes. Formatted views of a raw
    /// buffer can also be created similar to formatted buffer. If formatted views
    /// are to be created, the ElementByteStride member of BufferDesc must specify the
    /// size of the format.
    BUFFER_MODE_RAW,

    /// Helper value storing the total number of modes in the enumeration.
    BUFFER_MODE_NUM_MODES
};

/// Miscellaneous buffer flags.
DILIGENT_TYPED_ENUM(MISC_BUFFER_FLAGS, Uint8)
{
    MISC_BUFFER_FLAG_NONE          = 0,

    /// For a sparse buffer, allow binding the same memory region in different buffer ranges
    /// or in different sparse buffers.
    MISC_BUFFER_FLAG_SPARSE_ALIASING = 1u << 0,
};
DEFINE_FLAG_ENUM_OPERATORS(MISC_BUFFER_FLAGS)

/// Buffer description
struct BufferDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Size of the buffer, in bytes. For a uniform buffer, this must be a multiple of 16.
    Uint64 Size                    DEFAULT_INITIALIZER(0);

    /// Buffer bind flags, see Diligent::BIND_FLAGS for details

    /// The following bind flags are allowed:
    /// Diligent::BIND_VERTEX_BUFFER, Diligent::BIND_INDEX_BUFFER, Diligent::BIND_UNIFORM_BUFFER,
    /// Diligent::BIND_SHADER_RESOURCE, Diligent::BIND_STREAM_OUTPUT, Diligent::BIND_UNORDERED_ACCESS,
    /// Diligent::BIND_INDIRECT_DRAW_ARGS, Diligent::BIND_RAY_TRACING.
    /// Use SparseResourceProperties::BufferBindFlags to get allowed bind flags for a sparse buffer.
    BIND_FLAGS BindFlags            DEFAULT_INITIALIZER(BIND_NONE);

    /// Buffer usage, see Diligent::USAGE for details
    USAGE Usage                     DEFAULT_INITIALIZER(USAGE_DEFAULT);

    /// CPU access flags or 0 if no CPU access is allowed,
    /// see Diligent::CPU_ACCESS_FLAGS for details.
    CPU_ACCESS_FLAGS CPUAccessFlags DEFAULT_INITIALIZER(CPU_ACCESS_NONE);

    /// Buffer mode, see Diligent::BUFFER_MODE
    BUFFER_MODE Mode                DEFAULT_INITIALIZER(BUFFER_MODE_UNDEFINED);

    /// Miscellaneous flags, see Diligent::MISC_BUFFER_FLAGS for details.
    MISC_BUFFER_FLAGS MiscFlags     DEFAULT_INITIALIZER(MISC_BUFFER_FLAG_NONE);

    /// Buffer element stride, in bytes.

    /// For a structured buffer (BufferDesc::Mode equals Diligent::BUFFER_MODE_STRUCTURED) this member
    /// defines the size of each buffer element. For a formatted buffer
    /// (BufferDesc::Mode equals Diligent::BUFFER_MODE_FORMATTED) and optionally for a raw buffer
    /// (Diligent::BUFFER_MODE_RAW), this member defines the size of the format that will be used for views
    /// created for this buffer.
    Uint32 ElementByteStride        DEFAULT_INITIALIZER(0);

    /// Defines which immediate contexts are allowed to execute commands that use this buffer.

    /// When ImmediateContextMask contains a bit at position n, the buffer may be
    /// used in the immediate context with index n directly (see DeviceContextDesc::ContextId).
    /// It may also be used in a command list recorded by a deferred context that will be executed
    /// through that immediate context.
    ///
    /// \remarks    Only specify these bits that will indicate those immediate contexts where the buffer
    ///             will actually be used. Do not set unnecessary bits as this will result in extra overhead.
    Uint64 ImmediateContextMask     DEFAULT_INITIALIZER(1);


#if DILIGENT_CPP_INTERFACE
    // We have to explicitly define constructors because otherwise the following initialization fails on Apple's clang:
    //      BufferDesc{1024, BIND_UNIFORM_BUFFER, USAGE_DEFAULT}

    constexpr BufferDesc() noexcept {}

    constexpr BufferDesc(const Char*      _Name,
                         Uint64           _Size,
                         BIND_FLAGS       _BindFlags,
                         USAGE            _Usage                = BufferDesc{}.Usage,
                         CPU_ACCESS_FLAGS _CPUAccessFlags       = BufferDesc{}.CPUAccessFlags,
                         BUFFER_MODE      _Mode                 = BufferDesc{}.Mode,
                         Uint32           _ElementByteStride    = BufferDesc{}.ElementByteStride,
                         Uint64           _ImmediateContextMask = BufferDesc{}.ImmediateContextMask) noexcept :
        DeviceObjectAttribs  {_Name             },
        Size                 {_Size             },
        BindFlags            {_BindFlags        },
        Usage                {_Usage            },
        CPUAccessFlags       {_CPUAccessFlags   },
        Mode                 {_Mode             },
        ElementByteStride    {_ElementByteStride},
        ImmediateContextMask {_ImmediateContextMask}
    {
    }

    /// Tests if two buffer descriptions are equal.

    /// \param [in] RHS - reference to the structure to compare with.
    ///
    /// \return     true if all members of the two structures *except for the Name* are equal,
    ///             and false otherwise.
    ///
    /// \note   The operator ignores the Name field as it is used for debug purposes and
    ///         doesn't affect the buffer properties.
    constexpr bool operator == (const BufferDesc& RHS)const
    {
                // Name is primarily used for debug purposes and does not affect the state.
                // It is ignored in comparison operation.
        return  // strcmp(Name, RHS.Name) == 0          &&
                Size                 == RHS.Size              &&
                BindFlags            == RHS.BindFlags         &&
                Usage                == RHS.Usage             &&
                CPUAccessFlags       == RHS.CPUAccessFlags    &&
                Mode                 == RHS.Mode              &&
                ElementByteStride    == RHS.ElementByteStride &&
                ImmediateContextMask == RHS.ImmediateContextMask;
    }
#endif
};
typedef struct BufferDesc BufferDesc;

/// Describes the buffer initial data
struct BufferData
{
    /// Pointer to the data
    const void* pData DEFAULT_INITIALIZER(nullptr);

    /// Data size, in bytes
    Uint64 DataSize   DEFAULT_INITIALIZER(0);

    /// Defines which device context will be used to initialize the buffer.

    /// The buffer will be in write state after the initialization.
    /// If an application uses the buffer in another context afterwards, it
    /// must synchronize the access to the buffer using fence.
    /// When null is provided, the first context enabled by ImmediateContextMask
    /// will be used.
    struct IDeviceContext*  pContext  DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE

    constexpr BufferData() noexcept {}

    constexpr BufferData(const void*     _pData,
                         Uint64          _DataSize,
                         IDeviceContext* _pContext = nullptr) :
        pData   {_pData   },
        DataSize{_DataSize},
        pContext{_pContext}
    {}
#endif
};
typedef struct BufferData BufferData;

/// Describes the sparse buffer properties
struct SparseBufferProperties
{
    /// The size of the buffer's virtual address space.
    Uint64  AddressSpaceSize  DEFAULT_INITIALIZER(0);

    /// The size of the sparse memory block.
    ///
    /// \note Offset in the buffer, memory offset and memory size that are used in sparse resource
    ///       binding command, must be multiples of the block size.
    ///       In Direct3D11 and Direct3D12, the block size is always 64Kb.
    ///       In Vulkan, the block size is not documented, but is usually also 64Kb.
    Uint32  BlockSize  DEFAULT_INITIALIZER(0);
};
typedef struct SparseBufferProperties SparseBufferProperties;


#define DILIGENT_INTERFACE_NAME IBuffer
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferInclusiveMethods     \
    IDeviceObjectInclusiveMethods;  \
    IBufferMethods Buffer

/// Buffer interface

/// Defines the methods to manipulate a buffer object
DILIGENT_BEGIN_INTERFACE(IBuffer, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the buffer description used to create the object
    virtual const BufferDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Creates a new buffer view

    /// \param [in] ViewDesc - View description. See Diligent::BufferViewDesc for details.
    /// \param [out] ppView - Address of the memory location where the pointer to the view interface will be written to.
    ///
    /// \remarks To create a view addressing the entire buffer, set only BufferViewDesc::ViewType member
    ///          of the ViewDesc structure and leave all other members in their default values.\n
    ///          Buffer view will contain strong reference to the buffer, so the buffer will not be destroyed
    ///          until all views are released.\n
    ///          The function calls AddRef() for the created interface, so it must be released by
    ///          a call to Release() when it is no longer needed.
    VIRTUAL void METHOD(CreateView)(THIS_
                                    const BufferViewDesc REF ViewDesc,
                                    IBufferView** ppView) PURE;

    /// Returns the pointer to the default view.

    /// \param [in] ViewType - Type of the requested view. See Diligent::BUFFER_VIEW_TYPE.
    /// \return Pointer to the interface
    ///
    /// \remarks Default views are only created for structured and raw buffers. As for formatted buffers
    ///          the view format is unknown at buffer initialization time, no default views are created.
    ///
    /// \note The function does not increase the reference counter for the returned interface, so
    ///       Release() must *NOT* be called.
    VIRTUAL  IBufferView* METHOD(GetDefaultView)(THIS_
                                                 BUFFER_VIEW_TYPE ViewType) PURE;

    /// Returns native buffer handle specific to the underlying graphics API

    /// \return pointer to ID3D11Resource interface, for D3D11 implementation\n
    ///         pointer to ID3D12Resource interface, for D3D12 implementation\n
    ///         GL buffer handle, for GL implementation
    VIRTUAL Uint64 METHOD(GetNativeHandle)(THIS) PURE;

    /// Sets the buffer usage state.

    /// \note This method does not perform state transition, but
    ///       resets the internal buffer state to the given value.
    ///       This method should be used after the application finished
    ///       manually managing the buffer state and wants to hand over
    ///       state management back to the engine.
    VIRTUAL void METHOD(SetState)(THIS_
                                  RESOURCE_STATE State) PURE;

    /// Returns the internal buffer state
    VIRTUAL RESOURCE_STATE METHOD(GetState)(THIS) CONST PURE;


    /// Returns the buffer memory properties, see Diligent::MEMORY_PROPERTIES.

    /// The memory properties are only relevant for persistently mapped buffers.
    /// In particular, if the memory is not coherent, an application must call
    /// IBuffer::FlushMappedRange() to make writes by the CPU available to the GPU, and
    /// call IBuffer::InvalidateMappedRange() to make writes by the GPU visible to the CPU.
    VIRTUAL MEMORY_PROPERTIES METHOD(GetMemoryProperties)(THIS) CONST PURE;


    /// Flushes the specified range of non-coherent memory from the host cache to make
    /// it available to the GPU.

    /// \param [in] StartOffset - Offset, in bytes, from the beginning of the buffer to
    ///                           the start of the memory range to flush.
    /// \param [in] Size        - Size, in bytes, of the memory range to flush.
    ///
    /// This method should only be used for persistently-mapped buffers that do not
    /// report MEMORY_PROPERTY_HOST_COHERENT property. After an application modifies
    /// a mapped memory range on the CPU, it must flush the range to make it available
    /// to the GPU.
    ///
    /// \note   This method must never be used for USAGE_DYNAMIC buffers.
    ///
    ///         When a mapped buffer is unmapped it is automatically flushed by
    ///         the engine if necessary.
    VIRTUAL void METHOD(FlushMappedRange)(THIS_
                                          Uint64 StartOffset,
                                          Uint64 Size) PURE;


    /// Invalidates the specified range of non-coherent memory modified by the GPU to make
    /// it visible to the CPU.

    /// \param [in] StartOffset - Offset, in bytes, from the beginning of the buffer to
    ///                           the start of the memory range to invalidate.
    /// \param [in] Size        - Size, in bytes, of the memory range to invalidate.
    ///
    /// This method should only be used for persistently-mapped buffers that do not
    /// report MEMORY_PROPERTY_HOST_COHERENT property. After an application modifies
    /// a mapped memory range on the GPU, it must invalidate the range to make it visible
    /// to the CPU.
    ///
    /// \note   This method must never be used for USAGE_DYNAMIC buffers.
    ///
    ///         When a mapped buffer is unmapped it is automatically invalidated by
    ///         the engine if necessary.
    VIRTUAL void METHOD(InvalidateMappedRange)(THIS_
                                               Uint64 StartOffset,
                                               Uint64 Size) PURE;

    /// Returns the sparse buffer memory properties
    VIRTUAL SparseBufferProperties METHOD(GetSparseProperties)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IBuffer_GetDesc(This) (const struct BufferDesc*)IDeviceObject_GetDesc(This)

#    define IBuffer_CreateView(This, ...)            CALL_IFACE_METHOD(Buffer, CreateView,            This, __VA_ARGS__)
#    define IBuffer_GetDefaultView(This, ...)        CALL_IFACE_METHOD(Buffer, GetDefaultView,        This, __VA_ARGS__)
#    define IBuffer_GetNativeHandle(This)            CALL_IFACE_METHOD(Buffer, GetNativeHandle,       This)
#    define IBuffer_SetState(This, ...)              CALL_IFACE_METHOD(Buffer, SetState,              This, __VA_ARGS__)
#    define IBuffer_GetState(This)                   CALL_IFACE_METHOD(Buffer, GetState,              This)
#    define IBuffer_GetMemoryProperties(This)        CALL_IFACE_METHOD(Buffer, GetMemoryProperties,   This)
#    define IBuffer_FlushMappedRange(This, ...)      CALL_IFACE_METHOD(Buffer, FlushMappedRange,      This, __VA_ARGS__)
#    define IBuffer_InvalidateMappedRange(This, ...) CALL_IFACE_METHOD(Buffer, InvalidateMappedRange, This, __VA_ARGS__)
#    define IBuffer_GetSparseProperties(This)        CALL_IFACE_METHOD(Buffer, GetSparseProperties,   This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
