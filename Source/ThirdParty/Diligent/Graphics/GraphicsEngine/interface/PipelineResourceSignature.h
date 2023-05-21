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
/// Definition of the Diligent::IPipelineResourceSignature interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Platforms/interface/PlatformDefinitions.h"
#include "GraphicsTypes.h"
#include "Shader.h"
#include "Sampler.h"
#include "ShaderResourceVariable.h"
#include "ShaderResourceBinding.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


/// Immutable sampler description.

/// An immutable sampler is compiled into the pipeline state and can't be changed.
/// It is generally more efficient than a regular sampler and should be used
/// whenever possible.
struct ImmutableSamplerDesc
{
    /// Shader stages that this immutable sampler applies to. More than one shader stage can be specified.
    SHADER_TYPE ShaderStages         DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// The name of the sampler itself or the name of the texture variable that
    /// this immutable sampler is assigned to if combined texture samplers are used.
    const Char* SamplerOrTextureName DEFAULT_INITIALIZER(nullptr);

    /// Sampler description
    struct SamplerDesc Desc;

#if DILIGENT_CPP_INTERFACE
    constexpr ImmutableSamplerDesc() noexcept {}

    constexpr ImmutableSamplerDesc(SHADER_TYPE        _ShaderStages,
                                   const Char*        _SamplerOrTextureName,
                                   const SamplerDesc& _Desc)noexcept :
        ShaderStages        {_ShaderStages        },
        SamplerOrTextureName{_SamplerOrTextureName},
        Desc                {_Desc                }
    {}

    bool operator==(const ImmutableSamplerDesc& Rhs) const noexcept
    {
        return ShaderStages == Rhs.ShaderStages &&
               Desc         == Rhs.Desc &&
               SafeStrEqual(SamplerOrTextureName, Rhs.SamplerOrTextureName);
    }
    bool operator!=(const ImmutableSamplerDesc& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct ImmutableSamplerDesc ImmutableSamplerDesc;


/// Pipeline resource property flags.
DILIGENT_TYPED_ENUM(PIPELINE_RESOURCE_FLAGS, Uint8)
{
    /// Resource has no special properties
    PIPELINE_RESOURCE_FLAG_NONE            = 0,

    /// Indicates that dynamic buffers will never be bound to the resource
    /// variable. Applies to SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
    /// SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_TYPE_BUFFER_SRV resources.
    ///
    /// \remarks    In Vulkan and Direct3D12 backends, dynamic buffers require extra work
    ///             at run time. If an application knows it will never bind a dynamic buffer to
    ///             the variable, it should use PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag
    ///             to improve performance. This flag is not required and non-dynamic buffers
    ///             will still work even if the flag is not used. It is an error to bind a
    ///             dynamic buffer to resource that uses
    ///             PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag.
    PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS = 1u << 0,

    /// Indicates that a texture SRV will be combined with a sampler.
    /// Applies to SHADER_RESOURCE_TYPE_TEXTURE_SRV resources.
    PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER   = 1u << 1,

    /// Indicates that this variable will be used to bind formatted buffers.
    /// Applies to SHADER_RESOURCE_TYPE_BUFFER_UAV and SHADER_RESOURCE_TYPE_BUFFER_SRV
    /// resources.
    ///
    /// \remarks    In Vulkan backend formatted buffers require another descriptor type
    ///             as opposed to structured buffers. If an application will be using
    ///             formatted buffers with buffer UAVs and SRVs, it must specify the
    ///             PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER flag.
    PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER   = 1u << 2,

    /// Indicates that resource is a run-time sized shader array (e.g. an array without a specific size).
    PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY      = 1u << 3,

    /// Indicates that the resource is an input attachment in general layout, which allows simultaneously
    /// reading from the resource through the input attachment and writing to it via color or depth-stencil
    /// attachment.
    ///
    /// \note This flag is only valid in Vulkan.
    PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT = 1u << 4,

    PIPELINE_RESOURCE_FLAG_LAST               = PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT
};
DEFINE_FLAG_ENUM_OPERATORS(PIPELINE_RESOURCE_FLAGS);


/// Pipeline resource description.
struct PipelineResourceDesc
{
    /// Resource name in the shader
    const Char*                    Name          DEFAULT_INITIALIZER(nullptr);

    /// Shader stages that this resource applies to. When multiple shader stages are specified,
    /// all stages will share the same resource.
    ///
    /// \remarks    There may be multiple resources with the same name in different shader stages,
    ///             but the stages specified for different resources with the same name must not overlap.
    SHADER_TYPE                    ShaderStages  DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Resource array size (must be 1 for non-array resources).
    Uint32                         ArraySize     DEFAULT_INITIALIZER(1);

    /// Resource type, see Diligent::SHADER_RESOURCE_TYPE.
    SHADER_RESOURCE_TYPE           ResourceType  DEFAULT_INITIALIZER(SHADER_RESOURCE_TYPE_UNKNOWN);

    /// Resource variable type, see Diligent::SHADER_RESOURCE_VARIABLE_TYPE.
    SHADER_RESOURCE_VARIABLE_TYPE  VarType       DEFAULT_INITIALIZER(SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);

    /// Special resource flags, see Diligent::PIPELINE_RESOURCE_FLAGS.
    PIPELINE_RESOURCE_FLAGS        Flags         DEFAULT_INITIALIZER(PIPELINE_RESOURCE_FLAG_NONE);

#if DILIGENT_CPP_INTERFACE
    constexpr PipelineResourceDesc() noexcept {}

    constexpr PipelineResourceDesc(SHADER_TYPE                   _ShaderStages,
                                   const Char*                   _Name,
                                   Uint32                        _ArraySize,
                                   SHADER_RESOURCE_TYPE          _ResourceType,
                                   SHADER_RESOURCE_VARIABLE_TYPE _VarType = PipelineResourceDesc{}.VarType,
                                   PIPELINE_RESOURCE_FLAGS       _Flags   = PipelineResourceDesc{}.Flags) noexcept :
        Name        {_Name        },
        ShaderStages{_ShaderStages},
        ArraySize   {_ArraySize   },
        ResourceType{_ResourceType},
        VarType     {_VarType     },
        Flags       {_Flags       }
    {}

    constexpr PipelineResourceDesc(SHADER_TYPE                   _ShaderStages,
                                   const Char*                   _Name,
                                   SHADER_RESOURCE_TYPE          _ResourceType,
                                   SHADER_RESOURCE_VARIABLE_TYPE _VarType = PipelineResourceDesc{}.VarType,
                                   PIPELINE_RESOURCE_FLAGS       _Flags   = PipelineResourceDesc{}.Flags) noexcept :
        Name        {_Name        },
        ShaderStages{_ShaderStages},
        ResourceType{_ResourceType},
        VarType     {_VarType     },
        Flags       {_Flags       }
    {}

    bool operator==(const PipelineResourceDesc& Rhs) const noexcept
    {
        return ShaderStages == Rhs.ShaderStages &&
               ArraySize    == Rhs.ArraySize    &&
               ResourceType == Rhs.ResourceType &&
               VarType      == Rhs.VarType      &&
               Flags        == Rhs.Flags        &&
               SafeStrEqual(Name, Rhs.Name);
    }
    bool operator!=(const PipelineResourceDesc& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct PipelineResourceDesc PipelineResourceDesc;


/// Pipeline resource signature description.
struct PipelineResourceSignatureDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// A pointer to an array of resource descriptions. See Diligent::PipelineResourceDesc.
    const PipelineResourceDesc*  Resources  DEFAULT_INITIALIZER(nullptr);

    /// The number of resources in Resources array.
    Uint32  NumResources  DEFAULT_INITIALIZER(0);

    /// A pointer to an array of immutable samplers. See Diligent::ImmutableSamplerDesc.
    const ImmutableSamplerDesc*  ImmutableSamplers  DEFAULT_INITIALIZER(nullptr);

    /// The number of immutable samplers in ImmutableSamplers array.
    Uint32  NumImmutableSamplers  DEFAULT_INITIALIZER(0);

    /// Binding index that this resource signature uses.

    /// Every resource signature must be assign to one signature slot.
    /// The total number of slots is given by MAX_RESOURCE_SIGNATURES constant.
    /// All resource signatures used by a pipeline state must be assigned
    /// to different slots.
    Uint8  BindingIndex DEFAULT_INITIALIZER(0);

    /// If set to true, textures will be combined with texture samplers.
    /// The CombinedSamplerSuffix member defines the suffix added to the texture variable
    /// name to get corresponding sampler name. When using combined samplers,
    /// the sampler assigned to the shader resource view is automatically set when
    /// the view is bound. Otherwise samplers need to be explicitly set similar to other
    /// shader variables.
    Bool UseCombinedTextureSamplers DEFAULT_INITIALIZER(false);

    /// If UseCombinedTextureSamplers is true, defines the suffix added to the
    /// texture variable name to get corresponding sampler name.  For example,
    /// for default value "_sampler", a texture named "tex" will be combined
    /// with sampler named "tex_sampler".
    /// If UseCombinedTextureSamplers is false, this member is ignored.
    const Char* CombinedSamplerSuffix DEFAULT_INITIALIZER("_sampler");

    /// Shader resource binding allocation granularity

    /// This member defines the allocation granularity for internal resources required by
    /// the shader resource binding object instances.
    Uint32 SRBAllocationGranularity DEFAULT_INITIALIZER(1);

#if DILIGENT_CPP_INTERFACE

    /// Tests if two pipeline resource signature descriptions are equal.

    /// \param [in] Rhs - reference to the structure to compare with.
    ///
    /// \return     true if all members of the two structures *except for the Name* are equal,
    ///             and false otherwise.
    ///
    /// \note   The operator ignores the Name field as it is used for debug purposes and
    ///         doesn't affect the pipeline resource signature properties.
    bool operator==(const PipelineResourceSignatureDesc& Rhs) const noexcept
    {
        // Ignore Name. This is consistent with the hasher (HashCombiner<HasherType, PipelineResourceSignatureDesc>).
        if (NumResources               != Rhs.NumResources         ||
            NumImmutableSamplers       != Rhs.NumImmutableSamplers ||
            BindingIndex               != Rhs.BindingIndex         ||
            UseCombinedTextureSamplers != Rhs.UseCombinedTextureSamplers)
            return false;

        if (UseCombinedTextureSamplers && !SafeStrEqual(CombinedSamplerSuffix, Rhs.CombinedSamplerSuffix))
            return false;

        // ignore SRBAllocationGranularity

        for (Uint32 r = 0; r < NumResources; ++r)
        {
            if (!(Resources[r] == Rhs.Resources[r]))
                return false;
        }
        for (Uint32 s = 0; s < NumImmutableSamplers; ++s)
        {
            if (!(ImmutableSamplers[s] == Rhs.ImmutableSamplers[s]))
                return false;
        }
        return true;
    }
    bool operator!=(const PipelineResourceSignatureDesc& Rhs) const
    {
        return !(*this == Rhs);
    }
#endif
};
typedef struct PipelineResourceSignatureDesc PipelineResourceSignatureDesc;

// clang-format on


// {DCE499A5-F812-4C93-B108-D684A0B56118}
static const INTERFACE_ID IID_PipelineResourceSignature =
    {0xdce499a5, 0xf812, 0x4c93, {0xb1, 0x8, 0xd6, 0x84, 0xa0, 0xb5, 0x61, 0x18}};

#define DILIGENT_INTERFACE_NAME IPipelineResourceSignature
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineResourceSignatureInclusiveMethods \
    IDeviceObjectInclusiveMethods;                 \
    IPipelineResourceSignatureMethods PipelineResourceSignature

// clang-format off

/// Pipeline resource signature interface
DILIGENT_BEGIN_INTERFACE(IPipelineResourceSignature, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the pipeline resource signature description, see Diligent::PipelineResourceSignatureDesc.
    virtual const PipelineResourceSignatureDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Creates a shader resource binding object

    /// \param [out] ppShaderResourceBinding - Memory location where pointer to the new shader resource
    ///                                        binding object is written.
    /// \param [in] InitStaticResources      - If set to true, the method will initialize static resources in
    ///                                        the created object, which has the exact same effect as calling
    ///                                        IPipelineResourceSignature::InitializeStaticSRBResources().
    VIRTUAL void METHOD(CreateShaderResourceBinding)(THIS_
                                                     IShaderResourceBinding** ppShaderResourceBinding,
                                                     Bool                     InitStaticResources DEFAULT_VALUE(false)) PURE;


    /// Binds static resources for the specified shader stages in the pipeline resource signature.

    /// \param [in] ShaderStages     - Flags that specify shader stages, for which resources will be bound.
    ///                                Any combination of Diligent::SHADER_TYPE may be used.
    /// \param [in] pResourceMapping - Pointer to the resource mapping interface.
    /// \param [in] Flags            - Additional flags. See Diligent::BIND_SHADER_RESOURCES_FLAGS.
    VIRTUAL void METHOD(BindStaticResources)(THIS_
                                             SHADER_TYPE                 ShaderStages,
                                             IResourceMapping*           pResourceMapping,
                                             BIND_SHADER_RESOURCES_FLAGS Flags) PURE;


    /// Returns static shader resource variable. If the variable is not found,
    /// returns nullptr.

    /// \param [in] ShaderType - Type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Name       - Name of the variable.
    ///
    /// \remarks    If a variable is shared between multiple shader stages,
    ///             it can be accessed using any of those shader stages. Even
    ///             though IShaderResourceVariable instances returned by the method
    ///             may be different for different stages, internally they will
    ///             reference the same resource.
    ///
    ///             Only static shader resource variables can be accessed using this method.
    ///             Mutable and dynamic variables are accessed through Shader Resource
    ///             Binding object.
    ///
    ///             The method does not increment the reference counter of the
    ///             returned interface, and the application must *not* call Release()
    ///             unless it explicitly called AddRef().
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByName)(THIS_
                                                                     SHADER_TYPE ShaderType,
                                                                     const Char* Name) PURE;


    /// Returns static shader resource variable by its index.

    /// \param [in] ShaderType - Type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Index      - Shader variable index. The index must be between
    ///                          0 and the total number of variables returned by
    ///                          GetStaticVariableCount().
    ///
    ///
    /// \remarks    If a variable is shared between multiple shader stages,
    ///             it can be accessed using any of those shader stages. Even
    ///             though IShaderResourceVariable instances returned by the method
    ///             may be different for different stages, internally they will
    ///             reference the same resource.
    ///
    ///             Only static shader resource variables can be accessed using this method.
    ///             Mutable and dynamic variables are accessed through Shader Resource
    ///             Binding object.
    ///
    ///             The method does not increment the reference counter of the
    ///             returned interface, and the application must *not* call Release()
    ///             unless it explicitly called AddRef().
    VIRTUAL IShaderResourceVariable* METHOD(GetStaticVariableByIndex)(THIS_
                                                                      SHADER_TYPE ShaderType,
                                                                      Uint32      Index) PURE;


    /// Returns the number of static shader resource variables.

    /// \param [in] ShaderType - Type of the shader.
    ///
    /// \remarks   Only static variables (that can be accessed directly through the PSO) are counted.
    ///            Mutable and dynamic variables are accessed through Shader Resource Binding object.
    VIRTUAL Uint32 METHOD(GetStaticVariableCount)(THIS_
                                                  SHADER_TYPE ShaderType) CONST PURE;

    /// Initializes static resources in the shader binding object.

    /// If static shader resources were not initialized when the SRB was created,
    /// this method must be called to initialize them before the SRB can be used.
    /// The method should be called after all static variables have been initialized
    /// in the signature.
    ///
    /// \param [in] pShaderResourceBinding - Shader resource binding object to initialize.
    ///                                      The pipeline resource signature must be compatible
    ///                                      with the shader resource binding object.
    ///
    /// \note   If static resources have already been initialized in the SRB and the method
    ///         is called again, it will have no effect and a warning message will be displayed.
    VIRTUAL void METHOD(InitializeStaticSRBResources)(THIS_
                                                      struct IShaderResourceBinding* pShaderResourceBinding) CONST PURE;

    /// Copies static resource bindings to the destination signature.

    /// \param [in] pDstSignature - Destination pipeline resource signature.
    ///
    /// \note   Destination signature must be compatible with this signature.
    VIRTUAL void METHOD(CopyStaticResources)(THIS_
                                             IPipelineResourceSignature* pDstSignature) CONST PURE;

    /// Returns true if the signature is compatible with another one.

    /// \remarks    Two signatures are compatible if they contain identical resources and immutabke samplers,
    ///             defined in the same order disregarding their names.
    VIRTUAL Bool METHOD(IsCompatibleWith)(THIS_
                                          const struct IPipelineResourceSignature* pPRS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineResourceSignature_GetDesc(This) (const struct PipelineResourceSignatureDesc*)IDeviceObject_GetDesc(This)

#    define IPipelineResourceSignature_CreateShaderResourceBinding(This, ...)  CALL_IFACE_METHOD(PipelineResourceSignature, CreateShaderResourceBinding, This, __VA_ARGS__)
#    define IPipelineResourceSignature_BindStaticResources(This, ...)          CALL_IFACE_METHOD(PipelineResourceSignature, BindStaticResources,         This, __VA_ARGS__)
#    define IPipelineResourceSignature_GetStaticVariableByName(This, ...)      CALL_IFACE_METHOD(PipelineResourceSignature, GetStaticVariableByName,     This, __VA_ARGS__)
#    define IPipelineResourceSignature_GetStaticVariableByIndex(This, ...)     CALL_IFACE_METHOD(PipelineResourceSignature, GetStaticVariableByIndex,    This, __VA_ARGS__)
#    define IPipelineResourceSignature_GetStaticVariableCount(This, ...)       CALL_IFACE_METHOD(PipelineResourceSignature, GetStaticVariableCount,      This, __VA_ARGS__)
#    define IPipelineResourceSignature_InitializeStaticSRBResources(This, ...) CALL_IFACE_METHOD(PipelineResourceSignature, InitializeStaticSRBResources,This, __VA_ARGS__)
#    define IPipelineResourceSignature_CopyStaticResources(This, ...)          CALL_IFACE_METHOD(PipelineResourceSignature, CopyStaticResources,         This, __VA_ARGS__)
#    define IPipelineResourceSignature_IsCompatibleWith(This, ...)             CALL_IFACE_METHOD(PipelineResourceSignature, IsCompatibleWith,            This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE
