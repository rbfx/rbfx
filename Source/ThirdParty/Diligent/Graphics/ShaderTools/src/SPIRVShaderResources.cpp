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

#include <iomanip>
#include "SPIRVShaderResources.hpp"
#include "spirv_parser.hpp"
#include "spirv_cross.hpp"
#include "ShaderBase.hpp"
#include "GraphicsAccessories.hpp"
#include "StringTools.hpp"
#include "Align.hpp"
#include "ShaderToolsCommon.hpp"

namespace Diligent
{

template <typename Type>
Type GetResourceArraySize(const diligent_spirv_cross::Compiler& Compiler,
                          const diligent_spirv_cross::Resource& Res)
{
    const auto& type    = Compiler.get_type(Res.type_id);
    uint32_t    arrSize = 1;
    if (!type.array.empty())
    {
        // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide#querying-array-types
        VERIFY(type.array.size() == 1, "Only one-dimensional arrays are currently supported");
        arrSize = type.array[0];
    }
    VERIFY(arrSize <= std::numeric_limits<Type>::max(), "Array size exceeds maximum representable value ", std::numeric_limits<Type>::max());
    return static_cast<Type>(arrSize);
}

static RESOURCE_DIMENSION GetResourceDimension(const diligent_spirv_cross::Compiler& Compiler,
                                               const diligent_spirv_cross::Resource& Res)
{
    const auto& type = Compiler.get_type(Res.type_id);
    if (type.basetype == diligent_spirv_cross::SPIRType::BaseType::Image ||
        type.basetype == diligent_spirv_cross::SPIRType::BaseType::SampledImage)
    {
        switch (type.image.dim)
        {
            // clang-format off
            case spv::Dim1D:     return type.image.arrayed ? RESOURCE_DIM_TEX_1D_ARRAY : RESOURCE_DIM_TEX_1D;
            case spv::Dim2D:     return type.image.arrayed ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            case spv::Dim3D:     return RESOURCE_DIM_TEX_3D;
            case spv::DimCube:   return type.image.arrayed ? RESOURCE_DIM_TEX_CUBE_ARRAY : RESOURCE_DIM_TEX_CUBE;
            case spv::DimBuffer: return RESOURCE_DIM_BUFFER;
            // clang-format on
            default: return RESOURCE_DIM_UNDEFINED;
        }
    }
    else
    {
        return RESOURCE_DIM_UNDEFINED;
    }
}

static bool IsMultisample(const diligent_spirv_cross::Compiler& Compiler,
                          const diligent_spirv_cross::Resource& Res)
{
    const auto& type = Compiler.get_type(Res.type_id);
    if (type.basetype == diligent_spirv_cross::SPIRType::BaseType::Image ||
        type.basetype == diligent_spirv_cross::SPIRType::BaseType::SampledImage)
    {
        return type.image.ms;
    }
    else
    {
        return RESOURCE_DIM_UNDEFINED;
    }
}

static uint32_t GetDecorationOffset(const diligent_spirv_cross::Compiler& Compiler,
                                    const diligent_spirv_cross::Resource& Res,
                                    spv::Decoration                       Decoration)
{
    VERIFY(Compiler.has_decoration(Res.id, Decoration), "Resource \'", Res.name, "\' has no requested decoration");
    uint32_t offset   = 0;
    auto     declared = Compiler.get_binary_offset_for_decoration(Res.id, Decoration, offset);
    VERIFY(declared, "Requested decoration is not declared");
    (void)declared;
    return offset;
}

SPIRVShaderResourceAttribs::SPIRVShaderResourceAttribs(const diligent_spirv_cross::Compiler& Compiler,
                                                       const diligent_spirv_cross::Resource& Res,
                                                       const char*                           _Name,
                                                       ResourceType                          _Type,
                                                       Uint32                                _BufferStaticSize,
                                                       Uint32                                _BufferStride) noexcept :
    // clang-format off
    Name                          {_Name},
    ArraySize                     {GetResourceArraySize<decltype(ArraySize)>(Compiler, Res)},
    Type                          {_Type},
    ResourceDim                   {Diligent::GetResourceDimension(Compiler, Res)},
    IsMS                          {Diligent::IsMultisample(Compiler, Res) ? Uint8{1} : Uint8{0}},
    BindingDecorationOffset       {GetDecorationOffset(Compiler, Res, spv::Decoration::DecorationBinding)},
    DescriptorSetDecorationOffset {GetDecorationOffset(Compiler, Res, spv::Decoration::DecorationDescriptorSet)},
    BufferStaticSize              {_BufferStaticSize},
    BufferStride                  {_BufferStride}
// clang-format on
{}


SHADER_RESOURCE_TYPE SPIRVShaderResourceAttribs::GetShaderResourceType(ResourceType Type)
{
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please handle the new resource type below");
    switch (Type)
    {
        case SPIRVShaderResourceAttribs::ResourceType::UniformBuffer:
            return SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;

        case SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer:
            // Read-only storage buffers map to buffer SRV
            // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide#read-write-vs-read-only-resources-for-hlsl
            return SHADER_RESOURCE_TYPE_BUFFER_SRV;

        case SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer:
            return SHADER_RESOURCE_TYPE_BUFFER_UAV;

        case SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer:
            return SHADER_RESOURCE_TYPE_BUFFER_SRV;

        case SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer:
            return SHADER_RESOURCE_TYPE_BUFFER_UAV;

        case SPIRVShaderResourceAttribs::ResourceType::StorageImage:
            return SHADER_RESOURCE_TYPE_TEXTURE_UAV;

        case SPIRVShaderResourceAttribs::ResourceType::SampledImage:
            return SHADER_RESOURCE_TYPE_TEXTURE_SRV;

        case SPIRVShaderResourceAttribs::ResourceType::AtomicCounter:
            LOG_WARNING_MESSAGE("There is no appropriate shader resource type for atomic counter");
            return SHADER_RESOURCE_TYPE_BUFFER_UAV;

        case SPIRVShaderResourceAttribs::ResourceType::SeparateImage:
            return SHADER_RESOURCE_TYPE_TEXTURE_SRV;

        case SPIRVShaderResourceAttribs::ResourceType::SeparateSampler:
            return SHADER_RESOURCE_TYPE_SAMPLER;

        case SPIRVShaderResourceAttribs::ResourceType::InputAttachment:
            return SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT;

        case SPIRVShaderResourceAttribs::ResourceType::AccelerationStructure:
            return SHADER_RESOURCE_TYPE_ACCEL_STRUCT;

        default:
            UNEXPECTED("Unknown SPIRV resource type");
            return SHADER_RESOURCE_TYPE_UNKNOWN;
    }
}

PIPELINE_RESOURCE_FLAGS SPIRVShaderResourceAttribs::GetPipelineResourceFlags(ResourceType Type)
{
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please handle the new resource type below");
    switch (Type)
    {
        case SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer:
        case SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer:
            return PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER;

        case SPIRVShaderResourceAttribs::ResourceType::SampledImage:
            return PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;

        default:
            return PIPELINE_RESOURCE_FLAG_NONE;
    }
}

spv::ExecutionModel ShaderTypeToSpvExecutionModel(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return spv::ExecutionModelVertex;
        case SHADER_TYPE_HULL:             return spv::ExecutionModelTessellationControl;
        case SHADER_TYPE_DOMAIN:           return spv::ExecutionModelTessellationEvaluation;
        case SHADER_TYPE_GEOMETRY:         return spv::ExecutionModelGeometry;
        case SHADER_TYPE_PIXEL:            return spv::ExecutionModelFragment;
        case SHADER_TYPE_COMPUTE:          return spv::ExecutionModelGLCompute;
        case SHADER_TYPE_AMPLIFICATION:    return spv::ExecutionModelTaskEXT;
        case SHADER_TYPE_MESH:             return spv::ExecutionModelMeshEXT;
        case SHADER_TYPE_RAY_GEN:          return spv::ExecutionModelRayGenerationKHR;
        case SHADER_TYPE_RAY_MISS:         return spv::ExecutionModelMissKHR;
        case SHADER_TYPE_RAY_CLOSEST_HIT:  return spv::ExecutionModelClosestHitKHR;
        case SHADER_TYPE_RAY_ANY_HIT:      return spv::ExecutionModelAnyHitKHR;
        case SHADER_TYPE_RAY_INTERSECTION: return spv::ExecutionModelIntersectionKHR;
        case SHADER_TYPE_CALLABLE:         return spv::ExecutionModelCallableKHR;
        // clang-format on
        case SHADER_TYPE_TILE:
            UNEXPECTED("Unsupported shader type");
            return spv::ExecutionModelMax;
        default:
            UNEXPECTED("Unexpected shader type");
            return spv::ExecutionModelMax;
    }
}

const std::string& GetUBName(diligent_spirv_cross::Compiler&               Compiler,
                             const diligent_spirv_cross::Resource&         UB,
                             const diligent_spirv_cross::ParsedIR::Source& IRSource)
{
    // Consider the following HLSL constant buffer:
    //
    //    cbuffer Constants
    //    {
    //        float4x4 g_WorldViewProj;
    //    };
    //
    // glslang emits SPIRV as if the following GLSL was written:
    //
    //    uniform Constants // UB.name
    //    {
    //        float4x4 g_WorldViewProj;
    //    }; // no instance name
    //
    // DXC emits the byte code that corresponds to the following GLSL:
    //
    //    uniform type_Constants // UB.name
    //    {
    //        float4x4 g_WorldViewProj;
    //    }Constants; // get_name(UB.id)
    //
    //
    //                            |     glslang      |         DXC
    //  -------------------------------------------------------------------
    //  UB.name                   |   "Constants"    |   "type_Constants"
    //  Compiler.get_name(UB.id)  |   ""             |   "Constants"
    //
    // Note that for the byte code produced from GLSL, we must always
    // use UB.name even if the instance name is present

    const auto& instance_name = Compiler.get_name(UB.id);
    return (IRSource.hlsl && !instance_name.empty()) ? instance_name : UB.name;
}

static SHADER_CODE_BASIC_TYPE SpirvBaseTypeToShaderCodeBasicType(diligent_spirv_cross::SPIRType::BaseType SpvBaseType)
{
    switch (SpvBaseType)
    {
        // clang-format off
        case diligent_spirv_cross::SPIRType::Unknown:               return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::Void:                  return SHADER_CODE_BASIC_TYPE_VOID;
        case diligent_spirv_cross::SPIRType::Boolean:               return SHADER_CODE_BASIC_TYPE_BOOL;
        case diligent_spirv_cross::SPIRType::SByte:                 return SHADER_CODE_BASIC_TYPE_INT8;
        case diligent_spirv_cross::SPIRType::UByte:                 return SHADER_CODE_BASIC_TYPE_UINT8;
        case diligent_spirv_cross::SPIRType::Short:                 return SHADER_CODE_BASIC_TYPE_INT16;
        case diligent_spirv_cross::SPIRType::UShort:                return SHADER_CODE_BASIC_TYPE_UINT16;
        case diligent_spirv_cross::SPIRType::Int:                   return SHADER_CODE_BASIC_TYPE_INT;
        case diligent_spirv_cross::SPIRType::UInt:                  return SHADER_CODE_BASIC_TYPE_UINT;
        case diligent_spirv_cross::SPIRType::Int64:                 return SHADER_CODE_BASIC_TYPE_INT64;
        case diligent_spirv_cross::SPIRType::UInt64:                return SHADER_CODE_BASIC_TYPE_UINT64;
        case diligent_spirv_cross::SPIRType::AtomicCounter:         return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::Half:                  return SHADER_CODE_BASIC_TYPE_FLOAT16;
        case diligent_spirv_cross::SPIRType::Float:                 return SHADER_CODE_BASIC_TYPE_FLOAT;
        case diligent_spirv_cross::SPIRType::Double:                return SHADER_CODE_BASIC_TYPE_DOUBLE;
        case diligent_spirv_cross::SPIRType::Struct:                return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::Image:                 return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::SampledImage:          return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::Sampler:               return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::AccelerationStructure: return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        case diligent_spirv_cross::SPIRType::RayQuery:              return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        // clang-format on
        default:
            UNEXPECTED("Unknown SPIRV base type");
            return SHADER_CODE_BASIC_TYPE_UNKNOWN;
    }
}

void LoadShaderCodeVariableDesc(const diligent_spirv_cross::Compiler& Compiler,
                                const diligent_spirv_cross::TypeID&   TypeID,
                                const diligent_spirv_cross::Bitset&   Decoration,
                                bool                                  IsHLSLSource,
                                ShaderCodeVariableDescX&              TypeDesc)
{
    const auto& SpvType = Compiler.get_type(TypeID);
    if (SpvType.basetype == diligent_spirv_cross::SPIRType::Struct)
    {
        TypeDesc.Class = SHADER_CODE_VARIABLE_CLASS_STRUCT;
    }
    else if (SpvType.vecsize > 1 && SpvType.columns > 1)
    {
        if (Decoration.get(spv::Decoration::DecorationRowMajor))
            TypeDesc.Class = IsHLSLSource ? SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS : SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS;
        else
            TypeDesc.Class = IsHLSLSource ? SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS : SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS;
    }
    else if (SpvType.vecsize > 1)
    {
        TypeDesc.Class = SHADER_CODE_VARIABLE_CLASS_VECTOR;
    }
    else
    {
        TypeDesc.Class = SHADER_CODE_VARIABLE_CLASS_SCALAR;
    }

    if (TypeDesc.Class != SHADER_CODE_VARIABLE_CLASS_STRUCT)
    {
        TypeDesc.BasicType  = SpirvBaseTypeToShaderCodeBasicType(SpvType.basetype);
        TypeDesc.NumRows    = StaticCast<decltype(TypeDesc.NumRows)>(SpvType.vecsize);
        TypeDesc.NumColumns = StaticCast<decltype(TypeDesc.NumColumns)>(SpvType.columns);
        if (IsHLSLSource)
            std::swap(TypeDesc.NumRows, TypeDesc.NumColumns);
    }

    TypeDesc.SetTypeName(Compiler.get_name(TypeID));
    if (TypeDesc.TypeName == nullptr || TypeDesc.TypeName[0] == '\0')
        TypeDesc.SetTypeName(Compiler.get_name(SpvType.parent_type));
    if (TypeDesc.TypeName == nullptr || TypeDesc.TypeName[0] == '\0')
        TypeDesc.SetDefaultTypeName(IsHLSLSource ? SHADER_SOURCE_LANGUAGE_HLSL : SHADER_SOURCE_LANGUAGE_GLSL);

    TypeDesc.ArraySize = !SpvType.array.empty() ? SpvType.array[0] : 0;

    for (uint32_t i = 0; i < SpvType.member_types.size(); ++i)
    {
        ShaderCodeVariableDesc VarDesc;
        VarDesc.Name = Compiler.get_member_name(TypeID, i).c_str();
        if (VarDesc.Name[0] == '\0')
            VarDesc.Name = Compiler.get_member_name(SpvType.parent_type, i).c_str();

        VarDesc.Offset = Compiler.type_struct_member_offset(SpvType, i);

        auto idx = TypeDesc.AddMember(VarDesc);
        VERIFY_EXPR(idx == i);
        LoadShaderCodeVariableDesc(Compiler, SpvType.member_types[i], Compiler.get_member_decoration_bitset(TypeID, i), IsHLSLSource, TypeDesc.GetMember(i));
    }
}

ShaderCodeBufferDescX LoadUBReflection(const diligent_spirv_cross::Compiler& Compiler, const diligent_spirv_cross::Resource& UB, bool IsHLSLSource)
{
    const auto& SpvType = Compiler.get_type(UB.type_id);
    const auto  Size    = Compiler.get_declared_struct_size(SpvType);

    ShaderCodeBufferDescX UBDesc;
    UBDesc.Size = StaticCast<decltype(UBDesc.Size)>(Size);
    for (uint32_t i = 0; i < SpvType.member_types.size(); ++i)
    {
        ShaderCodeVariableDesc VarDesc;
        VarDesc.Name   = Compiler.get_member_name(UB.base_type_id, i).c_str();
        VarDesc.Offset = Compiler.type_struct_member_offset(SpvType, i);

        auto idx = UBDesc.AddVariable(VarDesc);
        VERIFY_EXPR(idx == i);
        LoadShaderCodeVariableDesc(Compiler, SpvType.member_types[i], Compiler.get_member_decoration_bitset(SpvType.self, i), IsHLSLSource, UBDesc.GetVariable(idx));
    }

    return UBDesc;
}


SPIRVShaderResources::SPIRVShaderResources(IMemoryAllocator&     Allocator,
                                           std::vector<uint32_t> spirv_binary,
                                           const ShaderDesc&     shaderDesc,
                                           const char*           CombinedSamplerSuffix,
                                           bool                  LoadShaderStageInputs,
                                           bool                  LoadUniformBufferReflection,
                                           std::string&          EntryPoint) noexcept(false) :
    m_ShaderType{shaderDesc.ShaderType}
{
    // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
    diligent_spirv_cross::Parser parser{std::move(spirv_binary)};
    parser.parse();
    const auto ParsedIRSource = parser.get_parsed_ir().source;
    m_IsHLSLSource            = ParsedIRSource.hlsl;
    diligent_spirv_cross::Compiler Compiler{std::move(parser.get_parsed_ir())};

    spv::ExecutionModel ExecutionModel = ShaderTypeToSpvExecutionModel(shaderDesc.ShaderType);
    auto                EntryPoints    = Compiler.get_entry_points_and_stages();
    for (const auto& CurrEntryPoint : EntryPoints)
    {
        if (CurrEntryPoint.execution_model == ExecutionModel)
        {
            if (!EntryPoint.empty())
            {
                LOG_WARNING_MESSAGE("More than one entry point of type ", GetShaderTypeLiteralName(shaderDesc.ShaderType), " found in SPIRV binary for shader '", shaderDesc.Name, "'. The first one ('", EntryPoint, "') will be used.");
            }
            else
            {
                EntryPoint = CurrEntryPoint.name;
            }
        }
    }
    if (EntryPoint.empty())
    {
        LOG_ERROR_AND_THROW("Unable to find entry point of type ", GetShaderTypeLiteralName(shaderDesc.ShaderType), " in SPIRV binary for shader '", shaderDesc.Name, "'");
    }
    Compiler.set_entry_point(EntryPoint, ExecutionModel);

    // The SPIR-V is now parsed, and we can perform reflection on it.
    diligent_spirv_cross::ShaderResources resources = Compiler.get_shader_resources();

    size_t ResourceNamesPoolSize = 0;
    for (const auto& ub : resources.uniform_buffers)
        ResourceNamesPoolSize += GetUBName(Compiler, ub, ParsedIRSource).length() + 1;
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please account for the new resource type below");
    for (auto* pResType :
         {
             &resources.storage_buffers,
             &resources.storage_images,
             &resources.sampled_images,
             &resources.atomic_counters,
             &resources.separate_images,
             &resources.separate_samplers,
             &resources.subpass_inputs,
             &resources.acceleration_structures //
         })                                     //
    {
        for (const auto& res : *pResType)
            ResourceNamesPoolSize += res.name.length() + 1;
    }

    if (CombinedSamplerSuffix != nullptr)
    {
        ResourceNamesPoolSize += strlen(CombinedSamplerSuffix) + 1;
    }

    VERIFY_EXPR(shaderDesc.Name != nullptr);
    ResourceNamesPoolSize += strlen(shaderDesc.Name) + 1;

    Uint32 NumShaderStageInputs = 0;

    if (!m_IsHLSLSource || resources.stage_inputs.empty())
        LoadShaderStageInputs = false;
    if (LoadShaderStageInputs)
    {
        const auto& Extensions         = Compiler.get_declared_extensions();
        bool        HlslFunctionality1 = false;
        for (const auto& ext : Extensions)
        {
            HlslFunctionality1 = (ext == "SPV_GOOGLE_hlsl_functionality1");
            if (HlslFunctionality1)
                break;
        }

        if (HlslFunctionality1)
        {
            for (const auto& Input : resources.stage_inputs)
            {
                if (Compiler.has_decoration(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE))
                {
                    const auto& Semantic = Compiler.get_decoration_string(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE);
                    ResourceNamesPoolSize += Semantic.length() + 1;
                    ++NumShaderStageInputs;
                }
                else
                {
                    LOG_ERROR_MESSAGE("Shader input '", Input.name, "' does not have DecorationHlslSemanticGOOGLE decoration, which is unexpected as the shader declares SPV_GOOGLE_hlsl_functionality1 extension");
                }
            }
        }
        else
        {
            LoadShaderStageInputs = false;
            if (m_IsHLSLSource)
            {
                LOG_WARNING_MESSAGE("SPIRV byte code of shader '", shaderDesc.Name,
                                    "' does not use SPV_GOOGLE_hlsl_functionality1 extension. "
                                    "As a result, it is not possible to get semantics of shader inputs and map them to proper locations. "
                                    "The shader will still work correctly if all attributes are declared in ascending order without any gaps. "
                                    "Enable SPV_GOOGLE_hlsl_functionality1 in your compiler to allow proper mapping of vertex shader inputs.");
            }
        }
    }

    ResourceCounters ResCounters;
    ResCounters.NumUBs          = static_cast<Uint32>(resources.uniform_buffers.size());
    ResCounters.NumSBs          = static_cast<Uint32>(resources.storage_buffers.size());
    ResCounters.NumImgs         = static_cast<Uint32>(resources.storage_images.size());
    ResCounters.NumSmpldImgs    = static_cast<Uint32>(resources.sampled_images.size());
    ResCounters.NumACs          = static_cast<Uint32>(resources.atomic_counters.size());
    ResCounters.NumSepSmplrs    = static_cast<Uint32>(resources.separate_samplers.size());
    ResCounters.NumSepImgs      = static_cast<Uint32>(resources.separate_images.size());
    ResCounters.NumInptAtts     = static_cast<Uint32>(resources.subpass_inputs.size());
    ResCounters.NumAccelStructs = static_cast<Uint32>(resources.acceleration_structures.size());
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please set the new resource type counter here");

    // Resource names pool is only needed to facilitate string allocation.
    StringPool ResourceNamesPool;
    Initialize(Allocator, ResCounters, NumShaderStageInputs, ResourceNamesPoolSize, ResourceNamesPool);

    // Uniform buffer reflections
    std::vector<ShaderCodeBufferDescX> UBReflections;

    {
        Uint32 CurrUB = 0;
        for (const auto& UB : resources.uniform_buffers)
        {
            const auto& name = GetUBName(Compiler, UB, ParsedIRSource);
            const auto& Type = Compiler.get_type(UB.type_id);
            const auto  Size = Compiler.get_declared_struct_size(Type);
            new (&GetUB(CurrUB++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    UB,
                    ResourceNamesPool.CopyString(name),
                    SPIRVShaderResourceAttribs::ResourceType::UniformBuffer,
                    static_cast<Uint32>(Size) //
                };

            if (LoadUniformBufferReflection)
            {
                UBReflections.emplace_back(LoadUBReflection(Compiler, UB, m_IsHLSLSource));
            }
        }
        VERIFY_EXPR(CurrUB == GetNumUBs());
    }

    {
        Uint32 CurrSB = 0;
        for (const auto& SB : resources.storage_buffers)
        {
            auto BufferFlags = Compiler.get_buffer_block_flags(SB.id);
            auto IsReadOnly  = BufferFlags.get(spv::DecorationNonWritable);
            auto ResType     = IsReadOnly ?
                SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer :
                SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer;
            const auto& Type   = Compiler.get_type(SB.type_id);
            const auto  Size   = Compiler.get_declared_struct_size(Type);
            const auto  Stride = Compiler.get_declared_struct_size_runtime_array(Type, 1) - Size;
            new (&GetSB(CurrSB++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    SB,
                    ResourceNamesPool.CopyString(SB.name),
                    ResType,
                    static_cast<Uint32>(Size),
                    static_cast<Uint32>(Stride) //
                };
        }
        VERIFY_EXPR(CurrSB == GetNumSBs());
    }

    {
        Uint32 CurrSmplImg = 0;
        for (const auto& SmplImg : resources.sampled_images)
        {
            const auto& type    = Compiler.get_type(SmplImg.type_id);
            auto        ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::SampledImage;
            new (&GetSmpldImg(CurrSmplImg++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    SmplImg,
                    ResourceNamesPool.CopyString(SmplImg.name),
                    ResType //
                };
        }
        VERIFY_EXPR(CurrSmplImg == GetNumSmpldImgs());
    }

    {
        Uint32 CurrImg = 0;
        for (const auto& Img : resources.storage_images)
        {
            const auto& type    = Compiler.get_type(Img.type_id);
            auto        ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::StorageImage;
            new (&GetImg(CurrImg++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    Img,
                    ResourceNamesPool.CopyString(Img.name),
                    ResType //
                };
        }
        VERIFY_EXPR(CurrImg == GetNumImgs());
    }

    {
        Uint32 CurrAC = 0;
        for (const auto& AC : resources.atomic_counters)
        {
            new (&GetAC(CurrAC++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    AC,
                    ResourceNamesPool.CopyString(AC.name),
                    SPIRVShaderResourceAttribs::ResourceType::AtomicCounter //
                };
        }
        VERIFY_EXPR(CurrAC == GetNumACs());
    }

    {
        Uint32 CurrSepSmpl = 0;
        for (const auto& SepSam : resources.separate_samplers)
        {
            new (&GetSepSmplr(CurrSepSmpl++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    SepSam,
                    ResourceNamesPool.CopyString(SepSam.name),
                    SPIRVShaderResourceAttribs::ResourceType::SeparateSampler //
                };
        }
        VERIFY_EXPR(CurrSepSmpl == GetNumSepSmplrs());
    }

    {
        Uint32 CurrSepImg = 0;
        for (const auto& SepImg : resources.separate_images)
        {
            const auto& type    = Compiler.get_type(SepImg.type_id);
            const auto  ResType = type.image.dim == spv::DimBuffer ?
                SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer :
                SPIRVShaderResourceAttribs::ResourceType::SeparateImage;

            new (&GetSepImg(CurrSepImg++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    SepImg,
                    ResourceNamesPool.CopyString(SepImg.name),
                    ResType //
                };
        }
        VERIFY_EXPR(CurrSepImg == GetNumSepImgs());
    }

    {
        Uint32 CurrSubpassInput = 0;
        for (const auto& SubpassInput : resources.subpass_inputs)
        {
            new (&GetInptAtt(CurrSubpassInput++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    SubpassInput,
                    ResourceNamesPool.CopyString(SubpassInput.name),
                    SPIRVShaderResourceAttribs::ResourceType::InputAttachment //
                };
        }
        VERIFY_EXPR(CurrSubpassInput == GetNumInptAtts());
    }

    {
        Uint32 CurrAccelStruct = 0;
        for (const auto& AccelStruct : resources.acceleration_structures)
        {
            new (&GetAccelStruct(CurrAccelStruct++)) SPIRVShaderResourceAttribs //
                {
                    Compiler,
                    AccelStruct,
                    ResourceNamesPool.CopyString(AccelStruct.name),
                    SPIRVShaderResourceAttribs::ResourceType::AccelerationStructure //
                };
        }
        VERIFY_EXPR(CurrAccelStruct == GetNumAccelStructs());
    }

    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please initialize SPIRVShaderResourceAttribs for the new resource type here");

    if (CombinedSamplerSuffix != nullptr)
    {
        m_CombinedSamplerSuffix = ResourceNamesPool.CopyString(CombinedSamplerSuffix);
    }

    m_ShaderName = ResourceNamesPool.CopyString(shaderDesc.Name);

    if (LoadShaderStageInputs)
    {
        Uint32 CurrStageInput = 0;
        for (const auto& Input : resources.stage_inputs)
        {
            if (Compiler.has_decoration(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE))
            {
                const auto& Semantic = Compiler.get_decoration_string(Input.id, spv::Decoration::DecorationHlslSemanticGOOGLE);
                new (&GetShaderStageInputAttribs(CurrStageInput++)) SPIRVShaderStageInputAttribs //
                    {
                        ResourceNamesPool.CopyString(Semantic),
                        GetDecorationOffset(Compiler, Input, spv::Decoration::DecorationLocation) //
                    };
            }
        }
        VERIFY_EXPR(CurrStageInput == GetNumShaderStageInputs());
    }

    VERIFY(ResourceNamesPool.GetRemainingSize() == 0, "Names pool must be empty");

    if (shaderDesc.ShaderType == SHADER_TYPE_COMPUTE)
    {
        for (uint32_t i = 0; i < m_ComputeGroupSize.size(); ++i)
            m_ComputeGroupSize[i] = Compiler.get_execution_mode_argument(spv::ExecutionModeLocalSize, i);
    }

    if (!UBReflections.empty())
    {
        VERIFY_EXPR(LoadUniformBufferReflection);
        VERIFY_EXPR(UBReflections.size() == GetNumUBs());
        m_UBReflectionBuffer = ShaderCodeBufferDescX::PackArray(UBReflections.cbegin(), UBReflections.cend(), GetRawAllocator());
    }
    //LOG_INFO_MESSAGE(DumpResources());
}

void SPIRVShaderResources::Initialize(IMemoryAllocator&       Allocator,
                                      const ResourceCounters& Counters,
                                      Uint32                  NumShaderStageInputs,
                                      size_t                  ResourceNamesPoolSize,
                                      StringPool&             ResourceNamesPool)
{
    Uint32           CurrentOffset = 0;
    constexpr Uint32 MaxOffset     = std::numeric_limits<OffsetType>::max();
    auto             AdvanceOffset = [&CurrentOffset, MaxOffset](Uint32 NumResources) {
        VERIFY(CurrentOffset <= MaxOffset, "Current offset (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        (void)MaxOffset;
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumResources;
        return Offset;
    };

    auto UniformBufferOffset = AdvanceOffset(Counters.NumUBs);
    (void)UniformBufferOffset;
    m_StorageBufferOffset   = AdvanceOffset(Counters.NumSBs);
    m_StorageImageOffset    = AdvanceOffset(Counters.NumImgs);
    m_SampledImageOffset    = AdvanceOffset(Counters.NumSmpldImgs);
    m_AtomicCounterOffset   = AdvanceOffset(Counters.NumACs);
    m_SeparateSamplerOffset = AdvanceOffset(Counters.NumSepSmplrs);
    m_SeparateImageOffset   = AdvanceOffset(Counters.NumSepImgs);
    m_InputAttachmentOffset = AdvanceOffset(Counters.NumInptAtts);
    m_AccelStructOffset     = AdvanceOffset(Counters.NumAccelStructs);
    m_TotalResources        = AdvanceOffset(0);
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please update the new resource type offset");

    VERIFY(NumShaderStageInputs <= MaxOffset, "Max offset exceeded");
    m_NumShaderStageInputs = static_cast<OffsetType>(NumShaderStageInputs);

    auto AlignedResourceNamesPoolSize = AlignUp(ResourceNamesPoolSize, sizeof(void*));

    static_assert(sizeof(SPIRVShaderResourceAttribs) % sizeof(void*) == 0, "Size of SPIRVShaderResourceAttribs struct must be multiple of sizeof(void*)");
    // clang-format off
    auto MemorySize = m_TotalResources              * sizeof(SPIRVShaderResourceAttribs) +
                      m_NumShaderStageInputs        * sizeof(SPIRVShaderStageInputAttribs) +
                      AlignedResourceNamesPoolSize  * sizeof(char);

    VERIFY_EXPR(GetNumUBs()          == Counters.NumUBs);
    VERIFY_EXPR(GetNumSBs()          == Counters.NumSBs);
    VERIFY_EXPR(GetNumImgs()         == Counters.NumImgs);
    VERIFY_EXPR(GetNumSmpldImgs()    == Counters.NumSmpldImgs);
    VERIFY_EXPR(GetNumACs()          == Counters.NumACs);
    VERIFY_EXPR(GetNumSepSmplrs()    == Counters.NumSepSmplrs);
    VERIFY_EXPR(GetNumSepImgs()      == Counters.NumSepImgs);
    VERIFY_EXPR(GetNumInptAtts()     == Counters.NumInptAtts);
    VERIFY_EXPR(GetNumAccelStructs() == Counters.NumAccelStructs);
    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please update the new resource count verification");
    // clang-format on

    if (MemorySize)
    {
        auto* pRawMem   = Allocator.Allocate(MemorySize, "Memory for shader resources", __FILE__, __LINE__);
        m_MemoryBuffer  = std::unique_ptr<void, STDDeleterRawMem<void>>(pRawMem, Allocator);
        char* NamesPool = reinterpret_cast<char*>(m_MemoryBuffer.get()) +
            m_TotalResources * sizeof(SPIRVShaderResourceAttribs) +
            m_NumShaderStageInputs * sizeof(SPIRVShaderStageInputAttribs);
        ResourceNamesPool.AssignMemory(NamesPool, ResourceNamesPoolSize);
    }
}

SPIRVShaderResources::~SPIRVShaderResources()
{
    for (Uint32 n = 0; n < GetNumUBs(); ++n)
        GetUB(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSBs(); ++n)
        GetSB(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumImgs(); ++n)
        GetImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSmpldImgs(); ++n)
        GetSmpldImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumACs(); ++n)
        GetAC(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSepSmplrs(); ++n)
        GetSepSmplr(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumSepImgs(); ++n)
        GetSepImg(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumInptAtts(); ++n)
        GetInptAtt(n).~SPIRVShaderResourceAttribs();

    for (Uint32 n = 0; n < GetNumShaderStageInputs(); ++n)
        GetShaderStageInputAttribs(n).~SPIRVShaderStageInputAttribs();

    for (Uint32 n = 0; n < GetNumAccelStructs(); ++n)
        GetAccelStruct(n).~SPIRVShaderResourceAttribs();

    static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please add destructor for the new resource");
}

void SPIRVShaderResources::MapHLSLVertexShaderInputs(std::vector<uint32_t>& SPIRV) const
{
    VERIFY(IsHLSLSource(), "This method is only relevant for HLSL source");

    for (Uint32 i = 0; i < GetNumShaderStageInputs(); ++i)
    {
        const SPIRVShaderStageInputAttribs& Input = GetShaderStageInputAttribs(i);

        const char*        s      = Input.Semantic;
        static const char* Prefix = "attrib";
        const char*        p      = Prefix;
        while (*s != 0 && *p != 0 && *p == std::tolower(static_cast<unsigned char>(*s)))
        {
            ++p;
            ++s;
        }

        if (*p != 0)
        {
            LOG_ERROR_MESSAGE("Unable to map semantic '", Input.Semantic, "' to input location: semantics must have '", Prefix, "x' format.");
            continue;
        }

        char*    EndPtr   = nullptr;
        uint32_t Location = static_cast<uint32_t>(strtol(s, &EndPtr, 10));
        if (*EndPtr != 0)
        {
            LOG_ERROR_MESSAGE("Unable to map semantic '", Input.Semantic, "' to input location: semantics must have '", Prefix, "x' format.");
            continue;
        }
        SPIRV[Input.LocationDecorationOffset] = Location;
    }
}

std::string SPIRVShaderResources::DumpResources() const
{
    std::stringstream ss;
    ss << "Shader '" << m_ShaderName << "' resource stats: total resources: " << GetTotalResources() << ":" << std::endl
       << "UBs: " << GetNumUBs() << "; SBs: " << GetNumSBs() << "; Imgs: " << GetNumImgs() << "; Smpl Imgs: " << GetNumSmpldImgs()
       << "; ACs: " << GetNumACs() << "; Sep Imgs: " << GetNumSepImgs() << "; Sep Smpls: " << GetNumSepSmplrs() << '.' << std::endl
       << "Resources:";

    Uint32 ResNum       = 0;
    auto   DumpResource = [&ss, &ResNum](const SPIRVShaderResourceAttribs& Res) {
        std::stringstream FullResNameSS;
        FullResNameSS << '\'' << Res.Name;
        if (Res.ArraySize > 1)
            FullResNameSS << '[' << Res.ArraySize << ']';
        FullResNameSS << '\'';
        ss << std::setw(32) << FullResNameSS.str();
        ++ResNum;
    };

    ProcessResources(
        [&](const SPIRVShaderResourceAttribs& UB, Uint32) //
        {
            VERIFY(UB.Type == SPIRVShaderResourceAttribs::ResourceType::UniformBuffer, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Uniform Buffer     ";
            DumpResource(UB);
        },
        [&](const SPIRVShaderResourceAttribs& SB, Uint32) //
        {
            VERIFY(SB.Type == SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer ||
                       SB.Type == SPIRVShaderResourceAttribs::ResourceType::RWStorageBuffer,
                   "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum
               << (SB.Type == SPIRVShaderResourceAttribs::ResourceType::ROStorageBuffer ? " RO Storage Buffer  " : " RW Storage Buffer  ");
            DumpResource(SB);
        },
        [&](const SPIRVShaderResourceAttribs& Img, Uint32) //
        {
            if (Img.Type == SPIRVShaderResourceAttribs::ResourceType::StorageImage)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Storage Image    ";
            }
            else if (Img.Type == SPIRVShaderResourceAttribs::ResourceType::StorageTexelBuffer)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Storage Txl Buff ";
            }
            else
                UNEXPECTED("Unexpected resource type");
            DumpResource(Img);
        },
        [&](const SPIRVShaderResourceAttribs& SmplImg, Uint32) //
        {
            if (SmplImg.Type == SPIRVShaderResourceAttribs::ResourceType::SampledImage)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Sampled Image    ";
            }
            else if (SmplImg.Type == SPIRVShaderResourceAttribs::ResourceType::UniformTexelBuffer)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Uniform Txl Buff ";
            }
            else
                UNEXPECTED("Unexpected resource type");
            DumpResource(SmplImg);
        },
        [&](const SPIRVShaderResourceAttribs& AC, Uint32) //
        {
            VERIFY(AC.Type == SPIRVShaderResourceAttribs::ResourceType::AtomicCounter, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Atomic Cntr      ";
            DumpResource(AC);
        },
        [&](const SPIRVShaderResourceAttribs& SepSmpl, Uint32) //
        {
            VERIFY(SepSmpl.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateSampler, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Separate Smpl    ";
            DumpResource(SepSmpl);
        },
        [&](const SPIRVShaderResourceAttribs& SepImg, Uint32) //
        {
            VERIFY(SepImg.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateImage, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Separate Img     ";
            DumpResource(SepImg);
        },
        [&](const SPIRVShaderResourceAttribs& InptAtt, Uint32) //
        {
            VERIFY(InptAtt.Type == SPIRVShaderResourceAttribs::ResourceType::InputAttachment, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Input Attachment ";
            DumpResource(InptAtt);
        },
        [&](const SPIRVShaderResourceAttribs& AccelStruct, Uint32) //
        {
            VERIFY(AccelStruct.Type == SPIRVShaderResourceAttribs::ResourceType::AccelerationStructure, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Accel Struct     ";
            DumpResource(AccelStruct);
        } //
    );
    VERIFY_EXPR(ResNum == GetTotalResources());

    return ss.str();
}

} // namespace Diligent
