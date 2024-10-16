/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Definition of the Diligent::IDeviceContext interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/FlagEnum.h"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "Buffer.h"
#include "InputLayout.h"
#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"
#include "ResourceMapping.h"
#include "TextureView.h"
#include "BufferView.h"
#include "DepthStencilState.h"
#include "BlendState.h"
#include "PipelineState.h"
#include "Fence.h"
#include "Query.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandList.h"
#include "SwapChain.h"
#include "BottomLevelAS.h"
#include "TopLevelAS.h"
#include "ShaderBindingTable.h"
#include "DeviceMemory.h"
#include "CommandQueue.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {DC92711B-A1BE-4319-B2BD-C662D1CC19E4}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_DeviceContext =
    {0xdc92711b, 0xa1be, 0x4319, {0xb2, 0xbd, 0xc6, 0x62, 0xd1, 0xcc, 0x19, 0xe4}};

/// Device context description.
struct DeviceContextDesc
{
    /// Device context name. This name is what was specified in
    /// ImmediateContextCreateInfo::Name when the engine was initialized.
    const Char*  Name            DEFAULT_INITIALIZER(nullptr);

    /// Command queue type that this context uses.

    /// For immediate contexts, this type matches GraphicsAdapterInfo::Queues[QueueId].QueueType.
    /// For deferred contexts, the type is only defined between IDeviceContext::Begin and IDeviceContext::FinishCommandList
    /// calls and matches the type of the immediate context where the command list will be executed.
    COMMAND_QUEUE_TYPE QueueType DEFAULT_INITIALIZER(COMMAND_QUEUE_TYPE_UNKNOWN);

    /// Indicates if this is a deferred context.
    Bool         IsDeferred      DEFAULT_INITIALIZER(False);

    /// Device context ID. This value corresponds to the index of the device context
    /// in ppContexts array when the engine was initialized.
    /// When starting recording commands with a deferred context, the context id
    /// of the immediate context where the command list will be executed should be
    /// given to IDeviceContext::Begin() method.
    Uint8        ContextId       DEFAULT_INITIALIZER(0);

    /// Hardware queue index in GraphicsAdapterInfo::Queues array.

    /// \remarks  This member is only defined for immediate contexts and matches
    ///           QueueId member of ImmediateContextCreateInfo struct that was used to
    ///           initialize the context.
    ///
    ///           - Vulkan backend: same as queue family index.
    ///           - Direct3D12 backend:
    ///             - 0 - Graphics queue
    ///             - 1 - Compute queue
    ///             - 2 - Transfer queue
    ///           - Metal backend: index of the unique command queue.
    Uint8        QueueId    DEFAULT_INITIALIZER(DEFAULT_QUEUE_ID);


    /// Required texture granularity for copy operations, for a transfer queue.

    /// \remarks  For graphics and compute queues, the granularity is always {1,1,1}.
    ///           For transfer queues, an application must align the texture offsets and sizes
    ///           by the granularity defined by this member.
    ///
    ///           For deferred contexts, this member is only defined between IDeviceContext::Begin and
    ///           IDeviceContext::FinishCommandList calls and matches the texture copy granularity of
    ///           the immediate context where the command list will be executed.
    Uint32       TextureCopyGranularity[3] DEFAULT_INITIALIZER({});

#if DILIGENT_CPP_INTERFACE
    constexpr DeviceContextDesc() noexcept {}

    /// Initializes the structure with user-specified values.
    constexpr DeviceContextDesc(const Char*        _Name,
                                COMMAND_QUEUE_TYPE _QueueType,
                                Bool               _IsDeferred,
                                Uint32             _ContextId,
                                Uint32             _QueueId = DeviceContextDesc{}.QueueId) noexcept :
        Name      {_Name      },
        QueueType {_QueueType },
        IsDeferred{_IsDeferred},
        ContextId {static_cast<decltype(ContextId)>(_ContextId)},
        QueueId   {static_cast<decltype(QueueId)>(_QueueId)}
    {
        if (!IsDeferred)
        {
            TextureCopyGranularity[0] = 1;
            TextureCopyGranularity[1] = 1;
            TextureCopyGranularity[2] = 1;
        }
        else
        {
            // For deferred contexts texture copy granularity is set by IDeviceContext::Begin() method.
        }
    }
#endif
};
typedef struct DeviceContextDesc DeviceContextDesc;


/// Draw command flags
DILIGENT_TYPED_ENUM(DRAW_FLAGS, Uint8)
{
    /// No flags.
    DRAW_FLAG_NONE                            = 0x00,

    /// Verify the state of index and vertex buffers (if any) used by the draw
    /// command. State validation is only performed in debug and development builds
    /// and the flag has no effect in release build.
    DRAW_FLAG_VERIFY_STATES                   = 0x01,

    /// Verify correctness of parameters passed to the draw command.
    ///
    /// \remarks This flag only has effect in debug and development builds.
    ///          Verification is always disabled in release configuration.
    DRAW_FLAG_VERIFY_DRAW_ATTRIBS             = 0x02,

    /// Verify that render targets bound to the context are consistent with the pipeline state.
    ///
    /// \remarks This flag only has effect in debug and development builds.
    ///          Verification is always disabled in release configuration.
    DRAW_FLAG_VERIFY_RENDER_TARGETS           = 0x04,

    /// Perform all state validation checks
    ///
    /// \remarks This flag only has effect in debug and development builds.
    ///          Verification is always disabled in release configuration.
    DRAW_FLAG_VERIFY_ALL                      = DRAW_FLAG_VERIFY_STATES | DRAW_FLAG_VERIFY_DRAW_ATTRIBS | DRAW_FLAG_VERIFY_RENDER_TARGETS,

    /// Indicates that none of the dynamic resource buffers used by the draw command
    /// have been modified by the CPU since the last command.
    ///
    /// \remarks This flag should be used to improve performance when an application issues a
    ///          series of draw commands that use the same pipeline state and shader resources and
    ///          no dynamic buffers (constant or bound as shader resources) are updated between the
    ///          commands.
    ///          Any buffer variable not created with SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS or
    ///          PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flags is counted as dynamic.
    ///          The flag has no effect on dynamic vertex and index buffers.
    ///
    ///          Details
    ///
    ///          D3D12 and Vulkan back-ends have to perform some work to make data in buffers
    ///          available to draw commands. When a dynamic buffer is mapped, the engine allocates
    ///          new memory and assigns a new GPU address to this buffer. When a draw command is issued,
    ///          this GPU address needs to be used. By default the engine assumes that the CPU may
    ///          map the buffer before any command (to write new transformation matrices for example)
    ///          and that all GPU addresses need to always be refreshed. This is not always the case,
    ///          and the application may use the flag to inform the engine that the data in the buffer
    ///          stay intact to avoid extra work.\n
    ///          Note that after a new PSO is bound or an SRB is committed, the engine will always set all
    ///          required buffer addresses/offsets regardless of the flag. The flag will only take effect
    ///          on the second and subsequent draw calls that use the same PSO and SRB.\n
    ///          The flag has no effect in D3D11 and OpenGL backends.
    ///
    ///          Implementation details
    ///
    ///          Vulkan backend allocates VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC descriptors for all uniform (constant),
    ///          buffers and VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC descriptors for storage buffers.
    ///          Note that HLSL structured buffers are mapped to read-only storage buffers in SPIRV and RW buffers
    ///          are mapped to RW-storage buffers.
    ///          By default, all dynamic descriptor sets that have dynamic buffers bound are updated every time a draw command is
    ///          issued (see PipelineStateVkImpl::BindDescriptorSetsWithDynamicOffsets). When DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT
    ///          is specified, dynamic descriptor sets are only bound by the first draw command that uses the PSO and the SRB.
    ///          The flag avoids binding descriptors with the same offsets if none of the dynamic offsets have changed.
    ///
    ///          Direct3D12 backend binds constant buffers to root views. By default the engine assumes that virtual GPU addresses
    ///          of all dynamic buffers may change between the draw commands and always binds dynamic buffers to root views
    ///          (see RootSignature::CommitRootViews). When DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT is set, root views are only bound
    ///          by the first draw command that uses the PSO + SRB pair. The flag avoids setting the same GPU virtual addresses when
    ///          they stay unchanged.
    DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT = 0x08
};
DEFINE_FLAG_ENUM_OPERATORS(DRAW_FLAGS)


/// Defines resource state transition mode performed by various commands.

/// Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
/// of resource state management in Diligent Engine.
DILIGENT_TYPED_ENUM(RESOURCE_STATE_TRANSITION_MODE, Uint8)
{
    /// Perform no state transitions and no state validation.
    /// Resource states are not accessed (either read or written) by the command.
    RESOURCE_STATE_TRANSITION_MODE_NONE = 0,

    /// Transition resources to the states required by the specific command.
    /// Resources in unknown state are ignored.
    ///
    /// \note    Any method that uses this mode may alter the state of the resources it works with.
    ///          As automatic state management is not thread-safe, no other thread is allowed to read
    ///          or write the state of the resources being transitioned.
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    /// \note    If a resource is used in multiple threads by multiple contexts, there will be race condition accessing
    ///          internal resource state. An application should use manual resource state management in this case.
    RESOURCE_STATE_TRANSITION_MODE_TRANSITION,

    /// Do not transition, but verify that states are correct.
    /// No validation is performed if the state is unknown to the engine.
    /// This mode only has effect in debug and development builds. No validation
    /// is performed in release build.
    ///
    /// \note    Any method that uses this mode will read the state of resources it works with.
    ///          As automatic state management is not thread-safe, no other thread is allowed to alter
    ///          the state of resources being used by the command. It is safe to read these states.
    RESOURCE_STATE_TRANSITION_MODE_VERIFY
};


/// Defines the draw command attributes.

/// This structure is used by IDeviceContext::Draw().
struct DrawAttribs
{
    /// The number of vertices to draw.
    Uint32     NumVertices           DEFAULT_INITIALIZER(0);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// The number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (or INDEX, but NOT the byte offset) of the first vertex in the
    /// vertex buffer to start reading vertices from.
    Uint32     StartVertexLocation   DEFAULT_INITIALIZER(0);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex buffer to start
    /// reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.

    /// Default values:
    ///
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// NumVertices                              | 0
    /// Flags                                    | DRAW_FLAG_NONE
    /// NumInstances                             | 1
    /// StartVertexLocation                      | 0
    /// FirstInstanceLocation                    | 0
    constexpr DrawAttribs() noexcept {}

    /// Initializes the structure with user-specified values.
    constexpr DrawAttribs(Uint32     _NumVertices,
                          DRAW_FLAGS _Flags,
                          Uint32     _NumInstances          = 1,
                          Uint32     _StartVertexLocation   = 0,
                          Uint32     _FirstInstanceLocation = 0) noexcept :
        NumVertices          {_NumVertices          },
        Flags                {_Flags                },
        NumInstances         {_NumInstances         },
        StartVertexLocation  {_StartVertexLocation  },
        FirstInstanceLocation{_FirstInstanceLocation}
    {}
#endif
};
typedef struct DrawAttribs DrawAttribs;


/// Defines the indexed draw command attributes.

/// This structure is used by IDeviceContext::DrawIndexed().
struct DrawIndexedAttribs
{
    /// The number of indices to draw.
    Uint32     NumIndices            DEFAULT_INITIALIZER(0);

    /// The type of elements in the index buffer.
    /// Allowed values: VT_UINT16 and VT_UINT32.
    VALUE_TYPE IndexType             DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// Number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (NOT the byte offset) of the first index in
    /// the index buffer to start reading indices from.
    Uint32     FirstIndexLocation    DEFAULT_INITIALIZER(0);

    /// A constant which is added to each index before accessing the vertex buffer.
    Uint32     BaseVertex            DEFAULT_INITIALIZER(0);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex
    /// buffer to start reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.

    /// Default values:
    /// Member                                   | Default value
    /// -----------------------------------------|--------------------------------------
    /// NumIndices                               | 0
    /// IndexType                                | VT_UNDEFINED
    /// Flags                                    | DRAW_FLAG_NONE
    /// NumInstances                             | 1
    /// FirstIndexLocation                       | 0
    /// BaseVertex                               | 0
    /// FirstInstanceLocation                    | 0
    constexpr DrawIndexedAttribs() noexcept {}

    /// Initializes the structure members with user-specified values.
    constexpr DrawIndexedAttribs(Uint32      _NumIndices,
                                 VALUE_TYPE  _IndexType,
                                 DRAW_FLAGS  _Flags,
                                 Uint32      _NumInstances          = 1,
                                 Uint32      _FirstIndexLocation    = 0,
                                 Uint32      _BaseVertex            = 0,
                                 Uint32      _FirstInstanceLocation = 0) noexcept :
        NumIndices           {_NumIndices           },
        IndexType            {_IndexType            },
        Flags                {_Flags                },
        NumInstances         {_NumInstances         },
        FirstIndexLocation   {_FirstIndexLocation   },
        BaseVertex           {_BaseVertex           },
        FirstInstanceLocation{_FirstInstanceLocation}
    {}
#endif
};
typedef struct DrawIndexedAttribs DrawIndexedAttribs;


/// Defines the indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawIndirect().
struct DrawIndirectAttribs
{
    /// A pointer to the buffer, from which indirect draw attributes will be read.
    ///
    /// The buffer must contain the following arguments at the specified offset:
    ///     Uint32 NumVertices;
    ///     Uint32 NumInstances;
    ///     Uint32 StartVertexLocation;
    ///     Uint32 FirstInstanceLocation;
    IBuffer*   pAttribsBuffer       DEFAULT_VALUE(nullptr);

    /// Offset from the beginning of the buffer to the location of draw command attributes.
    Uint64 DrawArgsOffset           DEFAULT_INITIALIZER(0);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// The number of draw commands to execute. When the pCounterBuffer is not null, this member
    /// defines the maximum number of commands that will be executed.
    /// Must be less than DrawCommandProperties::MaxDrawIndirectCount.
    Uint32 DrawCount                DEFAULT_INITIALIZER(1);

    /// When DrawCount > 1, the byte stride between successive sets of draw parameters.
    /// Must be a multiple of 4 and greater than or equal to 16 bytes (sizeof(Uint32) * 4).
    Uint32 DrawArgsStride           DEFAULT_INITIALIZER(16);

    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE  AttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


    /// A pointer to the optional buffer, from which Uint32 value with the draw count will be read.
    IBuffer* pCounterBuffer         DEFAULT_VALUE(nullptr);

    /// When pCounterBuffer is not null, an offset from the beginning of the buffer to the
    /// location of the command counter.
    Uint64 CounterOffset            DEFAULT_INITIALIZER(0);

    /// When counter buffer is not null, state transition mode for the count buffer.
    RESOURCE_STATE_TRANSITION_MODE  CounterBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values
    constexpr DrawIndirectAttribs() noexcept {}

    /// Initializes the structure members with user-specified values.
    explicit constexpr DrawIndirectAttribs(IBuffer*                       _pAttribsBuffer,
                                           DRAW_FLAGS                     _Flags,
                                           Uint32                         _DrawCount                   = DrawIndirectAttribs{}.DrawCount,
                                           Uint64                         _DrawArgsOffset              = DrawIndirectAttribs{}.DrawArgsOffset,
                                           Uint32                         _DrawArgsStride              = DrawIndirectAttribs{}.DrawArgsStride,
                                           RESOURCE_STATE_TRANSITION_MODE _AttribsBufferTransitionMode = DrawIndirectAttribs{}.AttribsBufferStateTransitionMode,
                                           IBuffer*                       _pCounterBuffer              = DrawIndirectAttribs{}.pCounterBuffer,
                                           Uint64                         _CounterOffset               = DrawIndirectAttribs{}.CounterOffset,
                                           RESOURCE_STATE_TRANSITION_MODE _CounterBufferTransitionMode = DrawIndirectAttribs{}.CounterBufferStateTransitionMode) noexcept :
        pAttribsBuffer                  {_pAttribsBuffer             },
        DrawArgsOffset                  {_DrawArgsOffset             },
        Flags                           {_Flags                      },
        DrawCount                       {_DrawCount                  },
        DrawArgsStride                  {_DrawArgsStride             },
        AttribsBufferStateTransitionMode{_AttribsBufferTransitionMode},
        pCounterBuffer                  {_pCounterBuffer             },
        CounterOffset                   {_CounterOffset              },
        CounterBufferStateTransitionMode{_CounterBufferTransitionMode}
    {}
#endif
};
typedef struct DrawIndirectAttribs DrawIndirectAttribs;


/// Defines the indexed indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawIndexedIndirect().
struct DrawIndexedIndirectAttribs
{
    /// The type of the elements in the index buffer.
    /// Allowed values: VT_UINT16 and VT_UINT32.
    VALUE_TYPE IndexType            DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// A pointer to the buffer, from which indirect draw attributes will be read.
    ///
    /// The buffer must contain the following arguments at the specified offset:
    ///     Uint32 NumIndices;
    ///     Uint32 NumInstances;
    ///     Uint32 FirstIndexLocation;
    ///     Uint32 BaseVertex;
    ///     Uint32 FirstInstanceLocation
    IBuffer*  pAttribsBuffer        DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer to the location of the draw command attributes.
    Uint64 DrawArgsOffset           DEFAULT_INITIALIZER(0);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// The number of draw commands to execute. When the pCounterBuffer is not null, this member
    /// defines the maximum number of commands that will be executed.
    /// Must be less than DrawCommandProperties::MaxDrawIndirectCount.
    Uint32 DrawCount                DEFAULT_INITIALIZER(1);

    /// When DrawCount > 1, the byte stride between successive sets of draw parameters.
    /// Must be a multiple of 4 and greater than or equal to 20 bytes (sizeof(Uint32) * 5).
    Uint32 DrawArgsStride           DEFAULT_INITIALIZER(20);

    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE AttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


    /// A pointer to the optional buffer, from which Uint32 value with the draw count will be read.
    IBuffer* pCounterBuffer         DEFAULT_INITIALIZER(nullptr);

    /// When pCounterBuffer is not null, offset from the beginning of the counter buffer to the
    /// location of the command counter.
    Uint64 CounterOffset            DEFAULT_INITIALIZER(0);

    /// When counter buffer is not null, state transition mode for the count buffer.
    RESOURCE_STATE_TRANSITION_MODE CounterBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values
    constexpr DrawIndexedIndirectAttribs() noexcept {}

    /// Initializes the structure members with user-specified values.
    constexpr DrawIndexedIndirectAttribs(VALUE_TYPE                     _IndexType,
                                         IBuffer*                       _pAttribsBuffer,
                                         DRAW_FLAGS                     _Flags,
                                         Uint32                         _DrawCount                   = DrawIndexedIndirectAttribs{}.DrawCount,
                                         Uint64                         _DrawArgsOffset              = DrawIndexedIndirectAttribs{}.DrawArgsOffset,
                                         Uint32                         _DrawArgsStride              = DrawIndexedIndirectAttribs{}.DrawArgsStride,
                                         RESOURCE_STATE_TRANSITION_MODE _AttribsBufferTransitionMode = DrawIndexedIndirectAttribs{}.AttribsBufferStateTransitionMode,
                                         IBuffer*                       _pCounterBuffer              = DrawIndexedIndirectAttribs{}.pCounterBuffer,
                                         Uint64                         _CounterOffset               = DrawIndexedIndirectAttribs{}.CounterOffset,
                                         RESOURCE_STATE_TRANSITION_MODE _CounterBufferTransitionMode = DrawIndexedIndirectAttribs{}.CounterBufferStateTransitionMode) noexcept :
        IndexType                       {_IndexType                  },
        pAttribsBuffer                  {_pAttribsBuffer             },
        DrawArgsOffset                  {_DrawArgsOffset             },
        Flags                           {_Flags                      },
        DrawCount                       {_DrawCount                  },
        DrawArgsStride                  {_DrawArgsStride             },
        AttribsBufferStateTransitionMode{_AttribsBufferTransitionMode},
        pCounterBuffer                  {_pCounterBuffer             },
        CounterOffset                   {_CounterOffset              },
        CounterBufferStateTransitionMode{_CounterBufferTransitionMode}
    {}
#endif
};
typedef struct DrawIndexedIndirectAttribs DrawIndexedIndirectAttribs;


/// Defines the mesh draw command attributes.

/// This structure is used by IDeviceContext::DrawMesh().
struct DrawMeshAttribs
{
    /// The number of groups dispatched in X direction.
    Uint32 ThreadGroupCountX DEFAULT_INITIALIZER(1);

    /// The number of groups dispatched in Y direction.
    Uint32 ThreadGroupCountY DEFAULT_INITIALIZER(1);

    /// The number of groups dispatched in Y direction.
    Uint32 ThreadGroupCountZ DEFAULT_INITIALIZER(1);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags         DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values.
    constexpr DrawMeshAttribs() noexcept {}

    explicit constexpr DrawMeshAttribs(Uint32     _ThreadGroupCountX,
                                       DRAW_FLAGS _Flags = DRAW_FLAG_NONE) noexcept :
        ThreadGroupCountX{_ThreadGroupCountX},
        Flags            {_Flags}
    {}

    constexpr DrawMeshAttribs(Uint32     _ThreadGroupCountX,
                              Uint32     _ThreadGroupCountY,
                              DRAW_FLAGS _Flags = DRAW_FLAG_NONE) noexcept :
        ThreadGroupCountX{_ThreadGroupCountX},
        ThreadGroupCountY{_ThreadGroupCountY},
        Flags            {_Flags}
    {}

    constexpr DrawMeshAttribs(Uint32     _ThreadGroupCountX,
                              Uint32     _ThreadGroupCountY,
                              Uint32     _ThreadGroupCountZ,
                              DRAW_FLAGS _Flags = DRAW_FLAG_NONE) noexcept :
        ThreadGroupCountX{_ThreadGroupCountX},
        ThreadGroupCountY{_ThreadGroupCountY},
        ThreadGroupCountZ{_ThreadGroupCountZ},
        Flags            {_Flags}
    {}
#endif
};
typedef struct DrawMeshAttribs DrawMeshAttribs;


/// Defines the mesh indirect draw command attributes.

/// This structure is used by IDeviceContext::DrawMeshIndirect().
struct DrawMeshIndirectAttribs
{
    /// A pointer to the buffer, from which indirect draw attributes will be read.
    /// The buffer must contain the following arguments at the specified offset:
    ///   Direct3D12:
    ///        Uint32 ThreadGroupCountX;
    ///        Uint32 ThreadGroupCountY;
    ///        Uint32 ThreadGroupCountZ;
    ///   Vulkan:
    ///        Uint32 TaskCount;
    ///        Uint32 FirstTask;
    /// Size of the buffer must be sizeof(Uint32[3]) * Attribs.MaxDrawCommands.
    IBuffer*   pAttribsBuffer   DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the attribs buffer to the location of the draw command attributes.
    Uint64 DrawArgsOffset       DEFAULT_INITIALIZER(0);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags            DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// When pCounterBuffer is null, the number of commands to run.
    /// When pCounterBuffer is not null, the maximum number of commands
    /// that will be read from the count buffer.
    Uint32 CommandCount         DEFAULT_INITIALIZER(1);

    /// State transition mode for indirect draw arguments buffer.
    RESOURCE_STATE_TRANSITION_MODE AttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


    /// A pointer to the optional buffer, from which Uint32 value with the draw count will be read.
    IBuffer* pCounterBuffer DEFAULT_INITIALIZER(nullptr);

    /// When pCounterBuffer is not null, an offset from the beginning of the buffer to the location of the command counter.
    Uint64 CounterOffset    DEFAULT_INITIALIZER(0);

    /// When pCounterBuffer is not null, state transition mode for the count buffer.
    RESOURCE_STATE_TRANSITION_MODE CounterBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure members with default values
    constexpr DrawMeshIndirectAttribs() noexcept {}

    /// Initializes the structure members with user-specified values.
    constexpr DrawMeshIndirectAttribs(IBuffer*                       _pAttribsBuffer,
                                      DRAW_FLAGS                     _Flags,
                                      Uint32                         _CommandCount,
                                      Uint64                         _DrawArgsOffset                   = DrawMeshIndirectAttribs{}.DrawArgsOffset,
                                      RESOURCE_STATE_TRANSITION_MODE _AttribsBufferStateTransitionMode = DrawMeshIndirectAttribs{}.AttribsBufferStateTransitionMode,
                                      IBuffer*                       _pCounterBuffer                   = DrawMeshIndirectAttribs{}.pCounterBuffer,
                                      Uint64                         _CounterOffset                    = DrawMeshIndirectAttribs{}.CounterOffset,
                                      RESOURCE_STATE_TRANSITION_MODE _CounterBufferStateTransitionMode = DrawMeshIndirectAttribs{}.CounterBufferStateTransitionMode) noexcept :
        pAttribsBuffer                  {_pAttribsBuffer                  },
        DrawArgsOffset                  {_DrawArgsOffset                  },
        Flags                           {_Flags                           },
        CommandCount                    {_CommandCount                    },
        AttribsBufferStateTransitionMode{_AttribsBufferStateTransitionMode},
        pCounterBuffer                  {_pCounterBuffer                  },
        CounterOffset                   {_CounterOffset                   },
        CounterBufferStateTransitionMode{_CounterBufferStateTransitionMode}
    {}
#endif
};
typedef struct DrawMeshIndirectAttribs DrawMeshIndirectAttribs;


/// Multi-draw command item.
struct MultiDrawItem
{
    /// The number of vertices to draw.
    Uint32     NumVertices           DEFAULT_INITIALIZER(0);

    /// LOCATION (or INDEX, but NOT the byte offset) of the first vertex in the
    /// vertex buffer to start reading vertices from.
    Uint32     StartVertexLocation   DEFAULT_INITIALIZER(0);
};
typedef struct MultiDrawItem MultiDrawItem;

/// MultiDraw command attributes.
struct MultiDrawAttribs
{
    /// The number of draw items to execute.
    Uint32               DrawCount   DEFAULT_INITIALIZER(0);

    /// A pointer to the array of DrawCount draw command items.
    const MultiDrawItem* pDrawItems  DEFAULT_INITIALIZER(nullptr);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// The number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex buffer to start
    /// reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    constexpr MultiDrawAttribs() noexcept {}

    constexpr MultiDrawAttribs(Uint32               _DrawCount,
                               const MultiDrawItem* _pDrawItems,
							   DRAW_FLAGS           _Flags,
							   Uint32               _NumInstances          = 1,
							   Uint32               _FirstInstanceLocation = 0) noexcept :
		DrawCount            {_DrawCount            },
		pDrawItems           {_pDrawItems           },
		Flags                {_Flags                },
		NumInstances         {_NumInstances         },
		FirstInstanceLocation{_FirstInstanceLocation}
	{}
#endif
};
typedef struct MultiDrawAttribs MultiDrawAttribs;


/// Multi-draw indexed command item.
struct MultiDrawIndexedItem
{
    /// The number of indices to draw.
    Uint32     NumIndices            DEFAULT_INITIALIZER(0);

    /// LOCATION (NOT the byte offset) of the first index in
    /// the index buffer to start reading indices from.
    Uint32     FirstIndexLocation    DEFAULT_INITIALIZER(0);

    /// A constant which is added to each index before accessing the vertex buffer.
    Uint32     BaseVertex            DEFAULT_INITIALIZER(0);
};
typedef struct MultiDrawIndexedItem MultiDrawIndexedItem;

/// MultiDraw command attributes.
struct MultiDrawIndexedAttribs
{
    /// The number of draw items to execute.
    Uint32                      DrawCount  DEFAULT_INITIALIZER(0);

    /// A pointer to the array of DrawCount draw command items.
    const MultiDrawIndexedItem* pDrawItems DEFAULT_INITIALIZER(nullptr);

    /// The type of elements in the index buffer.
    /// Allowed values: VT_UINT16 and VT_UINT32.
    VALUE_TYPE IndexType             DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags                 DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

    /// Number of instances to draw. If more than one instance is specified,
    /// instanced draw call will be performed.
    Uint32     NumInstances          DEFAULT_INITIALIZER(1);

    /// LOCATION (or INDEX, but NOT the byte offset) in the vertex
    /// buffer to start reading instance data from.
    Uint32     FirstInstanceLocation DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
constexpr MultiDrawIndexedAttribs() noexcept {}

	constexpr MultiDrawIndexedAttribs(Uint32                      _DrawCount,
									  const MultiDrawIndexedItem* _pDrawItems,
									  VALUE_TYPE                  _IndexType,
									  DRAW_FLAGS                  _Flags,
									  Uint32                      _NumInstances          = 1,
									  Uint32                      _FirstInstanceLocation = 0) noexcept :
		DrawCount            {_DrawCount            },
		pDrawItems           {_pDrawItems           },
		IndexType            {_IndexType            },
		Flags                {_Flags                },
		NumInstances         {_NumInstances         },
		FirstInstanceLocation{_FirstInstanceLocation}
	{}
#endif
};
typedef struct MultiDrawIndexedAttribs MultiDrawIndexedAttribs;


/// Defines which parts of the depth-stencil buffer to clear.

/// These flags are used by IDeviceContext::ClearDepthStencil().
DILIGENT_TYPED_ENUM(CLEAR_DEPTH_STENCIL_FLAGS, Uint32)
{
    /// Perform no clear.
    CLEAR_DEPTH_FLAG_NONE = 0x00,

    /// Clear depth part of the buffer.
    CLEAR_DEPTH_FLAG      = 0x01,

    /// Clear stencil part of the buffer.
    CLEAR_STENCIL_FLAG    = 0x02
};
DEFINE_FLAG_ENUM_OPERATORS(CLEAR_DEPTH_STENCIL_FLAGS)


/// Describes dispatch command arguments.

/// This structure is used by IDeviceContext::DispatchCompute().
struct DispatchComputeAttribs
{
    /// The number of groups dispatched in X direction.
    Uint32 ThreadGroupCountX DEFAULT_INITIALIZER(1);

    /// The number of groups dispatched in Y direction.
    Uint32 ThreadGroupCountY DEFAULT_INITIALIZER(1);

    /// The number of groups dispatched in Z direction.
    Uint32 ThreadGroupCountZ DEFAULT_INITIALIZER(1);


    /// Compute group X size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeX DEFAULT_INITIALIZER(0);

    /// Compute group Y size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeY DEFAULT_INITIALIZER(0);

    /// Compute group Z size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeZ DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    constexpr DispatchComputeAttribs() noexcept {}

    /// Initializes the structure with user-specified values.
    constexpr DispatchComputeAttribs(Uint32 GroupsX, Uint32 GroupsY, Uint32 GroupsZ = 1) noexcept :
        ThreadGroupCountX {GroupsX},
        ThreadGroupCountY {GroupsY},
        ThreadGroupCountZ {GroupsZ}
    {}
#endif
};
typedef struct DispatchComputeAttribs DispatchComputeAttribs;


/// Describes dispatch command arguments.

/// This structure is used by IDeviceContext::DispatchComputeIndirect().
struct DispatchComputeIndirectAttribs
{
    /// A pointer to the buffer containing indirect dispatch attributes.

    /// The buffer must contain the following arguments at the specified offset:
    ///    Uint32 ThreadGroupCountX;
    ///    Uint32 ThreadGroupCountY;
    ///    Uint32 ThreadGroupCountZ;
    IBuffer*                       pAttribsBuffer                   DEFAULT_INITIALIZER(nullptr);

    /// State transition mode for indirect dispatch attributes buffer.
    RESOURCE_STATE_TRANSITION_MODE AttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// The offset from the beginning of the buffer to the dispatch command arguments.
    Uint64  DispatchArgsByteOffset    DEFAULT_INITIALIZER(0);


    /// Compute group X size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeX DEFAULT_INITIALIZER(0);

    /// Compute group Y size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeY DEFAULT_INITIALIZER(0);

    /// Compute group Z size. This member is only used
    /// by Metal backend and is ignored by others.
    Uint32 MtlThreadGroupSizeZ DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    constexpr DispatchComputeIndirectAttribs() noexcept {}

    /// Initializes the structure with user-specified values.
    constexpr DispatchComputeIndirectAttribs(IBuffer*                       _pAttribsBuffer,
                                             RESOURCE_STATE_TRANSITION_MODE _StateTransitionMode,
                                             Uint64                         _Offset              = 0) :
        pAttribsBuffer                  {_pAttribsBuffer     },
        AttribsBufferStateTransitionMode{_StateTransitionMode},
        DispatchArgsByteOffset          {_Offset             }
    {}
#endif
};
typedef struct DispatchComputeIndirectAttribs DispatchComputeIndirectAttribs;


/// Describes tile dispatch command arguments.

/// This structure is used by IDeviceContext::DispatchTile().
struct DispatchTileAttribs
{
    /// The number of threads in one tile dispatched in X direction.
    /// Must not be greater than TileSizeX returned by IDeviceContext::GetTileSize().
    Uint32 ThreadsPerTileX DEFAULT_INITIALIZER(1);

    /// The number of threads in one tile dispatched in Y direction.
    /// Must not be greater than TileSizeY returned by IDeviceContext::GetTileSize().
    Uint32 ThreadsPerTileY DEFAULT_INITIALIZER(1);

    /// Additional flags, see Diligent::DRAW_FLAGS.
    DRAW_FLAGS Flags DEFAULT_INITIALIZER(DRAW_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr DispatchTileAttribs() noexcept {}

    /// Initializes the structure with user-specified values.
    constexpr DispatchTileAttribs(Uint32     _ThreadsX,
                                  Uint32     _ThreadsY,
                                  DRAW_FLAGS _Flags = DRAW_FLAG_NONE) noexcept :
        ThreadsPerTileX {_ThreadsX},
        ThreadsPerTileY {_ThreadsY},
        Flags           {_Flags  }
    {}
#endif
};
typedef struct DispatchTileAttribs DispatchTileAttribs;


/// Describes multi-sampled texture resolve command arguments.

/// This structure is used by IDeviceContext::ResolveTextureSubresource().
struct ResolveTextureSubresourceAttribs
{
    /// Mip level of the source multi-sampled texture to resolve.
    Uint32 SrcMipLevel   DEFAULT_INITIALIZER(0);

    /// Array slice of the source multi-sampled texture to resolve.
    Uint32 SrcSlice      DEFAULT_INITIALIZER(0);

    /// Source texture state transition mode, see Diligent::RESOURCE_STATE_TRANSITION_MODE.
    RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Mip level of the destination non-multi-sampled texture.
    Uint32 DstMipLevel   DEFAULT_INITIALIZER(0);

    /// Array slice of the destination non-multi-sampled texture.
    Uint32 DstSlice      DEFAULT_INITIALIZER(0);

    /// Destination texture state transition mode, see Diligent::RESOURCE_STATE_TRANSITION_MODE.
    RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// If one or both textures are typeless, specifies the type of the typeless texture.
    /// If both texture formats are not typeless, in which case they must be identical, this member must be
    /// either TEX_FORMAT_UNKNOWN, or match this format.
    TEXTURE_FORMAT Format DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);
};
typedef struct ResolveTextureSubresourceAttribs ResolveTextureSubresourceAttribs;


/// Defines allowed flags for IDeviceContext::SetVertexBuffers() function.
DILIGENT_TYPED_ENUM(SET_VERTEX_BUFFERS_FLAGS, Uint8)
{
    /// No extra operations.
    SET_VERTEX_BUFFERS_FLAG_NONE  = 0x00,

    /// Reset the vertex buffers to only the buffers specified in this
    /// call. All buffers previously bound to the pipeline will be unbound.
    SET_VERTEX_BUFFERS_FLAG_RESET = 0x01
};
DEFINE_FLAG_ENUM_OPERATORS(SET_VERTEX_BUFFERS_FLAGS)


/// Describes the viewport.

/// This structure is used by IDeviceContext::SetViewports().
struct Viewport
{
    /// X coordinate of the left boundary of the viewport.
    Float32 TopLeftX    DEFAULT_INITIALIZER(0.f);

    /// Y coordinate of the top boundary of the viewport.
    /// When defining a viewport, DirectX convention is used:
    /// window coordinate systems originates in the LEFT TOP corner
    /// of the screen with Y axis pointing down.
    Float32 TopLeftY    DEFAULT_INITIALIZER(0.f);

    /// Viewport width.
    Float32 Width       DEFAULT_INITIALIZER(0.f);

    /// Viewport Height.
    Float32 Height      DEFAULT_INITIALIZER(0.f);

    /// Minimum depth of the viewport. Ranges between 0 and 1.
    Float32 MinDepth    DEFAULT_INITIALIZER(0.f);

    /// Maximum depth of the viewport. Ranges between 0 and 1.
    Float32 MaxDepth    DEFAULT_INITIALIZER(1.f);

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure.
    constexpr Viewport(Float32 _TopLeftX,     Float32 _TopLeftY,
                       Float32 _Width,        Float32 _Height,
                       Float32 _MinDepth = 0, Float32 _MaxDepth = 1) noexcept :
        TopLeftX {_TopLeftX},
        TopLeftY {_TopLeftY},
        Width    {_Width   },
        Height   {_Height  },
        MinDepth {_MinDepth},
        MaxDepth {_MaxDepth}
    {}

    constexpr Viewport(Uint32  _Width,        Uint32  _Height,
                       Float32 _MinDepth = 0, Float32 _MaxDepth = 1) noexcept :
        Width   {static_cast<Float32>(_Width) },
        Height  {static_cast<Float32>(_Height)},
        MinDepth{_MinDepth},
        MaxDepth{_MaxDepth}
    {}

    constexpr Viewport(Float32 _Width, Float32 _Height) noexcept :
        Width {_Width },
        Height{_Height}
    {}

    explicit constexpr Viewport(const SwapChainDesc& SCDesc) noexcept :
        Viewport{SCDesc.Width, SCDesc.Height}
    {}

    constexpr bool operator == (const Viewport& vp) const
	{
		return TopLeftX == vp.TopLeftX &&
			   TopLeftY == vp.TopLeftY &&
			   Width    == vp.Width    &&
			   Height   == vp.Height   &&
			   MinDepth == vp.MinDepth &&
			   MaxDepth == vp.MaxDepth;
	}

    constexpr bool operator != (const Viewport& vp) const
    {
        return !(*this == vp);
	}

    constexpr Viewport() noexcept {}
#endif
};
typedef struct Viewport Viewport;


/// Describes the rectangle.

/// This structure is used by IDeviceContext::SetScissorRects().
///
/// \remarks When defining a viewport, Windows convention is used:
///          window coordinate systems originates in the LEFT TOP corner
///          of the screen with Y axis pointing down.
struct Rect
{
    Int32 left   DEFAULT_INITIALIZER(0);  ///< X coordinate of the left boundary of the viewport.
    Int32 top    DEFAULT_INITIALIZER(0);  ///< Y coordinate of the top boundary of the viewport.
    Int32 right  DEFAULT_INITIALIZER(0);  ///< X coordinate of the right boundary of the viewport.
    Int32 bottom DEFAULT_INITIALIZER(0);  ///< Y coordinate of the bottom boundary of the viewport.

#if DILIGENT_CPP_INTERFACE
    /// Initializes the structure
    constexpr Rect(Int32 _left, Int32 _top, Int32 _right, Int32 _bottom) noexcept :
        left   {_left  },
        top    {_top   },
        right  {_right },
        bottom {_bottom}
    {}

    constexpr Rect() noexcept {}

    constexpr bool IsValid() const
    {
        return right > left && bottom > top;
    }

    constexpr bool operator == (const Rect& rc) const
	{
		return left   == rc.left   &&
			   top    == rc.top    &&
			   right  == rc.right  &&
			   bottom == rc.bottom;
	}

    constexpr bool operator != (const Rect& rc) const
	{
		return !(*this == rc);
	}
#endif
};
typedef struct Rect Rect;


/// Defines copy texture command attributes.

/// This structure is used by IDeviceContext::CopyTexture().
struct CopyTextureAttribs
{
    /// Source texture to copy data from.
    ITexture*                      pSrcTexture              DEFAULT_INITIALIZER(nullptr);

    /// Mip level of the source texture to copy data from.
    Uint32                         SrcMipLevel              DEFAULT_INITIALIZER(0);

    /// Array slice of the source texture to copy data from. Must be 0 for non-array textures.
    Uint32                         SrcSlice                 DEFAULT_INITIALIZER(0);

    /// Source region to copy. Use nullptr to copy the entire subresource.
    const Box*                     pSrcBox                  DEFAULT_INITIALIZER(nullptr);

    /// Source texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination texture.
    ITexture*                      pDstTexture              DEFAULT_INITIALIZER(nullptr);

    /// Destination mip level.
    Uint32                         DstMipLevel              DEFAULT_INITIALIZER(0);

    /// Destination array slice. Must be 0 for non-array textures.
    Uint32                         DstSlice                 DEFAULT_INITIALIZER(0);

    /// X offset on the destination subresource.
    Uint32                         DstX                     DEFAULT_INITIALIZER(0);

    /// Y offset on the destination subresource.
    Uint32                         DstY                     DEFAULT_INITIALIZER(0);

    /// Z offset on the destination subresource
    Uint32                         DstZ                     DEFAULT_INITIALIZER(0);

    /// Destination texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);


#if DILIGENT_CPP_INTERFACE
    constexpr CopyTextureAttribs() noexcept {}

    constexpr CopyTextureAttribs(ITexture*                      _pSrcTexture,
                                 RESOURCE_STATE_TRANSITION_MODE _SrcTextureTransitionMode,
                                 ITexture*                      _pDstTexture,
                                 RESOURCE_STATE_TRANSITION_MODE _DstTextureTransitionMode) noexcept :
        pSrcTexture             {_pSrcTexture             },
        SrcTextureTransitionMode{_SrcTextureTransitionMode},
        pDstTexture             {_pDstTexture             },
        DstTextureTransitionMode{_DstTextureTransitionMode}
    {}
#endif
};
typedef struct CopyTextureAttribs CopyTextureAttribs;


/// SetRenderTargetsExt command attributes.

/// This structure is used by IDeviceContext::SetRenderTargetsExt().
struct SetRenderTargetsAttribs
{
    /// Number of render targets to bind.
    Uint32                         NumRenderTargets     DEFAULT_INITIALIZER(0);

    /// Array of pointers to ITextureView that represent the render
    /// targets to bind to the device. The type of each view in the
    /// array must be Diligent::TEXTURE_VIEW_RENDER_TARGET.
    ITextureView**                 ppRenderTargets      DEFAULT_INITIALIZER(nullptr);

    /// Pointer to the ITextureView that represents the depth stencil to
    /// bind to the device. The view type must be
    /// Diligent::TEXTURE_VIEW_DEPTH_STENCIL or Diligent::TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL.
    ITextureView*                  pDepthStencil        DEFAULT_INITIALIZER(nullptr);

    /// Shading rate texture view. Set null to disable variable rate shading.
    ITextureView*                  pShadingRateMap      DEFAULT_INITIALIZER(nullptr);

    /// State transition mode of the render targets, depth stencil buffer
    /// and shading rate map being set (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode  DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr SetRenderTargetsAttribs() noexcept {}

    constexpr SetRenderTargetsAttribs(Uint32                         _NumRenderTargets,
                                      ITextureView*                  _ppRenderTargets[],
                                      ITextureView*                  _pDepthStencil       = nullptr,
                                      RESOURCE_STATE_TRANSITION_MODE _StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE,
                                      ITextureView*                  _pShadingRateMap     = nullptr) noexcept :
        NumRenderTargets   {_NumRenderTargets   },
        ppRenderTargets    {_ppRenderTargets    },
        pDepthStencil      {_pDepthStencil      },
        pShadingRateMap    {_pShadingRateMap    },
        StateTransitionMode{_StateTransitionMode}
    {}
#endif
};
typedef struct SetRenderTargetsAttribs SetRenderTargetsAttribs;


/// BeginRenderPass command attributes.

/// This structure is used by IDeviceContext::BeginRenderPass().
struct BeginRenderPassAttribs
{
    /// Render pass to begin.
    IRenderPass*    pRenderPass     DEFAULT_INITIALIZER(nullptr);

    /// Framebuffer containing the attachments that are used with the render pass.
    IFramebuffer*   pFramebuffer    DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in pClearValues array.
    Uint32 ClearValueCount          DEFAULT_INITIALIZER(0);

    /// A pointer to an array of ClearValueCount OptimizedClearValue structures that contains
    /// clear values for each attachment, if the attachment uses a LoadOp value of ATTACHMENT_LOAD_OP_CLEAR
    /// or if the attachment has a depth/stencil format and uses a StencilLoadOp value of ATTACHMENT_LOAD_OP_CLEAR.
    /// The array is indexed by attachment number. Only elements corresponding to cleared attachments are used.
    /// Other elements of pClearValues are ignored.
    OptimizedClearValue* pClearValues   DEFAULT_INITIALIZER(nullptr);

    /// Framebuffer attachments state transition mode before the render pass begins.

    /// This parameter also indicates how attachment states should be handled when
    /// transitioning between subpasses as well as after the render pass ends.
    /// When RESOURCE_STATE_TRANSITION_MODE_TRANSITION is used, attachment states will be
    /// updated so that they match the state in the current subpass as well as the final states
    /// specified by the render pass when the pass ends.
    /// Note that resources are always transitioned. The flag only indicates if the internal
    /// state variables should be updated.
    /// When RESOURCE_STATE_TRANSITION_MODE_NONE or RESOURCE_STATE_TRANSITION_MODE_VERIFY is used,
    /// internal state variables are not updated and it is the application responsibility to set them
    /// manually to match the actual states.
    RESOURCE_STATE_TRANSITION_MODE StateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);
};
typedef struct BeginRenderPassAttribs BeginRenderPassAttribs;


/// TLAS instance flags that are used in IDeviceContext::BuildTLAS().
DILIGENT_TYPED_ENUM(RAYTRACING_INSTANCE_FLAGS, Uint8)
{
    RAYTRACING_INSTANCE_NONE = 0,

    /// Disables face culling for this instance.
    RAYTRACING_INSTANCE_TRIANGLE_FACING_CULL_DISABLE = 0x01,

    /// Indicates that the front face of the triangle for culling purposes is the face that is counter
    /// clockwise in object space relative to the ray origin. Because the facing is determined in object
    /// space, an instance transform matrix does not change the winding, but a geometry transform does.
    RAYTRACING_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x02,

    /// Causes this instance to act as though RAYTRACING_GEOMETRY_FLAGS_OPAQUE were specified on all
    /// geometries referenced by this instance. This behavior can be overridden in the shader with ray flags.
    RAYTRACING_INSTANCE_FORCE_OPAQUE = 0x04,

    /// Causes this instance to act as though RAYTRACING_GEOMETRY_FLAGS_OPAQUE were not specified on all
    /// geometries referenced by this instance. This behavior can be overridden in the shader with ray flags.
    RAYTRACING_INSTANCE_FORCE_NO_OPAQUE = 0x08,

    RAYTRACING_INSTANCE_FLAG_LAST = RAYTRACING_INSTANCE_FORCE_NO_OPAQUE
};
DEFINE_FLAG_ENUM_OPERATORS(RAYTRACING_INSTANCE_FLAGS)


/// Defines acceleration structure copy mode.

/// These the flags used by IDeviceContext::CopyBLAS() and IDeviceContext::CopyTLAS().
DILIGENT_TYPED_ENUM(COPY_AS_MODE, Uint8)
{
    /// Creates a direct copy of the acceleration structure specified in pSrc into the one specified by pDst.
    /// The pDst acceleration structure must have been created with the same parameters as pSrc.
    COPY_AS_MODE_CLONE = 0,

    /// Creates a more compact version of an acceleration structure pSrc into pDst.
    /// The acceleration structure pDst must have been created with a CompactedSize corresponding
    /// to the one returned by IDeviceContext::WriteBLASCompactedSize() or IDeviceContext::WriteTLASCompactedSize()
    /// after the build of the acceleration structure specified by pSrc.
    COPY_AS_MODE_COMPACT,

    COPY_AS_MODE_LAST = COPY_AS_MODE_COMPACT,
};


/// Defines geometry flags for ray tracing.
DILIGENT_TYPED_ENUM(RAYTRACING_GEOMETRY_FLAGS, Uint8)
{
    RAYTRACING_GEOMETRY_FLAG_NONE = 0,

    /// Indicates that this geometry does not invoke the any-hit shaders even if present in a hit group.
    RAYTRACING_GEOMETRY_FLAG_OPAQUE = 0x01,

    /// Indicates that the implementation must only call the any-hit shader a single time for each primitive in this geometry.
    /// If this bit is absent an implementation may invoke the any-hit shader more than once for this geometry.
    RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION = 0x02,

    RAYTRACING_GEOMETRY_FLAG_LAST = RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION
};
DEFINE_FLAG_ENUM_OPERATORS(RAYTRACING_GEOMETRY_FLAGS)


/// Triangle geometry data description.
struct BLASBuildTriangleData
{
    /// Geometry name used to map a geometry to a hit group in the shader binding table.
    /// Add geometry data to the geometry that is allocated by BLASTriangleDesc with the same name.
    const Char* GeometryName          DEFAULT_INITIALIZER(nullptr);

    /// Triangle vertices data source.
    /// Triangles are considered "inactive" if the x component of each vertex is NaN.
    /// The buffer must be created with BIND_RAY_TRACING flag.
    IBuffer*    pVertexBuffer         DEFAULT_INITIALIZER(nullptr);

    /// Data offset, in bytes, in pVertexBuffer.
    /// D3D12 and Vulkan: offset must be a multiple of the VertexValueType size.
    /// Metal:            stride must be aligned by RayTracingProperties::VertexBufferAlignment
    ///                   and must be a multiple of the VertexStride.
    Uint64      VertexOffset          DEFAULT_INITIALIZER(0);

    /// Stride, in bytes, between vertices.
    /// D3D12 and Vulkan: stride must be a multiple of the VertexValueType size.
    /// Metal:            stride must be aligned by RayTracingProperties::VertexBufferAlignment.
    Uint32      VertexStride          DEFAULT_INITIALIZER(0);

    /// The number of triangle vertices.
    /// Must be less than or equal to BLASTriangleDesc::MaxVertexCount.
    Uint32      VertexCount           DEFAULT_INITIALIZER(0);

    /// The type of the vertex components.
    /// This is an optional value. Must be undefined or same as in BLASTriangleDesc.
    VALUE_TYPE  VertexValueType       DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// The number of vertex components.
    /// This is an optional value. Must be undefined or same as in BLASTriangleDesc.
    Uint8       VertexComponentCount  DEFAULT_INITIALIZER(0);

    /// The number of triangles.
    /// Must equal to VertexCount / 3 if pIndexBuffer is null or must be equal to index count / 3.
    Uint32      PrimitiveCount        DEFAULT_INITIALIZER(0);

    /// Triangle indices data source.
    /// Must be null if BLASTriangleDesc::IndexType is undefined.
    /// The buffer must be created with BIND_RAY_TRACING flag.
    IBuffer*    pIndexBuffer          DEFAULT_INITIALIZER(nullptr);

    /// Data offset in bytes in pIndexBuffer.
    /// Offset must be aligned by RayTracingProperties::IndexBufferAlignment
    /// and must be a multiple of the IndexType size.
    Uint64      IndexOffset           DEFAULT_INITIALIZER(0);

    /// The type of triangle indices, see Diligent::VALUE_TYPE.
    /// This is an optional value. Must be undefined or same as in BLASTriangleDesc.
    VALUE_TYPE  IndexType             DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Geometry transformation data source, must contain a float4x3 matrix aka Diligent::InstanceMatrix.
    /// The buffer must be created with BIND_RAY_TRACING flag.
    /// \note Transform buffer is not supported in Metal backend.
    IBuffer*    pTransformBuffer      DEFAULT_INITIALIZER(nullptr);

    /// Data offset in bytes in pTransformBuffer.
    /// Offset must be aligned by RayTracingProperties::TransformBufferAlignment.
    Uint64      TransformBufferOffset DEFAULT_INITIALIZER(0);

    /// Geometry flags, se Diligent::RAYTRACING_GEOMETRY_FLAGS.
    RAYTRACING_GEOMETRY_FLAGS Flags DEFAULT_INITIALIZER(RAYTRACING_GEOMETRY_FLAG_NONE);
};
typedef struct BLASBuildTriangleData BLASBuildTriangleData;


/// AABB geometry data description.
struct BLASBuildBoundingBoxData
{
    /// Geometry name used to map geometry to hit group in shader binding table.
    /// Put geometry data to geometry that allocated by BLASBoundingBoxDesc with the same name.
    const Char* GeometryName DEFAULT_INITIALIZER(nullptr);

    /// AABB data source.
    /// Each AABB defined as { float3 Min; float3 Max } structure.
    /// AABB are considered inactive if AABB.Min.x is NaN.
    /// Buffer must be created with BIND_RAY_TRACING flag.
    IBuffer*    pBoxBuffer   DEFAULT_INITIALIZER(nullptr);

    /// Data offset in bytes in pBoxBuffer.
    /// D3D12 and Vulkan: offset must be aligned by RayTracingProperties::BoxBufferAlignment.
    /// Metal:            offset must be aligned by RayTracingProperties::BoxBufferAlignment
    ///                   and must be a multiple of the BoxStride.
    Uint64      BoxOffset    DEFAULT_INITIALIZER(0);

    /// Stride in bytes between each AABB.
    /// Stride must be aligned by RayTracingProperties::BoxBufferAlignment.
    Uint32      BoxStride    DEFAULT_INITIALIZER(0);

    /// Number of AABBs.
    /// Must be less than or equal to BLASBoundingBoxDesc::MaxBoxCount.
    Uint32      BoxCount     DEFAULT_INITIALIZER(0);

    /// Geometry flags, see Diligent::RAYTRACING_GEOMETRY_FLAGS.
    RAYTRACING_GEOMETRY_FLAGS Flags DEFAULT_INITIALIZER(RAYTRACING_GEOMETRY_FLAG_NONE);
};
typedef struct BLASBuildBoundingBoxData BLASBuildBoundingBoxData;


/// This structure is used by IDeviceContext::BuildBLAS().
struct BuildBLASAttribs
{
    /// Target bottom-level AS.
    /// Access to the BLAS must be externally synchronized.
    IBottomLevelAS*                 pBLAS                       DEFAULT_INITIALIZER(nullptr);

    /// Bottom-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  BLASTransitionMode          DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Geometry data source buffers state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  GeometryTransitionMode      DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// A pointer to an array of TriangleDataCount BLASBuildTriangleData structures that contains triangle geometry data.
    /// If Update is true:
    ///     - Only vertex positions (in pVertexBuffer) and transformation (in pTransformBuffer) can be changed.
    ///     - All other content in BLASBuildTriangleData and buffers must be the same as what was used to build BLAS.
    ///     - To disable geometry, make all triangles inactive, see BLASBuildTriangleData::pVertexBuffer description.
    BLASBuildTriangleData const*    pTriangleData               DEFAULT_INITIALIZER(nullptr);

    /// The number of triangle geometries.
    /// Must be less than or equal to BottomLevelASDesc::TriangleCount.
    /// If Update is true then the count must be the same as the one used to build BLAS.
    Uint32                          TriangleDataCount           DEFAULT_INITIALIZER(0);

    /// A pointer to an array of BoxDataCount BLASBuildBoundingBoxData structures that contain AABB geometry data.
    /// If Update is true:
    ///     - AABB coordinates (in pBoxBuffer) can be changed.
    ///     - All other content in BLASBuildBoundingBoxData must be same as used to build BLAS.
    ///     - To disable geometry make all AAABBs inactive, see BLASBuildBoundingBoxData::pBoxBuffer description.
    BLASBuildBoundingBoxData const* pBoxData                    DEFAULT_INITIALIZER(nullptr);

    /// The number of AABB geometries.
    /// Must be less than or equal to BottomLevelASDesc::BoxCount.
    /// If Update is true then the count must be the same as the one used to build BLAS.
    Uint32                          BoxDataCount                DEFAULT_INITIALIZER(0);

    /// The buffer that is used for acceleration structure building.
    /// Must be created with BIND_RAY_TRACING.
    /// Call IBottomLevelAS::GetScratchBufferSizes().Build to get the minimal size for the scratch buffer.
    IBuffer*                        pScratchBuffer              DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer.
    /// Offset must be aligned by RayTracingProperties::ScratchBufferAlignment.
    Uint64                          ScratchBufferOffset         DEFAULT_INITIALIZER(0);

    /// Scratch buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  ScratchBufferTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// if false then BLAS will be built from scratch.
    /// If true then previous content of BLAS will be updated.
    /// pBLAS must be created with RAYTRACING_BUILD_AS_ALLOW_UPDATE flag.
    /// An update will be faster than building an acceleration structure from scratch.
    Bool                            Update                      DEFAULT_INITIALIZER(False);
};
typedef struct BuildBLASAttribs BuildBLASAttribs;


/// Can be used to calculate the TLASBuildInstanceData::ContributionToHitGroupIndex depending on instance count,
/// geometry count in each instance (in TLASBuildInstanceData::pBLAS) and shader binding mode in BuildTLASAttribs::BindingMode.
///
/// Example:
///  InstanceOffset = BaseContributionToHitGroupIndex;
///  For each instance in TLAS
///     if (Instance.ContributionToHitGroupIndex == TLAS_INSTANCE_OFFSET_AUTO)
///         Instance.ContributionToHitGroupIndex = InstanceOffset;
///         if (BindingMode == HIT_GROUP_BINDING_MODE_PER_GEOMETRY) InstanceOffset += Instance.pBLAS->GeometryCount() * HitGroupStride;
///         if (BindingMode == HIT_GROUP_BINDING_MODE_PER_INSTANCE) InstanceOffset += HitGroupStride;

#define DILIGENT_TLAS_INSTANCE_OFFSET_AUTO 0xFFFFFFFFU

static DILIGENT_CONSTEXPR Uint32 TLAS_INSTANCE_OFFSET_AUTO = DILIGENT_TLAS_INSTANCE_OFFSET_AUTO;


/// Row-major matrix
struct InstanceMatrix
{
    ///        rotation        translation
    /// ([0,0]  [0,1]  [0,2])   ([0,3])
    /// ([1,0]  [1,1]  [1,2])   ([1,3])
    /// ([2,0]  [2,1]  [2,2])   ([2,3])
    float data [3][4];

#if DILIGENT_CPP_INTERFACE
    /// Construct identity matrix.
    constexpr InstanceMatrix() noexcept :
        data{{1.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 1.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 1.0f, 0.0f}}
    {}

    constexpr InstanceMatrix(const InstanceMatrix&)  noexcept = default;
    constexpr InstanceMatrix(      InstanceMatrix&&) noexcept = default;
    constexpr InstanceMatrix& operator=(const InstanceMatrix&)  noexcept = default;
    constexpr InstanceMatrix& operator=(      InstanceMatrix&&) noexcept = default;

    /// Sets the translation part.
    InstanceMatrix& SetTranslation(float x, float y, float z) noexcept
    {
        data[0][3] = x;
        data[1][3] = y;
        data[2][3] = z;
        return *this;
    }

    /// Sets the rotation part.
    InstanceMatrix& SetRotation(const float* pMatrix, Uint32 RowSize = 3) noexcept
    {
        for (Uint32 r = 0; r < 3; ++r)
        {
            for (Uint32 c = 0; c < 3; ++c)
                data[r][c] = pMatrix[c * RowSize + r];
        }

        return *this;
    }
#endif
};
typedef struct InstanceMatrix InstanceMatrix;


/// This structure is used by BuildTLASAttribs.
struct TLASBuildInstanceData
{
    /// Instance name that is used to map an instance to a hit group in shader binding table.
    const Char*               InstanceName    DEFAULT_INITIALIZER(nullptr);

    /// Bottom-level AS that represents instance geometry.
    /// Once built, TLAS will hold strong reference to pBLAS until next build or copy operation.
    /// Access to the BLAS must be externally synchronized.
    IBottomLevelAS*           pBLAS           DEFAULT_INITIALIZER(nullptr);

    /// Instance to world transformation.
    InstanceMatrix            Transform;

    /// User-defined value that can be accessed in the shader:
    ///   HLSL: `InstanceID()` in closest-hit and intersection shaders.
    ///   HLSL: `RayQuery::CommittedInstanceID()` within inline ray tracing.
    ///   GLSL: `gl_InstanceCustomIndex` in closest-hit and intersection shaders.
    ///   GLSL: `rayQueryGetIntersectionInstanceCustomIndex` within inline ray tracing.
    ///   MSL:  `intersection_result< instancing >::instance_id`.
    /// Only the lower 24 bits are used.
    Uint32                    CustomId        DEFAULT_INITIALIZER(0);

    /// Instance flags, see Diligent::RAYTRACING_INSTANCE_FLAGS.
    RAYTRACING_INSTANCE_FLAGS Flags           DEFAULT_INITIALIZER(RAYTRACING_INSTANCE_NONE);

    /// Visibility mask for the geometry, the instance may only be hit if rayMask & instance.Mask != 0.
    /// ('rayMask' in GLSL is a 'cullMask' argument of traceRay(), 'rayMask' in HLSL is an 'InstanceInclusionMask' argument of TraceRay()).
    Uint8                     Mask            DEFAULT_INITIALIZER(0xFF);

    /// The index used to calculate the hit group location in the shader binding table.
    /// Must be TLAS_INSTANCE_OFFSET_AUTO if BuildTLASAttribs::BindingMode is not SHADER_BINDING_USER_DEFINED.
    /// Only the lower 24 bits are used.
    Uint32                    ContributionToHitGroupIndex DEFAULT_INITIALIZER(TLAS_INSTANCE_OFFSET_AUTO);
};
typedef struct TLASBuildInstanceData TLASBuildInstanceData;


/// Top-level AS instance size in bytes in GPU side.
/// Used to calculate size of BuildTLASAttribs::pInstanceBuffer.

#define DILIGENT_TLAS_INSTANCE_DATA_SIZE 64

static DILIGENT_CONSTEXPR Uint32 TLAS_INSTANCE_DATA_SIZE = DILIGENT_TLAS_INSTANCE_DATA_SIZE;


/// This structure is used by IDeviceContext::BuildTLAS().
struct BuildTLASAttribs
{
    /// Target top-level AS.
    /// Access to the TLAS must be externally synchronized.
    ITopLevelAS*                    pTLAS                         DEFAULT_INITIALIZER(nullptr);

    /// Top-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  TLASTransitionMode            DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Bottom-level AS (in TLASBuildInstanceData::pBLAS) state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  BLASTransitionMode            DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// A pointer to an array of InstanceCount TLASBuildInstanceData structures that contain instance data.
    /// If Update is true:
    ///     - Any instance data can be changed.
    ///     - To disable an instance set TLASBuildInstanceData::Mask to zero or set empty TLASBuildInstanceData::BLAS to pBLAS.
    TLASBuildInstanceData const*    pInstances                    DEFAULT_INITIALIZER(nullptr);

    /// The number of instances.
    /// Must be less than or equal to TopLevelASDesc::MaxInstanceCount.
    /// If Update is true then count must be the same as used to build TLAS.
    Uint32                          InstanceCount                 DEFAULT_INITIALIZER(0);

    /// The buffer that will be used to store instance data during AS building.
    /// The buffer size must be at least TLAS_INSTANCE_DATA_SIZE * InstanceCount.
    /// The buffer must be created with BIND_RAY_TRACING flag.
    IBuffer*                        pInstanceBuffer               DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer to the location of instance data.
    /// Offset must be aligned by RayTracingProperties::InstanceBufferAlignment.
    Uint64                          InstanceBufferOffset          DEFAULT_INITIALIZER(0);

    /// Instance buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  InstanceBufferTransitionMode  DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// The number of hit shaders that can be bound for a single geometry or an instance (depends on BindingMode).
    ///   - Used to calculate TLASBuildInstanceData::ContributionToHitGroupIndex.
    ///   - Ignored if BindingMode is SHADER_BINDING_USER_DEFINED.
    /// You should use the same value in a shader:
    /// 'MultiplierForGeometryContributionToHitGroupIndex' argument in TraceRay() in HLSL, 'sbtRecordStride' argument in traceRay() in GLSL.
    Uint32                          HitGroupStride         DEFAULT_INITIALIZER(1);

    /// Base offset for the hit group location.
    /// Can be used to bind hit shaders for multiple acceleration structures, see IShaderBindingTable::BindHitGroupForGeometry().
    ///   - Used to calculate TLASBuildInstanceData::ContributionToHitGroupIndex.
    ///   - Ignored if BindingMode is SHADER_BINDING_USER_DEFINED.
    Uint32                          BaseContributionToHitGroupIndex DEFAULT_INITIALIZER(0);

    /// Hit shader binding mode, see Diligent::SHADER_BINDING_MODE.
    /// Used to calculate TLASBuildInstanceData::ContributionToHitGroupIndex.
    HIT_GROUP_BINDING_MODE          BindingMode                   DEFAULT_INITIALIZER(HIT_GROUP_BINDING_MODE_PER_GEOMETRY);

    /// Buffer that is used for acceleration structure building.
    /// Must be created with BIND_RAY_TRACING.
    /// Call ITopLevelAS::GetScratchBufferSizes().Build to get the minimal size for the scratch buffer.
    /// Access to the TLAS must be externally synchronized.
    IBuffer*                        pScratchBuffer                DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer.
    /// Offset must be aligned by RayTracingProperties::ScratchBufferAlignment.
    Uint64                          ScratchBufferOffset           DEFAULT_INITIALIZER(0);

    /// Scratch buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE  ScratchBufferTransitionMode   DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// if false then TLAS will be built from scratch.
    /// If true then previous content of TLAS will be updated.
    /// pTLAS must be created with RAYTRACING_BUILD_AS_ALLOW_UPDATE flag.
    /// An update will be faster than building an acceleration structure from scratch.
    Bool                            Update                        DEFAULT_INITIALIZER(False);
};
typedef struct BuildTLASAttribs BuildTLASAttribs;


/// This structure is used by IDeviceContext::CopyBLAS().
struct CopyBLASAttribs
{
    /// Source bottom-level AS.
    /// Access to the BLAS must be externally synchronized.
    IBottomLevelAS*                pSrc              DEFAULT_INITIALIZER(nullptr);

    /// Destination bottom-level AS.
    /// If Mode is COPY_AS_MODE_COMPACT then pDst must be created with CompactedSize
    /// that is greater or equal to the size returned by IDeviceContext::WriteBLASCompactedSize.
    /// Access to the BLAS must be externally synchronized.
    IBottomLevelAS*                pDst              DEFAULT_INITIALIZER(nullptr);

    /// Acceleration structure copy mode, see Diligent::COPY_AS_MODE.
    COPY_AS_MODE                   Mode              DEFAULT_INITIALIZER(COPY_AS_MODE_CLONE);

    /// Source bottom-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE SrcTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination bottom-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE DstTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr CopyBLASAttribs() noexcept {}

    constexpr CopyBLASAttribs(IBottomLevelAS*                _pSrc,
                              IBottomLevelAS*                _pDst,
                              COPY_AS_MODE                   _Mode              = CopyBLASAttribs{}.Mode,
                              RESOURCE_STATE_TRANSITION_MODE _SrcTransitionMode = CopyBLASAttribs{}.SrcTransitionMode,
                              RESOURCE_STATE_TRANSITION_MODE _DstTransitionMode = CopyBLASAttribs{}.DstTransitionMode) noexcept :
        pSrc             {_pSrc             },
        pDst             {_pDst             },
        Mode             {_Mode             },
        SrcTransitionMode{_SrcTransitionMode},
        DstTransitionMode{_DstTransitionMode}
    {
    }
#endif
};
typedef struct CopyBLASAttribs CopyBLASAttribs;


/// This structure is used by IDeviceContext::CopyTLAS().
struct CopyTLASAttribs
{
    /// Source top-level AS.
    /// Access to the TLAS must be externally synchronized.
    ITopLevelAS*                   pSrc              DEFAULT_INITIALIZER(nullptr);

    /// Destination top-level AS.
    /// If Mode is COPY_AS_MODE_COMPACT then pDst must be created with CompactedSize
    /// that is greater or equal to size that returned by IDeviceContext::WriteTLASCompactedSize.
    /// Access to the TLAS must be externally synchronized.
    ITopLevelAS*                   pDst              DEFAULT_INITIALIZER(nullptr);

    /// Acceleration structure copy mode, see Diligent::COPY_AS_MODE.
    COPY_AS_MODE                   Mode              DEFAULT_INITIALIZER(COPY_AS_MODE_CLONE);

    /// Source top-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE SrcTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination top-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE DstTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr CopyTLASAttribs() noexcept {}

    constexpr CopyTLASAttribs(ITopLevelAS*                   _pSrc,
                              ITopLevelAS*                   _pDst,
                              COPY_AS_MODE                   _Mode              = CopyTLASAttribs{}.Mode,
                              RESOURCE_STATE_TRANSITION_MODE _SrcTransitionMode = CopyTLASAttribs{}.SrcTransitionMode,
                              RESOURCE_STATE_TRANSITION_MODE _DstTransitionMode = CopyTLASAttribs{}.DstTransitionMode) noexcept :
        pSrc             {_pSrc             },
        pDst             {_pDst             },
        Mode             {_Mode             },
        SrcTransitionMode{_SrcTransitionMode},
        DstTransitionMode{_DstTransitionMode}
    {
    }
#endif
};
typedef struct CopyTLASAttribs CopyTLASAttribs;


/// This structure is used by IDeviceContext::WriteBLASCompactedSize().
struct WriteBLASCompactedSizeAttribs
{
    /// Bottom-level AS.
    IBottomLevelAS*                pBLAS                DEFAULT_INITIALIZER(nullptr);

    /// The destination buffer into which a 64-bit value representing the acceleration structure compacted size will be written to.
    /// \remarks  Metal backend writes a 32-bit value.
    IBuffer*                       pDestBuffer          DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer to the location of the AS compacted size.
    Uint64                         DestBufferOffset     DEFAULT_INITIALIZER(0);

    /// Bottom-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE BLASTransitionMode   DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr WriteBLASCompactedSizeAttribs() noexcept {}

    constexpr WriteBLASCompactedSizeAttribs(IBottomLevelAS*                _pBLAS,
                                            IBuffer*                       _pDestBuffer,
                                            Uint64                         _DestBufferOffset     = WriteBLASCompactedSizeAttribs{}.DestBufferOffset,
                                            RESOURCE_STATE_TRANSITION_MODE _BLASTransitionMode   = WriteBLASCompactedSizeAttribs{}.BLASTransitionMode,
                                            RESOURCE_STATE_TRANSITION_MODE _BufferTransitionMode = WriteBLASCompactedSizeAttribs{}.BufferTransitionMode) noexcept :
        pBLAS               {_pBLAS               },
        pDestBuffer         {_pDestBuffer         },
        DestBufferOffset    {_DestBufferOffset    },
        BLASTransitionMode  {_BLASTransitionMode  },
        BufferTransitionMode{_BufferTransitionMode}
    {}
#endif
};
typedef struct WriteBLASCompactedSizeAttribs WriteBLASCompactedSizeAttribs;


/// This structure is used by IDeviceContext::WriteTLASCompactedSize().
struct WriteTLASCompactedSizeAttribs
{
    /// Top-level AS.
    ITopLevelAS*                   pTLAS                DEFAULT_INITIALIZER(nullptr);

    /// The destination buffer into which a 64-bit value representing the acceleration structure compacted size will be written to.
    /// \remarks  Metal backend writes a 32-bit value.
    IBuffer*                       pDestBuffer          DEFAULT_INITIALIZER(nullptr);

    /// Offset from the beginning of the buffer to the location of the AS compacted size.
    Uint64                         DestBufferOffset     DEFAULT_INITIALIZER(0);

    /// Top-level AS state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE TLASTransitionMode   DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// Destination buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr WriteTLASCompactedSizeAttribs() noexcept {}

    constexpr WriteTLASCompactedSizeAttribs(ITopLevelAS*                   _pTLAS,
                                            IBuffer*                       _pDestBuffer,
                                            Uint64                         _DestBufferOffset     = WriteTLASCompactedSizeAttribs{}.DestBufferOffset,
                                            RESOURCE_STATE_TRANSITION_MODE _TLASTransitionMode   = WriteTLASCompactedSizeAttribs{}.TLASTransitionMode,
                                            RESOURCE_STATE_TRANSITION_MODE _BufferTransitionMode = WriteTLASCompactedSizeAttribs{}.BufferTransitionMode) noexcept :
        pTLAS               {_pTLAS               },
        pDestBuffer         {_pDestBuffer         },
        DestBufferOffset    {_DestBufferOffset    },
        TLASTransitionMode  {_TLASTransitionMode  },
        BufferTransitionMode{_BufferTransitionMode}
    {}
#endif
};
typedef struct WriteTLASCompactedSizeAttribs WriteTLASCompactedSizeAttribs;


/// This structure is used by IDeviceContext::TraceRays().
struct TraceRaysAttribs
{
    /// Shader binding table.
    const IShaderBindingTable* pSBT  DEFAULT_INITIALIZER(nullptr);

    Uint32               DimensionX  DEFAULT_INITIALIZER(1); ///< The number of rays dispatched in X direction.
    Uint32               DimensionY  DEFAULT_INITIALIZER(1); ///< The number of rays dispatched in Y direction.
    Uint32               DimensionZ  DEFAULT_INITIALIZER(1); ///< The number of rays dispatched in Z direction.

#if DILIGENT_CPP_INTERFACE
    constexpr TraceRaysAttribs() noexcept {}

    constexpr TraceRaysAttribs(const IShaderBindingTable* _pSBT,
                               Uint32                     _DimensionX,
                               Uint32                     _DimensionY,
                               Uint32                     _DimensionZ = TraceRaysAttribs{}.DimensionZ) noexcept :
        pSBT      {_pSBT      },
        DimensionX{_DimensionX},
        DimensionY{_DimensionY},
        DimensionZ{_DimensionZ}
    {}
#endif
};
typedef struct TraceRaysAttribs TraceRaysAttribs;


/// This structure is used by IDeviceContext::TraceRaysIndirect().
struct TraceRaysIndirectAttribs
{
    /// Shader binding table.
    const IShaderBindingTable* pSBT  DEFAULT_INITIALIZER(nullptr);

    /// A pointer to the buffer containing indirect trace rays attributes.
    /// The buffer must contain the following arguments at the specified offset:
    ///    [88 bytes reserved] - for Direct3D12 backend
    ///    Uint32 DimensionX;
    ///    Uint32 DimensionY;
    ///    Uint32 DimensionZ;
    ///
    /// \remarks  Use IDeviceContext::UpdateSBT() to initialize the first 88 bytes with the
    ///           same shader binding table as specified in TraceRaysIndirectAttribs::pSBT.
    IBuffer*                       pAttribsBuffer                   DEFAULT_INITIALIZER(nullptr);

    /// State transition mode for indirect trace rays attributes buffer.
    RESOURCE_STATE_TRANSITION_MODE AttribsBufferStateTransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

    /// The offset from the beginning of the buffer to the trace rays command arguments.
    Uint64  ArgsByteOffset  DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    constexpr TraceRaysIndirectAttribs() noexcept {}

    constexpr TraceRaysIndirectAttribs(
        const IShaderBindingTable*     _pSBT,
        IBuffer*                       _pAttribsBuffer,
        RESOURCE_STATE_TRANSITION_MODE _TransitionMode = TraceRaysIndirectAttribs{}.AttribsBufferStateTransitionMode,
        Uint64                         _ArgsByteOffset = TraceRaysIndirectAttribs{}.ArgsByteOffset) noexcept :
        pSBT                            {_pSBT},
        pAttribsBuffer                  {_pAttribsBuffer},
        AttribsBufferStateTransitionMode{_TransitionMode},
        ArgsByteOffset                  {_ArgsByteOffset}
    {}
#endif
};
typedef struct TraceRaysIndirectAttribs TraceRaysIndirectAttribs;


/// This structure is used by IDeviceContext::UpdateSBT().
struct UpdateIndirectRTBufferAttribs
{
    /// Indirect buffer that can be used by IDeviceContext::TraceRaysIndirect() command.
    IBuffer* pAttribsBuffer DEFAULT_INITIALIZER(nullptr);

    /// Offset in bytes from the beginning of the buffer where SBT data will be recorded.
    Uint64 AttribsBufferOffset DEFAULT_INITIALIZER(0);

    /// State transition mode of the attribs buffer (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    RESOURCE_STATE_TRANSITION_MODE TransitionMode DEFAULT_INITIALIZER(RESOURCE_STATE_TRANSITION_MODE_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr UpdateIndirectRTBufferAttribs() noexcept {}

    explicit constexpr UpdateIndirectRTBufferAttribs(
        IBuffer*                       _pAttribsBuffer,
        Uint64                         _AttribsBufferOffset = UpdateIndirectRTBufferAttribs{}.AttribsBufferOffset,
        RESOURCE_STATE_TRANSITION_MODE _TransitionMode      = UpdateIndirectRTBufferAttribs{}.TransitionMode) noexcept :
        pAttribsBuffer     {_pAttribsBuffer     },
        AttribsBufferOffset{_AttribsBufferOffset},
        TransitionMode     {_TransitionMode     }
    {}
#endif
};
typedef struct UpdateIndirectRTBufferAttribs UpdateIndirectRTBufferAttribs;


/// Defines the sparse buffer memory binding range.
/// This structure is used by SparseBufferMemoryBindInfo.
struct SparseBufferMemoryBindRange
{
    /// Offset in buffer address space where memory will be bound/unbound.
    /// Must be a multiple of the SparseBufferProperties::BlockSize.
    Uint64           BufferOffset  DEFAULT_INITIALIZER(0);

    /// Memory range offset in pMemory.
    /// Must be a multiple of the SparseBufferProperties::BlockSize.
    Uint64           MemoryOffset  DEFAULT_INITIALIZER(0);

    /// Size of the memory which will be bound/unbound.
    /// Must be a multiple of the SparseBufferProperties::BlockSize.
    Uint64           MemorySize    DEFAULT_INITIALIZER(0);

    /// Pointer to the memory object.
    /// If non-null, the memory will be bound to the Region; otherwise the memory will be unbound.
    ///
    /// \remarks
    ///     Direct3D11: the entire buffer must use a single memory object. When a resource is bound to a new memory object,
    ///                 all previous bindings are invalidated.
    ///     Vulkan & Direct3D12: different resource regions may be bound to different memory objects.
    ///     Vulkan: memory object must be compatible with the resource, use IDeviceMemory::IsCompatible() to ensure that.
    ///
    /// \note  Memory object can be created by the engine using IRenderDevice::CreateDeviceMemory() or can be implemented by the user.
    ///        Memory object must implement interface methods for each backend (IDeviceMemoryD3D11, IDeviceMemoryD3D12, IDeviceMemoryVk).
    IDeviceMemory*   pMemory       DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    constexpr SparseBufferMemoryBindRange() noexcept {}

    constexpr SparseBufferMemoryBindRange(Uint64         _BufferOffset,
                                          Uint64         _MemoryOffset,
                                          Uint64         _MemorySize,
                                          IDeviceMemory* _pMemory) noexcept :
        BufferOffset{_BufferOffset},
        MemoryOffset{_MemoryOffset},
        MemorySize  {_MemorySize  },
        pMemory     {_pMemory     }
    {}
#endif
};
typedef struct SparseBufferMemoryBindRange SparseBufferMemoryBindRange;

/// Defines the sparse buffer memory binding information.
/// This structure is used by BindSparseResourceMemoryAttribs.
struct SparseBufferMemoryBindInfo
{
    /// Buffer for which sparse binding command will be executed.
    IBuffer*                           pBuffer    DEFAULT_INITIALIZER(nullptr);

    /// An array of NumRanges buffer memory ranges to bind/unbind,
    /// see Diligent::SparseBufferMemoryBindRange.
    const SparseBufferMemoryBindRange* pRanges    DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in pRanges array.
    Uint32                             NumRanges  DEFAULT_INITIALIZER(0);
};
typedef struct SparseBufferMemoryBindInfo SparseBufferMemoryBindInfo;

/// Defines the sparse texture memory binding range.
/// This structure is used by SparseTextureMemoryBind.
struct SparseTextureMemoryBindRange
{
    /// Mip level that contains the region to bind.
    ///
    /// \note If this level is equal to SparseTextureProperties::FirstMipInTail,
    ///       all subsequent mip levels will also be affected.
    Uint32           MipLevel      DEFAULT_INITIALIZER(0);

    /// Texture array slice index.
    Uint32           ArraySlice    DEFAULT_INITIALIZER(0);

    /// Region in pixels where to bind/unbind memory.
    /// Must be a multiple of SparseTextureProperties::TileSize.
    /// If MipLevel is equal to SparseTextureProperties::FirstMipInTail, this field is ignored
    /// and OffsetInMipTail is used instead.
    /// If Region contains multiple tiles, they are bound in the row-major order.
    Box              Region        DEFAULT_INITIALIZER({});

    /// When mip tail consists of multiple memory blocks, this member
    /// defines the starting offset to bind/unbind memory in the tail.
    /// If MipLevel is less than SparseTextureProperties::FirstMipInTail,
    /// this field is ignored and Region is used.
    Uint64           OffsetInMipTail DEFAULT_INITIALIZER(0);

    /// Size of the memory that will be bound/unbound to this Region.
    /// Memory size must be equal to the number of tiles in Region multiplied by the
    /// sparse memory block size.
    /// It must be a multiple of the SparseTextureProperties::BlockSize.
    /// Ignored in Metal.
    Uint64           MemorySize    DEFAULT_INITIALIZER(0);

    /// Memory range offset in the pMemory.
    /// Must be a multiple of the SparseTextureProperties::BlockSize.
    /// Ignored in Metal.
    Uint64           MemoryOffset  DEFAULT_INITIALIZER(0);

    /// Pointer to the memory object.
    /// If non-null, the memory will be bound to Region; otherwise the memory will be unbound.
    ///
    /// \remarks
    ///     Direct3D11: the entire texture must use a single memory object; when a resource is bound to a new memory
    ///                 object, all previous bindings are invalidated.
    ///     Vulkan & Direct3D12: different resource regions may be bound to different memory objects.
    ///     Metal: must be the same memory object that was used to create the sparse texture,
    ///            see IRenderDeviceMtl::CreateSparseTexture().
    ///     Vulkan: memory object must be compatible with the resource, use IDeviceMemory::IsCompatible() to ensure that.
    ///
    /// \note  Memory object can be created by the engine using IRenderDevice::CreateDeviceMemory() or can be implemented by the user.
    ///        Memory object must implement interface methods for each backend (IDeviceMemoryD3D11, IDeviceMemoryD3D12, IDeviceMemoryVk).
    IDeviceMemory*   pMemory       DEFAULT_INITIALIZER(nullptr);
};
typedef struct SparseTextureMemoryBindRange SparseTextureMemoryBindRange;

/// Sparse texture memory binding information.
/// This structure is used by BindSparseResourceMemoryAttribs.
struct SparseTextureMemoryBindInfo
{
    /// Texture for which sparse binding command will be executed.
    ITexture*                           pTexture   DEFAULT_INITIALIZER(nullptr);

    /// An array of NumRanges texture memory ranges to bind/unbind, see Diligent::SparseTextureMemoryBindRange.
    const SparseTextureMemoryBindRange* pRanges    DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in the pRanges array.
    Uint32                              NumRanges  DEFAULT_INITIALIZER(0);
};
typedef struct SparseTextureMemoryBindInfo SparseTextureMemoryBindInfo;

/// Attributes of the IDeviceContext::BindSparseResourceMemory() command.
struct BindSparseResourceMemoryAttribs
{
    /// An array of NumBufferBinds sparse buffer bind commands.
    /// All commands must bind/unbind unique range in the buffer.
    /// Not supported in Metal.
    const SparseBufferMemoryBindInfo*  pBufferBinds   DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in the pBufferBinds array.
    Uint32                             NumBufferBinds DEFAULT_INITIALIZER(0);

    /// An array of NumTextureBinds sparse texture bind commands.
    /// All commands must bind/unbind unique region in the texture.
    const SparseTextureMemoryBindInfo* pTextureBinds   DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in the pTextureBinds.
    Uint32                             NumTextureBinds DEFAULT_INITIALIZER(0);

    /// An array of NumWaitFences fences to wait.

    /// \remarks The context will wait until all fences have reached the values
    ///          specified in pWaitFenceValues.
    IFence**      ppWaitFences       DEFAULT_INITIALIZER(nullptr);

    /// An array of NumWaitFences values that the context should wait for the
    /// fences to reach.
    const Uint64* pWaitFenceValues   DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in the ppWaitFences and pWaitFenceValues arrays.
    Uint32        NumWaitFences      DEFAULT_INITIALIZER(0);

    /// An array of NumSignalFences fences to signal.
    IFence**      ppSignalFences     DEFAULT_INITIALIZER(nullptr);

    /// An array of NumSignalFences values to set the fences to.
    const Uint64* pSignalFenceValues DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in the ppSignalFences and pSignalFenceValues arrays.
    Uint32        NumSignalFences    DEFAULT_INITIALIZER(0);
};
typedef struct BindSparseResourceMemoryAttribs BindSparseResourceMemoryAttribs;

/// Special constant for all remaining mipmap levels.
#define DILIGENT_REMAINING_MIP_LEVELS 0xFFFFFFFFU

/// Special constant for all remaining array slices.
#define DILIGENT_REMAINING_ARRAY_SLICES 0xFFFFFFFFU

static DILIGENT_CONSTEXPR Uint32 REMAINING_MIP_LEVELS   = DILIGENT_REMAINING_MIP_LEVELS;
static DILIGENT_CONSTEXPR Uint32 REMAINING_ARRAY_SLICES = DILIGENT_REMAINING_ARRAY_SLICES;

/// Resource state transition flags.
DILIGENT_TYPED_ENUM(STATE_TRANSITION_FLAGS, Uint8)
{
    /// No flags.
    STATE_TRANSITION_FLAG_NONE            = 0,

    /// Indicates that the internal resource state should be updated to the new state
    /// specified by StateTransitionDesc, and the engine should take over the resource state
    /// management. If an application was managing the resource state manually, it is
    /// responsible for making sure that all subresources are indeed in the designated state.
    /// If not used, internal resource state will be unchanged.
    ///
    /// \note This flag cannot be used when StateTransitionDesc.TransitionType is STATE_TRANSITION_TYPE_BEGIN.
    STATE_TRANSITION_FLAG_UPDATE_STATE    = 1u << 0,

    /// If set, the contents of the resource will be discarded, when possible.
    /// This may avoid potentially expensive operations such as render target decompression
    /// or a pipeline stall when transitioning to COMMON or UAV state.
    STATE_TRANSITION_FLAG_DISCARD_CONTENT = 1u << 1,

    /// Indicates state transition between aliased resources that share the same memory.
    /// Currently it is only supported for sparse resources that were created with aliasing flag.
    STATE_TRANSITION_FLAG_ALIASING        = 1u << 2
};
DEFINE_FLAG_ENUM_OPERATORS(STATE_TRANSITION_FLAGS);


/// Resource state transition barrier description
struct StateTransitionDesc
{
    /// Previous resource for aliasing transition.
    ///
    /// This member is only used for aliasing transition (STATE_TRANSITION_FLAG_ALIASING flag is set),
    /// and ignored otherwise, and must point to a texture or a buffer object.
    ///
    /// \note pResourceBefore may be null, which indicates that any sparse or
    ///       normal resource could cause aliasing.
    IDeviceObject* pResourceBefore DEFAULT_INITIALIZER(nullptr);

    /// Resource to transition.
    /// Can be ITexture, IBuffer, IBottomLevelAS, ITopLevelAS.
    ///
    /// \note For aliasing transition (STATE_TRANSITION_FLAG_ALIASING flag is set),
    ///       pResource may be null, which indicates that any sparse or
    ///       normal resource could cause aliasing.
    IDeviceObject* pResource       DEFAULT_INITIALIZER(nullptr);

    /// When transitioning a texture, first mip level of the subresource range to transition.
    Uint32 FirstMipLevel     DEFAULT_INITIALIZER(0);

    /// When transitioning a texture, number of mip levels of the subresource range to transition.
    Uint32 MipLevelsCount    DEFAULT_INITIALIZER(REMAINING_MIP_LEVELS);

    /// When transitioning a texture, first array slice of the subresource range to transition.
    Uint32 FirstArraySlice   DEFAULT_INITIALIZER(0);

    /// When transitioning a texture, number of array slices of the subresource range to transition.
    Uint32 ArraySliceCount   DEFAULT_INITIALIZER(REMAINING_ARRAY_SLICES);

    /// Resource state before transition.
    /// If this value is RESOURCE_STATE_UNKNOWN, internal resource state will be used, which must be defined in this case.
    ///
    /// \note  Resource state must be compatible with the context type.
    RESOURCE_STATE OldState  DEFAULT_INITIALIZER(RESOURCE_STATE_UNKNOWN);

    /// Resource state after transition.
    /// Must not be RESOURCE_STATE_UNKNOWN or RESOURCE_STATE_UNDEFINED.
    ///
    /// \note  Resource state must be compatible with the context type.
    RESOURCE_STATE NewState  DEFAULT_INITIALIZER(RESOURCE_STATE_UNKNOWN);

    /// State transition type, see Diligent::STATE_TRANSITION_TYPE.

    /// \note When issuing UAV barrier (i.e. OldState and NewState equal RESOURCE_STATE_UNORDERED_ACCESS),
    ///       TransitionType must be STATE_TRANSITION_TYPE_IMMEDIATE.
    STATE_TRANSITION_TYPE TransitionType DEFAULT_INITIALIZER(STATE_TRANSITION_TYPE_IMMEDIATE);

    /// State transition flags, see Diligent::STATE_TRANSITION_FLAGS.
    STATE_TRANSITION_FLAGS Flags DEFAULT_INITIALIZER(STATE_TRANSITION_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr  StateTransitionDesc() noexcept {}

    constexpr StateTransitionDesc(ITexture*              _pTexture,
                                  RESOURCE_STATE         _OldState,
                                  RESOURCE_STATE         _NewState,
                                  Uint32                 _FirstMipLevel   = 0,
                                  Uint32                 _MipLevelsCount  = REMAINING_MIP_LEVELS,
                                  Uint32                 _FirstArraySlice = 0,
                                  Uint32                 _ArraySliceCount = REMAINING_ARRAY_SLICES,
                                  STATE_TRANSITION_TYPE  _TransitionType  = STATE_TRANSITION_TYPE_IMMEDIATE,
                                  STATE_TRANSITION_FLAGS _Flags           = STATE_TRANSITION_FLAG_NONE) noexcept :
        pResource       {static_cast<IDeviceObject*>(_pTexture)},
        FirstMipLevel   {_FirstMipLevel  },
        MipLevelsCount  {_MipLevelsCount },
        FirstArraySlice {_FirstArraySlice},
        ArraySliceCount {_ArraySliceCount},
        OldState        {_OldState       },
        NewState        {_NewState       },
        TransitionType  {_TransitionType },
        Flags           {_Flags          }
    {}

    constexpr StateTransitionDesc(ITexture*              _pTexture,
                                  RESOURCE_STATE         _OldState,
                                  RESOURCE_STATE         _NewState,
                                  STATE_TRANSITION_FLAGS _Flags) noexcept :
        StateTransitionDesc
        {
            _pTexture,
            _OldState,
            _NewState,
            0,
            REMAINING_MIP_LEVELS,
            0,
            REMAINING_ARRAY_SLICES,
            STATE_TRANSITION_TYPE_IMMEDIATE,
            _Flags
        }
    {}

    constexpr StateTransitionDesc(IBuffer*               _pBuffer,
                                  RESOURCE_STATE         _OldState,
                                  RESOURCE_STATE         _NewState,
                                  STATE_TRANSITION_FLAGS _Flags = STATE_TRANSITION_FLAG_NONE) noexcept :
        pResource {_pBuffer },
        OldState  {_OldState},
        NewState  {_NewState},
        Flags     {_Flags   }
    {}

    constexpr StateTransitionDesc(IBottomLevelAS*        _pBLAS,
                                  RESOURCE_STATE         _OldState,
                                  RESOURCE_STATE         _NewState,
                                  STATE_TRANSITION_FLAGS _Flags = STATE_TRANSITION_FLAG_NONE) noexcept :
        pResource {_pBLAS   },
        OldState  {_OldState},
        NewState  {_NewState},
        Flags     {_Flags   }
    {}

    constexpr StateTransitionDesc(ITopLevelAS*           _pTLAS,
                                  RESOURCE_STATE         _OldState,
                                  RESOURCE_STATE         _NewState,
                                  STATE_TRANSITION_FLAGS _Flags = STATE_TRANSITION_FLAG_NONE) noexcept :
        pResource {_pTLAS   },
        OldState  {_OldState},
        NewState  {_NewState},
        Flags     {_Flags   }
    {}

    /// Aliasing barrier
    constexpr StateTransitionDesc(IDeviceObject* _pResourceBefore,
                                  IDeviceObject* _pResourceAfter) noexcept :
        pResourceBefore{_pResourceBefore},
        pResource      {_pResourceAfter},
        Flags          {STATE_TRANSITION_FLAG_ALIASING}
    {}
#endif
};
typedef struct StateTransitionDesc StateTransitionDesc;

/// Device context command counters.
struct DeviceContextCommandCounters
{
    /// The total number of SetPipelineState calls.
    Uint32 SetPipelineState DEFAULT_INITIALIZER(0);

    /// The total number of CommitShaderResources calls.
    Uint32 CommitShaderResources DEFAULT_INITIALIZER(0);

    /// The total number of SetVertexBuffers calls.
    Uint32 SetVertexBuffers DEFAULT_INITIALIZER(0);

    /// The total number of SetIndexBuffer calls.
    Uint32 SetIndexBuffer DEFAULT_INITIALIZER(0);

    /// The total number of SetRenderTargets calls.
    Uint32 SetRenderTargets DEFAULT_INITIALIZER(0);

    /// The total number of SetBlendFactors calls.
    Uint32 SetBlendFactors DEFAULT_INITIALIZER(0);

    /// The total number of SetStencilRef calls.
    Uint32 SetStencilRef DEFAULT_INITIALIZER(0);

    /// The total number of SetViewports calls.
    Uint32 SetViewports DEFAULT_INITIALIZER(0);

    /// The total number of SetScissorRects calls.
    Uint32 SetScissorRects DEFAULT_INITIALIZER(0);

    /// The total number of ClearRenderTarget calls.
    Uint32 ClearRenderTarget DEFAULT_INITIALIZER(0);

    /// The total number of ClearDepthStencil calls.
    Uint32 ClearDepthStencil DEFAULT_INITIALIZER(0);

    /// The total number of Draw calls.
    Uint32 Draw DEFAULT_INITIALIZER(0);

    /// The total number of DrawIndexed calls.
    Uint32 DrawIndexed DEFAULT_INITIALIZER(0);

    /// The total number of indirect DrawIndirect calls.
    Uint32 DrawIndirect DEFAULT_INITIALIZER(0);

    /// The total number of indexed indirect DrawIndexedIndirect calls.
    Uint32 DrawIndexedIndirect DEFAULT_INITIALIZER(0);

    /// The total number of MultiDraw calls.
    Uint32 MultiDraw DEFAULT_INITIALIZER(0);

    /// The total number of MultiDrawIndexed calls.
    Uint32 MultiDrawIndexed DEFAULT_INITIALIZER(0);

    /// The total number of DispatchCompute calls.
    Uint32 DispatchCompute DEFAULT_INITIALIZER(0);

    /// The total number of DispatchComputeIndirect calls.
    Uint32 DispatchComputeIndirect DEFAULT_INITIALIZER(0);

    /// The total number of DispatchTile calls.
    Uint32 DispatchTile DEFAULT_INITIALIZER(0);

    /// The total number of DrawMesh calls.
    Uint32 DrawMesh DEFAULT_INITIALIZER(0);

    /// The total number of DrawMeshIndirect calls.
    Uint32 DrawMeshIndirect DEFAULT_INITIALIZER(0);

    /// The total number of BuildBLAS calls.
    Uint32 BuildBLAS DEFAULT_INITIALIZER(0);

    /// The total number of BuildTLAS calls.
    Uint32 BuildTLAS DEFAULT_INITIALIZER(0);

    /// The total number of CopyBLAS calls.
    Uint32 CopyBLAS DEFAULT_INITIALIZER(0);

    /// The total number of CopyTLAS calls.
    Uint32 CopyTLAS DEFAULT_INITIALIZER(0);

    /// The total number of WriteBLASCompactedSize calls.
    Uint32 WriteBLASCompactedSize DEFAULT_INITIALIZER(0);

    /// The total number of WriteTLASCompactedSize calls.
    Uint32 WriteTLASCompactedSize DEFAULT_INITIALIZER(0);

    /// The total number of TraceRays calls.
    Uint32 TraceRays DEFAULT_INITIALIZER(0);

    /// The total number of TraceRaysIndirect calls.
    Uint32 TraceRaysIndirect DEFAULT_INITIALIZER(0);

    /// The total number of UpdateSBT calls.
    Uint32 UpdateSBT DEFAULT_INITIALIZER(0);

    /// The total number of UpdateBuffer calls.
    Uint32 UpdateBuffer DEFAULT_INITIALIZER(0);

    /// The total number of CopyBuffer calls.
    Uint32 CopyBuffer DEFAULT_INITIALIZER(0);

    /// The total number of MapBuffer calls.
    Uint32 MapBuffer DEFAULT_INITIALIZER(0);

    /// The total number of UpdateTexture calls.
    Uint32 UpdateTexture DEFAULT_INITIALIZER(0);

    /// The total number of CopyTexture calls.
    Uint32 CopyTexture DEFAULT_INITIALIZER(0);

    /// The total number of MapTextureSubresource calls.
    Uint32 MapTextureSubresource DEFAULT_INITIALIZER(0);

    /// The total number of BeginQuery calls.
    Uint32 BeginQuery DEFAULT_INITIALIZER(0);

    /// The total number of GenerateMips calls.
    Uint32 GenerateMips DEFAULT_INITIALIZER(0);

    /// The total number of ResolveTextureSubresource calls.
    Uint32 ResolveTextureSubresource DEFAULT_INITIALIZER(0);

    /// The total number of BindSparseResourceMemory calls.
    Uint32 BindSparseResourceMemory DEFAULT_INITIALIZER(0);
};
typedef struct DeviceContextCommandCounters DeviceContextCommandCounters;

/// Device context statistics.
struct DeviceContextStats
{
    /// The total number of primitives rendered, for each primitive topology.
    Uint32 PrimitiveCounts[PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES] DEFAULT_INITIALIZER({});
    
    /// Command counters, see Diligent::DeviceContextCommandCounters.
    DeviceContextCommandCounters CommandCounters DEFAULT_INITIALIZER({});

#if DILIGENT_CPP_INTERFACE
    constexpr Uint32 GetTotalTriangleCount() const noexcept
    {
        return PrimitiveCounts[PRIMITIVE_TOPOLOGY_TRIANGLE_LIST] +
               PrimitiveCounts[PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] +
               PrimitiveCounts[PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ];
    }

    constexpr Uint32 GetTotalLineCount() const noexcept
    {
        return PrimitiveCounts[PRIMITIVE_TOPOLOGY_LINE_LIST] +
               PrimitiveCounts[PRIMITIVE_TOPOLOGY_LINE_STRIP] +
               PrimitiveCounts[PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ];
    }

    constexpr Uint32 GetTotalPointCount() const noexcept
	{
    	return PrimitiveCounts[PRIMITIVE_TOPOLOGY_POINT_LIST];
    }
#endif
};
typedef struct DeviceContextStats DeviceContextStats;


#define DILIGENT_INTERFACE_NAME IDeviceContext
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextInclusiveMethods  \
    IObjectInclusiveMethods;            \
    IDeviceContextMethods DeviceContext

/// Device context interface.

/// \remarks Device context keeps strong references to all objects currently bound to
///          the pipeline: buffers, states, samplers, shaders, etc.
///          The context also keeps strong reference to the device and
///          the swap chain.
DILIGENT_BEGIN_INTERFACE(IDeviceContext, IObject)
{
    /// Returns the context description
    VIRTUAL const DeviceContextDesc REF METHOD(GetDesc)(THIS) CONST PURE;

    /// Begins recording commands in the deferred context.

    /// This method must be called before any command in the deferred context may be recorded.
    ///
    /// \param [in] ImmediateContextId - the ID of the immediate context where commands from this
    ///                                  deferred context will be executed,
    ///                                  see Diligent::DeviceContextDesc::ContextId.
    ///
    /// \warning Command list recorded by the context must not be submitted to any other immediate context
    ///          other than one identified by ImmediateContextId.
    VIRTUAL void METHOD(Begin)(THIS_
                               Uint32 ImmediateContextId) PURE;

    /// Sets the pipeline state.

    /// \param [in] pPipelineState - Pointer to IPipelineState interface to bind to the context.
    ///
    /// \remarks Supported contexts for graphics and mesh pipeline:        graphics.
    ///          Supported contexts for compute and ray tracing pipeline:  graphics and compute.
    VIRTUAL void METHOD(SetPipelineState)(THIS_
                                          IPipelineState* pPipelineState) PURE;


    /// Transitions shader resources to the states required by Draw or Dispatch command.
    ///
    /// \param [in] pShaderResourceBinding - Shader resource binding whose resources will be transitioned.
    ///
    /// \remarks This method explicitly transitions all resources except ones in unknown state to the states required
    ///          by Draw or Dispatch command.
    ///          If this method was called, there is no need to use Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    ///          when calling IDeviceContext::CommitShaderResources()
    ///
    /// \remarks Resource state transitioning is not thread safe. As the method may alter the states
    ///          of resources referenced by the shader resource binding, no other thread is allowed to read or
    ///          write these states.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    VIRTUAL void METHOD(TransitionShaderResources)(THIS_
                                                   IShaderResourceBinding* pShaderResourceBinding) PURE;

    /// Commits shader resources to the device context.

    /// \param [in] pShaderResourceBinding - Shader resource binding whose resources will be committed.
    ///                                      If pipeline state contains no shader resources, this parameter
    ///                                      can be null.
    /// \param [in] StateTransitionMode    - State transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks If Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is used,
    ///          the engine will also transition all shader resources to required states. If the flag
    ///          is not set, it is assumed that all resources are already in correct states.\n
    ///          Resources can be explicitly transitioned to required states by calling
    ///          IDeviceContext::TransitionShaderResources() or IDeviceContext::TransitionResourceStates().\n
    ///
    /// \remarks Automatic resource state transitioning is not thread-safe.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is used, the method may alter the states
    ///            of resources referenced by the shader resource binding and no other thread is allowed to read or write these states.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY mode is used, the method will read the states, so no other thread
    ///            should alter the states by calling any of the methods that use Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode.
    ///            It is safe for other threads to read the states.
    ///
    ///          - If Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE mode is used, the method does not access the states of resources.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it should manage the states
    ///          manually by setting the state to Diligent::RESOURCE_STATE_UNKNOWN (which will disable automatic state
    ///          management) using IBuffer::SetState() or ITexture::SetState() and explicitly transitioning the states with
    ///          IDeviceContext::TransitionResourceStates().
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    ///          If an application calls any method that changes the state of any resource after it has been committed, the
    ///          application is responsible for transitioning the resource back to correct state using one of the available methods
    ///          before issuing the next draw or dispatch command.
    VIRTUAL void METHOD(CommitShaderResources)(THIS_
                                               IShaderResourceBinding*        pShaderResourceBinding,
                                               RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;

    /// Sets the stencil reference value.

    /// \param [in] StencilRef - Stencil reference value.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetStencilRef)(THIS_
                                       Uint32 StencilRef) PURE;


    /// \param [in] pBlendFactors - Array of four blend factors, one for each RGBA component.
    ///                             These factors are used if the blend state uses one of the
    ///                             Diligent::BLEND_FACTOR_BLEND_FACTOR or
    ///                             Diligent::BLEND_FACTOR_INV_BLEND_FACTOR
    ///                             blend factors. If nullptr is provided,
    ///                             default blend factors array {1,1,1,1} will be used.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetBlendFactors)(THIS_
                                         const float* pBlendFactors DEFAULT_VALUE(nullptr)) PURE;


    /// Binds vertex buffers to the pipeline.

    /// \param [in] StartSlot           - The first input slot for binding. The first vertex buffer is
    ///                                   explicitly bound to the start slot; each additional vertex buffer
    ///                                   in the array is implicitly bound to each subsequent input slot.
    /// \param [in] NumBuffersSet       - The number of vertex buffers in the array.
    /// \param [in] ppBuffers           - A pointer to an array of vertex buffers.
    ///                                   The buffers must have been created with the Diligent::BIND_VERTEX_BUFFER flag.
    /// \param [in] pOffsets            - Pointer to an array of offset values; one offset value for each buffer
    ///                                   in the vertex-buffer array. Each offset is the number of bytes between
    ///                                   the first element of a vertex buffer and the first element that will be
    ///                                   used. If this parameter is nullptr, zero offsets for all buffers will be used.
    /// \param [in] StateTransitionMode - State transition mode for buffers being set (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    /// \param [in] Flags               - Additional flags. See Diligent::SET_VERTEX_BUFFERS_FLAGS for a list of allowed values.
    ///
    /// \remarks The device context keeps strong references to all bound vertex buffers.
    ///          Thus a buffer cannot be released until it is unbound from the context.\n
    ///          It is suggested to specify Diligent::SET_VERTEX_BUFFERS_FLAG_RESET flag
    ///          whenever possible. This will assure that no buffers from previous draw calls
    ///          are bound to the pipeline.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will
    ///          transition all buffers in known states to Diligent::RESOURCE_STATE_VERTEX_BUFFER. Resource state
    ///          transitioning is not thread safe, so no other thread is allowed to read or write the states of
    ///          these buffers.
    ///
    ///          If the application intends to use the same resources in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetVertexBuffers)(THIS_
                                          Uint32                         StartSlot,
                                          Uint32                         NumBuffersSet,
                                          IBuffer* const*                ppBuffers,
                                          const Uint64*                  pOffsets,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                          SET_VERTEX_BUFFERS_FLAGS       Flags DEFAULT_VALUE(SET_VERTEX_BUFFERS_FLAG_NONE)) PURE;


    /// Invalidates the cached context state.

    /// This method should be called by an application to invalidate
    /// internal cached states.
    VIRTUAL void METHOD(InvalidateState)(THIS) PURE;


    /// Binds an index buffer to the pipeline.

    /// \param [in] pIndexBuffer        - Pointer to the index buffer. The buffer must have been created
    ///                                   with the Diligent::BIND_INDEX_BUFFER flag.
    /// \param [in] ByteOffset          - Offset from the beginning of the buffer to
    ///                                   the start of index data.
    /// \param [in] StateTransitionMode - State transition mode for the index buffer to bind (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks The device context keeps strong reference to the index buffer.
    ///          Thus an index buffer object cannot be released until it is unbound
    ///          from the context.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will
    ///          transition the buffer to Diligent::RESOURCE_STATE_INDEX_BUFFER (if its state is not unknown). Resource
    ///          state transitioning is not thread safe, so no other thread is allowed to read or write the state of
    ///          the buffer.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetIndexBuffer)(THIS_
                                        IBuffer*                       pIndexBuffer,
                                        Uint64                         ByteOffset,
                                        RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Sets an array of viewports.

    /// \param [in] NumViewports - Number of viewports to set.
    /// \param [in] pViewports   - An array of Viewport structures describing the viewports to bind.
    /// \param [in] RTWidth      - Render target width. If 0 is provided, width of the currently bound render target will be used.
    /// \param [in] RTHeight     - Render target height. If 0 is provided, height of the currently bound render target will be used.
    ///
    /// \remarks
    /// DirectX and OpenGL use different window coordinate systems. In DirectX, the coordinate system origin
    /// is in the left top corner of the screen with Y axis pointing down. In OpenGL, the origin
    /// is in the left bottom corner of the screen with Y axis pointing up. Render target size is
    /// required to convert viewport from DirectX to OpenGL coordinate system if OpenGL device is used.\n\n
    /// All viewports must be set atomically as one operation. Any viewports not
    /// defined by the call are disabled.\n\n
    /// You can set the viewport size to match the currently bound render target using the
    /// following call:
    ///
    ///     pContext->SetViewports(1, nullptr, 0, 0);
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetViewports)(THIS_
                                      Uint32          NumViewports,
                                      const Viewport* pViewports,
                                      Uint32          RTWidth,
                                      Uint32          RTHeight) PURE;


    /// Sets active scissor rects.

    /// \param [in] NumRects - Number of scissor rectangles to set.
    /// \param [in] pRects   - An array of Rect structures describing the scissor rectangles to bind.
    /// \param [in] RTWidth  - Render target width. If 0 is provided, width of the currently bound render target will be used.
    /// \param [in] RTHeight - Render target height. If 0 is provided, height of the currently bound render target will be used.
    ///
    /// \remarks
    /// DirectX and OpenGL use different window coordinate systems. In DirectX, the coordinate system origin
    /// is in the left top corner of the screen with Y axis pointing down. In OpenGL, the origin
    /// is in the left bottom corner of the screen with Y axis pointing up. Render target size is
    /// required to convert viewport from DirectX to OpenGL coordinate system if OpenGL device is used.\n\n
    /// All scissor rects must be set atomically as one operation. Any rects not
    /// defined by the call are disabled.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetScissorRects)(THIS_
                                         Uint32      NumRects,
                                         const Rect* pRects,
                                         Uint32      RTWidth,
                                         Uint32      RTHeight) PURE;


    /// Binds one or more render targets and the depth-stencil buffer to the context. It also
    /// sets the viewport to match the first non-null render target or depth-stencil buffer.

    /// \param [in] NumRenderTargets    - Number of render targets to bind.
    /// \param [in] ppRenderTargets     - Array of pointers to ITextureView that represent the render
    ///                                   targets to bind to the device. The type of each view in the
    ///                                   array must be Diligent::TEXTURE_VIEW_RENDER_TARGET.
    /// \param [in] pDepthStencil       - Pointer to the ITextureView that represents the depth stencil to
    ///                                   bind to the device. The view type must be
    ///                                   Diligent::TEXTURE_VIEW_DEPTH_STENCIL or Diligent::TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL.
    /// \param [in] StateTransitionMode - State transition mode of the render targets and depth stencil buffer being set (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks     The device context will keep strong references to all bound render target
    ///              and depth-stencil views. Thus these views (and consequently referenced textures)
    ///              cannot be released until they are unbound from the context.\n
    ///              Any render targets not defined by this call are set to nullptr.\n\n
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will
    ///          transition all render targets in known states to Diligent::RESOURCE_STATE_RENDER_TARGET,
    ///          and the depth-stencil buffer to Diligent::RESOURCE_STATE_DEPTH_WRITE state.
    ///          Resource state transitioning is not thread safe, so no other thread is allowed to read or write
    ///          the states of resources used by the command.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetRenderTargets)(THIS_
                                          Uint32                         NumRenderTargets,
                                          ITextureView*                  ppRenderTargets[],
                                          ITextureView*                  pDepthStencil,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Binds one or more render targets, the depth-stencil buffer and shading rate map to the context.
    /// It also sets the viewport to match the first non-null render target or depth-stencil buffer.

    /// \param [in] Attribs - The command attributes, see Diligent::SetRenderTargetsAttribs for details.
    ///
    /// \remarks     The device context will keep strong references to all bound render target
    ///              and depth-stencil views as well as to shading rate map. Thus these views
    ///              (and consequently referenced textures) cannot be released until they are
    ///              unbound from the context.\n
    ///              Any render targets not defined by this call are set to nullptr.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetRenderTargetsExt)(THIS_
                                             const SetRenderTargetsAttribs REF Attribs) PURE;


    /// Begins a new render pass.

    /// \param [in] Attribs - The command attributes, see Diligent::BeginRenderPassAttribs for details.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(BeginRenderPass)(THIS_
                                         const BeginRenderPassAttribs REF Attribs) PURE;


    /// Transitions to the next subpass in the render pass instance.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(NextSubpass)(THIS) PURE;


    /// Ends current render pass.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(EndRenderPass)(THIS) PURE;


    /// Executes a draw command.

    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawAttribs for details.
    ///
    /// \remarks  If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(Draw)(THIS_
                              const DrawAttribs REF Attribs) PURE;


    /// Executes an indexed draw command.

    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawIndexedAttribs for details.
    ///
    /// \remarks  If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(DrawIndexed)(THIS_
                                     const DrawIndexedAttribs REF Attribs) PURE;


    /// Executes an indirect draw command.

    /// \param [in] Attribs - Structure describing the command attributes, see Diligent::DrawIndirectAttribs for details.
    ///
    /// \remarks  Draw command arguments are read from the attributes buffer at the offset given by
    ///           Attribs.IndirectDrawArgsOffset. If Attribs.DrawCount > 1, the arguments for command N
    ///           will be read at the offset
    ///
    ///             Attribs.IndirectDrawArgsOffset + N * Attribs.IndirectDrawArgsStride.
    ///
    ///           If pCountBuffer is not null, the number of commands to execute will be read from the buffer at the offset
    ///           given by Attribs.CounterOffset. The number of commands will be the lesser of the value read from the buffer
    ///           and Attribs.DrawCount:
    ///
    ///             NumCommands = min(CountBuffer[Attribs.CounterOffset], Attribs.DrawCount)
    ///
    ///           If Attribs.IndirectAttribsBufferStateTransitionMode or Attribs.CounterBufferStateTransitionMode is
    ///           Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method may transition the state of the indirect
    ///           draw arguments buffer, and the state of the counter buffer . This is not a thread safe operation,
    ///           so no other thread is allowed to read or write the state of the buffer.
    ///
    ///           If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(DrawIndirect)(THIS_
                                      const DrawIndirectAttribs REF Attribs) PURE;


    /// Executes an indexed indirect draw command.

    /// \param [in] Attribs - Structure describing the command attributes, see Diligent::DrawIndexedIndirectAttribs for details.
    ///
    /// \remarks  Draw command arguments are read from the attributes buffer at the offset given by
    ///           Attribs.IndirectDrawArgsOffset. If Attribs.DrawCount > 1, the arguments for command N
    ///           will be read at the offset
    ///
    ///             Attribs.IndirectDrawArgsOffset + N * Attribs.IndirectDrawArgsStride.
    ///
    ///           If pCountBuffer is not null, the number of commands to execute will be read from the buffer at the offset
    ///           given by Attribs.CounterOffset. The number of commands will be the lesser of the value read from the buffer
    ///           and Attribs.DrawCount:
    ///
    ///             NumCommands = min(CountBuffer[Attribs.CounterOffset], Attribs.DrawCount)
    ///
    ///           If Attribs.IndirectAttribsBufferStateTransitionMode or Attribs.CounterBufferStateTransitionMode is
    ///           Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method may transition the state of the indirect
    ///           draw arguments buffer, and the state of the counter buffer . This is not a thread safe operation,
    ///           so no other thread is allowed to read or write the state of the buffer.
    ///
    ///           If Diligent::DRAW_FLAG_VERIFY_STATES flag is set, the method reads the state of vertex/index
    ///           buffers, so no other threads are allowed to alter the states of the same resources.
    ///           It is OK to read these states.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \remarks  In OpenGL backend, index buffer offset set by SetIndexBuffer() can't be applied in
    ///           indirect draw command and must be zero.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(DrawIndexedIndirect)(THIS_
                                             const DrawIndexedIndirectAttribs REF Attribs) PURE;


    /// Executes a mesh draw command.

    /// \param [in] Attribs - Draw command attributes, see Diligent::DrawMeshAttribs for details.
    ///
    /// \remarks  For compatibility between Direct3D12 and Vulkan, only a single work group dimension is used.
    ///           Also in the shader, 'numthreads' and 'local_size' attributes must define only the first dimension,
    ///           for example: '[numthreads(ThreadCount, 1, 1)]' or 'layout(local_size_x = ThreadCount) in'.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(DrawMesh)(THIS_
                                  const DrawMeshAttribs REF Attribs) PURE;


    /// Executes an indirect mesh draw command.

    /// \param [in] Attribs - Structure describing the command attributes, see Diligent::DrawMeshIndirectAttribs for details.
    ///
    /// \remarks  For compatibility between Direct3D12 and Vulkan and with direct call (DrawMesh) use the first element in the structure,
    ///           for example: Direct3D12 {TaskCount, 1, 1}, Vulkan {TaskCount, 0}.
    ///
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of the indirect draw arguments buffer. This is not a thread safe operation,
    ///           so no other thread is allowed to read or write the state of the buffer.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(DrawMeshIndirect)(THIS_
                                          const DrawMeshIndirectAttribs REF Attribs) PURE;


    /// Executes a multi-draw command.

    /// \param [in] Attribs - Multi-draw command attributes, see Diligent::MultiDrawAttribs for details.
    ///
    /// \remarks  If the device does not support the NativeMultiDraw feature, the method will emulate it by
    ///           issuing a sequence of individual draw commands. Note that draw command index is only
    ///           available in the shader when the NativeMultiDraw feature is supported.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(MultiDraw)(THIS_
                                   const MultiDrawAttribs REF Attribs) PURE;
    

    /// Executes an indexed multi-draw command.

    /// \param [in] Attribs - Multi-draw command attributes, see Diligent::MultiDrawIndexedAttribs for details.
    ///
    /// \remarks  If the device does not support the NativeMultiDraw feature, the method will emulate it by
    ///           issuing a sequence of individual draw commands. Note that draw command index is only
    ///           available in the shader when the NativeMultiDraw feature is supported.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(MultiDrawIndexed)(THIS_
                                          const MultiDrawIndexedAttribs REF Attribs) PURE;


    /// Executes a dispatch compute command.

    /// \param [in] Attribs - Dispatch command attributes, see Diligent::DispatchComputeAttribs for details.
    ///
    /// \remarks    In Metal, the compute group sizes are defined by the dispatch command rather than by
    ///             the compute shader. When the shader is compiled from HLSL or GLSL, the engine will
    ///             use the group size information from the shader. When using MSL, an application should
    ///             provide the compute group dimensions through MtlThreadGroupSizeX, MtlThreadGroupSizeY,
    ///             and MtlThreadGroupSizeZ members.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(DispatchCompute)(THIS_
                                         const DispatchComputeAttribs REF Attribs) PURE;


    /// Executes an indirect dispatch compute command.

    /// \param [in] Attribs - The command attributes, see Diligent::DispatchComputeIndirectAttribs for details.
    ///
    /// \remarks  If IndirectAttribsBufferStateTransitionMode member is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    ///           the method may transition the state of indirect dispatch arguments buffer. This is not a thread safe operation,
    ///           so no other thread is allowed to read or write the state of the same resource.
    ///
    ///           If the application intends to use the same resources in other threads simultaneously, it needs to
    ///           explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    ///           In Metal, the compute group sizes are defined by the dispatch command rather than by
    ///           the compute shader. When the shader is compiled from HLSL or GLSL, the engine will
    ///           use the group size information from the shader. When using MSL, an application should
    ///           provide the compute group dimensions through MtlThreadGroupSizeX, MtlThreadGroupSizeY,
    ///           and MtlThreadGroupSizeZ members.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(DispatchComputeIndirect)(THIS_
                                                 const DispatchComputeIndirectAttribs REF Attribs) PURE;


    /// Executes a dispatch tile command.

    /// \param [in] Attribs - The command attributes, see Diligent::DispatchTileAttribs for details.
    VIRTUAL void METHOD(DispatchTile)(THIS_
                                      const DispatchTileAttribs REF Attribs) PURE;


    /// Returns current render pass tile size.

    /// \param [out] TileSizeX - Tile size in X direction.
    /// \param [out] TileSizeY - Tile size in X direction.
    ///
    /// \remarks Result will be zero if there are no active render pass or render targets.
    VIRTUAL void METHOD(GetTileSize)(THIS_
                                     Uint32 REF TileSizeX,
                                     Uint32 REF TileSizeY) PURE;


    /// Clears a depth-stencil view.

    /// \param [in] pView               - Pointer to ITextureView interface to clear. The view type must be
    ///                                   Diligent::TEXTURE_VIEW_DEPTH_STENCIL.
    /// \param [in] StateTransitionMode - state transition mode of the depth-stencil buffer to clear.
    /// \param [in] ClearFlags          - Indicates which parts of the buffer to clear, see Diligent::CLEAR_DEPTH_STENCIL_FLAGS.
    /// \param [in] fDepth              - Value to clear depth part of the view with.
    /// \param [in] Stencil             - Value to clear stencil part of the view with.
    ///
    /// \remarks The full extent of the view is always cleared. Viewport and scissor settings are not applied.
    /// \note The depth-stencil view must be bound to the pipeline for clear operation to be performed.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will
    ///          transition the state of the texture to the state required by clear operation.
    ///          In Direct3D12, this state is always Diligent::RESOURCE_STATE_DEPTH_WRITE, however in Vulkan
    ///          the state depends on whether the depth buffer is bound to the pipeline.
    ///
    ///          Resource state transitioning is not thread safe, so no other thread is allowed to read or write
    ///          the state of resources used by the command.
    ///          Refer to http://diligentgraphics.com/2018/12/09/resource-state-management/ for detailed explanation
    ///          of resource state management in Diligent Engine.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(ClearDepthStencil)(THIS_
                                           ITextureView*                  pView,
                                           CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                           float                          fDepth,
                                           Uint8                          Stencil,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Clears a render target view

    /// \param [in] pView               - Pointer to ITextureView interface to clear. The view type must be
    ///                                   Diligent::TEXTURE_VIEW_RENDER_TARGET.
    /// \param [in] RGBA                - A 4-component array that represents the color to fill the render target with:
    ///                                   - Float values for floating point render target formats.
    ///                                   - Uint32 values for unsigned integer render target formats.
    ///                                   - Int32 values for signed integer render target formats.
    ///                                   If nullptr is provided, the default array {0,0,0,0} will be used.
    /// \param [in] StateTransitionMode - Defines required state transitions (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    ///
    /// \remarks The full extent of the view is always cleared. Viewport and scissor settings are not applied.
    ///
    ///          The render target view must be bound to the pipeline for clear operation to be performed in OpenGL backend.
    ///
    /// \remarks When StateTransitionMode is Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, the method will
    ///          transition the texture to the state required by the command. Resource state transitioning is not
    ///          thread safe, so no other thread is allowed to read or write the states of the same textures.
    ///
    ///          If the application intends to use the same resource in other threads simultaneously, it needs to
    ///          explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
    ///
    /// \note    In D3D12 backend clearing render targets requires textures to always be transitioned to
    ///          Diligent::RESOURCE_STATE_RENDER_TARGET state. In Vulkan backend however this depends on whether a
    ///          render pass has been started. To clear render target outside of a render pass, the texture must be transitioned to
    ///          Diligent::RESOURCE_STATE_COPY_DEST state. Inside a render pass it must be in Diligent::RESOURCE_STATE_RENDER_TARGET
    ///          state. When using Diligent::RESOURCE_STATE_TRANSITION_TRANSITION mode, the engine takes care of proper
    ///          resource state transition, otherwise it is the responsibility of the application.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(ClearRenderTarget)(THIS_
                                           ITextureView*                  pView,
                                           const void*                    RGBA,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Finishes recording commands and generates a command list.

    /// \param [out] ppCommandList - Memory location where pointer to the recorded command list will be written.
    VIRTUAL void METHOD(FinishCommandList)(THIS_
                                           ICommandList** ppCommandList) PURE;


    /// Submits an array of recorded command lists for execution.

    /// \param [in] NumCommandLists - The number of command lists to execute.
    /// \param [in] ppCommandLists  - Pointer to the array of NumCommandLists command lists to execute.
    /// \remarks After a command list is executed, it is no longer valid and must be released.
    VIRTUAL void METHOD(ExecuteCommandLists)(THIS_
                                             Uint32               NumCommandLists,
                                             ICommandList* const* ppCommandLists) PURE;


    /// Tells the GPU to set a fence to a specified value after all previous work has completed.

    /// \param [in] pFence - The fence to signal.
    /// \param [in] Value  - The value to set the fence to. This value must be greater than the
    ///                      previously signaled value on the same fence.
    ///
    /// \note The method does not flush the context (an application can do this explicitly if needed)
    ///       and the fence will be signaled only when the command context is flushed next time.
    ///       If an application needs to wait for the fence in a loop, it must flush the context
    ///       after signalling the fence.
    ///
    /// \note In Direct3D11 backend, the access to the fence is not thread-safe and
    ///       must be externally synchronized.
    ///       In Direct3D12 and Vulkan backends, the access to the fence is thread-safe.
    VIRTUAL void METHOD(EnqueueSignal)(THIS_
                                       IFence*    pFence,
                                       Uint64     Value) PURE;


    /// Waits until the specified fence reaches or exceeds the specified value, on the device.

    /// \param [in] pFence - The fence to wait. Fence must be created with type FENCE_TYPE_GENERAL.
    /// \param [in] Value  - The value that the context is waiting for the fence to reach.
    ///
    /// \note  If NativeFence feature is not enabled (see Diligent::DeviceFeatures) then
    ///        Value must be less than or equal to the last signaled or pending value.
    ///        Value becomes pending when the context is flushed.
    ///        Waiting for a value that is greater than any pending value will cause a deadlock.
    ///
    /// \note  If NativeFence feature is enabled then waiting for a value that is greater than
    ///        any pending value will cause a GPU stall.
    ///
    /// \note  Direct3D12 and Vulkan backend: access to the fence is thread-safe.
    ///
    /// \remarks  Wait is only allowed for immediate contexts.
    VIRTUAL void METHOD(DeviceWaitForFence)(THIS_
                                            IFence*  pFence,
                                            Uint64   Value) PURE;

    /// Submits all outstanding commands for execution to the GPU and waits until they are complete.

    /// \note The method blocks the execution of the calling thread until the wait is complete.
    ///
    /// \remarks    Only immediate contexts can be idled.\n
    ///             The methods implicitly flushes the context (see IDeviceContext::Flush()), so an
    ///             application must explicitly reset the PSO and bind all required shader resources after
    ///             idling the context.\n
    VIRTUAL void METHOD(WaitForIdle)(THIS) PURE;


    /// Marks the beginning of a query.

    /// \param [in] pQuery - A pointer to a query object.
    ///
    /// \remarks    Only immediate contexts can begin a query.
    ///
    ///             Vulkan requires that a query must either begin and end inside the same
    ///             subpass of a render pass instance, or must both begin and end outside of
    ///             a render pass instance. This means that an application must either begin
    ///             and end a query while preserving render targets, or begin it when no render
    ///             targets are bound to the context. In the latter case the engine will automatically
    ///             end the render pass, if needed, when the query is ended.
    ///             Also note that resource transitions must be performed outside of a render pass,
    ///             and may thus require ending current render pass.
    ///             To explicitly end current render pass, call
    ///             SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE).
    ///
    /// \warning    OpenGL and Vulkan do not support nested queries of the same type.
    ///
    /// \remarks    On some devices, queries for a single draw or dispatch command may not be supported.
    ///             In this case, the query will begin at the next available moment (for example,
    ///             when the next render pass begins or ends).
    ///
    /// \remarks Supported contexts for graphics queries: graphics.
    ///          Supported contexts for time queries:     graphics, compute.
    VIRTUAL void METHOD(BeginQuery)(THIS_
                                    IQuery* pQuery) PURE;


    /// Marks the end of a query.

    /// \param [in] pQuery - A pointer to a query object.
    ///
    /// \remarks    A query must be ended by the same context that began it.
    ///
    ///             In Direct3D12 and Vulkan, queries (except for timestamp queries)
    ///             cannot span command list boundaries, so the engine will never flush
    ///             the context even if the number of commands exceeds the user-specified limit
    ///             when there is an active query.
    ///             It is an error to explicitly flush the context while a query is active.
    ///
    ///             All queries must be ended when IDeviceContext::FinishFrame() is called.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(EndQuery)(THIS_
                                  IQuery* pQuery) PURE;


    /// Submits all pending commands in the context for execution to the command queue.

    /// \remarks    Only immediate contexts can be flushed.\n
    ///             Internally the method resets the state of the current command list/buffer.
    ///             When the next draw command is issued, the engine will restore all states
    ///             (rebind render targets and depth-stencil buffer as well as index and vertex buffers,
    ///             restore viewports and scissor rects, etc.) except for the pipeline state and shader resource
    ///             bindings. An application must explicitly reset the PSO and bind all required shader
    ///             resources after flushing the context.
    VIRTUAL void METHOD(Flush)(THIS) PURE;


    /// Updates the data in the buffer.

    /// \param [in] pBuffer             - Pointer to the buffer to update.
    /// \param [in] Offset              - Offset in bytes from the beginning of the buffer to the update region.
    /// \param [in] Size                - Size in bytes of the data region to update.
    /// \param [in] pData               - Pointer to the data to write to the buffer.
    /// \param [in] StateTransitionMode - Buffer state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(UpdateBuffer)(THIS_
                                      IBuffer*                       pBuffer,
                                      Uint64                         Offset,
                                      Uint64                         Size,
                                      const void*                    pData,
                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) PURE;


    /// Copies the data from one buffer to another.

    /// \param [in] pSrcBuffer              - Source buffer to copy data from.
    /// \param [in] SrcOffset               - Offset in bytes from the beginning of the source buffer to the beginning of data to copy.
    /// \param [in] SrcBufferTransitionMode - State transition mode of the source buffer (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    /// \param [in] pDstBuffer              - Destination buffer to copy data to.
    /// \param [in] DstOffset               - Offset in bytes from the beginning of the destination buffer to the beginning
    ///                                       of the destination region.
    /// \param [in] Size                    - Size in bytes of data to copy.
    /// \param [in] DstBufferTransitionMode - State transition mode of the destination buffer (see Diligent::RESOURCE_STATE_TRANSITION_MODE).
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(CopyBuffer)(THIS_
                                    IBuffer*                       pSrcBuffer,
                                    Uint64                         SrcOffset,
                                    RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                    IBuffer*                       pDstBuffer,
                                    Uint64                         DstOffset,
                                    Uint64                         Size,
                                    RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) PURE;


    /// Maps the buffer.

    /// \param [in] pBuffer      - Pointer to the buffer to map.
    /// \param [in] MapType      - Type of the map operation. See Diligent::MAP_TYPE.
    /// \param [in] MapFlags     - Special map flags. See Diligent::MAP_FLAGS.
    /// \param [out] pMappedData - Reference to the void pointer to store the address of the mapped region.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(MapBuffer)(THIS_
                                   IBuffer*     pBuffer,
                                   MAP_TYPE     MapType,
                                   MAP_FLAGS    MapFlags,
                                   PVoid REF    pMappedData) PURE;


    /// Unmaps the previously mapped buffer.

    /// \param [in] pBuffer - Pointer to the buffer to unmap.
    /// \param [in] MapType - Type of the map operation. This parameter must match the type that was
    ///                       provided to the Map() method.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(UnmapBuffer)(THIS_
                                     IBuffer*   pBuffer,
                                     MAP_TYPE   MapType) PURE;


    /// Updates the data in the texture.

    /// \param [in] pTexture    - Pointer to the device context interface to be used to perform the operation.
    /// \param [in] MipLevel    - Mip level of the texture subresource to update.
    /// \param [in] Slice       - Array slice. Should be 0 for non-array textures.
    /// \param [in] DstBox      - Destination region on the texture to update.
    /// \param [in] SubresData  - Source data to copy to the texture.
    /// \param [in] SrcBufferTransitionMode - If pSrcBuffer member of TextureSubResData structure is not null, this
    ///                                       parameter defines state transition mode of the source buffer.
    ///                                       If pSrcBuffer is null, this parameter is ignored.
    /// \param [in] TextureTransitionMode   - Texture state transition mode (see Diligent::RESOURCE_STATE_TRANSITION_MODE)
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(UpdateTexture)(THIS_
                                       ITexture*                        pTexture,
                                       Uint32                           MipLevel,
                                       Uint32                           Slice,
                                       const Box REF                    DstBox,
                                       const TextureSubResData REF      SubresData,
                                       RESOURCE_STATE_TRANSITION_MODE   SrcBufferTransitionMode,
                                       RESOURCE_STATE_TRANSITION_MODE   TextureTransitionMode) PURE;


    /// Copies data from one texture to another.

    /// \param [in] CopyAttribs - Structure describing copy command attributes, see Diligent::CopyTextureAttribs for details.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(CopyTexture)(THIS_
                                     const CopyTextureAttribs REF CopyAttribs) PURE;


    /// Maps the texture subresource.

    /// \param [in] pTexture    - Pointer to the texture to map.
    /// \param [in] MipLevel    - Mip level to map.
    /// \param [in] ArraySlice  - Array slice to map. This parameter must be 0 for non-array textures.
    /// \param [in] MapType     - Type of the map operation. See Diligent::MAP_TYPE.
    /// \param [in] MapFlags    - Special map flags. See Diligent::MAP_FLAGS.
    /// \param [in] pMapRegion  - Texture region to map. If this parameter is null, the entire subresource is mapped.
    /// \param [out] MappedData - Mapped texture region data
    ///
    /// \remarks This method is supported in D3D11, D3D12 and Vulkan backends. In D3D11 backend, only the entire
    ///          subresource can be mapped, so pMapRegion must either be null, or cover the entire subresource.
    ///          In D3D11 and Vulkan backends, dynamic textures are no different from non-dynamic textures, and mapping
    ///          with MAP_FLAG_DISCARD has exactly the same behavior.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(MapTextureSubresource)(THIS_
                                               ITexture*                    pTexture,
                                               Uint32                       MipLevel,
                                               Uint32                       ArraySlice,
                                               MAP_TYPE                     MapType,
                                               MAP_FLAGS                    MapFlags,
                                               const Box*                   pMapRegion,
                                               MappedTextureSubresource REF MappedData) PURE;


    /// Unmaps the texture subresource.

    /// \param [in] pTexture    - Pointer to the texture to unmap.
    /// \param [in] MipLevel    - Mip level to unmap.
    /// \param [in] ArraySlice  - Array slice to unmap. This parameter must be 0 for non-array textures.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(UnmapTextureSubresource)(THIS_
                                                 ITexture* pTexture,
                                                 Uint32    MipLevel,
                                                 Uint32    ArraySlice) PURE;


    /// Generates a mipmap chain.

    /// \param [in] pTextureView - Texture view to generate mip maps for.
    /// \remarks This function can only be called for a shader resource view.
    ///          The texture must be created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(GenerateMips)(THIS_
                                      ITextureView* pTextureView) PURE;


    /// Finishes the current frame and releases dynamic resources allocated by the context.

    /// For immediate context, this method is called automatically by ISwapChain::Present() of the primary
    /// swap chain, but can also be called explicitly. For deferred contexts, the method must be called by the
    /// application to release dynamic resources. The method has some overhead, so it is better to call it once
    /// per frame, though it can be called with different frequency. Note that unless the GPU is idled,
    /// the resources may actually be released several frames after the one they were used in last time.
    /// \note After the call all dynamic resources become invalid and must be written again before the next use.
    ///       Also, all committed resources become invalid.\n
    ///       For deferred contexts, this method must be called after all command lists referencing dynamic resources
    ///       have been executed through immediate context.\n
    ///       The method does not Flush() the context.
    VIRTUAL void METHOD(FinishFrame)(THIS) PURE;


    /// Returns the current frame number.

    /// \note The frame number is incremented every time FinishFrame() is called.
    VIRTUAL Uint64 METHOD(GetFrameNumber)(THIS) CONST PURE;


    /// Transitions resource states.

    /// \param [in] BarrierCount      - Number of barriers in pResourceBarriers array
    /// \param [in] pResourceBarriers - Pointer to the array of resource barriers
    /// \remarks When both old and new states are RESOURCE_STATE_UNORDERED_ACCESS, the engine
    ///          executes UAV barrier on the resource. The barrier makes sure that all UAV accesses
    ///          (reads or writes) are complete before any future UAV accesses (read or write) can begin.\n
    ///
    ///          There are two main usage scenarios for this method:
    ///          1. An application knows specifics of resource state transitions not available to the engine.
    ///             For example, only single mip level needs to be transitioned.
    ///          2. An application manages resource states in multiple threads in parallel.
    ///
    ///          The method always reads the states of all resources to transition. If the state of a resource is managed
    ///          by multiple threads in parallel, the resource must first be transitioned to unknown state
    ///          (Diligent::RESOURCE_STATE_UNKNOWN) to disable automatic state management in the engine.
    ///
    ///          When StateTransitionDesc::UpdateResourceState is set to true, the method may update the state of the
    ///          corresponding resource which is not thread safe. No other threads should read or write the state of that
    ///          resource.
    ///
    /// \note  Resource states for shader access (e.g. RESOURCE_STATE_CONSTANT_BUFFER, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE)
    ///        may map to different native state depending on what context type is used (see DeviceContextDesc::ContextType).
    ///        To synchronize write access in compute shader in compute context with a pixel shader read in graphics context, an
    ///        application should call TransitionResourceStates() in graphics context.
    ///        Using TransitionResourceStates() with NewState = RESOURCE_STATE_SHADER_RESOURCE will not invalidate cache in graphics shaders
    ///        and may cause undefined behaviour.
    VIRTUAL void METHOD(TransitionResourceStates)(THIS_
                                                  Uint32                     BarrierCount,
                                                  const StateTransitionDesc* pResourceBarriers) PURE;


    /// Resolves a multi-sampled texture subresource into a non-multi-sampled texture subresource.

    /// \param [in] pSrcTexture    - Source multi-sampled texture.
    /// \param [in] pDstTexture    - Destination non-multi-sampled texture.
    /// \param [in] ResolveAttribs - Resolve command attributes, see Diligent::ResolveTextureSubresourceAttribs for details.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(ResolveTextureSubresource)(THIS_
                                                   ITexture*                                  pSrcTexture,
                                                   ITexture*                                  pDstTexture,
                                                   const ResolveTextureSubresourceAttribs REF ResolveAttribs) PURE;


    /// Builds a bottom-level acceleration structure with the specified geometries.

    /// \param [in] Attribs - Structure describing build BLAS command attributes, see Diligent::BuildBLASAttribs for details.
    ///
    /// \note Don't call build or copy operation on the same BLAS in a different contexts, because BLAS has CPU-side data
    ///       that will not match with GPU-side, so shader binding were incorrect.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(BuildBLAS)(THIS_
                                   const BuildBLASAttribs REF Attribs) PURE;


    /// Builds a top-level acceleration structure with the specified instances.

    /// \param [in] Attribs - Structure describing build TLAS command attributes, see Diligent::BuildTLASAttribs for details.
    ///
    /// \note Don't call build or copy operation on the same TLAS in a different contexts, because TLAS has CPU-side data
    ///       that will not match with GPU-side, so shader binding were incorrect.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(BuildTLAS)(THIS_
                                   const BuildTLASAttribs REF Attribs) PURE;


    /// Copies data from one acceleration structure to another.

    /// \param [in] Attribs - Structure describing copy BLAS command attributes, see Diligent::CopyBLASAttribs for details.
    ///
    /// \note Don't call build or copy operation on the same BLAS in a different contexts, because BLAS has CPU-side data
    ///       that will not match with GPU-side, so shader binding were incorrect.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(CopyBLAS)(THIS_
                                  const CopyBLASAttribs REF Attribs) PURE;


    /// Copies data from one acceleration structure to another.

    /// \param [in] Attribs - Structure describing copy TLAS command attributes, see Diligent::CopyTLASAttribs for details.
    ///
    /// \note Don't call build or copy operation on the same TLAS in a different contexts, because TLAS has CPU-side data
    ///       that will not match with GPU-side, so shader binding were incorrect.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(CopyTLAS)(THIS_
                                  const CopyTLASAttribs REF Attribs) PURE;


    /// Writes a bottom-level acceleration structure memory size required for compacting operation to a buffer.

    /// \param [in] Attribs - Structure describing write BLAS compacted size command attributes, see Diligent::WriteBLASCompactedSizeAttribs for details.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(WriteBLASCompactedSize)(THIS_
                                                const WriteBLASCompactedSizeAttribs REF Attribs) PURE;


    /// Writes a top-level acceleration structure memory size required for compacting operation to a buffer.

    /// \param [in] Attribs - Structure describing write TLAS compacted size command attributes, see Diligent::WriteTLASCompactedSizeAttribs for details.
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(WriteTLASCompactedSize)(THIS_
                                                const WriteTLASCompactedSizeAttribs REF Attribs) PURE;


    /// Executes a trace rays command.

    /// \param [in] Attribs - Trace rays command attributes, see Diligent::TraceRaysAttribs for details.
    ///
    /// \remarks  The method is not thread-safe. An application must externally synchronize the access
    ///           to the shader binding table (SBT) passed as an argument to the function.
    ///           The function does not modify the state of the SBT and can be executed in parallel with other
    ///           functions that don't modify the SBT (e.g. TraceRaysIndirect).
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(TraceRays)(THIS_
                                   const TraceRaysAttribs REF Attribs) PURE;


    /// Executes an indirect trace rays command.

    /// \param [in] Attribs - Indirect trace rays command attributes, see Diligent::TraceRaysIndirectAttribs.
    ///
    /// \remarks  The method is not thread-safe. An application must externally synchronize the access
    ///           to the shader binding table (SBT) passed as an argument to the function.
    ///           The function does not modify the state of the SBT and can be executed in parallel with other
    ///           functions that don't modify the SBT (e.g. TraceRays).
    ///
    /// \remarks Supported contexts: graphics, compute.
    VIRTUAL void METHOD(TraceRaysIndirect)(THIS_
                                           const TraceRaysIndirectAttribs REF Attribs) PURE;


    /// Updates SBT with the pending data that were recorded in IShaderBindingTable::Bind*** calls.

    /// \param [in] pSBT                         - Shader binding table that will be updated if there are pending data.
    /// \param [in] pUpdateIndirectBufferAttribs - Indirect ray tracing attributes buffer update attributes (optional, may be null).
    ///
    /// \note  When pUpdateIndirectBufferAttribs is not null, the indirect ray tracing attributes will be written to the pAttribsBuffer buffer
    ///        specified by the structure and can be used by IDeviceContext::TraceRaysIndirect() command.
    ///        In Direct3D12 backend, the pAttribsBuffer buffer will be initialized with D3D12_DISPATCH_RAYS_DESC structure that contains
    ///        GPU addresses of the ray tracing shaders in the first 88 bytes and 12 bytes for dimension
    ///        (see IDeviceContext::TraceRaysIndirect() description).
    ///        In Vulkan backend, the pAttribsBuffer buffer will not be modified, because the SBT is used directly
    ///        in IDeviceContext::TraceRaysIndirect().
    ///
    /// \remarks  The method is not thread-safe. An application must externally synchronize the access
    ///           to the shader binding table (SBT) passed as an argument to the function.
    ///           The function modifies the data in the SBT and must not run in parallel with any other command that uses the same SBT.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(UpdateSBT)(THIS_
                                   IShaderBindingTable*                 pSBT,
                                   const UpdateIndirectRTBufferAttribs* pUpdateIndirectBufferAttribs DEFAULT_INITIALIZER(nullptr)) PURE;


    /// Stores a pointer to the user-provided data object, which
    /// may later be retrieved through GetUserData().
    ///
    /// \param [in] pUserData - Pointer to the user data object to store.
    ///
    /// \note   The method is not thread-safe and an application
    ///         must externally synchronize the access.
    ///
    ///         The method keeps strong reference to the user data object.
    ///         If an application needs to release the object, it
    ///         should call SetUserData(nullptr);
    VIRTUAL void METHOD(SetUserData)(THIS_
                                     IObject* pUserData) PURE;


    /// Returns a pointer to the user data object previously
    /// set with SetUserData() method.
    ///
    /// \return     The pointer to the user data object.
    ///
    /// \remarks    The method does *NOT* call AddRef()
    ///             for the object being returned.
    VIRTUAL IObject* METHOD(GetUserData)(THIS) CONST PURE;


    /// Begins a debug group with name and color.
    /// External debug tools may use this information when displaying context commands.

    /// \param [in] Name   - Group name.
    /// \param [in] pColor - Region color.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    VIRTUAL void METHOD(BeginDebugGroup)(THIS_
                                        const Char*  Name,
                                        const float* pColor DEFAULT_INITIALIZER(nullptr)) PURE;

    /// Ends a debug group that was previously started with IDeviceContext::BeginDebugGroup.
    VIRTUAL void METHOD(EndDebugGroup)(THIS) PURE;


    /// Inserts a debug label with name and color.
    /// External debug tools may use this information when displaying context commands.

    /// \param [in] Label  - Label name.
    /// \param [in] pColor - Label color.
    ///
    /// \remarks Supported contexts: graphics, compute, transfer.
    ///          Not supported in Metal backend.
    VIRTUAL void METHOD(InsertDebugLabel)(THIS_
                                          const Char*  Label,
                                          const float* pColor DEFAULT_INITIALIZER(nullptr)) PURE;

    /// Locks the internal mutex and returns a pointer to the command queue that is associated with this device context.

    /// \return - a pointer to ICommandQueue interface of the command queue associated with the context.
    ///
    /// \remarks  Only immediate device contexts have associated command queues.
    ///
    ///           The engine locks the internal mutex to prevent simultaneous access to the command queue.
    ///           An application must release the lock by calling IDeviceContext::UnlockCommandQueue()
    ///           when it is done working with the queue or the engine will not be able to submit any command
    ///           list to the queue. Nested calls to LockCommandQueue() are not allowed.
    ///           The queue pointer never changes while the context is alive, so an application may cache and
    ///           use the pointer if it does not need to prevent potential simultaneous access to the queue from
    ///           other threads.
    ///
    ///           The engine manages the lifetimes of command queues and all other device objects,
    ///           so an application must not call AddRef/Release methods on the returned interface.
    VIRTUAL ICommandQueue* METHOD(LockCommandQueue)(THIS) PURE;

    /// Unlocks the command queue that was previously locked by IDeviceContext::LockCommandQueue().
    VIRTUAL void METHOD(UnlockCommandQueue)(THIS) PURE;


    /// Sets the shading base rate and combiners.

    /// \param [in] BaseRate          - Base shading rate used for combiner operations.
    /// \param [in] PrimitiveCombiner - Combiner operation for the per primitive shading rate (the output of the vertex or geometry shader).
    /// \param [in] TextureCombiner   - Combiner operation for texture-based shading rate (fetched from the shading rate texture),
    ///                                 see SetRenderTargetsAttribs::pShadingRateMap and SubpassDesc::pShadingRateAttachment.
    ///
    /// \remarks  The final shading rate is calculated before the triangle is rasterized by the following algorithm:
    ///               PrimitiveRate = ApplyCombiner(PrimitiveCombiner, BaseRate, PerPrimitiveRate)
    ///               FinalRate     = ApplyCombiner(TextureCombiner, PrimitiveRate, TextureRate)
    ///           Where
    ///               PerPrimitiveRate - vertex shader output value (HLSL: SV_ShadingRate; GLSL: gl_PrimitiveShadingRateEXT).
    ///               TextureRate      - texel value from the shading rate texture, see SetRenderTargetsAttribs::pShadingRateMap.
    ///
    ///               SHADING_RATE ApplyCombiner(SHADING_RATE_COMBINER Combiner, SHADING_RATE OriginalRate, SHADING_RATE NewRate)
    ///               {
    ///                   switch (Combiner)
    ///                   {
    ///                       case SHADING_RATE_COMBINER_PASSTHROUGH: return OriginalRate;
    ///                       case SHADING_RATE_COMBINER_OVERRIDE:    return NewRate;
    ///                       case SHADING_RATE_COMBINER_MIN:         return Min(OriginalRate, NewRate);
    ///                       case SHADING_RATE_COMBINER_MAX:         return Max(OriginalRate, NewRate);
    ///                       case SHADING_RATE_COMBINER_SUM:         return OriginalRate + NewRate;
    ///                       case SHADING_RATE_COMBINER_MUL:         return OriginalRate * NewRate;
    ///                   }
    ///               }
    ///
    /// \remarks If SHADING_RATE_CAP_FLAG_PER_PRIMITIVE capability is not supported, then PrimitiveCombiner must be SHADING_RATE_COMBINER_PASSTHROUGH.
    ///          if SHADING_RATE_CAP_FLAG_TEXTURE_BASED capability is not supported, then TextureCombiner must be SHADING_RATE_COMBINER_PASSTHROUGH.
    ///          TextureCombiner must be one of the supported combiners in ShadingRateProperties::Combiners.
    ///          BaseRate must be SHADING_RATE_1X1 or one of the supported rates in ShadingRateProperties::ShadingRates.
    ///
    /// \remarks Supported contexts: graphics.
    VIRTUAL void METHOD(SetShadingRate)(THIS_
                                        SHADING_RATE          BaseRate,
                                        SHADING_RATE_COMBINER PrimitiveCombiner,
                                        SHADING_RATE_COMBINER TextureCombiner) PURE;


    /// Binds or unbinds memory objects to sparse buffer and sparse textures.

    /// \param [in] Attribs - command attributes, see Diligent::BindSparseResourceMemoryAttribs.
    ///
    /// \remarks Metal backend:
    ///            Since resource uses a single preallocated memory storage,
    ///            memory offset is ignored and the driver acquires any free memory block from
    ///            the storage.
    ///            If there is no free memory in the storage, the region remains unbound.
    ///            Access to the device memory object on the GPU side
    ///            must be explicitly synchronized using fences.
    ///
    /// \note    Direct3D12, Vulkan and Metal backends require explicitly synchronizing
    ///          access to the resource using fences or by IDeviceContext::WaitForIdle().
    ///
    /// \remarks This command implicitly calls Flush().
    ///
    /// \remarks This command may only be executed by immediate context whose
    ///          internal queue supports COMMAND_QUEUE_TYPE_SPARSE_BINDING.
    VIRTUAL void METHOD(BindSparseResourceMemory)(THIS_
                                                  const BindSparseResourceMemoryAttribs REF Attribs) PURE;

    /// Clears the device context statistics.
    VIRTUAL void METHOD(ClearStats)(THIS) PURE;

    /// Returns the device context statistics, see Diligent::DeviceContextStats.
    VIRTUAL const DeviceContextStats REF METHOD(GetStats)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IDeviceContext_GetDesc(This)                            CALL_IFACE_METHOD(DeviceContext, GetDesc,                   This)
#    define IDeviceContext_Begin(This, ...)                         CALL_IFACE_METHOD(DeviceContext, Begin,                     This, __VA_ARGS__)
#    define IDeviceContext_SetPipelineState(This, ...)              CALL_IFACE_METHOD(DeviceContext, SetPipelineState,          This, __VA_ARGS__)
#    define IDeviceContext_TransitionShaderResources(This, ...)     CALL_IFACE_METHOD(DeviceContext, TransitionShaderResources, This, __VA_ARGS__)
#    define IDeviceContext_CommitShaderResources(This, ...)         CALL_IFACE_METHOD(DeviceContext, CommitShaderResources,     This, __VA_ARGS__)
#    define IDeviceContext_SetStencilRef(This, ...)                 CALL_IFACE_METHOD(DeviceContext, SetStencilRef,             This, __VA_ARGS__)
#    define IDeviceContext_SetBlendFactors(This, ...)               CALL_IFACE_METHOD(DeviceContext, SetBlendFactors,           This, __VA_ARGS__)
#    define IDeviceContext_SetVertexBuffers(This, ...)              CALL_IFACE_METHOD(DeviceContext, SetVertexBuffers,          This, __VA_ARGS__)
#    define IDeviceContext_InvalidateState(This)                    CALL_IFACE_METHOD(DeviceContext, InvalidateState,           This)
#    define IDeviceContext_SetIndexBuffer(This, ...)                CALL_IFACE_METHOD(DeviceContext, SetIndexBuffer,            This, __VA_ARGS__)
#    define IDeviceContext_SetViewports(This, ...)                  CALL_IFACE_METHOD(DeviceContext, SetViewports,              This, __VA_ARGS__)
#    define IDeviceContext_SetScissorRects(This, ...)               CALL_IFACE_METHOD(DeviceContext, SetScissorRects,           This, __VA_ARGS__)
#    define IDeviceContext_SetRenderTargets(This, ...)              CALL_IFACE_METHOD(DeviceContext, SetRenderTargets,          This, __VA_ARGS__)
#    define IDeviceContext_SetRenderTargetsExt(This, ...)           CALL_IFACE_METHOD(DeviceContext, SetRenderTargetsExt,       This, __VA_ARGS__)
#    define IDeviceContext_BeginRenderPass(This, ...)               CALL_IFACE_METHOD(DeviceContext, BeginRenderPass,           This, __VA_ARGS__)
#    define IDeviceContext_NextSubpass(This)                        CALL_IFACE_METHOD(DeviceContext, NextSubpass,               This)
#    define IDeviceContext_EndRenderPass(This)                      CALL_IFACE_METHOD(DeviceContext, EndRenderPass,             This)
#    define IDeviceContext_Draw(This, ...)                          CALL_IFACE_METHOD(DeviceContext, Draw,                      This, __VA_ARGS__)
#    define IDeviceContext_DrawIndexed(This, ...)                   CALL_IFACE_METHOD(DeviceContext, DrawIndexed,               This, __VA_ARGS__)
#    define IDeviceContext_DrawIndirect(This, ...)                  CALL_IFACE_METHOD(DeviceContext, DrawIndirect,              This, __VA_ARGS__)
#    define IDeviceContext_DrawIndexedIndirect(This, ...)           CALL_IFACE_METHOD(DeviceContext, DrawIndexedIndirect,       This, __VA_ARGS__)
#    define IDeviceContext_DrawMesh(This, ...)                      CALL_IFACE_METHOD(DeviceContext, DrawMesh,                  This, __VA_ARGS__)
#    define IDeviceContext_DrawMeshIndirect(This, ...)              CALL_IFACE_METHOD(DeviceContext, DrawMeshIndirect,          This, __VA_ARGS__)
#    define IDeviceContext_DispatchCompute(This, ...)               CALL_IFACE_METHOD(DeviceContext, DispatchCompute,           This, __VA_ARGS__)
#    define IDeviceContext_DispatchComputeIndirect(This, ...)       CALL_IFACE_METHOD(DeviceContext, DispatchComputeIndirect,   This, __VA_ARGS__)
#    define IDeviceContext_DispatchTile(This, ...)                  CALL_IFACE_METHOD(DeviceContext, DispatchTile,              This, __VA_ARGS__)
#    define IDeviceContext_GetTileSize(This, ...)                   CALL_IFACE_METHOD(DeviceContext, GetTileSize,               This, __VA_ARGS__)
#    define IDeviceContext_ClearDepthStencil(This, ...)             CALL_IFACE_METHOD(DeviceContext, ClearDepthStencil,         This, __VA_ARGS__)
#    define IDeviceContext_ClearRenderTarget(This, ...)             CALL_IFACE_METHOD(DeviceContext, ClearRenderTarget,         This, __VA_ARGS__)
#    define IDeviceContext_FinishCommandList(This, ...)             CALL_IFACE_METHOD(DeviceContext, FinishCommandList,         This, __VA_ARGS__)
#    define IDeviceContext_ExecuteCommandLists(This, ...)           CALL_IFACE_METHOD(DeviceContext, ExecuteCommandLists,       This, __VA_ARGS__)
#    define IDeviceContext_EnqueueSignal(This, ...)                 CALL_IFACE_METHOD(DeviceContext, EnqueueSignal,             This, __VA_ARGS__)
#    define IDeviceContext_DeviceWaitForFence(This, ...)            CALL_IFACE_METHOD(DeviceContext, DeviceWaitForFence,        This, __VA_ARGS__)
#    define IDeviceContext_WaitForIdle(This)                        CALL_IFACE_METHOD(DeviceContext, WaitForIdle,               This)
#    define IDeviceContext_BeginQuery(This, ...)                    CALL_IFACE_METHOD(DeviceContext, BeginQuery,                This, __VA_ARGS__)
#    define IDeviceContext_EndQuery(This, ...)                      CALL_IFACE_METHOD(DeviceContext, EndQuery,                  This, __VA_ARGS__)
#    define IDeviceContext_Flush(This)                              CALL_IFACE_METHOD(DeviceContext, Flush,                     This)
#    define IDeviceContext_UpdateBuffer(This, ...)                  CALL_IFACE_METHOD(DeviceContext, UpdateBuffer,              This, __VA_ARGS__)
#    define IDeviceContext_CopyBuffer(This, ...)                    CALL_IFACE_METHOD(DeviceContext, CopyBuffer,                This, __VA_ARGS__)
#    define IDeviceContext_MapBuffer(This, ...)                     CALL_IFACE_METHOD(DeviceContext, MapBuffer,                 This, __VA_ARGS__)
#    define IDeviceContext_UnmapBuffer(This, ...)                   CALL_IFACE_METHOD(DeviceContext, UnmapBuffer,               This, __VA_ARGS__)
#    define IDeviceContext_UpdateTexture(This, ...)                 CALL_IFACE_METHOD(DeviceContext, UpdateTexture,             This, __VA_ARGS__)
#    define IDeviceContext_CopyTexture(This, ...)                   CALL_IFACE_METHOD(DeviceContext, CopyTexture,               This, __VA_ARGS__)
#    define IDeviceContext_MapTextureSubresource(This, ...)         CALL_IFACE_METHOD(DeviceContext, MapTextureSubresource,     This, __VA_ARGS__)
#    define IDeviceContext_UnmapTextureSubresource(This, ...)       CALL_IFACE_METHOD(DeviceContext, UnmapTextureSubresource,   This, __VA_ARGS__)
#    define IDeviceContext_GenerateMips(This, ...)                  CALL_IFACE_METHOD(DeviceContext, GenerateMips,              This, __VA_ARGS__)
#    define IDeviceContext_FinishFrame(This)                        CALL_IFACE_METHOD(DeviceContext, FinishFrame,               This)
#    define IDeviceContext_GetFrameNumber(This)                     CALL_IFACE_METHOD(DeviceContext, GetFrameNumber,            This)
#    define IDeviceContext_TransitionResourceStates(This, ...)      CALL_IFACE_METHOD(DeviceContext, TransitionResourceStates,  This, __VA_ARGS__)
#    define IDeviceContext_ResolveTextureSubresource(This, ...)     CALL_IFACE_METHOD(DeviceContext, ResolveTextureSubresource, This, __VA_ARGS__)
#    define IDeviceContext_BuildBLAS(This, ...)                     CALL_IFACE_METHOD(DeviceContext, BuildBLAS,                 This, __VA_ARGS__)
#    define IDeviceContext_BuildTLAS(This, ...)                     CALL_IFACE_METHOD(DeviceContext, BuildTLAS,                 This, __VA_ARGS__)
#    define IDeviceContext_CopyBLAS(This, ...)                      CALL_IFACE_METHOD(DeviceContext, CopyBLAS,                  This, __VA_ARGS__)
#    define IDeviceContext_CopyTLAS(This, ...)                      CALL_IFACE_METHOD(DeviceContext, CopyTLAS,                  This, __VA_ARGS__)
#    define IDeviceContext_WriteBLASCompactedSize(This, ...)        CALL_IFACE_METHOD(DeviceContext, WriteBLASCompactedSize,    This, __VA_ARGS__)
#    define IDeviceContext_WriteTLASCompactedSize(This, ...)        CALL_IFACE_METHOD(DeviceContext, WriteTLASCompactedSize,    This, __VA_ARGS__)
#    define IDeviceContext_TraceRays(This, ...)                     CALL_IFACE_METHOD(DeviceContext, TraceRays,                 This, __VA_ARGS__)
#    define IDeviceContext_TraceRaysIndirect(This, ...)             CALL_IFACE_METHOD(DeviceContext, TraceRaysIndirect,         This, __VA_ARGS__)
#    define IDeviceContext_UpdateSBT(This, ...)                     CALL_IFACE_METHOD(DeviceContext, UpdateSBT,                 This, __VA_ARGS__)
#    define IDeviceContext_SetUserData(This, ...)                   CALL_IFACE_METHOD(DeviceContext, SetUserData,               This, __VA_ARGS__)
#    define IDeviceContext_GetUserData(This)                        CALL_IFACE_METHOD(DeviceContext, GetUserData,               This)
#    define IDeviceContext_BeginDebugGroup(This, ...)               CALL_IFACE_METHOD(DeviceContext, BeginDebugGroup,           This, __VA_ARGS__)
#    define IDeviceContext_EndDebugGroup(This)                      CALL_IFACE_METHOD(DeviceContext, EndDebugGroup,             This)
#    define IDeviceContext_InsertDebugLabel(This, ...)              CALL_IFACE_METHOD(DeviceContext, InsertDebugLabel,          This, __VA_ARGS__)
#    define IDeviceContext_LockCommandQueue(This)                   CALL_IFACE_METHOD(DeviceContext, LockCommandQueue,          This)
#    define IDeviceContext_UnlockCommandQueue(This)                 CALL_IFACE_METHOD(DeviceContext, UnlockCommandQueue,        This)
#    define IDeviceContext_SetShadingRate(This, ...)                CALL_IFACE_METHOD(DeviceContext, SetShadingRate,            This, __VA_ARGS__)
#    define IDeviceContext_BindSparseResourceMemory(This, ...)      CALL_IFACE_METHOD(DeviceContext, BindSparseResourceMemory,  This, __VA_ARGS__)
#    define IDeviceContext_ClearStats(This)                         CALL_IFACE_METHOD(DeviceContext, ClearStats,                This)
#    define IDeviceContext_GetStats(This)                           CALL_IFACE_METHOD(DeviceContext, GetStats,                  This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
