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
/// Definition of the Diligent::IPipelineState interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Platforms/interface/PlatformDefinitions.h"
#include "../../../Common/interface/StringTools.h"
#include "GraphicsTypes.h"
#include "BlendState.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "InputLayout.h"
#include "ShaderResourceBinding.h"
#include "ShaderResourceVariable.h"
#include "Shader.h"
#include "Sampler.h"
#include "RenderPass.h"
#include "PipelineResourceSignature.h"
#include "PipelineStateCache.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


/// Sample description

/// This structure is used by GraphicsPipelineDesc to describe multisampling parameters
struct SampleDesc
{
    /// Sample count
    Uint8 Count     DEFAULT_INITIALIZER(1);

    /// Quality
    Uint8 Quality   DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    constexpr SampleDesc() noexcept {}

    constexpr SampleDesc(Uint8 _Count, Uint8 _Quality) noexcept :
        Count   {_Count  },
        Quality {_Quality}
    {}

    /// Comparison operator tests if two structures are equivalent

    /// \param [in] RHS - reference to the structure to perform comparison with
    /// \return
    /// - True if all members of the two structures are equal.
    /// - False otherwise.
    constexpr bool operator==(const SampleDesc& RHS) const
    {
        return Count == RHS.Count && Quality == RHS.Quality;
    }
    constexpr bool operator!=(const SampleDesc& RHS) const
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct SampleDesc SampleDesc;


/// Shader variable property flags.
DILIGENT_TYPED_ENUM(SHADER_VARIABLE_FLAGS, Uint8)
{
    /// Shader variable has no special properties.
    SHADER_VARIABLE_FLAG_NONE               = 0,

    /// Indicates that dynamic buffers will never be bound to the resource
    /// variable. Applies to SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
    /// SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_TYPE_BUFFER_SRV resources.
    ///
    /// \remarks    This flag directly translates to the PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS
    ///             flag in the internal pipeline resource signature.
    SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS = 1u << 0,

    /// Indicates that the resource is an input attachment in general layout, which allows simultaneously
    /// reading from the resource through the input attachment and writing to it via color or depth-stencil
    /// attachment.
    ///
    /// \note This flag is only valid in Vulkan.
    SHADER_VARIABLE_FLAG_GENERAL_INPUT_ATTACHMENT_VK = 1u << 1,

    /// Indicates that the resource is an unfilterable-float texture.
    ///
    /// \note This flag is only valid in WebGPU and ignored in other backends.
    SHADER_VARIABLE_FLAG_UNFILTERABLE_FLOAT_TEXTURE_WEBGPU = 1u << 2,

    /// Indicates that the resource is a non-filtering sampler.
    ///
    /// \note This flag is only valid in WebGPU and ignored in other backends.
    SHADER_VARIABLE_FLAG_NON_FILTERING_SAMPLER_WEBGPU = 1u << 3,

    SHADER_VARIABLE_FLAG_LAST = SHADER_VARIABLE_FLAG_NON_FILTERING_SAMPLER_WEBGPU
};
DEFINE_FLAG_ENUM_OPERATORS(SHADER_VARIABLE_FLAGS);


/// Describes shader variable
struct ShaderResourceVariableDesc
{
    /// Shader variable name
    const Char*                   Name         DEFAULT_INITIALIZER(nullptr);

    /// Shader stages this resources variable applies to. If more than one shader stage is specified,
    /// the variable will be shared between these stages. Shader stages used by different variables
    /// with the same name must not overlap.
    SHADER_TYPE                   ShaderStages DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Shader variable type. See Diligent::SHADER_RESOURCE_VARIABLE_TYPE for a list of allowed types
    SHADER_RESOURCE_VARIABLE_TYPE Type         DEFAULT_INITIALIZER(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

    SHADER_VARIABLE_FLAGS         Flags        DEFAULT_INITIALIZER(SHADER_VARIABLE_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr ShaderResourceVariableDesc() noexcept {}

    constexpr ShaderResourceVariableDesc(SHADER_TYPE                   _ShaderStages,
                                         const Char*                   _Name,
                                         SHADER_RESOURCE_VARIABLE_TYPE _Type,
                                         SHADER_VARIABLE_FLAGS         _Flags = SHADER_VARIABLE_FLAG_NONE) noexcept :
        Name        {_Name        },
        ShaderStages{_ShaderStages},
        Type        {_Type        },
        Flags       {_Flags       }
    {}

    /// Comparison operator tests if two structures are equivalent

    /// \param [in] RHS - reference to the structure to perform comparison with
    /// \return
    /// - True if all members of the two structures are equal.
    /// - False otherwise.
    bool operator==(const ShaderResourceVariableDesc& RHS) const noexcept
    {
        return ShaderStages == RHS.ShaderStages &&
               Type         == RHS.Type         &&
               Flags        == RHS.Flags        &&
               SafeStrEqual(Name, RHS.Name);
    }

    bool operator!=(const ShaderResourceVariableDesc& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct ShaderResourceVariableDesc ShaderResourceVariableDesc;


/// Pipeline state shading rate flags.
DILIGENT_TYPED_ENUM(PIPELINE_SHADING_RATE_FLAGS, Uint8)
{
    /// Shading rate is not used.
    PIPELINE_SHADING_RATE_FLAG_NONE          = 0,

    /// Indicates that the pipeline state will be used with per draw or per primitive shading rate.
    /// See IDeviceContext::SetShadingRate().
    PIPELINE_SHADING_RATE_FLAG_PER_PRIMITIVE = 1u << 0u,

    /// Indicates that the pipeline state will be used with texture-based shading rate.
    /// See IDeviceContext::SetShadingRate() and IDeviceContext::SetRenderTargetsExt().
    PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED = 1u << 1u,

    PIPELINE_SHADING_RATE_FLAG_LAST          = PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED,
};
DEFINE_FLAG_ENUM_OPERATORS(PIPELINE_SHADING_RATE_FLAGS);

/// Pipeline layout description
struct PipelineResourceLayoutDesc
{
    /// Default shader resource variable type. This type will be used if shader
    /// variable description is not found in the Variables array
    /// or if Variables == nullptr
    SHADER_RESOURCE_VARIABLE_TYPE       DefaultVariableType  DEFAULT_INITIALIZER(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

    /// By default, all variables not found in the Variables array define separate resources.
    /// For example, if there is resource "g_Texture" in the vertex and pixel shader stages, there
    /// will be two separate resources in both stages. This member defines shader stages
    /// in which default variables will be combined.
    /// For example, if DefaultVariableMergeStages == SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL,
    /// then both resources in the example above will be combined into a single one.
    /// If there is another "g_Texture" in geometry shader, it will be separate from combined
    /// vertex-pixel "g_Texture".
    /// This member has no effect on variables defined in Variables array.
    SHADER_TYPE                         DefaultVariableMergeStages DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Number of elements in Variables array
    Uint32                              NumVariables         DEFAULT_INITIALIZER(0);

    /// Array of shader resource variable descriptions

    /// There may be multiple variables with the same name that use different shader stages,
    /// but the stages must not overlap.
    const ShaderResourceVariableDesc*   Variables            DEFAULT_INITIALIZER(nullptr);

    /// Number of immutable samplers in ImmutableSamplers array
    Uint32                              NumImmutableSamplers DEFAULT_INITIALIZER(0);

    /// Array of immutable sampler descriptions
    const ImmutableSamplerDesc*         ImmutableSamplers    DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    /// Returns true if two resource layout descriptions are equal, and false otherwise.
    /// Optionally ignores variable descriptions and/or sampler descriptions.
    static bool IsEqual(const PipelineResourceLayoutDesc& Desc1,
                        const PipelineResourceLayoutDesc& Desc2,
                        bool                              IgnoreVariables = false,
                        bool                              IgnoreSamplers  = false)
    {
        if (!(Desc1.DefaultVariableType        == Desc2.DefaultVariableType        &&
              Desc1.DefaultVariableMergeStages == Desc2.DefaultVariableMergeStages &&
              Desc1.NumVariables               == Desc2.NumVariables               &&
              Desc1.NumImmutableSamplers       == Desc2.NumImmutableSamplers))
           return false;

        if (!IgnoreVariables)
        {
            for (Uint32 i = 0; i < Desc1.NumVariables; ++i)
                if (Desc1.Variables[i] != Desc2.Variables[i])
                    return false;
        }

        if (!IgnoreSamplers)
        {
            for (Uint32 i = 0; i < Desc1.NumImmutableSamplers; ++i)
                if (Desc1.ImmutableSamplers[i] != Desc2.ImmutableSamplers[i])
                    return false;
        }

        return true;
    }

    /// Comparison operator tests if two structures are equivalent
    bool operator==(const PipelineResourceLayoutDesc& RHS) const noexcept
    {
        return IsEqual(*this, RHS);
    }
    bool operator!=(const PipelineResourceLayoutDesc& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif

};
typedef struct PipelineResourceLayoutDesc PipelineResourceLayoutDesc;


/// Graphics pipeline state description

/// This structure describes the graphics pipeline state and is part of the GraphicsPipelineStateCreateInfo structure.
struct GraphicsPipelineDesc
{
    /// Blend state description.
    BlendStateDesc BlendDesc;

    /// 32-bit sample mask that determines which samples get updated
    /// in all the active render targets. A sample mask is always applied;
    /// it is independent of whether multisampling is enabled, and does not
    /// depend on whether an application uses multisample render targets.
    Uint32 SampleMask DEFAULT_INITIALIZER(0xFFFFFFFF);

    /// Rasterizer state description.
    RasterizerStateDesc RasterizerDesc;

    /// Depth-stencil state description.
    DepthStencilStateDesc DepthStencilDesc;

    /// Input layout, ignored in a mesh pipeline.
    InputLayoutDesc InputLayout;

    /// Primitive topology type, ignored in a mesh pipeline.
    PRIMITIVE_TOPOLOGY PrimitiveTopology DEFAULT_INITIALIZER(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    /// The number of viewports used by this pipeline
    Uint8 NumViewports           DEFAULT_INITIALIZER(1);

    /// The number of render targets in the RTVFormats array.
    /// Must be 0 when pRenderPass is not null.
    Uint8 NumRenderTargets       DEFAULT_INITIALIZER(0);

    /// When pRenderPass is not null, the subpass
    /// index within the render pass.
    /// When pRenderPass is null, this member must be 0.
    Uint8 SubpassIndex           DEFAULT_INITIALIZER(0);

    /// Shading rate flags that specify which type of the shading rate will be used with this pipeline.
    PIPELINE_SHADING_RATE_FLAGS ShadingRateFlags DEFAULT_INITIALIZER(PIPELINE_SHADING_RATE_FLAG_NONE);

    /// Render target formats.
    /// All formats must be TEX_FORMAT_UNKNOWN when pRenderPass is not null.
    TEXTURE_FORMAT RTVFormats[DILIGENT_MAX_RENDER_TARGETS] DEFAULT_INITIALIZER({});

    /// Depth-stencil format.
    /// Must be TEX_FORMAT_UNKNOWN when pRenderPass is not null.
    TEXTURE_FORMAT DSVFormat     DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Indicates that the pipeline will be used with read-only depth-stencil buffer.
    /// Must be false when pRenderPass is not null.
    Bool ReadOnlyDSV             DEFAULT_INITIALIZER(False);

    /// Multisampling parameters.
    SampleDesc SmplDesc;

    /// Pointer to the render pass object.

    /// When non-null render pass is specified, NumRenderTargets must be 0,
    /// and all RTV formats as well as DSV format must be TEX_FORMAT_UNKNOWN.
    IRenderPass* pRenderPass     DEFAULT_INITIALIZER(nullptr);

    /// Node mask.
    Uint32 NodeMask DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    bool operator==(const GraphicsPipelineDesc& Rhs) const noexcept
    {
        if (!(BlendDesc         == Rhs.BlendDesc         &&
              SampleMask        == Rhs.SampleMask        &&
              RasterizerDesc    == Rhs.RasterizerDesc    &&
              DepthStencilDesc  == Rhs.DepthStencilDesc  &&
              InputLayout       == Rhs.InputLayout       &&
              PrimitiveTopology == Rhs.PrimitiveTopology &&
              NumViewports      == Rhs.NumViewports      &&
              NumRenderTargets  == Rhs.NumRenderTargets  &&
              SubpassIndex      == Rhs.SubpassIndex      &&
              ShadingRateFlags  == Rhs.ShadingRateFlags  &&
              DSVFormat         == Rhs.DSVFormat         &&
              ReadOnlyDSV       == Rhs.ReadOnlyDSV    &&
              SmplDesc          == Rhs.SmplDesc          &&
              NodeMask          == Rhs.NodeMask))
            return false;

        for (Uint32 i = 0; i < NumRenderTargets; ++i)
        {
            if (RTVFormats[i] != Rhs.RTVFormats[i])
                return false;
        }

        if ((pRenderPass != nullptr) != (Rhs.pRenderPass != nullptr))
            return false;

        if (pRenderPass != nullptr)
        {
            if (!(pRenderPass->GetDesc() == Rhs.pRenderPass->GetDesc()))
                return false;
        }
        return true;
    }
    bool operator!=(const GraphicsPipelineDesc& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct GraphicsPipelineDesc GraphicsPipelineDesc;


/// Ray tracing general shader group description
struct RayTracingGeneralShaderGroup
{
    /// Unique group name.
    const Char* Name    DEFAULT_INITIALIZER(nullptr);

    /// Shader type must be SHADER_TYPE_RAY_GEN, SHADER_TYPE_RAY_MISS or SHADER_TYPE_CALLABLE.
    IShader*    pShader DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    constexpr RayTracingGeneralShaderGroup() noexcept
    {}

    constexpr RayTracingGeneralShaderGroup(const Char* _Name,
                                           IShader*    _pShader) noexcept:
        Name   {_Name   },
        pShader{_pShader}
    {}

    bool operator==(const RayTracingGeneralShaderGroup& Rhs) const noexcept
    {
        return SafeStrEqual(Name, Rhs.Name) && pShader == Rhs.pShader;
    }
    bool operator!=(const RayTracingGeneralShaderGroup& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct RayTracingGeneralShaderGroup RayTracingGeneralShaderGroup;

/// Ray tracing triangle hit shader group description.
struct RayTracingTriangleHitShaderGroup
{
    /// Unique group name.
    const Char* Name              DEFAULT_INITIALIZER(nullptr);

    /// Closest hit shader.
    /// The shader type must be SHADER_TYPE_RAY_CLOSEST_HIT.
    IShader*    pClosestHitShader DEFAULT_INITIALIZER(nullptr);

    /// Any-hit shader. Can be null.
    /// The shader type must be SHADER_TYPE_RAY_ANY_HIT.
    IShader*    pAnyHitShader     DEFAULT_INITIALIZER(nullptr); // can be null

#if DILIGENT_CPP_INTERFACE
    constexpr RayTracingTriangleHitShaderGroup() noexcept
    {}

    constexpr RayTracingTriangleHitShaderGroup(const Char* _Name,
                                               IShader*    _pClosestHitShader,
                                               IShader*    _pAnyHitShader    = nullptr) noexcept:
        Name             {_Name             },
        pClosestHitShader{_pClosestHitShader},
        pAnyHitShader    {_pAnyHitShader    }
    {}

    bool operator==(const RayTracingTriangleHitShaderGroup& Rhs) const noexcept
    {
        return SafeStrEqual(Name, Rhs.Name) &&
            pClosestHitShader == Rhs.pClosestHitShader &&
            pAnyHitShader     == Rhs.pAnyHitShader;
    }
    bool operator!=(const RayTracingTriangleHitShaderGroup& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct RayTracingTriangleHitShaderGroup RayTracingTriangleHitShaderGroup;

/// Ray tracing procedural hit shader group description.
struct RayTracingProceduralHitShaderGroup
{
    /// Unique group name.
    const Char* Name                DEFAULT_INITIALIZER(nullptr);

    /// Intersection shader.
    /// The shader type must be SHADER_TYPE_RAY_INTERSECTION.
    IShader*    pIntersectionShader DEFAULT_INITIALIZER(nullptr);

    /// Closest hit shader. Can be null.
    /// The shader type must be SHADER_TYPE_RAY_CLOSEST_HIT.
    IShader*    pClosestHitShader   DEFAULT_INITIALIZER(nullptr);

    /// Any-hit shader. Can be null.
    /// The shader type must be SHADER_TYPE_RAY_ANY_HIT.
    IShader*    pAnyHitShader       DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    constexpr RayTracingProceduralHitShaderGroup() noexcept
    {}

    constexpr RayTracingProceduralHitShaderGroup(const Char* _Name,
                                                 IShader*    _pIntersectionShader,
                                                 IShader*    _pClosestHitShader  = nullptr,
                                                 IShader*    _pAnyHitShader      = nullptr) noexcept:
        Name               {_Name               },
        pIntersectionShader{_pIntersectionShader},
        pClosestHitShader  {_pClosestHitShader  },
        pAnyHitShader      {_pAnyHitShader      }
    {}

    bool operator==(const RayTracingProceduralHitShaderGroup& Rhs) const noexcept
    {
        return SafeStrEqual(Name, Rhs.Name) &&
            pIntersectionShader == Rhs.pIntersectionShader &&
            pClosestHitShader   == Rhs.pClosestHitShader   &&
            pAnyHitShader       == Rhs.pAnyHitShader;
    }
    bool operator!=(const RayTracingProceduralHitShaderGroup& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct RayTracingProceduralHitShaderGroup RayTracingProceduralHitShaderGroup;

/// This structure describes the ray tracing pipeline state and is part of the RayTracingPipelineStateCreateInfo structure.
struct RayTracingPipelineDesc
{
    /// Size of the additional data passed to the shader.
    /// Shader record size plus shader group size (32 bytes) must be aligned to 32 bytes.
    /// Shader record size plus shader group size (32 bytes) must not exceed 4096 bytes.
    Uint16  ShaderRecordSize   DEFAULT_INITIALIZER(0);

    /// Number of recursive calls of TraceRay() in HLSL or traceRay() in GLSL.
    /// Zero means no tracing of rays at all, only ray-gen shader will be executed.
    /// See DeviceProperties::MaxRayTracingRecursionDepth.
    Uint8   MaxRecursionDepth  DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    /// Comparison operator tests if two structures are equivalent
    constexpr bool operator==(const RayTracingPipelineDesc& RHS) const
    {
        return ShaderRecordSize  == RHS.ShaderRecordSize &&
               MaxRecursionDepth == RHS.MaxRecursionDepth;
    }
    constexpr bool operator!=(const RayTracingPipelineDesc& RHS) const
    {
        return !(*this == RHS);
    }
#endif

};
typedef struct RayTracingPipelineDesc RayTracingPipelineDesc;

/// Pipeline type
DILIGENT_TYPED_ENUM(PIPELINE_TYPE, Uint8)
{
    /// Graphics pipeline, which is used by IDeviceContext::Draw(), IDeviceContext::DrawIndexed(),
    /// IDeviceContext::DrawIndirect(), IDeviceContext::DrawIndexedIndirect().
    PIPELINE_TYPE_GRAPHICS,

    /// Compute pipeline, which is used by IDeviceContext::DispatchCompute(), IDeviceContext::DispatchComputeIndirect().
    PIPELINE_TYPE_COMPUTE,

    /// Mesh pipeline, which is used by IDeviceContext::DrawMesh(), IDeviceContext::DrawMeshIndirect().
    PIPELINE_TYPE_MESH,

    /// Ray tracing pipeline, which is used by IDeviceContext::TraceRays().
    PIPELINE_TYPE_RAY_TRACING,

    /// Tile pipeline, which is used by IDeviceContext::DispatchTile().
    PIPELINE_TYPE_TILE,

    PIPELINE_TYPE_LAST = PIPELINE_TYPE_TILE,

    PIPELINE_TYPE_COUNT,

    PIPELINE_TYPE_INVALID = 0xFF
};


/// Pipeline state description
struct PipelineStateDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Pipeline type
    PIPELINE_TYPE PipelineType      DEFAULT_INITIALIZER(PIPELINE_TYPE_GRAPHICS);

    /// Shader resource binding allocation granularity

    /// This member defines allocation granularity for internal resources required by the shader resource
    /// binding object instances.
    /// Has no effect if the PSO is created with explicit pipeline resource signature(s).
    Uint32 SRBAllocationGranularity DEFAULT_INITIALIZER(1);

    /// Defines which immediate contexts are allowed to execute commands that use this pipeline state.

    /// When ImmediateContextMask contains a bit at position n, the pipeline state may be
    /// used in the immediate context with index n directly (see DeviceContextDesc::ContextId).
    /// It may also be used in a command list recorded by a deferred context that will be executed
    /// through that immediate context.
    ///
    /// \remarks    Only specify these bits that will indicate those immediate contexts where the PSO
    ///             will actually be used. Do not set unnecessary bits as this will result in extra overhead.
    Uint64 ImmediateContextMask     DEFAULT_INITIALIZER(1);

    /// Pipeline layout description
    PipelineResourceLayoutDesc ResourceLayout;

#if DILIGENT_CPP_INTERFACE
    constexpr PipelineStateDesc() noexcept {}

    explicit constexpr PipelineStateDesc(const Char* _Name, PIPELINE_TYPE Type = PipelineStateDesc{}.PipelineType) :
        DeviceObjectAttribs{_Name},
        PipelineType       {Type  }
    {}

    /// Tests if two pipeline state descriptions are equal.

    /// \param [in] RHS - reference to the structure to compare with.
    ///
    /// \return     true if all members of the two structures *except for the Name* are equal,
    ///             and false otherwise.
    ///
    /// \note   The operator ignores the Name field as it is used for debug purposes and
    ///         doesn't affect the pipeline state properties.
    bool operator==(const PipelineStateDesc& RHS) const noexcept
    {
        // Ignore Name. This is consistent with the hasher (HashCombiner<HasherType, PipelineStateDesc>).
        return PipelineType             == RHS.PipelineType             &&
               SRBAllocationGranularity == RHS.SRBAllocationGranularity &&
               ImmediateContextMask     == RHS.ImmediateContextMask     &&
               ResourceLayout           == RHS.ResourceLayout;
    }
    bool operator!=(const PipelineStateDesc& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    bool IsAnyGraphicsPipeline() const { return PipelineType == PIPELINE_TYPE_GRAPHICS || PipelineType == PIPELINE_TYPE_MESH; }
    bool IsComputePipeline()     const { return PipelineType == PIPELINE_TYPE_COMPUTE; }
    bool IsRayTracingPipeline()  const { return PipelineType == PIPELINE_TYPE_RAY_TRACING; }
    bool IsTilePipeline()        const { return PipelineType == PIPELINE_TYPE_TILE; }
#endif
};
typedef struct PipelineStateDesc PipelineStateDesc;


/// Pipeline state creation flags
DILIGENT_TYPED_ENUM(PSO_CREATE_FLAGS, Uint32)
{
    /// Null flag.
    PSO_CREATE_FLAG_NONE                              = 0u,

    /// Ignore missing variables.

    /// By default, the engine outputs a warning for every variable
    /// provided as part of the pipeline resource layout description
    /// that is not found in any of the designated shader stages.
    /// Use this flag to silence these warnings.
    PSO_CREATE_FLAG_IGNORE_MISSING_VARIABLES          = 1u << 0u,

    /// Ignore missing immutable samplers.

    /// By default, the engine outputs a warning for every immutable sampler
    /// provided as part of the pipeline resource layout description
    /// that is not found in any of the designated shader stages.
    /// Use this flag to silence these warnings.
    PSO_CREATE_FLAG_IGNORE_MISSING_IMMUTABLE_SAMPLERS = 1u << 1u,

    /// Do not remap shader resources when creating the pipeline.
    /// Resource bindings in all shaders must match the bindings expected
    /// by the PSO's resource signatures.
    PSO_CREATE_FLAG_DONT_REMAP_SHADER_RESOURCES       = 1u << 2u,

    /// Create the pipeline state asynchronously.
    
    /// \remarks	When this flag is set to true and if the devices supports
    ///             AsyncShaderCompilation feature, the pipeline will be created
    ///             asynchronously in the background. An application should use
    ///             the IPipelineState::GetStatus() method to check the pipeline status.
    ///             If the device does not support asynchronous shader compilation,
    ///             the flag is ignored and the pipeline is created synchronously.
    PSO_CREATE_FLAG_ASYNCHRONOUS                      = 1u << 3u,

    PSO_CREATE_FLAG_LAST = PSO_CREATE_FLAG_ASYNCHRONOUS
};
DEFINE_FLAG_ENUM_OPERATORS(PSO_CREATE_FLAGS);


/// Pipeline state creation attributes
struct PipelineStateCreateInfo
{
    /// Pipeline state description
    PipelineStateDesc PSODesc;

    /// Pipeline state creation flags, see Diligent::PSO_CREATE_FLAGS.
    PSO_CREATE_FLAGS  Flags      DEFAULT_INITIALIZER(PSO_CREATE_FLAG_NONE);

    /// The number of elements in ppResourceSignatures array.
    Uint32 ResourceSignaturesCount DEFAULT_INITIALIZER(0);

    /// An array of ResourceSignaturesCount shader resource signatures that
    /// define the layout of shader resources in this pipeline state object.
    /// See Diligent::IPipelineResourceSignature.
    ///
    /// \remarks    When this member is null, the pipeline resource layout will be defined
    ///             by PSODesc.ResourceLayout member. In this case the PSO will implicitly
    ///             create a resource signature that can be queried through GetResourceSignature()
    ///             method.
    ///             When ppResourceSignatures is not null, PSODesc.ResourceLayout is ignored and
    ///             should be in it default state.
    IPipelineResourceSignature** ppResourceSignatures DEFAULT_INITIALIZER(nullptr);

    /// Optional pipeline state cache that is used to accelerate
    /// PSO creation. If PSODesc.Name is found in the cache, the cache
    /// data is used to create the PSO. Otherwise, the PSO
    /// is added to the cache.
    IPipelineStateCache* pPSOCache DEFAULT_INITIALIZER(nullptr);

    /// For internal use only. Must always be null.
    void* pInternalData DEFAULT_INITIALIZER(nullptr);

#ifdef DILIGENT_PLATFORM_32
    Uint32 _Padding DEFAULT_INITIALIZER(0);
#endif

#if DILIGENT_CPP_INTERFACE
    constexpr PipelineStateCreateInfo() noexcept {}

    constexpr PipelineStateCreateInfo(const Char* Name, PIPELINE_TYPE Type) :
        PSODesc{Name, Type}
    {}

    bool operator==(const PipelineStateCreateInfo& RHS) const noexcept
    {
        if (PSODesc                 != RHS.PSODesc ||
            Flags                   != RHS.Flags   ||
            ResourceSignaturesCount != RHS.ResourceSignaturesCount)
            return false;

        if (ppResourceSignatures != RHS.ppResourceSignatures)
        {
            if ((ppResourceSignatures == nullptr) != (RHS.ppResourceSignatures == nullptr))
                return false;

            for (Uint32 i = 0; i < ResourceSignaturesCount; ++i)
            {
                const auto* pSign0 = ppResourceSignatures[i];
                const auto* pSign1 = RHS.ppResourceSignatures[i];
                if (pSign0 == pSign1)
                    continue;

                if ((pSign0 == nullptr) != (pSign1 == nullptr))
                    return false;

                if (!pSign0->IsCompatibleWith(pSign1))
                    return false;
            }
        }

        // Ignore PSO cache and pInternalData

        return true;
    }
    bool operator!=(const PipelineStateCreateInfo& RHS) const noexcept
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct PipelineStateCreateInfo PipelineStateCreateInfo;


/// Graphics pipeline state initialization information.
struct GraphicsPipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)

    /// Graphics pipeline state description.
    GraphicsPipelineDesc GraphicsPipeline;

    /// Vertex shader to be used with the pipeline.
    IShader* pVS DEFAULT_INITIALIZER(nullptr);

    /// Pixel shader to be used with the pipeline.
    IShader* pPS DEFAULT_INITIALIZER(nullptr);

    /// Domain shader to be used with the pipeline.
    IShader* pDS DEFAULT_INITIALIZER(nullptr);

    /// Hull shader to be used with the pipeline.
    IShader* pHS DEFAULT_INITIALIZER(nullptr);

    /// Geometry shader to be used with the pipeline.
    IShader* pGS DEFAULT_INITIALIZER(nullptr);

    /// Amplification shader to be used with the pipeline.
    IShader* pAS DEFAULT_INITIALIZER(nullptr);

    /// Mesh shader to be used with the pipeline.
    IShader* pMS DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    constexpr GraphicsPipelineStateCreateInfo() noexcept 
    {
        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    }

    explicit constexpr GraphicsPipelineStateCreateInfo(const Char* Name) :
        PipelineStateCreateInfo{Name, PIPELINE_TYPE_GRAPHICS}
    {}

    bool operator==(const GraphicsPipelineStateCreateInfo& Rhs) const noexcept
    {
        if (static_cast<const PipelineStateCreateInfo&>(*this) != static_cast<const PipelineStateCreateInfo&>(Rhs))
            return false;

        if (GraphicsPipeline != Rhs.GraphicsPipeline)
            return false;

        return (pVS == Rhs.pVS &&
                pPS == Rhs.pPS &&
                pDS == Rhs.pDS &&
                pHS == Rhs.pHS &&
                pGS == Rhs.pGS &&
                pAS == Rhs.pAS &&
                pMS == Rhs.pMS);
    }
    bool operator!=(const GraphicsPipelineStateCreateInfo& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct GraphicsPipelineStateCreateInfo GraphicsPipelineStateCreateInfo;


/// Compute pipeline state description.
struct ComputePipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)

    /// Compute shader to be used with the pipeline
    IShader* pCS DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    ComputePipelineStateCreateInfo() noexcept
    {
        PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    }

    explicit constexpr ComputePipelineStateCreateInfo(const Char* Name) :
        PipelineStateCreateInfo{Name, PIPELINE_TYPE_COMPUTE}
    {}

    bool operator==(const ComputePipelineStateCreateInfo& Rhs) const noexcept
    {
        if (static_cast<const PipelineStateCreateInfo&>(*this) != static_cast<const PipelineStateCreateInfo&>(Rhs))
            return false;

        return pCS == Rhs.pCS;
    }
    bool operator!=(const ComputePipelineStateCreateInfo& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct ComputePipelineStateCreateInfo ComputePipelineStateCreateInfo;


/// Ray tracing pipeline state initialization information.
struct RayTracingPipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)

    /// Ray tracing pipeline description.
    RayTracingPipelineDesc                    RayTracingPipeline;

    /// A pointer to an array of GeneralShaderCount RayTracingGeneralShaderGroup structures that contain shader group description.
    const RayTracingGeneralShaderGroup*       pGeneralShaders          DEFAULT_INITIALIZER(nullptr);

    /// The number of general shader groups.
    Uint32                                    GeneralShaderCount       DEFAULT_INITIALIZER(0);

    /// A pointer to an array of TriangleHitShaderCount RayTracingTriangleHitShaderGroup structures that contain shader group description.
    /// Can be null.
    const RayTracingTriangleHitShaderGroup*   pTriangleHitShaders      DEFAULT_INITIALIZER(nullptr);

    /// The number of triangle hit shader groups.
    Uint32                                    TriangleHitShaderCount   DEFAULT_INITIALIZER(0);

    /// A pointer to an array of ProceduralHitShaderCount RayTracingProceduralHitShaderGroup structures that contain shader group description.
    /// Can be null.
    const RayTracingProceduralHitShaderGroup* pProceduralHitShaders    DEFAULT_INITIALIZER(nullptr);

    /// The number of procedural shader groups.
    Uint32                                    ProceduralHitShaderCount DEFAULT_INITIALIZER(0);

    /// Direct3D12 only: the name of the constant buffer that will be used by the local root signature.
    /// Ignored if RayTracingPipelineDesc::ShaderRecordSize is zero.
    /// In Vulkan backend in HLSL add [[vk::shader_record_ext]] attribute to the constant buffer, in GLSL add shaderRecord layout to buffer.
    const Char*                               pShaderRecordName        DEFAULT_INITIALIZER(nullptr);

    /// Direct3D12 only: the maximum hit shader attribute size in bytes.
    /// If zero then maximum allowed size will be used.
    Uint32                                    MaxAttributeSize         DEFAULT_INITIALIZER(0);

    /// Direct3D12 only: the maximum payload size in bytes.
    /// If zero then maximum allowed size will be used.
    Uint32                                    MaxPayloadSize           DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    RayTracingPipelineStateCreateInfo() noexcept
    {
        PSODesc.PipelineType = PIPELINE_TYPE_RAY_TRACING;
    }

    explicit constexpr RayTracingPipelineStateCreateInfo(const Char* Name) :
        PipelineStateCreateInfo{Name, PIPELINE_TYPE_RAY_TRACING}
    {}

    bool operator==(const RayTracingPipelineStateCreateInfo& RHS)const noexcept
    {
        if (static_cast<const PipelineStateCreateInfo&>(*this) != static_cast<const PipelineStateCreateInfo&>(RHS))
            return false;

        if (RayTracingPipeline       != RHS.RayTracingPipeline       ||
            GeneralShaderCount       != RHS.GeneralShaderCount       ||
            TriangleHitShaderCount   != RHS.TriangleHitShaderCount   ||
            ProceduralHitShaderCount != RHS.ProceduralHitShaderCount ||
            MaxAttributeSize         != RHS.MaxAttributeSize         ||
            MaxPayloadSize           != RHS.MaxPayloadSize)
            return false;

        if (!((IsNullOrEmptyStr(pShaderRecordName) && IsNullOrEmptyStr(RHS.pShaderRecordName)) || SafeStrEqual(pShaderRecordName, RHS.pShaderRecordName)))
            return false;

        for (Uint32 i = 0; i < GeneralShaderCount; ++i)
        {
            if (pGeneralShaders[i] != RHS.pGeneralShaders[i])
                return false;
        }
        for (Uint32 i = 0; i < TriangleHitShaderCount; ++i)
        {
            if (pTriangleHitShaders[i] != RHS.pTriangleHitShaders[i])
                return false;
        }
        for (Uint32 i = 0; i < ProceduralHitShaderCount; ++i)
        {
            if (pProceduralHitShaders[i] != RHS.pProceduralHitShaders[i])
                return false;
        }

        return true;
    }
    bool operator!=(const RayTracingPipelineStateCreateInfo& RHS)const noexcept
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct RayTracingPipelineStateCreateInfo RayTracingPipelineStateCreateInfo;



/// Tile pipeline state description
struct TilePipelineDesc
{
    /// The number of render targets in the RTVFormats array.
    Uint8 NumRenderTargets       DEFAULT_INITIALIZER(0);

    /// The number of samples in render targets.
    Uint8 SampleCount            DEFAULT_INITIALIZER(1);

    /// Render target formats.
    TEXTURE_FORMAT RTVFormats[DILIGENT_MAX_RENDER_TARGETS] DEFAULT_INITIALIZER({});

#if DILIGENT_CPP_INTERFACE
    /// Comparison operator tests if two structures are equivalent
    constexpr bool operator==(const TilePipelineDesc& RHS) const
    {
        if (!(NumRenderTargets == RHS.NumRenderTargets && SampleCount == RHS.SampleCount))
            return false;

        for (Uint32 i = 0; i < NumRenderTargets; i++)
            if (RTVFormats[i] != RHS.RTVFormats[i])
                return false;

        return true;
    }
    constexpr bool operator!=(const TilePipelineDesc& RHS) const
    {
        return !(*this == RHS);
    }
#endif
};
typedef struct TilePipelineDesc TilePipelineDesc;


/// Tile pipeline state initialization information.
struct TilePipelineStateCreateInfo DILIGENT_DERIVE(PipelineStateCreateInfo)

    /// Tile pipeline description, see Diligent::TilePipelineDesc.
    TilePipelineDesc TilePipeline;

    /// Tile shader to be used with the pipeline.
    IShader* pTS DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    TilePipelineStateCreateInfo() noexcept
    {
        PSODesc.PipelineType = PIPELINE_TYPE_TILE;
    }

    explicit constexpr TilePipelineStateCreateInfo(const Char* Name) :
        PipelineStateCreateInfo{Name, PIPELINE_TYPE_TILE}
    {}

    bool operator==(const TilePipelineStateCreateInfo& Rhs) const noexcept
    {
        if (static_cast<const PipelineStateCreateInfo&>(*this) != static_cast<const PipelineStateCreateInfo&>(Rhs))
            return false;

        if (TilePipeline != Rhs.TilePipeline)
            return false;

        return pTS == Rhs.pTS;
    }
    bool operator!=(const TilePipelineStateCreateInfo& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct TilePipelineStateCreateInfo TilePipelineStateCreateInfo;


/// Pipeline state status
DILIGENT_TYPED_ENUM(PIPELINE_STATE_STATUS, Uint32)
{
    /// Initial state.
    PIPELINE_STATE_STATUS_UNINITIALIZED = 0,

    /// The pipeline state is being compiled.
	PIPELINE_STATE_STATUS_COMPILING,

	/// The pipeline state has been successfully compiled
    /// and is ready to be used.
	PIPELINE_STATE_STATUS_READY,

	/// The pipeline state compilation has failed.
	PIPELINE_STATE_STATUS_FAILED
};


// {06084AE5-6A71-4FE8-84B9-395DD489A28C}
static DILIGENT_CONSTEXPR struct INTERFACE_ID IID_PipelineState =
    {0x6084ae5, 0x6a71, 0x4fe8, {0x84, 0xb9, 0x39, 0x5d, 0xd4, 0x89, 0xa2, 0x8c}};

#define DILIGENT_INTERFACE_NAME IPipelineState
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateInclusiveMethods \
    IDeviceObjectInclusiveMethods;     \
    IPipelineStateMethods PipelineState

// clang-format off

/// Pipeline state interface
DILIGENT_BEGIN_INTERFACE(IPipelineState, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the pipeline description used to create the object
    virtual const PipelineStateDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns the graphics pipeline description used to create the object.
    /// This method must only be called for a graphics or mesh pipeline.
    VIRTUAL const GraphicsPipelineDesc REF METHOD(GetGraphicsPipelineDesc)(THIS) CONST PURE;

    /// Returns the ray tracing pipeline description used to create the object.
    /// This method must only be called for a ray tracing pipeline.
    VIRTUAL const RayTracingPipelineDesc REF METHOD(GetRayTracingPipelineDesc)(THIS) CONST PURE;

    /// Returns the tile pipeline description used to create the object.
    /// This method must only be called for a tile pipeline.
    VIRTUAL const TilePipelineDesc REF METHOD(GetTilePipelineDesc)(THIS) CONST PURE;

    /// Binds resources for all shaders in the pipeline state.

    /// \param [in] ShaderStages     - Flags that specify shader stages, for which resources will be bound.
    ///                                Any combination of Diligent::SHADER_TYPE may be used.
    /// \param [in] pResourceMapping - Pointer to the resource mapping interface.
    /// \param [in] Flags            - Additional flags. See Diligent::BIND_SHADER_RESOURCES_FLAGS.
    ///
    /// \remarks    This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::BindStaticResources() method.
    VIRTUAL void METHOD(BindStaticResources)(THIS_
                                             SHADER_TYPE                 ShaderStages,
                                             IResourceMapping*           pResourceMapping,
                                             BIND_SHADER_RESOURCES_FLAGS Flags) PURE;


    /// Returns the number of static shader resource variables.

    /// \param [in] ShaderType - Type of the shader.
    ///
    /// \remarks    Only static variables (that can be accessed directly through the PSO) are counted.
    ///             Mutable and dynamic variables are accessed through Shader Resource Binding object.
    ///
    ///             This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::GetStaticVariableCount() method.
    VIRTUAL Uint32 METHOD(GetStaticVariableCount)(THIS_
                                                  SHADER_TYPE ShaderType) CONST PURE;


    /// Returns static shader resource variable. If the variable is not found,
    /// returns nullptr.

    /// \param [in] ShaderType - The type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Name       - Name of the variable.
    ///
    /// \remarks    The method does not increment the reference counter
    ///             of the returned interface.
    ///
    ///             This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::GetStaticVariableByName() method.
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByName)(THIS_
                                                                     SHADER_TYPE ShaderType,
                                                                     const Char* Name) PURE;


    /// Returns static shader resource variable by its index.

    /// \param [in] ShaderType - The type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Index      - Shader variable index. The index must be between
    ///                          0 and the total number of variables returned by
    ///                          GetStaticVariableCount().
    ///
    /// \remarks    Only static shader resource variables can be accessed through this method.
    ///             Mutable and dynamic variables are accessed through Shader Resource
    ///             Binding object.
    ///
    ///             This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::GetStaticVariableByIndex() method.
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByIndex)(THIS_
                                                                      SHADER_TYPE ShaderType,
                                                                      Uint32      Index) PURE;


    /// Creates a shader resource binding object.

    /// \param [out] ppShaderResourceBinding - Memory location where pointer to the new shader resource
    ///                                        binding object is written.
    /// \param [in] InitStaticResources      - If set to true, the method will initialize static resources in
    ///                                        the created object, which has the exact same effect as calling
    ///                                        IPipelineState::InitializeStaticSRBResources().
    ///
    /// \remarks    This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::CreateShaderResourceBinding() method.
    VIRTUAL void METHOD(CreateShaderResourceBinding)(THIS_
                                                     IShaderResourceBinding** ppShaderResourceBinding,
                                                     Bool                     InitStaticResources DEFAULT_VALUE(false)) PURE;



    /// Initializes static resources in the shader binding object.

    /// If static shader resources were not initialized when the SRB was created,
    /// this method must be called to initialize them before the SRB can be used.
    /// The method should be called after all static variables have been initialized
    /// in the PSO.
    ///
    /// \param [in] pShaderResourceBinding - Shader resource binding object to initialize.
    ///                                      The pipeline state must be compatible
    ///                                      with the shader resource binding object.
    ///
    /// \note   If static resources have already been initialized in the SRB and the method
    ///         is called again, it will have no effect and a warning message will be displayed.
    ///
    /// \remarks    This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::InitializeStaticSRBResources() method.
    VIRTUAL void METHOD(InitializeStaticSRBResources)(THIS_
                                                      struct IShaderResourceBinding* pShaderResourceBinding) CONST PURE;


    /// Copies static resource bindings to the destination pipeline.

    /// \param [in] pDstPipeline - Destination pipeline state.
    ///
    /// \note   Destination pipeline state must be compatible with this pipeline.
    ///
    /// \remarks    This method is only allowed for pipelines that use implicit resource signature
    ///             (e.g. shader resources are defined through ResourceLayout member of the pipeline desc).
    ///             For pipelines that use explicit resource signatures, use
    ///             IPipelineResourceSignature::CopyStaticResources() method.
    VIRTUAL void METHOD(CopyStaticResources)(THIS_
                                             IPipelineState* pDstPipeline) CONST PURE;


    /// Checks if this pipeline state object is compatible with another PSO

    /// If two pipeline state objects are compatible, they can use shader resource binding
    /// objects interchangeably, i.e. SRBs created by one PSO can be committed
    /// when another PSO is bound.
    /// \param [in] pPSO - Pointer to the pipeline state object to check compatibility with.
    /// \return     true if this PSO is compatible with pPSO. false otherwise.
    /// \remarks    The function only checks that shader resource layouts are compatible, but
    ///             does not check if resource types match. For instance, if a pixel shader in one PSO
    ///             uses a texture at slot 0, and a pixel shader in another PSO uses texture array at slot 0,
    ///             the pipelines will be compatible. However, if you try to use SRB object from the first pipeline
    ///             to commit resources for the second pipeline, a runtime error will occur.\n
    ///             The function only checks compatibility of shader resource layouts. It does not take
    ///             into account vertex shader input layout, number of outputs, etc.
    ///
    ///             *Technical details*
    ///
    ///             PSOs may be partially compatible when some, but not all pipeline resource signatures are compatible.
    ///             In Vulkan backend, switching PSOs that are partially compatible may increase performance
    ///             as shader resource bindings (that map to descriptor sets) from compatible signatures may be preserved.
    ///             In Direct3D12 backend, only switching between fully compatible PSOs preserves shader resource bindings,
    ///             while switching partially compatible PSOs still requires re-binding all resource bindings from all signatures.
    ///             In other backends the behavior is emulated. Usually, the bindings from the first N compatible resource signatures
    ///             may be preserved.
    VIRTUAL bool METHOD(IsCompatibleWith)(THIS_
                                          const struct IPipelineState* pPSO) CONST PURE;


    /// Returns the number of pipeline resource signatures used by this pipeline.

    /// \remarks  After the PSO is created, pipeline resource signatures are arranged by their binding indices.
    ///           The value returned by this function is given by the maximum signature binding index plus one,
    ///           and thus may not be equal to PipelineStateCreateInfo::ResourceSignaturesCount.
    VIRTUAL Uint32 METHOD(GetResourceSignatureCount)(THIS) CONST PURE;

    /// Returns pipeline resource signature at the give index.

    /// \param [in] Index - Index of the resource signature, same as BindingIndex in PipelineResourceSignatureDesc.
    /// \return     Pointer to pipeline resource signature interface.
    VIRTUAL IPipelineResourceSignature* METHOD(GetResourceSignature)(THIS_
                                                                     Uint32 Index) CONST PURE;

    /// Returns the pipeline state status, see Diligent::PIPELINE_STATE_STATUS.

    /// \param [in] WaitForCompletion - If true, the method will wait until the pipeline state is compiled.
    ///                                 If false, the method will return the pipeline state status without waiting.
    /// 							    This parameter is ignored if the pipeline state was compiled synchronously.
    /// \return     The pipeline state status.
    VIRTUAL PIPELINE_STATE_STATUS METHOD(GetStatus)(THIS_
                                                    bool WaitForCompletion DEFAULT_VALUE(false)) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineState_GetDesc(This) (const struct PipelineStateDesc*)IDeviceObject_GetDesc(This)

#    define IPipelineState_GetGraphicsPipelineDesc(This)           CALL_IFACE_METHOD(PipelineState, GetGraphicsPipelineDesc,      This)
#    define IPipelineState_GetRayTracingPipelineDesc(This)         CALL_IFACE_METHOD(PipelineState, GetRayTracingPipelineDesc,    This)
#    define IPipelineState_GetTilePipelineDesc(This)               CALL_IFACE_METHOD(PipelineState, GetTilePipelineDesc,          This)
#    define IPipelineState_BindStaticResources(This, ...)          CALL_IFACE_METHOD(PipelineState, BindStaticResources,          This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableCount(This, ...)       CALL_IFACE_METHOD(PipelineState, GetStaticVariableCount,       This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableByName(This, ...)      CALL_IFACE_METHOD(PipelineState, GetStaticVariableByName,      This, __VA_ARGS__)
#    define IPipelineState_GetStaticVariableByIndex(This, ...)     CALL_IFACE_METHOD(PipelineState, GetStaticVariableByIndex,     This, __VA_ARGS__)
#    define IPipelineState_CreateShaderResourceBinding(This, ...)  CALL_IFACE_METHOD(PipelineState, CreateShaderResourceBinding,  This, __VA_ARGS__)
#    define IPipelineState_InitializeStaticSRBResources(This, ...) CALL_IFACE_METHOD(PipelineState, InitializeStaticSRBResources, This, __VA_ARGS__)
#    define IPipelineState_CopyStaticResources(This, ...)          CALL_IFACE_METHOD(PipelineState, CopyStaticResources,          This, __VA_ARGS__)
#    define IPipelineState_IsCompatibleWith(This, ...)             CALL_IFACE_METHOD(PipelineState, IsCompatibleWith,             This, __VA_ARGS__)
#    define IPipelineState_GetResourceSignatureCount(This)         CALL_IFACE_METHOD(PipelineState, GetResourceSignatureCount,    This)
#    define IPipelineState_GetResourceSignature(This, ...)         CALL_IFACE_METHOD(PipelineState, GetResourceSignature,         This, __VA_ARGS__)
#    define IPipelineState_GetStatus(This, ...)                    CALL_IFACE_METHOD(PipelineState, GetStatus,                    This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE
