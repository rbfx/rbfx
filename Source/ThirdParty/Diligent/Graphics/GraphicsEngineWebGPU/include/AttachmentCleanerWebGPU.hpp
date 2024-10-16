/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::AttachmentCleanerWebGPU class

#include "EngineWebGPUImplTraits.hpp"
#include "GraphicsTypes.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "WebGPUObjectWrappers.hpp"

namespace Diligent
{

class AttachmentCleanerWebGPU
{
public:
    struct RenderPassInfo
    {
        using RTVFormatArray = std::array<TEXTURE_FORMAT, MAX_RENDER_TARGETS>;

        bool operator==(const RenderPassInfo& rhs) const;

        size_t GetHash() const;

        Uint32         NumRenderTargets = 0;
        Uint8          SampleCount      = 1;
        TEXTURE_FORMAT DSVFormat        = TEX_FORMAT_UNKNOWN;
        RTVFormatArray RTVFormats       = {};
    };

    AttachmentCleanerWebGPU(RenderDeviceWebGPUImpl& DeviceWebGPU);

    // clang-format off
    AttachmentCleanerWebGPU             (const AttachmentCleanerWebGPU&) = delete;
    AttachmentCleanerWebGPU             (AttachmentCleanerWebGPU&&)      = delete;
    AttachmentCleanerWebGPU& operator = (const AttachmentCleanerWebGPU&) = delete;
    AttachmentCleanerWebGPU& operator = (AttachmentCleanerWebGPU&&)      = delete;
    // clang-format on

    void ClearColor(WGPURenderPassEncoder    wgpuCmdEncoder,
                    DeviceContextWebGPUImpl* pDeviceContext,
                    const RenderPassInfo&    RPInfo,
                    COLOR_MASK               ColorMask,
                    Uint32                   RTIndex,
                    const float              Color[]);

    void ClearDepthStencil(WGPURenderPassEncoder     wgpuCmdEncoder,
                           DeviceContextWebGPUImpl*  pDeviceContext,
                           const RenderPassInfo&     RPInfo,
                           CLEAR_DEPTH_STENCIL_FLAGS Flags,
                           float                     Depth,
                           Uint8                     Stencil);

private:
    struct ClearPSOHashKey
    {
        struct Hasher
        {
            size_t operator()(const ClearPSOHashKey& Key) const;
        };

        bool operator==(const ClearPSOHashKey& rhs) const;

        RenderPassInfo        RPInfo     = {};
        COLOR_MASK            ColorMask  = COLOR_MASK_ALL;
        Int32                 RTIndex    = 0; // -1 for depth
        WGPUDepthStencilState DepthState = {};

        mutable size_t Hash = 0;
    };

    WebGPURenderPipelineWrapper CreatePSO(const ClearPSOHashKey& Key);

    void ClearAttachment(WGPURenderPassEncoder    wgpuCmdEncoder,
                         DeviceContextWebGPUImpl* pDeviceContext,
                         const ClearPSOHashKey&   Key,
                         std::array<float, 8>&    ClearData);

    void InitializePipelineStates();

    void InitializeConstantBuffer();

    void InitializePipelineResourceLayout();

private:
    RenderDeviceWebGPUImpl& m_DeviceWebGPU;
    RefCntAutoPtr<IBuffer>  m_pBuffer;

    struct
    {
        WebGPUBindGroupLayoutWrapper wgpuBindGroupLayout;
        WebGPUPipelineLayoutWrapper  wgpuPipelineLayout;
        WebGPUBindGroupWrapper       wgpuBindGroup;
    } m_PipelineResourceLayout;

    WebGPUShaderModuleWrapper m_wgpuVSModule;

    using FSModuleCacheType = std::unordered_map<Int32, WebGPUShaderModuleWrapper>;
    FSModuleCacheType m_wgpuFSModules;

    using PSOCacheType = std::unordered_map<ClearPSOHashKey, WebGPURenderPipelineWrapper, ClearPSOHashKey::Hasher>;
    PSOCacheType m_PSOCache;

    WGPUDepthStencilState m_wgpuDisableDepth      = {};
    WGPUDepthStencilState m_wgpuWriteDepth        = {};
    WGPUDepthStencilState m_wgpuWriteStencil      = {};
    WGPUDepthStencilState m_wgpuWriteDepthStencil = {};
};

} // namespace Diligent
