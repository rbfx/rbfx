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

#pragma once

/// \file
/// Implementation of mipmap generation routines

#include "EngineWebGPUImplTraits.hpp"
#include "RefCntAutoPtr.hpp"
#include "WebGPUObjectWrappers.hpp"

namespace Diligent
{

class GenerateMipsHelperWebGPU
{
public:
    GenerateMipsHelperWebGPU(RenderDeviceWebGPUImpl& pDevice);

    // clang-format off
    GenerateMipsHelperWebGPU             (const GenerateMipsHelperWebGPU&)  = delete;
    GenerateMipsHelperWebGPU             (      GenerateMipsHelperWebGPU&&) = delete;
    GenerateMipsHelperWebGPU& operator = (const GenerateMipsHelperWebGPU&)  = delete;
    GenerateMipsHelperWebGPU& operator = (      GenerateMipsHelperWebGPU&&) = delete;
    // clang-format on

    void GenerateMips(DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView);

private:
    using UAVFormats = std::array<TEXTURE_FORMAT, 4>;

    struct ShaderModuleCacheKey
    {
        struct Hasher
        {
            size_t operator()(const ShaderModuleCacheKey& Key) const;
        };

        ShaderModuleCacheKey(const UAVFormats& _Formats, SHADER_TYPE _ShaderType) :
            Formats{_Formats},
            ShaderType{_ShaderType}
        {}

        bool operator==(const ShaderModuleCacheKey& rhs) const;

        size_t GetHash() const;

        UAVFormats  Formats    = {};
        SHADER_TYPE ShaderType = {};

    private:
        mutable size_t Hash = 0;
    };

    struct ComputePipelineHashKey
    {
        struct Hasher
        {
            size_t operator()(const ComputePipelineHashKey& Key) const;
        };

        ComputePipelineHashKey(const UAVFormats& Formats, Uint32 PowerOfTwo) :
            Formats{Formats},
            PowerOfTwo{PowerOfTwo}
        {}

        bool operator==(const ComputePipelineHashKey& rhs) const;

        size_t GetHash() const;

        UAVFormats Formats    = {};
        Uint32     PowerOfTwo = 0;

    private:
        mutable size_t Hash = 0;
    };

    struct RenderPipelineHashKey
    {
        struct Hasher
        {
            size_t operator()(const RenderPipelineHashKey& Key) const;
        };

        RenderPipelineHashKey(TEXTURE_FORMAT _Format) :
            Format{_Format} {};

        bool operator==(const RenderPipelineHashKey& rhs) const;

        TEXTURE_FORMAT Format = {};
    };

    using ComputePipelineGroupLayout = std::pair<WebGPUComputePipelineWrapper, WebGPUBindGroupLayoutWrapper>;
    using RenderPipelineGroupLayout  = std::pair<WebGPURenderPipelineWrapper, WebGPUBindGroupLayoutWrapper>;
    using ShaderModuleCache          = std::unordered_map<ShaderModuleCacheKey, WebGPUShaderModuleWrapper, ShaderModuleCacheKey::Hasher>;
    using ComputePipelineCache       = std::unordered_map<ComputePipelineHashKey, ComputePipelineGroupLayout, ComputePipelineHashKey::Hasher>;
    using RenderPipelineCache        = std::unordered_map<RenderPipelineHashKey, RenderPipelineGroupLayout, RenderPipelineHashKey::Hasher>;

    void InitializeConstantBuffer();

    void InitializeSampler();

    void InitializePlaceholderTextures();

    WebGPUShaderModuleWrapper& GetShaderModule(const UAVFormats& Formats, SHADER_TYPE ShaderType);

    ComputePipelineGroupLayout& GetComputePipelineAndGroupLayout(const UAVFormats& Formats, Uint32 PowerOfTwo);

    RenderPipelineGroupLayout& GetRenderPipelineAndGroupLayout(TEXTURE_FORMAT Format);

    void GenerateMips(WGPUComputePassEncoder wgpuCmdEncoder, DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView);

    void GenerateMips(WGPUCommandEncoder wgpuCmdEncoder, DeviceContextWebGPUImpl* pDeviceContext, TextureViewWebGPUImpl* pTexView);

private:
    static constexpr TEXTURE_FORMAT PlaceholderTextureFormat = TEX_FORMAT_RGBA8_UNORM;

    RenderDeviceWebGPUImpl& m_DeviceWebGPU;

    RefCntAutoPtr<ISampler>                  m_pSampler;
    RefCntAutoPtr<IBuffer>                   m_pBuffer;
    std::vector<RefCntAutoPtr<ITextureView>> m_PlaceholderTextureViews;

    ComputePipelineCache m_ComputePipelineLayoutCache;
    RenderPipelineCache  m_RenderPipelineLayoutCache;
    ShaderModuleCache    m_ShaderModuleCache;
};

} // namespace Diligent
