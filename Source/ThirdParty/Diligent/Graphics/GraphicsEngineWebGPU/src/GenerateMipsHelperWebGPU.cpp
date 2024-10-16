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

#include "pch.h"

#include "GenerateMipsHelperWebGPU.hpp"

#include "DeviceContextWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "TextureWebGPUImpl.hpp"
#include "TextureViewWebGPUImpl.hpp"
#include "WebGPUTypeConversions.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

namespace
{

struct ShaderConstants
{
    Uint32 NumMipLevels;
    Uint32 FirstArraySlice;
    float  TexelSize[2];
};

constexpr char ShaderSourceVS[] = R"(
struct VertexOutput 
{
    @builtin(position) Position: vec4f,
    @location(0)       Texcoord: vec2f,
}

@vertex
fn main(@builtin(vertex_index) VertexId: u32) -> VertexOutput 
{
    let Texcoord: vec2f = vec2f(f32((VertexId << 1u) & 2u), f32(VertexId & 2u));
    let Position: vec4f = vec4f(Texcoord * vec2f(2.0f, -2.0f) + vec2f(-1.0f, 1.0f), 1.0f, 1.0f);
    return VertexOutput(Position, Texcoord);
}
)";

constexpr char ShaderSourcePS[] = R"(
@group(0) @binding(0) var TextureSrc: texture_2d<f32>;
@group(0) @binding(1) var SamplerLinear: sampler;

struct VertexOutput 
{
    @builtin(position) Position: vec4f,
    @location(0)       Texcoord: vec2f,
}

@fragment
fn main(Input: VertexOutput) -> @location(0) vec4f 
{
    return textureSample(TextureSrc, SamplerLinear, Input.Texcoord);
}
)";

constexpr char ShaderSourceCS[] = R"(
override NON_POWER_OF_TWO: u32 = 0;
override CONVERT_TO_SRGB: bool = false;

struct UniformBuffer 
{
    NumMipLevels: u32,
    FirstArraySlice: u32,
    TexelSize: vec2<f32>,
};

@group(0) @binding(0) var<uniform> cb : UniformBuffer;
@group(0) @binding(1) var OutMip1 : texture_storage_2d_array<${UAV_FORMAT_0}, write>;
@group(0) @binding(2) var OutMip2 : texture_storage_2d_array<${UAV_FORMAT_1}, write>;
@group(0) @binding(3) var OutMip3 : texture_storage_2d_array<${UAV_FORMAT_2}, write>;
@group(0) @binding(4) var OutMip4 : texture_storage_2d_array<${UAV_FORMAT_3}, write>;
@group(0) @binding(5) var SrcTex  : texture_2d_array<f32>;
@group(0) @binding(6) var BilinearClamp : sampler;

// Group shared memory
var<workgroup> gs_R : array<f32, 64>;
var<workgroup> gs_G : array<f32, 64>;
var<workgroup> gs_B : array<f32, 64>;
var<workgroup> gs_A : array<f32, 64>;

fn StoreColor(index: u32, color: vec4<f32>) 
{
    gs_R[index] = color.r;
    gs_G[index] = color.g;
    gs_B[index] = color.b;
    gs_A[index] = color.a;
}

fn LoadColor(index: u32) -> vec4<f32> 
{
    return vec4<f32>(gs_R[index], gs_G[index], gs_B[index], gs_A[index]);
}

fn LinearToSRGB(x: vec3<f32>) -> vec3<f32> 
{
    return select(12.92 * x, 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719, x >= vec3<f32>(0.0031308));
}

fn PackColor(linear: vec4<f32>) -> vec4<f32> 
{
    if (CONVERT_TO_SRGB) {
        return vec4<f32>(LinearToSRGB(linear.rgb), linear.a);
    } else {
        return linear;
    }
}

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) GlobalIdx : vec3<u32>,
        @builtin(local_invocation_index) LocalIdx : u32) 
{
    var DstMipSize : vec2<u32>;
    var Elements : u32;
    var Src1 : vec4<f32> = vec4<f32>(0.0);

    DstMipSize.x = textureDimensions(SrcTex).x;
    DstMipSize.y = textureDimensions(SrcTex).y;
    let IsValidThread = all(GlobalIdx.xy < DstMipSize);
    let ArraySlice = cb.FirstArraySlice + GlobalIdx.z;

    if IsValidThread {
        let UV = cb.TexelSize * (vec2<f32>(GlobalIdx.xy) + vec2<f32>(0.5));
        switch NON_POWER_OF_TWO 
        {
            case 0: {
               Src1 = textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV), ArraySlice, 0.0);
            }
            case 1: { 
               Src1 = 0.5 * (
                    textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + vec2<f32>(0.25, 0.5)), ArraySlice, 0.0) +
                    textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + cb.TexelSize * vec2<f32>(0.5, 0.0)), ArraySlice, 0.0));
            }
            case 2: {
               Src1 = 0.5 * (
                    textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + vec2<f32>(0.5, 0.25)), ArraySlice, 0.0) +
                    textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + cb.TexelSize * vec2<f32>(0.0, 0.5)), ArraySlice, 0.0));
            }
            default {
                Src1 = 0.25 * (
                     textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + vec2<f32>(0.25, 0.25)), ArraySlice, 0.0) +
                     textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + cb.TexelSize * vec2<f32>(0.5, 0.0)), ArraySlice, 0.0) +
                     textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + cb.TexelSize * vec2<f32>(0.0, 0.5)), ArraySlice, 0.0) +
                     textureSampleLevel(SrcTex, BilinearClamp, vec2<f32>(UV + cb.TexelSize * vec2<f32>(0.5, 0.5)), ArraySlice, 0.0));
            }
        }
        textureStore(OutMip1, GlobalIdx.xy, ArraySlice, PackColor(Src1));
    }

    if cb.NumMipLevels == 1 {
        return;
    }

    if IsValidThread {
        StoreColor(LocalIdx, Src1);
    }

    workgroupBarrier();

    if IsValidThread && (LocalIdx & 0x9) == 0 {
        let Src2 = LoadColor(LocalIdx + 0x01);
        let Src3 = LoadColor(LocalIdx + 0x08);
        let Src4 = LoadColor(LocalIdx + 0x09);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);
        textureStore(OutMip2, GlobalIdx.xy / 2, ArraySlice, PackColor(Src1));
        StoreColor(LocalIdx, Src1);
    }

    if cb.NumMipLevels == 2 {
        return;
    }

    workgroupBarrier();

    if IsValidThread && (LocalIdx & 0x1B) == 0 {
        let Src2 = LoadColor(LocalIdx + 0x02);
        let Src3 = LoadColor(LocalIdx + 0x10);
        let Src4 = LoadColor(LocalIdx + 0x12);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);
        textureStore(OutMip3, GlobalIdx.xy / 4, ArraySlice, PackColor(Src1));
        StoreColor(LocalIdx, Src1);
    }

    if cb.NumMipLevels == 3 {
        return;
    }

    workgroupBarrier();

    if IsValidThread && LocalIdx == 0 {
        let Src2 = LoadColor(LocalIdx + 0x04);
        let Src3 = LoadColor(LocalIdx + 0x20);
        let Src4 = LoadColor(LocalIdx + 0x24);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);
        textureStore(OutMip4, GlobalIdx.xy / 8, ArraySlice,  PackColor(Src1));
    }
}
)";

std::string_view ConvertWebGPUFormatToString(WGPUTextureFormat TexFmt)
{
    switch (TexFmt)
    {
        case WGPUTextureFormat_RGBA8Unorm:
            return "rgba8unorm";
        case WGPUTextureFormat_RG8Unorm:
            return "rg8unorm";
        case WGPUTextureFormat_R8Unorm:
            return "r8unorm";

        case WGPUTextureFormat_RGBA8Snorm:
            return "rgba8snorm";
        case WGPUTextureFormat_RG8Snorm:
            return "rg8snorm";
        case WGPUTextureFormat_R8Snorm:
            return "r8snorm";

        case WGPUTextureFormat_BGRA8Unorm:
            return "bgra8unorm";

        case WGPUTextureFormat_RGBA16Float:
            return "rgba16float";
        case WGPUTextureFormat_RG16Float:
            return "rg16float";
        case WGPUTextureFormat_R16Float:
            return "r16float";

        case WGPUTextureFormat_RGBA32Float:
            return "rgba32float";
        case WGPUTextureFormat_RG32Float:
            return "rg32float";
        case WGPUTextureFormat_R32Float:
            return "r32float";

        default:
            UNEXPECTED("Unsupported texture format");
            return "rgba8unorm";
    }
}

void ReplaceTemplateInString(std::string& Source, const std::string_view Target, const std::string_view Replacement)
{
    size_t Location = 0;
    while ((Location = Source.find(Target, Location)) != std::string::npos)
    {
        Source.replace(Location, Target.length(), Replacement);
        Location += Replacement.length();
    }
}

} // namespace

size_t GenerateMipsHelperWebGPU::ShaderModuleCacheKey::Hasher::operator()(const ShaderModuleCacheKey& Key) const
{
    return Key.GetHash();
}

bool GenerateMipsHelperWebGPU::ShaderModuleCacheKey::operator==(const ShaderModuleCacheKey& rhs) const
{
    for (size_t FormatIdx = 0; FormatIdx < Formats.size(); ++FormatIdx)
        if (Formats[FormatIdx] != rhs.Formats[FormatIdx])
            return false;
    return ShaderType == rhs.ShaderType;
}

size_t GenerateMipsHelperWebGPU::ComputePipelineHashKey::Hasher::operator()(const ComputePipelineHashKey& Key) const
{
    return Key.GetHash();
}

bool GenerateMipsHelperWebGPU::ComputePipelineHashKey::operator==(const ComputePipelineHashKey& rhs) const
{
    for (size_t FormatIdx = 0; FormatIdx < Formats.size(); ++FormatIdx)
        if (Formats[FormatIdx] != rhs.Formats[FormatIdx])
            return false;
    return PowerOfTwo == rhs.PowerOfTwo;
}

size_t GenerateMipsHelperWebGPU::ComputePipelineHashKey::GetHash() const
{
    if (Hash == 0)
    {
        Hash = ComputeHash(PowerOfTwo);
        for (const auto& Format : Formats)
            HashCombine(Hash, Format);
    }
    return Hash;
}

size_t GenerateMipsHelperWebGPU::RenderPipelineHashKey::Hasher::operator()(const RenderPipelineHashKey& Key) const
{
    return ComputeHash(static_cast<Uint16>(Key.Format));
}

bool GenerateMipsHelperWebGPU::RenderPipelineHashKey::operator==(const RenderPipelineHashKey& rhs) const
{
    return Format == rhs.Format;
}

size_t GenerateMipsHelperWebGPU::ShaderModuleCacheKey::GetHash() const
{
    if (Hash == 0)
    {
        for (const auto& Format : Formats)
            HashCombine(Hash, Format);
        HashCombine(Hash, ShaderType);
    }
    return Hash;
}

GenerateMipsHelperWebGPU::GenerateMipsHelperWebGPU(RenderDeviceWebGPUImpl& DeviceWebGPU) :
    m_DeviceWebGPU{DeviceWebGPU}
{}

void GenerateMipsHelperWebGPU::GenerateMips(DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView)
{
    auto*       pTexWGPU = pTexView->GetTexture<TextureWebGPUImpl>();
    const auto& TexDesc  = pTexWGPU->GetDesc();
    const auto& FmtInfo  = m_DeviceWebGPU.GetTextureFormatInfoExt(SRGBFormatToUnorm(TexDesc.Format));

    if (FmtInfo.BindFlags & BIND_UNORDERED_ACCESS)
    {
        auto CmdEncoder = pDeviceContext->GetComputePassCommandEncoder();
        GenerateMips(CmdEncoder, pDeviceContext, pTexView);
    }
    else
    {
        auto CmdEncoder = pDeviceContext->GetCommandEncoder();
        GenerateMips(CmdEncoder, pDeviceContext, pTexView);
    }
}

void GenerateMipsHelperWebGPU::InitializeConstantBuffer()
{
    if (!m_pBuffer)
    {
        // Rework when push constants will be available https://github.com/gpuweb/gpuweb/pull/4612 in WebGPU
        BufferDesc CBDesc;
        CBDesc.Name           = "GenerateMipsHelperWebGPU::ConstantBuffer";
        CBDesc.Size           = sizeof(ShaderConstants);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_DeviceWebGPU.CreateBuffer(CBDesc, nullptr, &m_pBuffer, true);
    }
}

void GenerateMipsHelperWebGPU::InitializeSampler()
{
    if (!m_pSampler)
    {
        SamplerDesc SmpDesc{};
        SmpDesc.MagFilter = FILTER_TYPE_LINEAR;
        SmpDesc.MinFilter = FILTER_TYPE_LINEAR;
        SmpDesc.MipFilter = FILTER_TYPE_POINT;
        m_DeviceWebGPU.CreateSampler(SmpDesc, &m_pSampler, true);
    }
}

void GenerateMipsHelperWebGPU::InitializePlaceholderTextures()
{
    if (m_PlaceholderTextureViews.empty())
    {
        m_PlaceholderTextureViews.resize(4);
        for (Uint32 TextureIdx = 0; TextureIdx < 4; ++TextureIdx)
        {
            TextureDesc TexDesc;
            TexDesc.Name      = "GenerateMipsHelperWebGPU::Placeholder texture";
            TexDesc.Type      = RESOURCE_DIM_TEX_2D;
            TexDesc.Width     = 1;
            TexDesc.Height    = 1;
            TexDesc.ArraySize = 1;
            TexDesc.MipLevels = 1;
            TexDesc.Format    = PlaceholderTextureFormat;
            TexDesc.BindFlags = BIND_UNORDERED_ACCESS;
            TexDesc.Usage     = USAGE_DEFAULT;

            RefCntAutoPtr<ITexture> pTexture;
            m_DeviceWebGPU.CreateTexture(TexDesc, nullptr, &pTexture, true);

            TextureViewDesc ViewDesc;
            ViewDesc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
            ViewDesc.Format          = PlaceholderTextureFormat;
            ViewDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
            ViewDesc.NumArraySlices  = 1;
            ViewDesc.NumMipLevels    = 1;
            ViewDesc.MostDetailedMip = 0;
            ViewDesc.FirstArraySlice = 0;
            pTexture->CreateView(ViewDesc, &m_PlaceholderTextureViews[TextureIdx]);
        }
    }
}

WebGPUShaderModuleWrapper& GenerateMipsHelperWebGPU::GetShaderModule(const UAVFormats& Formats, SHADER_TYPE ShaderType)
{
    auto Iter = m_ShaderModuleCache.find({Formats, ShaderType});
    if (Iter != m_ShaderModuleCache.end())
        return Iter->second;

    std::string                    WGSL;
    WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDesc{};
    wgpuShaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;

    switch (ShaderType)
    {
        case SHADER_TYPE_COMPUTE:
            WGSL = ShaderSourceCS;
            // Replace template formats
            for (Uint32 UAVIndex = 0; UAVIndex < Formats.size(); UAVIndex++)
            {
                WGPUTextureFormat wgpuTexFmt = TextureFormatToWGPUFormat(SRGBFormatToUnorm(Formats[UAVIndex]));
                ReplaceTemplateInString(WGSL, "${UAV_FORMAT_" + std::to_string(UAVIndex) + "}", ConvertWebGPUFormatToString(wgpuTexFmt));
            }
            wgpuShaderCodeDesc.code = WGSL.c_str();
            break;

        case SHADER_TYPE_VERTEX:
            wgpuShaderCodeDesc.code = ShaderSourceVS;
            break;

        case SHADER_TYPE_PIXEL:
            wgpuShaderCodeDesc.code = ShaderSourcePS;
            break;

        default:
            UNEXPECTED("Unexpected shader type");
    }

    WGPUShaderModuleDescriptor wgpuShaderModuleDesc{};
    wgpuShaderModuleDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgpuShaderCodeDesc);
    WebGPUShaderModuleWrapper wgpuShaderModule{wgpuDeviceCreateShaderModule(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuShaderModuleDesc)};
    VERIFY(wgpuShaderModule, "Failed to create mip generation shader module");

    auto Condition = m_ShaderModuleCache.emplace(ShaderModuleCacheKey{Formats, ShaderType}, std::move(wgpuShaderModule));
    return Condition.first->second;
}

GenerateMipsHelperWebGPU::ComputePipelineGroupLayout& GenerateMipsHelperWebGPU::GetComputePipelineAndGroupLayout(const UAVFormats& Formats, Uint32 PowerOfTwo)
{
    auto Iter = m_ComputePipelineLayoutCache.find({Formats, PowerOfTwo});
    if (Iter != m_ComputePipelineLayoutCache.end())
        return Iter->second;

    WGPUBindGroupLayoutEntry wgpuBindGroupLayoutEntries[7]{};

    wgpuBindGroupLayoutEntries[0].binding                 = 0;
    wgpuBindGroupLayoutEntries[0].visibility              = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[0].buffer.type             = WGPUBufferBindingType_Uniform;
    wgpuBindGroupLayoutEntries[0].buffer.hasDynamicOffset = false;

    wgpuBindGroupLayoutEntries[1].binding                      = 1;
    wgpuBindGroupLayoutEntries[1].visibility                   = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[1].storageTexture.format        = TextureFormatToWGPUFormat(SRGBFormatToUnorm(Formats[0]));
    wgpuBindGroupLayoutEntries[1].storageTexture.access        = WGPUStorageTextureAccess_WriteOnly;
    wgpuBindGroupLayoutEntries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2DArray;

    wgpuBindGroupLayoutEntries[2].binding                      = 2;
    wgpuBindGroupLayoutEntries[2].visibility                   = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[2].storageTexture.format        = TextureFormatToWGPUFormat(SRGBFormatToUnorm(Formats[1]));
    wgpuBindGroupLayoutEntries[2].storageTexture.access        = WGPUStorageTextureAccess_WriteOnly;
    wgpuBindGroupLayoutEntries[2].storageTexture.viewDimension = WGPUTextureViewDimension_2DArray;

    wgpuBindGroupLayoutEntries[3].binding                      = 3;
    wgpuBindGroupLayoutEntries[3].visibility                   = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[3].storageTexture.format        = TextureFormatToWGPUFormat(SRGBFormatToUnorm(Formats[2]));
    wgpuBindGroupLayoutEntries[3].storageTexture.access        = WGPUStorageTextureAccess_WriteOnly;
    wgpuBindGroupLayoutEntries[3].storageTexture.viewDimension = WGPUTextureViewDimension_2DArray;

    wgpuBindGroupLayoutEntries[4].binding                      = 4;
    wgpuBindGroupLayoutEntries[4].visibility                   = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[4].storageTexture.format        = TextureFormatToWGPUFormat(SRGBFormatToUnorm(Formats[3]));
    wgpuBindGroupLayoutEntries[4].storageTexture.access        = WGPUStorageTextureAccess_WriteOnly;
    wgpuBindGroupLayoutEntries[4].storageTexture.viewDimension = WGPUTextureViewDimension_2DArray;

    wgpuBindGroupLayoutEntries[5].binding               = 5;
    wgpuBindGroupLayoutEntries[5].visibility            = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[5].texture.multisampled  = false;
    wgpuBindGroupLayoutEntries[5].texture.sampleType    = WGPUTextureSampleType_Float;
    wgpuBindGroupLayoutEntries[5].texture.viewDimension = WGPUTextureViewDimension_2DArray;

    wgpuBindGroupLayoutEntries[6].binding      = 6;
    wgpuBindGroupLayoutEntries[6].visibility   = WGPUShaderStage_Compute;
    wgpuBindGroupLayoutEntries[6].sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor wgpuBindGroupLayoutDesc{};
    wgpuBindGroupLayoutDesc.entryCount = _countof(wgpuBindGroupLayoutEntries);
    wgpuBindGroupLayoutDesc.entries    = wgpuBindGroupLayoutEntries;
    WebGPUBindGroupLayoutWrapper wgpuBindGroupLayout{wgpuDeviceCreateBindGroupLayout(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuBindGroupLayoutDesc)};
    VERIFY(wgpuBindGroupLayout, "Failed to create mip generation bind group layout");

    WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDesc{};
    wgpuPipelineLayoutDesc.bindGroupLayoutCount = 1;
    wgpuPipelineLayoutDesc.bindGroupLayouts     = &wgpuBindGroupLayout.Get();
    WebGPUPipelineLayoutWrapper wgpuPipelineLayout{wgpuDeviceCreatePipelineLayout(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuPipelineLayoutDesc)};
    VERIFY(wgpuPipelineLayout, "Failed to create mip generation pipeline layout");

    WGPUConstantEntry wgpuContants[] = {
        WGPUConstantEntry{nullptr, "CONVERT_TO_SRGB", static_cast<double>(IsSRGBFormat(Formats[0]))},
        WGPUConstantEntry{nullptr, "NON_POWER_OF_TWO", static_cast<double>(PowerOfTwo)}};

    WGPUComputePipelineDescriptor wgpuComputePipelineDesc{};
    wgpuComputePipelineDesc.layout                = wgpuPipelineLayout.Get();
    wgpuComputePipelineDesc.compute.module        = GetShaderModule(Formats, SHADER_TYPE_COMPUTE).Get();
    wgpuComputePipelineDesc.compute.entryPoint    = "main";
    wgpuComputePipelineDesc.compute.constants     = wgpuContants;
    wgpuComputePipelineDesc.compute.constantCount = _countof(wgpuContants);
    WebGPUComputePipelineWrapper wgpuComputePipeline{wgpuDeviceCreateComputePipeline(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuComputePipelineDesc)};
    VERIFY(wgpuComputePipeline, "Failed to create mip generation compute pipeline");

    auto Condition = m_ComputePipelineLayoutCache.emplace(ComputePipelineHashKey{Formats, PowerOfTwo}, std::make_pair(std::move(wgpuComputePipeline), std::move(wgpuBindGroupLayout)));
    return Condition.first->second;
}

GenerateMipsHelperWebGPU::RenderPipelineGroupLayout& GenerateMipsHelperWebGPU::GetRenderPipelineAndGroupLayout(TEXTURE_FORMAT Format)
{
    auto Iter = m_RenderPipelineLayoutCache.find({Format});
    if (Iter != m_RenderPipelineLayoutCache.end())
        return Iter->second;

    WGPUBindGroupLayoutEntry wgpuBindGroupLayoutEntries[2]{};

    wgpuBindGroupLayoutEntries[0].binding               = 0;
    wgpuBindGroupLayoutEntries[0].visibility            = WGPUShaderStage_Fragment;
    wgpuBindGroupLayoutEntries[0].texture.sampleType    = WGPUTextureSampleType_Float;
    wgpuBindGroupLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

    wgpuBindGroupLayoutEntries[1].binding      = 1;
    wgpuBindGroupLayoutEntries[1].visibility   = WGPUShaderStage_Fragment;
    wgpuBindGroupLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor wgpuBindGroupLayoutDesc{};
    wgpuBindGroupLayoutDesc.entryCount = _countof(wgpuBindGroupLayoutEntries);
    wgpuBindGroupLayoutDesc.entries    = wgpuBindGroupLayoutEntries;
    WebGPUBindGroupLayoutWrapper wgpuBindGroupLayout{wgpuDeviceCreateBindGroupLayout(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuBindGroupLayoutDesc)};
    VERIFY(wgpuBindGroupLayout, "Failed to create mip generation bind group layout");

    WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDesc{};
    wgpuPipelineLayoutDesc.bindGroupLayoutCount = 1;
    wgpuPipelineLayoutDesc.bindGroupLayouts     = &wgpuBindGroupLayout.Get();
    WebGPUPipelineLayoutWrapper wgpuPipelineLayout{wgpuDeviceCreatePipelineLayout(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuPipelineLayoutDesc)};
    VERIFY(wgpuPipelineLayout, "Failed to create mip generation pipeline layout");

    WGPUColorTargetState wgpuColorTargets[1]{};
    wgpuColorTargets[0].writeMask = WGPUColorWriteMask_All;
    wgpuColorTargets[0].format    = TextureFormatToWGPUFormat(Format);

    WGPUFragmentState wgpuFragmentState{};
    wgpuFragmentState.module      = GetShaderModule({}, SHADER_TYPE_PIXEL).Get();
    wgpuFragmentState.entryPoint  = "main";
    wgpuFragmentState.targetCount = _countof(wgpuColorTargets);
    wgpuFragmentState.targets     = wgpuColorTargets;

    WGPURenderPipelineDescriptor wgpuRenderPipelineDesc{};
    wgpuRenderPipelineDesc.layout             = wgpuPipelineLayout.Get();
    wgpuRenderPipelineDesc.vertex.module      = GetShaderModule({}, SHADER_TYPE_VERTEX).Get();
    wgpuRenderPipelineDesc.vertex.entryPoint  = "main";
    wgpuRenderPipelineDesc.fragment           = &wgpuFragmentState;
    wgpuRenderPipelineDesc.primitive.cullMode = WGPUCullMode_None;
    wgpuRenderPipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    wgpuRenderPipelineDesc.multisample.count  = 1;
    wgpuRenderPipelineDesc.multisample.mask   = 0xFFFFFFFF;

    WebGPURenderPipelineWrapper wgpuRenderPipeline{wgpuDeviceCreateRenderPipeline(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuRenderPipelineDesc)};
    VERIFY(wgpuRenderPipeline, "Failed to create mip generation render pipeline");

    auto Condition = m_RenderPipelineLayoutCache.emplace(RenderPipelineHashKey{Format}, std::make_pair(std::move(wgpuRenderPipeline), std::move(wgpuBindGroupLayout)));
    return Condition.first->second;
}

void GenerateMipsHelperWebGPU::GenerateMips(WGPUComputePassEncoder wgpuCmdEncoder, DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView)
{
    VERIFY_EXPR(m_DeviceWebGPU.GetNumImmediateContexts() == 1);

    InitializeConstantBuffer();
    InitializeSampler();
    InitializePlaceholderTextures();

    TextureWebGPUImpl*     pTextureImpl = pTexView->GetTexture<TextureWebGPUImpl>();
    const TextureDesc&     TexDesc      = pTextureImpl->GetDesc();
    const TextureViewDesc& ViewDesc     = pTexView->GetDesc();

    Uint32 BottomMip = ViewDesc.NumMipLevels - 1u;
    for (Uint32 TopMip = 0; TopMip < BottomMip;)
    {
        Uint32 SrcWidth  = std::max(TexDesc.Width >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        Uint32 SrcHeight = std::max(TexDesc.Height >> (TopMip + ViewDesc.MostDetailedMip), 1u);
        Uint32 DstWidth  = std::max(SrcWidth >> 1, 1u);
        Uint32 DstHeight = std::max(SrcHeight >> 1, 1u);

        // We can downsample up to four times, but if the ratio between levels is not
        // exactly 2:1, we have to shift our blend weights, which gets complicated or
        // expensive.  Maybe we can update the code later to compute sample weights for
        // each successive downsample.  We use _BitScanForward to count number of zeros
        // in the low bits.  Zeros indicate we can divide by two without truncating.
        Uint32 AdditionalMips = PlatformMisc::GetLSB(DstWidth | DstHeight);
        Uint32 NumMips        = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
        if (TopMip + NumMips > BottomMip)
            NumMips = BottomMip - TopMip;

        {
            ShaderConstants BufferData{TopMip, 0, {1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight)}};

            void* pMappedData = nullptr;
            pDeviceContext->MapBuffer(m_pBuffer, MAP_WRITE, MAP_FLAG_DISCARD, pMappedData);
            memcpy(pMappedData, &BufferData, sizeof(BufferData));
            pDeviceContext->UnmapBuffer(m_pBuffer, MAP_WRITE);
        }

        const BufferWebGPUImpl*  pBufferImpl  = ClassPtrCast<BufferWebGPUImpl>(m_pBuffer.RawPtr());
        const SamplerWebGPUImpl* pSamplerImpl = ClassPtrCast<SamplerWebGPUImpl>(m_pSampler.RawPtr());

        WGPUBindGroupEntry wgpuBindGroupEntries[7]{};
        wgpuBindGroupEntries[0].binding = 0;
        wgpuBindGroupEntries[0].buffer  = pBufferImpl->GetWebGPUBuffer();
        wgpuBindGroupEntries[0].offset  = pBufferImpl->GetDynamicOffset(pDeviceContext->GetContextId(), nullptr);
        wgpuBindGroupEntries[0].size    = pBufferImpl->GetDesc().Size;

        UAVFormats PipelineFormats{};
        for (Uint32 UAVIndex = 0; UAVIndex < 4; ++UAVIndex)
        {
            const auto* pTextureViewImpl                   = ClassPtrCast<TextureViewWebGPUImpl>(m_PlaceholderTextureViews[UAVIndex].RawPtr());
            wgpuBindGroupEntries[UAVIndex + 1].binding     = UAVIndex + 1;
            wgpuBindGroupEntries[UAVIndex + 1].textureView = UAVIndex < NumMips ? pTexView->GetMipLevelUAV(TopMip + UAVIndex + 1) : pTextureViewImpl->GetWebGPUTextureView();
            PipelineFormats[UAVIndex]                      = UAVIndex < NumMips ? pTexView->GetDesc().Format : PlaceholderTextureFormat;
        }

        wgpuBindGroupEntries[5].binding     = 5;
        wgpuBindGroupEntries[5].textureView = pTexView->GetMipLevelSRV(TopMip);

        wgpuBindGroupEntries[6].binding = 6;
        wgpuBindGroupEntries[6].sampler = pSamplerImpl->GetWebGPUSampler();

        // Determine if the first downsample is more than 2:1.  This happens whenever
        // the source width or height is odd.
        Uint32 NonPowerOfTwo          = (SrcWidth & 1) | (SrcHeight & 1) << 1;
        auto& [Pipeline, LayoutGroup] = GetComputePipelineAndGroupLayout(PipelineFormats, NonPowerOfTwo);

        WGPUBindGroupDescriptor wgpuBindGroupDesc{};
        wgpuBindGroupDesc.layout     = LayoutGroup.Get();
        wgpuBindGroupDesc.entryCount = _countof(wgpuBindGroupEntries);
        wgpuBindGroupDesc.entries    = wgpuBindGroupEntries;

        WebGPUBindGroupWrapper wgpuBindGroup{wgpuDeviceCreateBindGroup(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuBindGroupDesc)};
        wgpuComputePassEncoderSetPipeline(wgpuCmdEncoder, Pipeline.Get());
        wgpuComputePassEncoderSetBindGroup(wgpuCmdEncoder, 0, wgpuBindGroup.Get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(wgpuCmdEncoder, (DstWidth + 7) / 8, (DstHeight + 7) / 8, ViewDesc.NumArraySlices);

        TopMip += NumMips;
    }
}

void GenerateMipsHelperWebGPU::GenerateMips(WGPUCommandEncoder wgpuCmdEncoder, DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView)
{
    VERIFY_EXPR(m_DeviceWebGPU.GetNumImmediateContexts() == 1);

    InitializeSampler();
    pDeviceContext->EndCommandEncoders(DeviceContextWebGPUImpl::COMMAND_ENCODER_FLAG_ALL);

    const auto& ViewDesc = pTexView->GetDesc();

    for (Uint32 TopSlice = 0; TopSlice < ViewDesc.NumArraySlices; ++TopSlice)
    {
        for (Uint32 TopMip = 0; TopMip < ViewDesc.NumMipLevels - 1; ++TopMip)
        {
            WGPURenderPassColorAttachment wgpuColorAttachments[1]{};
            wgpuColorAttachments[0].view       = pTexView->GetMipLevelRTV(TopSlice, TopMip + 1);
            wgpuColorAttachments[0].storeOp    = WGPUStoreOp_Store;
            wgpuColorAttachments[0].loadOp     = WGPULoadOp_Load;
            wgpuColorAttachments[0].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

            WGPURenderPassDescriptor wgpuRenderPassDesc{};
            wgpuRenderPassDesc.colorAttachmentCount = _countof(wgpuColorAttachments);
            wgpuRenderPassDesc.colorAttachments     = wgpuColorAttachments;

            WebGPURenderPassEncoderWrapper wgpuRenderPassEncoder{wgpuCommandEncoderBeginRenderPass(wgpuCmdEncoder, &wgpuRenderPassDesc)};
            auto& [Pipeline, LayoutGroup] = GetRenderPipelineAndGroupLayout(ViewDesc.Format);

            WGPUBindGroupEntry wgpuBindGroupEntries[2]{};
            wgpuBindGroupEntries[0].binding     = 0;
            wgpuBindGroupEntries[0].textureView = pTexView->GetMipLevelSRV(TopSlice, TopMip);

            wgpuBindGroupEntries[1].binding = 1;
            wgpuBindGroupEntries[1].sampler = ClassPtrCast<SamplerWebGPUImpl>(m_pSampler.RawPtr())->GetWebGPUSampler();

            WGPUBindGroupDescriptor wgpuBindGroupDesc{};
            wgpuBindGroupDesc.layout     = LayoutGroup.Get();
            wgpuBindGroupDesc.entryCount = _countof(wgpuBindGroupEntries);
            wgpuBindGroupDesc.entries    = wgpuBindGroupEntries;

            WebGPUBindGroupWrapper wgpuBindGroup{wgpuDeviceCreateBindGroup(m_DeviceWebGPU.GetWebGPUDevice(), &wgpuBindGroupDesc)};
            wgpuRenderPassEncoderSetPipeline(wgpuRenderPassEncoder, Pipeline.Get());
            wgpuRenderPassEncoderSetBindGroup(wgpuRenderPassEncoder, 0, wgpuBindGroup.Get(), 0, nullptr);
            wgpuRenderPassEncoderDraw(wgpuRenderPassEncoder, 3, 1, 0, 0);
            wgpuRenderPassEncoderEnd(wgpuRenderPassEncoder);
        }
    }
}

} // namespace Diligent
