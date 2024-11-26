/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "WGSLShaderResources.hpp"
#include "Align.hpp"
#include "StringPool.hpp"
#include "WGSLUtils.hpp"
#include "DataBlobImpl.hpp"
#include "ShaderToolsCommon.hpp"

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324) //  warning C4324: structure was padded due to alignment specifier
#endif

#include <tint/tint.h>

#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/f16.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/scalar.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/wgsl/ast/module.h"
#include "src/tint/lang/wgsl/ast/identifier_expression.h"
#include "src/tint/lang/wgsl/ast/identifier.h"
#include "src/tint/lang/wgsl/ast/struct.h"
#include "src/tint/lang/wgsl/sem/struct.h"
#include "src/tint/lang/wgsl/sem/variable.h"

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

namespace Diligent
{

namespace
{

SHADER_TYPE TintPipelineStageToShaderType(tint::inspector::PipelineStage Stage)
{
    switch (Stage)
    {
        case tint::inspector::PipelineStage::kVertex:
            return SHADER_TYPE_VERTEX;

        case tint::inspector::PipelineStage::kFragment:
            return SHADER_TYPE_PIXEL;

        case tint::inspector::PipelineStage::kCompute:
            return SHADER_TYPE_COMPUTE;

        default:
            UNEXPECTED("Unexpected pipeline stage");
            return SHADER_TYPE_UNKNOWN;
    }
}

WGSLShaderResourceAttribs::ResourceType TintResourceTypeToWGSLShaderAttribsResourceType(tint::inspector::ResourceBinding::ResourceType TintResType)
{
    using TintResourceType = tint::inspector::ResourceBinding::ResourceType;
    switch (TintResType)
    {
        case TintResourceType::kUniformBuffer:
            return WGSLShaderResourceAttribs::ResourceType::UniformBuffer;

        case TintResourceType::kStorageBuffer:
            return WGSLShaderResourceAttribs::ResourceType::RWStorageBuffer;

        case TintResourceType::kReadOnlyStorageBuffer:
            return WGSLShaderResourceAttribs::ResourceType::ROStorageBuffer;

        case TintResourceType::kSampler:
            return WGSLShaderResourceAttribs::ResourceType::Sampler;

        case TintResourceType::kComparisonSampler:
            return WGSLShaderResourceAttribs::ResourceType::ComparisonSampler;

        case TintResourceType::kSampledTexture:
            return WGSLShaderResourceAttribs::ResourceType::Texture;

        case TintResourceType::kMultisampledTexture:
            return WGSLShaderResourceAttribs::ResourceType::TextureMS;

        case TintResourceType::kWriteOnlyStorageTexture:
            return WGSLShaderResourceAttribs::ResourceType::WOStorageTexture;

        case TintResourceType::kReadOnlyStorageTexture:
            return WGSLShaderResourceAttribs::ResourceType::ROStorageTexture;

        case TintResourceType::kReadWriteStorageTexture:
            return WGSLShaderResourceAttribs::ResourceType::RWStorageTexture;

        case TintResourceType::kDepthTexture:
            return WGSLShaderResourceAttribs::ResourceType::DepthTexture;

        case TintResourceType::kDepthMultisampledTexture:
            return WGSLShaderResourceAttribs::ResourceType::DepthTextureMS;

        case TintResourceType::kExternalTexture:
            return WGSLShaderResourceAttribs::ResourceType::ExternalTexture;

        case TintResourceType::kInputAttachment:
            UNEXPECTED("Input attachments are not currently supported");
            return WGSLShaderResourceAttribs::ResourceType::NumResourceTypes;

        default:
            UNEXPECTED("Unexpected resource type");
            return WGSLShaderResourceAttribs::ResourceType::NumResourceTypes;
    }
}

WGSLShaderResourceAttribs::TextureSampleType TintSampleKindToWGSLShaderAttribsSampleType(const tint::inspector::ResourceBinding& TintBinding)
{
    using TintResourceType = tint::inspector::ResourceBinding::ResourceType;
    using TintSampledKind  = tint::inspector::ResourceBinding::SampledKind;

    if (TintBinding.resource_type == TintResourceType::kSampledTexture ||
        TintBinding.resource_type == TintResourceType::kMultisampledTexture ||
        TintBinding.resource_type == TintResourceType::kWriteOnlyStorageTexture ||
        TintBinding.resource_type == TintResourceType::kReadOnlyStorageTexture ||
        TintBinding.resource_type == TintResourceType::kReadWriteStorageTexture ||
        TintBinding.resource_type == TintResourceType::kExternalTexture)
    {
        switch (TintBinding.sampled_kind)
        {
            case TintSampledKind::kFloat:
                return WGSLShaderResourceAttribs::TextureSampleType::Float;

            case TintSampledKind::kSInt:
                return WGSLShaderResourceAttribs::TextureSampleType::SInt;

            case TintSampledKind::kUInt:
                return WGSLShaderResourceAttribs::TextureSampleType::UInt;

            case TintSampledKind::kUnknown:
                return WGSLShaderResourceAttribs::TextureSampleType::Unknown;

            default:
                UNEXPECTED("Unexpected sample kind");
                return WGSLShaderResourceAttribs::TextureSampleType::Unknown;
        }
    }
    else if (TintBinding.resource_type == TintResourceType::kDepthTexture ||
             TintBinding.resource_type == TintResourceType::kDepthMultisampledTexture)
    {
        return WGSLShaderResourceAttribs::TextureSampleType::Depth;
    }
    else
    {
        return WGSLShaderResourceAttribs::TextureSampleType::Unknown;
    }
}

RESOURCE_DIMENSION TintTextureDimensionToResourceDimension(tint::inspector::ResourceBinding::TextureDimension TintTexDim)
{
    using TintTextureDim = tint::inspector::ResourceBinding::TextureDimension;
    switch (TintTexDim)
    {
        case TintTextureDim::k1d:
            return RESOURCE_DIM_TEX_1D;

        case TintTextureDim::k2d:
            return RESOURCE_DIM_TEX_2D;

        case TintTextureDim::k2dArray:
            return RESOURCE_DIM_TEX_2D_ARRAY;

        case TintTextureDim::k3d:
            return RESOURCE_DIM_TEX_3D;

        case TintTextureDim::kCube:
            return RESOURCE_DIM_TEX_CUBE;

        case TintTextureDim::kCubeArray:
            return RESOURCE_DIM_TEX_CUBE_ARRAY;

        case TintTextureDim::kNone:
            return RESOURCE_DIM_UNDEFINED;

        default:
            UNEXPECTED("Unexpected texture dimension");
            return RESOURCE_DIM_UNDEFINED;
    }
}

RESOURCE_DIMENSION TintBindingToResourceDimension(const tint::inspector::ResourceBinding& TintBinding)
{
    using TintResourceType = tint::inspector::ResourceBinding::ResourceType;
    switch (TintBinding.resource_type)
    {
        case TintResourceType::kUniformBuffer:
        case TintResourceType::kStorageBuffer:
        case TintResourceType::kReadOnlyStorageBuffer:
            return RESOURCE_DIM_BUFFER;

        case TintResourceType::kSampler:
        case TintResourceType::kComparisonSampler:
            return RESOURCE_DIM_UNDEFINED;

        case TintResourceType::kSampledTexture:
        case TintResourceType::kMultisampledTexture:
        case TintResourceType::kWriteOnlyStorageTexture:
        case TintResourceType::kReadOnlyStorageTexture:
        case TintResourceType::kReadWriteStorageTexture:
        case TintResourceType::kDepthTexture:
        case TintResourceType::kDepthMultisampledTexture:
        case TintResourceType::kExternalTexture:
            return TintTextureDimensionToResourceDimension(TintBinding.dim);

        case TintResourceType::kInputAttachment:
            return RESOURCE_DIM_UNDEFINED;

        default:
            UNEXPECTED("Unexpected resource type");
            return RESOURCE_DIM_UNDEFINED;
    }
}

TEXTURE_FORMAT TintTexelFormatToTextureFormat(const tint::inspector::ResourceBinding& TintBinding)
{
    using TintResourceType = tint::inspector::ResourceBinding::ResourceType;
    using TintTexelFormat  = tint::inspector::ResourceBinding::TexelFormat;

    if (TintBinding.resource_type != TintResourceType::kWriteOnlyStorageTexture &&
        TintBinding.resource_type != TintResourceType::kReadOnlyStorageTexture &&
        TintBinding.resource_type != TintResourceType::kReadWriteStorageTexture)
    {
        // Format is only defined for storage textures
        return TEX_FORMAT_UNKNOWN;
    }

    switch (TintBinding.image_format)
    {
            // clang-format off
        case TintTexelFormat::kBgra8Unorm:  return TEX_FORMAT_BGRA8_UNORM;
        case TintTexelFormat::kRgba8Unorm:  return TEX_FORMAT_RGBA8_UNORM;
        case TintTexelFormat::kRgba8Snorm:  return TEX_FORMAT_RGBA8_SNORM;
        case TintTexelFormat::kRgba8Uint:   return TEX_FORMAT_RGBA8_UINT;
        case TintTexelFormat::kRgba8Sint:   return TEX_FORMAT_RGBA8_SINT;
        case TintTexelFormat::kRgba16Uint:  return TEX_FORMAT_RGBA16_UINT;
        case TintTexelFormat::kRgba16Sint:  return TEX_FORMAT_RGBA16_SINT;
        case TintTexelFormat::kRgba16Float: return TEX_FORMAT_RGBA16_FLOAT;
        case TintTexelFormat::kR32Uint:     return TEX_FORMAT_R32_UINT;
        case TintTexelFormat::kR32Sint:     return TEX_FORMAT_R32_SINT;
        case TintTexelFormat::kR32Float:    return TEX_FORMAT_R32_FLOAT;
        case TintTexelFormat::kRg32Uint:    return TEX_FORMAT_RG32_UINT;
        case TintTexelFormat::kRg32Sint:    return TEX_FORMAT_RG32_SINT;
        case TintTexelFormat::kRg32Float:   return TEX_FORMAT_RG32_FLOAT;
        case TintTexelFormat::kRgba32Uint:  return TEX_FORMAT_RGBA32_UINT;
        case TintTexelFormat::kRgba32Sint:  return TEX_FORMAT_RGBA32_SINT;
        case TintTexelFormat::kRgba32Float: return TEX_FORMAT_RGBA32_FLOAT;
        case TintTexelFormat::kR8Unorm:     return TEX_FORMAT_R8_UNORM;
        case TintTexelFormat::kNone:        return TEX_FORMAT_UNKNOWN;
        // clang-format on
        default:
            UNEXPECTED("Unexpected texel format");
            return TEX_FORMAT_UNKNOWN;
    }
}

WEB_GPU_BINDING_TYPE GetWebGPUTextureBindingType(WGSLShaderResourceAttribs::TextureSampleType SampleType, bool IsMultisample, bool IsUnfilterable)
{
    using TextureSampleType = WGSLShaderResourceAttribs::TextureSampleType;
    switch (SampleType)
    {
        case TextureSampleType::Float:
            return IsMultisample ?
                (IsUnfilterable ? WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE_MS : WEB_GPU_BINDING_TYPE_FLOAT_TEXTURE_MS) :
                (IsUnfilterable ? WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE : WEB_GPU_BINDING_TYPE_FLOAT_TEXTURE);

        case TextureSampleType::UInt:
            return IsMultisample ?
                WEB_GPU_BINDING_TYPE_UINT_TEXTURE_MS :
                WEB_GPU_BINDING_TYPE_UINT_TEXTURE;

        case TextureSampleType::SInt:
            return IsMultisample ?
                WEB_GPU_BINDING_TYPE_SINT_TEXTURE_MS :
                WEB_GPU_BINDING_TYPE_SINT_TEXTURE;

        case TextureSampleType::UnfilterableFloat:
            return IsMultisample ?
                WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE_MS :
                WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE;

        case TextureSampleType::Depth:
            return IsMultisample ?
                WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE_MS :
                WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE;

        default:
            UNEXPECTED("Unexpected texture sample type");
            return WEB_GPU_BINDING_TYPE_DEFAULT;
    }
}

} // namespace

WGSLShaderResourceAttribs::WGSLShaderResourceAttribs(const char*                             _Name,
                                                     const tint::inspector::ResourceBinding& TintBinding,
                                                     Uint32                                  _ArraySize) noexcept :
    // clang-format off
    Name             {_Name},
    ArraySize        {static_cast<Uint16>(_ArraySize)},
    Type             {TintResourceTypeToWGSLShaderAttribsResourceType(TintBinding.resource_type)},
    ResourceDim      {TintBindingToResourceDimension(TintBinding)},
    Format			 {TintTexelFormatToTextureFormat(TintBinding)},
    BindGroup        {static_cast<Uint16>(TintBinding.bind_group)},
    BindIndex        {static_cast<Uint16>(TintBinding.binding)},
    SampleType       {TintSampleKindToWGSLShaderAttribsSampleType(TintBinding)},
    BufferStaticSize {TintBinding.resource_type == tint::inspector::ResourceBinding::ResourceType::kUniformBuffer ? static_cast<Uint32>(TintBinding.size) : 0}
// clang-format on
{}

WGSLShaderResourceAttribs::WGSLShaderResourceAttribs(const char*        _Name,
                                                     ResourceType       _Type,
                                                     Uint16             _ArraySize,
                                                     RESOURCE_DIMENSION _ResourceDim,
                                                     TEXTURE_FORMAT     _Format,
                                                     TextureSampleType  _SampleType,
                                                     uint16_t           _BindGroup,
                                                     uint16_t           _BindIndex,
                                                     Uint32             _BufferStaticSize) noexcept :
    Name{_Name},
    ArraySize{_ArraySize},
    Type{_Type},
    ResourceDim{_ResourceDim},
    Format{_Format},
    BindGroup{_BindGroup},
    BindIndex{_BindIndex},
    SampleType{_SampleType},
    BufferStaticSize{_BufferStaticSize}
{
}

SHADER_RESOURCE_TYPE WGSLShaderResourceAttribs::GetShaderResourceType(ResourceType Type)
{
    static_assert(Uint32{WGSLShaderResourceAttribs::ResourceType::NumResourceTypes} == 13, "Please handle the new resource type below");
    switch (Type)
    {
        case WGSLShaderResourceAttribs::ResourceType::UniformBuffer:
            return SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;

        case WGSLShaderResourceAttribs::ResourceType::ROStorageBuffer:
            return SHADER_RESOURCE_TYPE_BUFFER_SRV;

        case WGSLShaderResourceAttribs::ResourceType::RWStorageBuffer:
            return SHADER_RESOURCE_TYPE_BUFFER_UAV;

        case WGSLShaderResourceAttribs::ResourceType::Sampler:
        case WGSLShaderResourceAttribs::ResourceType::ComparisonSampler:
            return SHADER_RESOURCE_TYPE_SAMPLER;

        case WGSLShaderResourceAttribs::ResourceType::Texture:
        case WGSLShaderResourceAttribs::ResourceType::TextureMS:
        case WGSLShaderResourceAttribs::ResourceType::DepthTexture:
        case WGSLShaderResourceAttribs::ResourceType::DepthTextureMS:
            return SHADER_RESOURCE_TYPE_TEXTURE_SRV;

        case WGSLShaderResourceAttribs::ResourceType::WOStorageTexture:
        case WGSLShaderResourceAttribs::ResourceType::ROStorageTexture:
        case WGSLShaderResourceAttribs::ResourceType::RWStorageTexture:
            return SHADER_RESOURCE_TYPE_TEXTURE_UAV;

        case WGSLShaderResourceAttribs::ResourceType::ExternalTexture:
            LOG_WARNING_MESSAGE("External textures are not currently supported");
            return SHADER_RESOURCE_TYPE_UNKNOWN;

        default:
            UNEXPECTED("Unknown WGSL resource type");
            return SHADER_RESOURCE_TYPE_UNKNOWN;
    }
}

PIPELINE_RESOURCE_FLAGS WGSLShaderResourceAttribs::GetPipelineResourceFlags(ResourceType Type)
{
    return PIPELINE_RESOURCE_FLAG_NONE;
}

WebGPUResourceAttribs WGSLShaderResourceAttribs::GetWebGPUAttribs(SHADER_VARIABLE_FLAGS Flags) const
{
    WebGPUResourceAttribs WebGPUAttribs;
    static_assert(Uint32{WGSLShaderResourceAttribs::ResourceType::NumResourceTypes} == 13, "Please handle the new resource type below");
    switch (Type)
    {
        case ResourceType::UniformBuffer:
        case ResourceType::ROStorageBuffer:
        case ResourceType::RWStorageBuffer:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_DEFAULT;
            break;

        case ResourceType::Sampler:
            WebGPUAttribs.BindingType = (Flags & SHADER_VARIABLE_FLAG_NON_FILTERING_SAMPLER_WEBGPU) ? WEB_GPU_BINDING_TYPE_NON_FILTERING_SAMPLER : WEB_GPU_BINDING_TYPE_FILTERING_SAMPLER;
            break;

        case ResourceType::ComparisonSampler:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_COMPARISON_SAMPLER;
            break;

        case ResourceType::Texture:
        case ResourceType::TextureMS:
            WebGPUAttribs.BindingType = GetWebGPUTextureBindingType(SampleType, /* IsMultisample = */ Type == ResourceType::TextureMS, /* IsUnfilterable = */ (Flags & SHADER_VARIABLE_FLAG_UNFILTERABLE_FLOAT_TEXTURE_WEBGPU));
            break;

        case ResourceType::DepthTexture:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE;
            break;

        case ResourceType::DepthTextureMS:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE_MS;
            break;

        case ResourceType::WOStorageTexture:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_WRITE_ONLY_TEXTURE_UAV;
            break;

        case ResourceType::ROStorageTexture:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_READ_ONLY_TEXTURE_UAV;
            break;

        case ResourceType::RWStorageTexture:
            WebGPUAttribs.BindingType = WEB_GPU_BINDING_TYPE_READ_WRITE_TEXTURE_UAV;
            break;

        case ResourceType::ExternalTexture:
            LOG_WARNING_MESSAGE("External textures are not currently supported");
            break;

        default:
            UNEXPECTED("Unknown WGSL resource type");
    }

    if (Type == ResourceType::Texture ||
        Type == ResourceType::TextureMS ||
        Type == ResourceType::DepthTexture ||
        Type == ResourceType::DepthTextureMS ||
        Type == ResourceType::WOStorageTexture ||
        Type == ResourceType::ROStorageTexture ||
        Type == ResourceType::RWStorageTexture)
    {
        WebGPUAttribs.TextureViewDim = GetResourceDimension();
    }
    WebGPUAttribs.UAVTextureFormat = Format;
    return WebGPUAttribs;
}

namespace
{

bool ResourceBindingsCompatibile(const tint::inspector::ResourceBinding& Binding0,
                                 const tint::inspector::ResourceBinding& Binding1)
{
    if (Binding0.resource_type != Binding1.resource_type)
        return false;

    if (TintBindingToResourceDimension(Binding0) != TintBindingToResourceDimension(Binding1))
        return false;

    if (TintSampleKindToWGSLShaderAttribsSampleType(Binding0) != TintSampleKindToWGSLShaderAttribsSampleType(Binding1))
        return false;

    if (TintTexelFormatToTextureFormat(Binding0) != TintTexelFormatToTextureFormat(Binding1))
        return false;

    return true;
}

void MergeResources(std::vector<tint::inspector::ResourceBinding>& Bindings, std::vector<Uint32>& ArraySizes, const std::string& Suffix)
{
    std::vector<WGSLEmulatedResourceArrayElement> ArrayElements(Bindings.size());

    struct ArrayInfo
    {
        bool                IsValid     = true;
        int                 ResourceIdx = -1;
        std::vector<size_t> ElementInds;
    };
    std::unordered_map<std::string, ArrayInfo> Arrays;

    // Group resources into arrays
    for (size_t i = 0; i < Bindings.size(); ++i)
    {
        const tint::inspector::ResourceBinding& Binding = Bindings[i];
        WGSLEmulatedResourceArrayElement&       Element = ArrayElements[i];

        Element = GetWGSLEmulatedArrayElement(Binding.variable_name, Suffix);
        if (Element.IsValid())
        {
            Arrays[Element.Name].ElementInds.push_back(i);
        }
    }

    // Check that all array elements are compatible
    for (auto& array_it : Arrays)
    {
        const std::vector<size_t>&              ElementInds = array_it.second.ElementInds;
        const tint::inspector::ResourceBinding& Binding0    = Bindings[ElementInds[0]];
        for (size_t i = 1; i < ElementInds.size(); ++i)
        {
            const tint::inspector::ResourceBinding& Binding = Bindings[ElementInds[i]];
            if (!ResourceBindingsCompatibile(Binding0, Binding))
            {
                array_it.second.IsValid = false;
                break;
            }
        }
    }

    // Merge arrays
    std::vector<tint::inspector::ResourceBinding> MergedBindings;
    for (size_t i = 0; i < Bindings.size(); ++i)
    {
        tint::inspector::ResourceBinding&       Binding = Bindings[i];
        const WGSLEmulatedResourceArrayElement& Element = ArrayElements[i];
        if (Element.IsValid())
        {
            VERIFY_EXPR(Arrays.find(Element.Name) != Arrays.end());
            ArrayInfo& Array = Arrays[Element.Name];
            if (Array.IsValid)
            {
                if (Array.ResourceIdx < 0)
                {
                    Array.ResourceIdx = static_cast<int>(MergedBindings.size());
                    MergedBindings.push_back(std::move(Binding));
                    MergedBindings.back().variable_name = Element.Name;
                }
                tint::inspector::ResourceBinding& ArrayBinding = MergedBindings[Array.ResourceIdx];
                VERIFY_EXPR(ArrayBinding.variable_name == Element.Name);
                if (ArraySizes.size() <= static_cast<size_t>(Array.ResourceIdx))
                {
                    ArraySizes.resize(Array.ResourceIdx + 1, 0);
                }
                ArraySizes[Array.ResourceIdx] = std::max(ArraySizes[Array.ResourceIdx], static_cast<Uint32>(Element.Index + 1));
            }
            else
            {
                MergedBindings.push_back(std::move(Binding));
            }
        }
        else
        {
            // Not an array
            VERIFY_EXPR(Arrays.find(Element.Name) == Arrays.end());
            MergedBindings.push_back(std::move(Binding));
        }
    }
    Bindings.swap(MergedBindings);
}


void LoadShaderCodeVariableDesc(const tint::Program&          Program,
                                const tint::core::type::Type* WGSLType,
                                SHADER_SOURCE_LANGUAGE        Language,
                                ShaderCodeVariableDescX&      TypeDesc)
{
    auto GetBasicType = [](const tint::core::type::Type* Type) -> SHADER_CODE_BASIC_TYPE {
        if (Type->Is<tint::core::type::F32>())
        {
            return SHADER_CODE_BASIC_TYPE_FLOAT;
        }
        else if (Type->Is<tint::core::type::I32>())
        {
            return SHADER_CODE_BASIC_TYPE_INT;
        }
        else if (Type->Is<tint::core::type::U32>())
        {
            return SHADER_CODE_BASIC_TYPE_UINT;
        }
        else if (Type->Is<tint::core::type::F16>())
        {
            return SHADER_CODE_BASIC_TYPE_FLOAT16;
        }
        else
        {
            UNEXPECTED("Unexpected scalar type");
            return SHADER_CODE_BASIC_TYPE_UNKNOWN;
        }
    };

    auto GetArraySize = [](const tint::core::type::Array* ArrType) -> Uint32 {
        if (ArrType->Count()->Is<tint::core::type::ConstantArrayCount>())
        {
            return ArrType->Count()->As<tint::core::type::ConstantArrayCount>()->value;
        }
        UNEXPECTED("Unexpected type");
        return 0;
    };

    if (WGSLType->Is<tint::core::type::Array>())
    {
        const tint::core::type::Array* ArrType  = WGSLType->As<tint::core::type::Array>();
        const tint::core::type::Type*  ElemType = ArrType->ElemType();

        // Path for HLSL
        if (ElemType->FriendlyName() == "strided_arr" && Language == SHADER_SOURCE_LANGUAGE_HLSL)
        {
            const tint::core::type::StructMember* StructMember = ElemType->As<tint::core::type::Struct>()->Members().Front();
            const tint::core::type::Vector*       MemberType   = StructMember->Type()->As<tint::core::type::Vector>();

            TypeDesc.Class      = SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS;
            TypeDesc.BasicType  = GetBasicType(MemberType->type());
            TypeDesc.NumColumns = StaticCast<decltype(TypeDesc.NumColumns)>(MemberType->Width());
            TypeDesc.NumRows    = static_cast<decltype(TypeDesc.NumRows)>(GetArraySize(ArrType));
        }
        else
        {
            LoadShaderCodeVariableDesc(Program, ElemType, Language, TypeDesc);
            TypeDesc.ArraySize = GetArraySize(ArrType);
        }
    }
    else
    {
        if (WGSLType->Is<tint::core::type::Struct>())
        {
            TypeDesc.Class = SHADER_CODE_VARIABLE_CLASS_STRUCT;

            for (const tint::core::type::StructMember* Member : WGSLType->As<tint::core::type::Struct>()->Members())
            {
                ShaderCodeVariableDesc VarDesc;
                VarDesc.Name   = Member->Name().NameView().data();
                VarDesc.Offset = Member->Offset();

                size_t VariableIdx = TypeDesc.AddMember(VarDesc);
                LoadShaderCodeVariableDesc(Program, Member->Type(), Language, TypeDesc.GetMember(VariableIdx));
            }

            TypeDesc.SetTypeName(WGSLType->FriendlyName());
        }
        else
        {
            if (WGSLType->Is<tint::core::type::Scalar>())
            {
                TypeDesc.Class      = SHADER_CODE_VARIABLE_CLASS_SCALAR;
                TypeDesc.BasicType  = GetBasicType(WGSLType);
                TypeDesc.NumRows    = 1;
                TypeDesc.NumColumns = 1;
            }
            else if (WGSLType->Is<tint::core::type::Vector>())
            {
                const tint::core::type::Vector* VecType = WGSLType->As<tint::core::type::Vector>();

                TypeDesc.Class      = SHADER_CODE_VARIABLE_CLASS_VECTOR;
                TypeDesc.BasicType  = GetBasicType(VecType->type());
                TypeDesc.NumRows    = StaticCast<decltype(TypeDesc.NumColumns)>(VecType->Width());
                TypeDesc.NumColumns = 1;
            }
            else if (WGSLType->Is<tint::core::type::Matrix>())
            {
                const tint::core::type::Matrix* MatType = WGSLType->As<tint::core::type::Matrix>();

                TypeDesc.Class      = SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS;
                TypeDesc.BasicType  = GetBasicType(MatType->type());
                TypeDesc.NumRows    = StaticCast<decltype(TypeDesc.NumRows)>(MatType->rows());
                TypeDesc.NumColumns = StaticCast<decltype(TypeDesc.NumColumns)>(MatType->columns());
            }
            else
            {
                UNEXPECTED("Unexpected type");
            }
        }

        if (Language == SHADER_SOURCE_LANGUAGE_HLSL)
            std::swap(TypeDesc.NumRows, TypeDesc.NumColumns);
    }

    if (TypeDesc.TypeName == nullptr || TypeDesc.TypeName[0] == '\0')
    {
        if (Language == SHADER_SOURCE_LANGUAGE_WGSL || Language == SHADER_SOURCE_LANGUAGE_DEFAULT)
            TypeDesc.SetTypeName(WGSLType->FriendlyName());
        else
            TypeDesc.SetDefaultTypeName(Language);
    }
}

ShaderCodeBufferDescX LoadUBReflection(const tint::Program& Program, const tint::inspector::ResourceBinding& UB, SHADER_SOURCE_LANGUAGE Language)
{

    const tint::ast::Module& Ast = Program.AST();
    const tint::sem::Info&   Sem = Program.Sem();

    const tint::ast::Variable* Variable = nullptr;
    for (const tint::ast::Variable* Var : Ast.GlobalVariables())
    {
        if (Var->HasBindingPoint())
        {
            const tint::sem::GlobalVariable* SemVariable = Sem.Get(Var)->As<tint::sem::GlobalVariable>();
            if (SemVariable->Attributes().binding_point->group == UB.bind_group && SemVariable->Attributes().binding_point->binding == UB.binding)
            {
                Variable = Var;
                break;
            }
        }
    }
    VERIFY(Variable, "Unexpected error");


    const tint::core::type::Struct* WGSLType = Program.TypeOf(Variable->type)->As<tint::core::type::Struct>();
    const uint32_t                  Size     = WGSLType->Size();

    ShaderCodeBufferDescX UBDesc;
    UBDesc.Size = Size;
    for (const tint::core::type::StructMember* Member : WGSLType->Members())
    {
        ShaderCodeVariableDesc VarDesc;
        VarDesc.Name   = Member->Name().NameView().data();
        VarDesc.Offset = Member->Offset();

        size_t VariableIdx = UBDesc.AddVariable(VarDesc);
        LoadShaderCodeVariableDesc(Program, Member->Type(), Language, UBDesc.GetVariable(VariableIdx));
    }

    return UBDesc;
}

} // namespace

WGSLShaderResources::WGSLShaderResources(IMemoryAllocator&      Allocator,
                                         const std::string&     WGSL,
                                         SHADER_SOURCE_LANGUAGE SourceLanguage,
                                         const char*            ShaderName,
                                         const char*            CombinedSamplerSuffix,
                                         const char*            EntryPoint,
                                         const char*            EmulatedArrayIndexSuffix,
                                         bool                   LoadUniformBufferReflection,
                                         IDataBlob**            ppTintOutput) noexcept(false)
{
    VERIFY_EXPR(ShaderName != nullptr);

    tint::Source::File SrcFile{"", WGSL};
    tint::Program      Program = tint::wgsl::reader::Parse(&SrcFile, {tint::wgsl::AllowedFeatures::Everything()});

    const std::string Diagnostics = Program.Diagnostics().Str();
    if (!Diagnostics.empty() && ppTintOutput != nullptr)
    {
        RefCntAutoPtr<DataBlobImpl> pOutputDataBlob = DataBlobImpl::Create(WGSL.length() + 1 + Diagnostics.length() + 1);

        char* DataPtr = reinterpret_cast<char*>(pOutputDataBlob->GetDataPtr());
        memcpy(DataPtr, Diagnostics.data(), Diagnostics.length() + 1);
        memcpy(DataPtr + Diagnostics.length() + 1, WGSL.data(), WGSL.length() + 1);
        pOutputDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppTintOutput));
    }

    if (!Program.IsValid())
    {
        LOG_ERROR_AND_THROW("Failed to parse shader source '", ShaderName, "':\n",
                            Diagnostics, "\n");
    }

    tint::inspector::Inspector Inspector{Program};

    const auto EntryPoints = Inspector.GetEntryPoints();
    if (EntryPoints.empty())
    {
        LOG_ERROR_AND_THROW("The program does not contain any entry points");
    }

    size_t EntryPointIdx = 0;
    if (EntryPoint == nullptr)
    {
        VERIFY(EntryPoints.size() == 1, "The program contains more than one entry point. Please specify the entry point name.");
        EntryPoint    = EntryPoints[0].name.c_str();
        EntryPointIdx = 0;
    }
    else
    {
        for (EntryPointIdx = 0; EntryPointIdx < EntryPoints.size(); ++EntryPointIdx)
        {
            if (EntryPoints[EntryPointIdx].name == EntryPoint)
                break;
        }
        if (EntryPointIdx == EntryPoints.size())
        {
            LOG_ERROR_AND_THROW("Entry point '", EntryPoint, "' is not found in shader '", ShaderName, "'");
        }
    }
    m_ShaderType = TintPipelineStageToShaderType(EntryPoints[EntryPointIdx].stage);

    std::vector<tint::inspector::ResourceBinding> ResourceBindings = Inspector.GetResourceBindings(EntryPoint);
    if (SourceLanguage != SHADER_SOURCE_LANGUAGE_WGSL)
    {
        //   HLSL:
        //      struct BufferData0
        //      {
        //          float4 data;
        //      };
        //      StructuredBuffer<BufferData0> g_Buff0;
        //      StructuredBuffer<BufferData0> g_Buff1;
        //      StructuredBuffer<int>         g_AtomicBuff0; // Used in atomic operations
        //      StructuredBuffer<int>         g_AtomicBuff1; // Used in atomic operations
        //   WGSL:
        //      struct g_Buff0 {
        //        x_data : RTArr,
        //      }
        //      @group(0) @binding(0) var<storage, read> g_Buff0_1       : g_Buff0;
        //      @group(0) @binding(1) var<storage, read> g_Buff1         : g_Buff0;
        //      @group(0) @binding(2) var<storage, read> g_AtomicBuff0_1 : g_AtomicBuff0_atomic;
        //      @group(0) @binding(3) var<storage, read> g_AtomicBuff1   : g_AtomicBuff0_atomic;
        for (tint::inspector::ResourceBinding& Binding : ResourceBindings)
        {
            std::string AltName = GetWGSLResourceAlternativeName(Program, Binding);
            if (!AltName.empty())
            {
                Binding.variable_name = std::move(AltName);
            }
        }
    }

    std::vector<Uint32> ArraySizes;
    if (EmulatedArrayIndexSuffix != nullptr && EmulatedArrayIndexSuffix[0] != '\0')
    {
        MergeResources(ResourceBindings, ArraySizes, EmulatedArrayIndexSuffix);
    }

    using TintResourceType = tint::inspector::ResourceBinding::ResourceType;

    // Count resources
    ResourceCounters ResCounters;
    size_t           ResourceNamesPoolSize = 0;
    for (const auto& Binding : ResourceBindings)
    {
        switch (Binding.resource_type)
        {
            case TintResourceType::kUniformBuffer:
                ++ResCounters.NumUBs;
                break;

            case TintResourceType::kStorageBuffer:
            case TintResourceType::kReadOnlyStorageBuffer:
                ++ResCounters.NumSBs;
                break;

            case TintResourceType::kSampler:
            case TintResourceType::kComparisonSampler:
                ++ResCounters.NumSamplers;
                break;

            case TintResourceType::kSampledTexture:
            case TintResourceType::kMultisampledTexture:
            case TintResourceType::kDepthTexture:
            case TintResourceType::kDepthMultisampledTexture:
                ++ResCounters.NumTextures;
                break;

            case TintResourceType::kWriteOnlyStorageTexture:
            case TintResourceType::kReadOnlyStorageTexture:
            case TintResourceType::kReadWriteStorageTexture:
                ++ResCounters.NumStTextures;
                break;

            case TintResourceType::kExternalTexture:
                ++ResCounters.NumExtTextures;
                break;

            case TintResourceType::kInputAttachment:
                UNSUPPORTED("Input attachments are not currently supported");
                break;

            default:
                UNEXPECTED("Unexpected resource type");
        }

        ResourceNamesPoolSize += Binding.variable_name.length() + 1;
    }

    if (CombinedSamplerSuffix != nullptr)
    {
        ResourceNamesPoolSize += strlen(CombinedSamplerSuffix) + 1;
    }
    if (EmulatedArrayIndexSuffix != nullptr)
    {
        ResourceNamesPoolSize += strlen(EmulatedArrayIndexSuffix) + 1;
    }

    ResourceNamesPoolSize += strlen(ShaderName) + 1;
    ResourceNamesPoolSize += strlen(EntryPoint) + 1;

    // Resource names pool is only needed to facilitate string allocation.
    StringPool ResourceNamesPool;
    Initialize(Allocator, ResCounters, ResourceNamesPoolSize, ResourceNamesPool);

    // Uniform buffer reflections
    std::vector<ShaderCodeBufferDescX> UBReflections;

    // Allocate resources
    ResourceCounters CurrRes;
    for (size_t i = 0; i < ResourceBindings.size(); ++i)
    {
        const tint::inspector::ResourceBinding& Binding   = ResourceBindings[i];
        const char*                             Name      = ResourceNamesPool.CopyString(Binding.variable_name);
        const Uint32                            ArraySize = i < ArraySizes.size() && ArraySizes[i] != 0 ? ArraySizes[i] : 1;
        switch (Binding.resource_type)
        {
            case TintResourceType::kUniformBuffer:
            {
                new (&GetUB(CurrRes.NumUBs++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};

                if (LoadUniformBufferReflection)
                    UBReflections.emplace_back(LoadUBReflection(Program, Binding, SourceLanguage));
            }
            break;

            case TintResourceType::kStorageBuffer:
            case TintResourceType::kReadOnlyStorageBuffer:
            {
                new (&GetSB(CurrRes.NumSBs++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};
            }
            break;

            case TintResourceType::kSampler:
            case TintResourceType::kComparisonSampler:
            {
                new (&GetSampler(CurrRes.NumSamplers++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};
            }
            break;

            case TintResourceType::kSampledTexture:
            case TintResourceType::kMultisampledTexture:
            case TintResourceType::kDepthTexture:
            case TintResourceType::kDepthMultisampledTexture:
            {
                new (&GetTexture(CurrRes.NumTextures++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};
            }
            break;

            case TintResourceType::kWriteOnlyStorageTexture:
            case TintResourceType::kReadOnlyStorageTexture:
            case TintResourceType::kReadWriteStorageTexture:
            {
                new (&GetStTexture(CurrRes.NumStTextures++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};
            }
            break;

            case TintResourceType::kExternalTexture:
            {
                new (&GetExtTexture(CurrRes.NumExtTextures++)) WGSLShaderResourceAttribs{Name, Binding, ArraySize};
            }
            break;

            case TintResourceType::kInputAttachment:
                UNSUPPORTED("Input attachments are not currently supported");
                break;

            default:
                UNEXPECTED("Unexpected resource type");
        }
    }

    VERIFY_EXPR(CurrRes.NumUBs == GetNumUBs());
    VERIFY_EXPR(CurrRes.NumSBs == GetNumSBs());
    VERIFY_EXPR(CurrRes.NumTextures == GetNumTextures());
    VERIFY_EXPR(CurrRes.NumStTextures == GetNumStTextures());
    VERIFY_EXPR(CurrRes.NumSamplers == GetNumSamplers());
    VERIFY_EXPR(CurrRes.NumExtTextures == GetNumExtTextures());

    if (CombinedSamplerSuffix != nullptr)
    {
        m_CombinedSamplerSuffix = ResourceNamesPool.CopyString(CombinedSamplerSuffix);
    }
    if (EmulatedArrayIndexSuffix != nullptr)
    {
        m_EmulatedArrayIndexSuffix = ResourceNamesPool.CopyString(EmulatedArrayIndexSuffix);
    }

    m_ShaderName = ResourceNamesPool.CopyString(ShaderName);
    m_EntryPoint = ResourceNamesPool.CopyString(EntryPoint);
    VERIFY(ResourceNamesPool.GetRemainingSize() == 0, "Names pool must be empty");

    if (!UBReflections.empty())
    {
        VERIFY_EXPR(LoadUniformBufferReflection);
        VERIFY_EXPR(UBReflections.size() == GetNumUBs());
        m_UBReflectionBuffer = ShaderCodeBufferDescX::PackArray(UBReflections.cbegin(), UBReflections.cend(), Allocator);
    }
}

void WGSLShaderResources::Initialize(IMemoryAllocator&       Allocator,
                                     const ResourceCounters& Counters,
                                     size_t                  ResourceNamesPoolSize,
                                     StringPool&             ResourceNamesPool)
{
    Uint32           CurrentOffset = 0;
    constexpr Uint32 MaxOffset     = std::numeric_limits<OffsetType>::max();
    auto             AdvanceOffset = [&CurrentOffset, MaxOffset](Uint32 NumResources) -> OffsetType {
        VERIFY(CurrentOffset <= MaxOffset, "Current offset (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        (void)MaxOffset;
        OffsetType Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumResources;
        return Offset;
    };

    OffsetType UniformBufferOffset = AdvanceOffset(Counters.NumUBs);
    (void)UniformBufferOffset;
    m_StorageBufferOffset   = AdvanceOffset(Counters.NumSBs);
    m_TextureOffset         = AdvanceOffset(Counters.NumTextures);
    m_StorageTextureOffset  = AdvanceOffset(Counters.NumStTextures);
    m_SamplerOffset         = AdvanceOffset(Counters.NumSamplers);
    m_ExternalTextureOffset = AdvanceOffset(Counters.NumExtTextures);
    m_TotalResources        = AdvanceOffset(0);
    static_assert(Uint32{WGSLShaderResourceAttribs::ResourceType::NumResourceTypes} == 13, "Please update the new resource type offset");

    size_t AlignedResourceNamesPoolSize = AlignUp(ResourceNamesPoolSize, sizeof(void*));

    static_assert(sizeof(WGSLShaderResourceAttribs) % sizeof(void*) == 0, "Size of WGSLShaderResourceAttribs struct must be a multiple of sizeof(void*)");
    // clang-format off
    size_t MemorySize = m_TotalResources             * sizeof(WGSLShaderResourceAttribs) +
                        AlignedResourceNamesPoolSize * sizeof(char);

    VERIFY_EXPR(GetNumUBs()         == Counters.NumUBs);
    VERIFY_EXPR(GetNumSBs()         == Counters.NumSBs);
    VERIFY_EXPR(GetNumTextures()    == Counters.NumTextures);
    VERIFY_EXPR(GetNumStTextures()  == Counters.NumStTextures);
    VERIFY_EXPR(GetNumSamplers()    == Counters.NumSamplers);
    VERIFY_EXPR(GetNumExtTextures() == Counters.NumExtTextures);
    static_assert(Uint32{WGSLShaderResourceAttribs::ResourceType::NumResourceTypes} == 13, "Please update the new resource count verification");
    // clang-format on

    if (MemorySize > 0)
    {
        void* pRawMem   = Allocator.Allocate(MemorySize, "Memory for shader resources", __FILE__, __LINE__);
        m_MemoryBuffer  = std::unique_ptr<void, STDDeleterRawMem<void>>(pRawMem, Allocator);
        char* NamesPool = reinterpret_cast<char*>(m_MemoryBuffer.get()) +
            m_TotalResources * sizeof(WGSLShaderResourceAttribs);
        ResourceNamesPool.AssignMemory(NamesPool, ResourceNamesPoolSize);
    }
}

WGSLShaderResources::~WGSLShaderResources()
{
    for (Uint32 n = 0; n < GetTotalResources(); ++n)
        GetResource(n).~WGSLShaderResourceAttribs();
}

std::string WGSLShaderResources::DumpResources()
{
    std::stringstream ss;
    ss << "Shader '" << m_ShaderName << "' resource stats: total resources: " << GetTotalResources() << ":" << std::endl
       << "UBs: " << GetNumUBs() << "; SBs: " << GetNumSBs() << "; Textures: " << GetNumTextures() << "; St Textures: " << GetNumStTextures()
       << "; Samplers: " << GetNumSamplers() << "; Ext Textures: " << GetNumExtTextures() << '.' << std::endl
       << "Resources:";

    Uint32 ResNum       = 0;
    auto   DumpResource = [&ss, &ResNum](const WGSLShaderResourceAttribs& Res) {
        std::stringstream FullResNameSS;
        FullResNameSS << '\'' << Res.Name;
        if (Res.ArraySize > 1)
            FullResNameSS << '[' << Res.ArraySize << ']';
        FullResNameSS << '\'';
        ss << std::setw(32) << FullResNameSS.str();
        ++ResNum;
    };

    ProcessResources(
        [&](const WGSLShaderResourceAttribs& UB, Uint32) //
        {
            VERIFY(UB.Type == WGSLShaderResourceAttribs::ResourceType::UniformBuffer, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Uniform Buffer     ";
            DumpResource(UB);
        },
        [&](const WGSLShaderResourceAttribs& SB, Uint32) //
        {
            VERIFY((SB.Type == WGSLShaderResourceAttribs::ResourceType::ROStorageBuffer ||
                    SB.Type == WGSLShaderResourceAttribs::ResourceType::RWStorageBuffer),
                   "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum
               << (SB.Type == WGSLShaderResourceAttribs::ResourceType::ROStorageBuffer ? " RO Storage Buffer  " : " RW Storage Buffer  ");
            DumpResource(SB);
        },
        [&](const WGSLShaderResourceAttribs& Tex, Uint32) //
        {
            if (Tex.Type == WGSLShaderResourceAttribs::ResourceType::Texture)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Texture          ";
            }
            else if (Tex.Type == WGSLShaderResourceAttribs::ResourceType::TextureMS)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " TextureMS        ";
            }
            else if (Tex.Type == WGSLShaderResourceAttribs::ResourceType::DepthTexture)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Depth Texture    ";
            }
            else if (Tex.Type == WGSLShaderResourceAttribs::ResourceType::DepthTextureMS)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Depth TextureMS  ";
            }
            else
            {
                UNEXPECTED("Unexpected resource type");
            }
            DumpResource(Tex);
        },
        [&](const WGSLShaderResourceAttribs& StTex, Uint32) //
        {
            if (StTex.Type == WGSLShaderResourceAttribs::ResourceType::WOStorageTexture)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " WO Storage Tex   ";
            }
            else if (StTex.Type == WGSLShaderResourceAttribs::ResourceType::ROStorageTexture)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " RO Storage Tex   ";
            }
            else if (StTex.Type == WGSLShaderResourceAttribs::ResourceType::RWStorageTexture)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " RW Storage Tex   ";
            }
            else
            {
                UNEXPECTED("Unexpected resource type");
            }
            DumpResource(StTex);
        },
        [&](const WGSLShaderResourceAttribs& Sam, Uint32) //
        {
            if (Sam.Type == WGSLShaderResourceAttribs::ResourceType::Sampler)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Sampler          ";
            }
            else if (Sam.Type == WGSLShaderResourceAttribs::ResourceType::ComparisonSampler)
            {
                ss << std::endl
                   << std::setw(3) << ResNum << " Sampler Cmp      ";
            }
            else
            {
                UNEXPECTED("Unexpected resource type");
            }
            DumpResource(Sam);
        },
        [&](const WGSLShaderResourceAttribs& ExtTex, Uint32) //
        {
            VERIFY(ExtTex.Type == WGSLShaderResourceAttribs::ResourceType::ExternalTexture, "Unexpected resource type");
            ss << std::endl
               << std::setw(3) << ResNum << " Ext Texture     ";
            DumpResource(ExtTex);
        } //
    );
    VERIFY_EXPR(ResNum == GetTotalResources());

    return ss.str();
}

} // namespace Diligent
