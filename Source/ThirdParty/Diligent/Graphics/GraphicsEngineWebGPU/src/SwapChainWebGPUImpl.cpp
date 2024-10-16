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

#include "pch.h"

#include "SwapChainWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "DeviceContextWebGPUImpl.hpp"
#include "TextureViewWebGPU.h"
#include "WebGPUTypeConversions.hpp"
#include "WebGPUStubs.hpp"

#ifdef PLATFORM_WIN32
#    include <Windows.h>
#endif

#if PLATFORM_EMSCRIPTEN
#    include <emscripten/html5.h>
#endif

namespace Diligent
{

namespace
{

constexpr char VSSource[] = R"(
struct VertexOutput 
{
    @builtin(position) Position: vec4f,
}

@vertex
fn VSMain(@builtin(vertex_index) VertexId: u32) -> VertexOutput 
{
    let Texcoord: vec2f = vec2f(f32((VertexId << 1u) & 2u), f32(VertexId & 2u));
    let Position: vec4f = vec4f(Texcoord * vec2f(2.0f, -2.0f) + vec2f(-1.0f, 1.0f), 1.0f, 1.0f);
    return VertexOutput(Position);
}
)";

constexpr char PSSource[] = R"(
@group(0) @binding(0) var TextureSrc: texture_2d<f32>;

struct VertexOutput 
{
    @builtin(position) Position: vec4f,
}

@fragment
fn PSMain(Input: VertexOutput) -> @location(0) vec4f 
{
    return textureLoad(TextureSrc, vec2i(Input.Position.xy), 0);
}
)";

constexpr char PSSourceGamma[] = R"(
@group(0) @binding(0) var TextureSrc: texture_2d<f32>;

struct VertexOutput 
{
    @builtin(position) Position: vec4f,
}

fn LinearToSRGB(RGB: vec3<f32>) -> vec3<f32> {
    let threshold = vec3<f32>(0.0031308);
    let bGreater = step(threshold, RGB);
    let linearPart = RGB * 12.92;
    let sRGBPart = (pow(RGB, vec3<f32>(1.0 / 2.4)) * 1.055) - vec3<f32>(0.055);
    return mix(linearPart, sRGBPart, bGreater);
}

@fragment
fn PSMain(Input: VertexOutput) -> @location(0) vec4f 
{
    var Color: vec4f = textureLoad(TextureSrc, vec2i(Input.Position.xy), 0);
    return vec4f(LinearToSRGB(Color.rgb), Color.a);
}
)";

WGPUTextureFormat WGPUConvertUnormToSRGB(WGPUTextureFormat Format)
{
    switch (Format)
    {
        case WGPUTextureFormat_RGBA8Unorm:
            return WGPUTextureFormat_RGBA8UnormSrgb;
        case WGPUTextureFormat_BGRA8Unorm:
            return WGPUTextureFormat_BGRA8UnormSrgb;
        default:
            UNEXPECTED("Unexpected texture format");
            return Format;
    }
}

} // namespace

class SwapChainWebGPUImpl::PresentCommand
{
public:
    PresentCommand(IRenderDeviceWebGPU* pRenderDevice) :
        m_pRenderDevice{pRenderDevice}
    {
    }

    bool InitializePipelineState(WGPUTextureFormat wgpuFormat, bool ConvertToGamma)
    {
        if (m_wgpuRenderPipeline)
            return true;

        WGPUDevice wgpuDevice = m_pRenderDevice->GetWebGPUDevice();

        WebGPUShaderModuleWrapper wgpuVSShaderModule{};
        {
            WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDesc{};
            wgpuShaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
            wgpuShaderCodeDesc.code        = VSSource;

            WGPUShaderModuleDescriptor wgpuShaderModuleDesc{};
            wgpuShaderModuleDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgpuShaderCodeDesc);
            wgpuVSShaderModule.Reset(wgpuDeviceCreateShaderModule(wgpuDevice, &wgpuShaderModuleDesc));
            if (!wgpuVSShaderModule)
            {
                LOG_ERROR_MESSAGE("Failed to create shader module for swap chain present command");
                return false;
            }
        }

        WebGPUShaderModuleWrapper wgpuPSShaderModule{};
        {
            WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDesc{};
            wgpuShaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
            wgpuShaderCodeDesc.code        = ConvertToGamma ? PSSourceGamma : PSSource;

            WGPUShaderModuleDescriptor wgpuShaderModuleDesc{};
            wgpuShaderModuleDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgpuShaderCodeDesc);
            wgpuPSShaderModule.Reset(wgpuDeviceCreateShaderModule(wgpuDevice, &wgpuShaderModuleDesc));
            if (!wgpuPSShaderModule)
            {
                LOG_ERROR_MESSAGE("Failed to create shader module for swap chain present command");
                return false;
            }
        }

        WGPUBindGroupLayoutEntry wgpuBindGroupLayoutEntries[1]{};
        wgpuBindGroupLayoutEntries[0].binding               = 0;
        wgpuBindGroupLayoutEntries[0].visibility            = WGPUShaderStage_Fragment;
        wgpuBindGroupLayoutEntries[0].texture.sampleType    = WGPUTextureSampleType_Float;
        wgpuBindGroupLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        WGPUBindGroupLayoutDescriptor wgpuBindGroupLayoutDesc{};
        wgpuBindGroupLayoutDesc.entryCount = _countof(wgpuBindGroupLayoutEntries);
        wgpuBindGroupLayoutDesc.entries    = wgpuBindGroupLayoutEntries;
        m_wgpuBindGroupLayout.Reset(wgpuDeviceCreateBindGroupLayout(wgpuDevice, &wgpuBindGroupLayoutDesc));
        if (!m_wgpuBindGroupLayout)
        {
            LOG_ERROR_MESSAGE("Failed to create bind group layout for swap chain present command");
            return false;
        }

        WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDesc{};
        wgpuPipelineLayoutDesc.bindGroupLayoutCount = 1;
        wgpuPipelineLayoutDesc.bindGroupLayouts     = &m_wgpuBindGroupLayout.Get();
        m_wgpuPipelineLayout.Reset(wgpuDeviceCreatePipelineLayout(wgpuDevice, &wgpuPipelineLayoutDesc));
        if (!m_wgpuPipelineLayout)
        {
            LOG_ERROR_MESSAGE("Failed to create pipeline layout for swap chain present command");
            return false;
        }

        WGPUColorTargetState wgpuColorTargetState{};
        wgpuColorTargetState.format    = wgpuFormat;
        wgpuColorTargetState.blend     = nullptr;
        wgpuColorTargetState.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState wgpuFragmentState{};
        wgpuFragmentState.module      = wgpuPSShaderModule;
        wgpuFragmentState.entryPoint  = "PSMain";
        wgpuFragmentState.targets     = &wgpuColorTargetState;
        wgpuFragmentState.targetCount = 1;

        WGPURenderPipelineDescriptor wgpuRenderPipelineDesc{};
        wgpuRenderPipelineDesc.label              = "SwapChainPresentPSO";
        wgpuRenderPipelineDesc.layout             = m_wgpuPipelineLayout;
        wgpuRenderPipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        wgpuRenderPipelineDesc.primitive.cullMode = WGPUCullMode_None;
        wgpuRenderPipelineDesc.vertex.module      = wgpuVSShaderModule;
        wgpuRenderPipelineDesc.vertex.entryPoint  = "VSMain";
        wgpuRenderPipelineDesc.fragment           = &wgpuFragmentState;
        wgpuRenderPipelineDesc.multisample.count  = 1;
        wgpuRenderPipelineDesc.multisample.mask   = 0xFFFFFFFF;
        m_wgpuRenderPipeline.Reset(wgpuDeviceCreateRenderPipeline(wgpuDevice, &wgpuRenderPipelineDesc));
        if (!m_wgpuPipelineLayout)
        {
            LOG_ERROR_MESSAGE("Failed to create render pipeline for swap chain present command");
            return false;
        }

        return true;
    }

    WGPUSurfaceGetCurrentTextureStatus Execute(ITextureViewWebGPU* pTexture, ISwapChainWebGPU* pSwapChain, IDeviceContextWebGPU* pDeviceContext)
    {
        WGPUSurfaceTexture wgpuSurfaceTexture{};
        wgpuSurfaceGetCurrentTexture(pSwapChain->GetWebGPUSurface(), &wgpuSurfaceTexture);
        WebGPUTextureWrapper wgpuTexture{wgpuSurfaceTexture.texture};

        switch (wgpuSurfaceTexture.status)
        {
            case WGPUSurfaceGetCurrentTextureStatus_Success:
            case WGPUSurfaceGetCurrentTextureStatus_Outdated:
                break;

            case WGPUSurfaceGetCurrentTextureStatus_Timeout:
                break;

            case WGPUSurfaceGetCurrentTextureStatus_Lost:
                LOG_WARNING_MESSAGE("Unable to present: swap chain surface is lost");
                return wgpuSurfaceTexture.status;

            case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
                LOG_ERROR_MESSAGE("Unable to present: out of memory");
                return wgpuSurfaceTexture.status;

            case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
                LOG_ERROR_MESSAGE("Unable to present: device is lost");
                return wgpuSurfaceTexture.status;

            case WGPUSurfaceGetCurrentTextureStatus_Error:
                LOG_ERROR_MESSAGE("Unable to present: unknown error");
                return wgpuSurfaceTexture.status;

            default:
                UNEXPECTED("Unexpected status");
                return wgpuSurfaceTexture.status;
        }

        WGPUTextureFormat ViewFormat = wgpuTextureGetFormat(wgpuTexture);

        // Simplify this code once the bug for sRGB texture view is fixed in Dawn
        bool ConvertToGamma = false;
#if !PLATFORM_EMSCRIPTEN
        if (IsSRGBFormat(pSwapChain->GetDesc().ColorBufferFormat))
            ViewFormat = WGPUConvertUnormToSRGB(ViewFormat);
#else
        ConvertToGamma = IsSRGBFormat(pSwapChain->GetDesc().ColorBufferFormat);
#endif
        if (!InitializePipelineState(ViewFormat, ConvertToGamma))
            return WGPUSurfaceGetCurrentTextureStatus_Error;

        WGPUTextureViewDescriptor wgpuTextureViewDesc;
        wgpuTextureViewDesc.nextInChain     = nullptr;
        wgpuTextureViewDesc.label           = "SwapChainPresentTextureView";
        wgpuTextureViewDesc.format          = ViewFormat;
        wgpuTextureViewDesc.dimension       = WGPUTextureViewDimension_2D;
        wgpuTextureViewDesc.baseMipLevel    = 0;
        wgpuTextureViewDesc.mipLevelCount   = 1;
        wgpuTextureViewDesc.baseArrayLayer  = 0;
        wgpuTextureViewDesc.arrayLayerCount = 1;
        wgpuTextureViewDesc.aspect          = WGPUTextureAspect_All;

        WebGPUTextureViewWrapper wgpuTextureView{wgpuTextureCreateView(wgpuTexture, &wgpuTextureViewDesc)};
        if (!wgpuTextureView)
        {
            LOG_ERROR_MESSAGE("Failed to create texture view for WGPU surface texture");
            return WGPUSurfaceGetCurrentTextureStatus_Error;
        }

        WGPUBindGroupEntry wgpuBindGroupEntries[1]{};
        wgpuBindGroupEntries[0].binding     = 0;
        wgpuBindGroupEntries[0].textureView = pTexture->GetWebGPUTextureView();

        WGPUBindGroupDescriptor wgpuBindGroupDesc{};
        wgpuBindGroupDesc.entries    = wgpuBindGroupEntries;
        wgpuBindGroupDesc.entryCount = _countof(wgpuBindGroupEntries);
        wgpuBindGroupDesc.layout     = m_wgpuBindGroupLayout;

        WebGPUBindGroupWrapper wgpuBindGroup{wgpuDeviceCreateBindGroup(m_pRenderDevice->GetWebGPUDevice(), &wgpuBindGroupDesc)};

        WGPUCommandEncoderDescriptor wgpuCmdEncoderDesc{};
        WebGPUCommandEncoderWrapper  wgpuCmdEncoder{wgpuDeviceCreateCommandEncoder(m_pRenderDevice->GetWebGPUDevice(), &wgpuCmdEncoderDesc)};

        WGPURenderPassColorAttachment wgpuRenderPassColorAttachments[1]{};
        wgpuRenderPassColorAttachments[0].clearValue = WGPUColor{0.0, 0.0, 0.0, 1.0};
        wgpuRenderPassColorAttachments[0].loadOp     = WGPULoadOp_Clear;
        wgpuRenderPassColorAttachments[0].storeOp    = WGPUStoreOp_Store;
        wgpuRenderPassColorAttachments[0].view       = wgpuTextureView;
        wgpuRenderPassColorAttachments[0].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor wgpuRenderPassDesc{};
        wgpuRenderPassDesc.colorAttachmentCount = _countof(wgpuRenderPassColorAttachments);
        wgpuRenderPassDesc.colorAttachments     = wgpuRenderPassColorAttachments;

        WebGPURenderPassEncoderWrapper wgpuRenderPassEncoder{wgpuCommandEncoderBeginRenderPass(wgpuCmdEncoder, &wgpuRenderPassDesc)};
        wgpuRenderPassEncoderSetPipeline(wgpuRenderPassEncoder, m_wgpuRenderPipeline);
        wgpuRenderPassEncoderSetBindGroup(wgpuRenderPassEncoder, 0, wgpuBindGroup, 0, nullptr);
        wgpuRenderPassEncoderDraw(wgpuRenderPassEncoder, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(wgpuRenderPassEncoder);

        WGPUCommandBufferDescriptor wgpuCmdBufferDesc{};
        WebGPUCommandBufferWrapper  wgpuCmdBuffer{wgpuCommandEncoderFinish(wgpuCmdEncoder, &wgpuCmdBufferDesc)};

        wgpuQueueSubmit(pDeviceContext->GetWebGPUQueue(), 1, &wgpuCmdBuffer.Get());

#if PLATFORM_EMSCRIPTEN
        emscripten_request_animation_frame([](double Time, void* pUserData) -> EM_BOOL { return EM_FALSE; }, nullptr);
#else
        wgpuSurfacePresent(pSwapChain->GetWebGPUSurface());
#endif

        return wgpuSurfaceTexture.status;
    }

private:
    RefCntAutoPtr<IRenderDeviceWebGPU> m_pRenderDevice;
    WebGPUBindGroupLayoutWrapper       m_wgpuBindGroupLayout;
    WebGPUPipelineLayoutWrapper        m_wgpuPipelineLayout;
    WebGPURenderPipelineWrapper        m_wgpuRenderPipeline;
};

SwapChainWebGPUImpl::SwapChainWebGPUImpl(IReferenceCounters*      pRefCounters,
                                         const SwapChainDesc&     SCDesc,
                                         RenderDeviceWebGPUImpl*  pRenderDevice,
                                         DeviceContextWebGPUImpl* pDeviceContext,
                                         const NativeWindow&      Window) :
    // clang-format off
    TSwapChainBase
    {
        pRefCounters,
        pRenderDevice,
        pDeviceContext,
        SCDesc
    },
    m_NativeWindow{Window},
    m_pCmdPresent{std::make_unique<PresentCommand>(pRenderDevice)}
// clang-format on
{
    if (m_DesiredPreTransform != SURFACE_TRANSFORM_OPTIMAL && m_DesiredPreTransform != SURFACE_TRANSFORM_IDENTITY)
    {
        LOG_WARNING_MESSAGE(GetSurfaceTransformString(m_DesiredPreTransform),
                            " is not an allowed pretransform because WebGPU swap chains only support identity transform. "
                            "Use SURFACE_TRANSFORM_OPTIMAL (recommended) or SURFACE_TRANSFORM_IDENTITY.");
    }
    m_DesiredPreTransform        = SURFACE_TRANSFORM_OPTIMAL;
    m_SwapChainDesc.PreTransform = SURFACE_TRANSFORM_IDENTITY;

    CreateSurface();
    ConfigureSurface();
    CreateBuffersAndViews();
}

SwapChainWebGPUImpl::~SwapChainWebGPUImpl() = default;

void SwapChainWebGPUImpl::Present(Uint32 SyncInterval)
{
    if (SyncInterval != 0 && SyncInterval != 1)
        LOG_WARNING_MESSAGE_ONCE("WebGPU only supports 0 and 1 present intervals");

    RefCntAutoPtr<IDeviceContext> pDeviceContext = m_wpDeviceContext.Lock();
    RenderDeviceWebGPUImpl*       pRenderDevice  = m_pRenderDevice.RawPtr<RenderDeviceWebGPUImpl>();
    if (!pDeviceContext)
    {
        LOG_ERROR_MESSAGE("Immediate context has been released");
        return;
    }

    DeviceContextWebGPUImpl* pImmediateCtxWebGPU = pDeviceContext.RawPtr<DeviceContextWebGPUImpl>();

    pImmediateCtxWebGPU->Flush();
    WGPUSurfaceGetCurrentTextureStatus SurfaceStatus = m_pCmdPresent->Execute(m_pBackBufferSRV, this, pImmediateCtxWebGPU);

    if (m_SwapChainDesc.IsPrimary)
    {
        pImmediateCtxWebGPU->FinishFrame();
        pRenderDevice->ReleaseStaleResources();
    }

    const bool EnableVSync = SyncInterval != 0;
    if (SurfaceStatus == WGPUSurfaceGetCurrentTextureStatus_Outdated ||
        SurfaceStatus == WGPUSurfaceGetCurrentTextureStatus_Lost ||
        m_VSyncEnabled != EnableVSync)
    {
        m_VSyncEnabled = EnableVSync;
        RecreateSwapChain();
    }
}

void SwapChainWebGPUImpl::Resize(Uint32            NewWidth,
                                 Uint32            NewHeight,
                                 SURFACE_TRANSFORM NewPreTransform)
{
    if (TSwapChainBase::Resize(NewWidth, NewHeight, NewPreTransform))
        RecreateSwapChain();
}

void SwapChainWebGPUImpl::SetFullscreenMode(const DisplayModeAttribs& DisplayMode)
{
    UNSUPPORTED("WebGPU does not support switching to the fullscreen mode");
}

void SwapChainWebGPUImpl::SetWindowedMode()
{
    UNSUPPORTED("WebGPU does not support switching to the windowed mode");
}

void SwapChainWebGPUImpl::CreateSurface()
{
    const RenderDeviceWebGPUImpl* pRenderDeviceWebGPU = m_pRenderDevice.RawPtr<RenderDeviceWebGPUImpl>();

#if PLATFORM_WIN32
    WGPUSurfaceDescriptorFromWindowsHWND wgpuSurfaceNativeDesc{};
    wgpuSurfaceNativeDesc.chain     = {nullptr, WGPUSType_SurfaceDescriptorFromWindowsHWND};
    wgpuSurfaceNativeDesc.hwnd      = m_NativeWindow.hWnd;
    wgpuSurfaceNativeDesc.hinstance = GetModuleHandle(nullptr);
#elif PLATFORM_LINUX
    WGPUSurfaceDescriptorFromXcbWindow wgpuSurfaceNativeDesc{};
    wgpuSurfaceNativeDesc.chain = {nullptr, WGPUSType_SurfaceDescriptorFromXcbWindow};
    wgpuSurfaceNativeDesc.connection = m_NativeWindow.pXCBConnection;
    wgpuSurfaceNativeDesc.window = m_NativeWindow.WindowId;
#elif PLATFROM_MACOS
    WGPUSurfaceDescriptorFromMetalLayer wgpuSurfaceNativeDesc{};
    wgpuSurfaceNativeDesc.chain  = {nullptr, WGPUSType_SurfaceDescriptorFromMetalLayer};
    wgpuSurfaceNativeDesc.window = m_NativeWindow.MetalLayer;
#elif PLATFORM_EMSCRIPTEN
    WGPUSurfaceDescriptorFromCanvasHTMLSelector wgpuSurfaceNativeDesc{};
    wgpuSurfaceNativeDesc.chain    = {nullptr, WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector};
    wgpuSurfaceNativeDesc.selector = m_NativeWindow.pCanvasId;
#endif

    WGPUSurfaceDescriptor wgpuSurfaceDesc{};
    wgpuSurfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgpuSurfaceNativeDesc);

    m_wgpuSurface.Reset(wgpuInstanceCreateSurface(pRenderDeviceWebGPU->GetWebGPUInstance(), &wgpuSurfaceDesc));
    if (!m_wgpuSurface)
    {
        LOG_ERROR_MESSAGE("Failed to create OS-specific surface");
    }
}

void SwapChainWebGPUImpl::ConfigureSurface()
{
    const RenderDeviceWebGPUImpl* pRenderDeviceWebGPU = m_pRenderDevice.RawPtr<RenderDeviceWebGPUImpl>();

    WGPUSurfaceCapabilities wgpuSurfaceCapabilities{};
    wgpuSurfaceGetCapabilities(m_wgpuSurface.Get(), pRenderDeviceWebGPU->GetWebGPUAdapter(), &wgpuSurfaceCapabilities);

    auto SelectPresentMode = [&](bool IsVSyncEnabled) -> WGPUPresentMode {
        WGPUPresentMode Result = WGPUPresentMode_Fifo;

        std::vector<WGPUPresentMode> PreferredPresentModes;
        if (IsVSyncEnabled)
        {
            PreferredPresentModes.push_back(WGPUPresentMode_Fifo);
        }
        else
        {
            PreferredPresentModes.push_back(WGPUPresentMode_Mailbox);
            PreferredPresentModes.push_back(WGPUPresentMode_Immediate);
            PreferredPresentModes.push_back(WGPUPresentMode_Fifo);
        }

        const WGPUPresentMode* FindBegin = wgpuSurfaceCapabilities.presentModes;
        const WGPUPresentMode* FindEnd   = wgpuSurfaceCapabilities.presentModes + wgpuSurfaceCapabilities.presentModeCount;

        for (WGPUPresentMode PreferredMode : PreferredPresentModes)
        {
            if (std::find(FindBegin, FindEnd, PreferredMode) != FindEnd)
            {
                Result = PreferredMode;
                break;
            }
        }

        return Result;
    };

    auto SelectUsage = [&](SWAP_CHAIN_USAGE_FLAGS Flags) -> WGPUTextureUsageFlags {
        WGPUTextureUsageFlags Result = {};

        DEV_CHECK_ERR(Flags != 0, "No swap chain usage flags defined");
        static_assert(SWAP_CHAIN_USAGE_LAST == 8, "Please update this function to handle the new swap chain usage");

        if (Flags & SWAP_CHAIN_USAGE_RENDER_TARGET)
            Result |= WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
        if (Flags & SWAP_CHAIN_USAGE_SHADER_RESOURCE)
            Result |= WGPUTextureUsage_TextureBinding;
        if (Flags & SWAP_CHAIN_USAGE_COPY_SOURCE)
            Result |= WGPUTextureUsage_CopySrc;

        return Result;
    };

    if (m_SwapChainDesc.Width == 0 || m_SwapChainDesc.Height == 0)
    {
#if PLATFORM_WIN32
        RECT WindowRect;
        GetClientRect(static_cast<HWND>(m_NativeWindow.hWnd), &WindowRect);

        m_SwapChainDesc.Width  = WindowRect.right - WindowRect.left;
        m_SwapChainDesc.Height = WindowRect.bottom - WindowRect.top;
#elif PLATFORM_EMSCRIPTEN
        int32_t CanvasWidth = 0;
        int32_t CanvasHeight = 0;
        emscripten_get_canvas_element_size(m_NativeWindow.pCanvasId, &CanvasWidth, &CanvasHeight);

        m_SwapChainDesc.Width = static_cast<Uint32>(CanvasWidth);
        m_SwapChainDesc.Height = static_cast<Uint32>(CanvasHeight);
#endif

        m_SwapChainDesc.Width  = (std::max)(m_SwapChainDesc.Width, 1u);
        m_SwapChainDesc.Height = (std::max)(m_SwapChainDesc.Height, 1u);
    }

    const WGPUTextureFormat wgpuPreferredFormat = wgpuSurfaceGetPreferredFormat(m_wgpuSurface, pRenderDeviceWebGPU->GetWebGPUAdapter());

    WGPUTextureFormat wgpuRTVFormats[] = {
        wgpuPreferredFormat,
        WGPUConvertUnormToSRGB(wgpuPreferredFormat),
    };

    WGPUSurfaceConfiguration wgpuSurfaceConfig{};
    wgpuSurfaceConfig.nextInChain = nullptr;
    wgpuSurfaceConfig.device      = pRenderDeviceWebGPU->GetWebGPUDevice();
    wgpuSurfaceConfig.usage       = SelectUsage(m_SwapChainDesc.Usage);
    wgpuSurfaceConfig.width       = m_SwapChainDesc.Width;
    wgpuSurfaceConfig.height      = m_SwapChainDesc.Height;
    wgpuSurfaceConfig.format      = wgpuSurfaceGetPreferredFormat(m_wgpuSurface, pRenderDeviceWebGPU->GetWebGPUAdapter());
    wgpuSurfaceConfig.presentMode = SelectPresentMode(m_VSyncEnabled);
    wgpuSurfaceConfig.alphaMode   = WGPUCompositeAlphaMode_Auto;

    // https://github.com/emscripten-core/emscripten/blob/20800de9644315f075e27c8a67dd811b4ec8884a/src/library_webgpu.js#L2749
#if !PLATFORM_EMSCRIPTEN
    wgpuSurfaceConfig.viewFormats     = wgpuRTVFormats;
    wgpuSurfaceConfig.viewFormatCount = _countof(wgpuRTVFormats);
#else
    (void)wgpuRTVFormats;
#endif

    wgpuSurfaceConfigure(m_wgpuSurface, &wgpuSurfaceConfig);
    wgpuSurfaceCapabilitiesFreeMembers(wgpuSurfaceCapabilities);
}

void SwapChainWebGPUImpl::CreateBuffersAndViews()
{
    TextureDesc BackBufferDesc{};
    BackBufferDesc.Type        = RESOURCE_DIM_TEX_2D;
    BackBufferDesc.Width       = m_SwapChainDesc.Width;
    BackBufferDesc.Height      = m_SwapChainDesc.Height;
    BackBufferDesc.Format      = m_SwapChainDesc.ColorBufferFormat;
    BackBufferDesc.SampleCount = 1;
    BackBufferDesc.Usage       = USAGE_DEFAULT;
    BackBufferDesc.BindFlags   = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    BackBufferDesc.Name        = "Main back buffer";

    RefCntAutoPtr<ITexture> pBackBufferTex;
    m_pRenderDevice->CreateTexture(BackBufferDesc, nullptr, &pBackBufferTex);
    m_pBackBufferRTV = RefCntAutoPtr<ITextureViewWebGPU>{pBackBufferTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET), IID_TextureViewWebGPU};
    m_pBackBufferSRV = RefCntAutoPtr<ITextureViewWebGPU>{pBackBufferTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), IID_TextureViewWebGPU};

    if (m_SwapChainDesc.DepthBufferFormat != TEX_FORMAT_UNKNOWN)
    {
        TextureDesc DepthBufferDesc{};
        DepthBufferDesc.Type        = RESOURCE_DIM_TEX_2D;
        DepthBufferDesc.Width       = BackBufferDesc.Width;
        DepthBufferDesc.Height      = BackBufferDesc.Height;
        DepthBufferDesc.Format      = m_SwapChainDesc.DepthBufferFormat;
        DepthBufferDesc.SampleCount = 1;
        DepthBufferDesc.Usage       = USAGE_DEFAULT;
        DepthBufferDesc.BindFlags   = BIND_DEPTH_STENCIL;

        DepthBufferDesc.ClearValue.Format               = DepthBufferDesc.Format;
        DepthBufferDesc.ClearValue.DepthStencil.Depth   = m_SwapChainDesc.DefaultDepthValue;
        DepthBufferDesc.ClearValue.DepthStencil.Stencil = m_SwapChainDesc.DefaultStencilValue;
        DepthBufferDesc.Name                            = "Main depth buffer";
        RefCntAutoPtr<ITexture> pDepthBufferTex;
        m_pRenderDevice->CreateTexture(DepthBufferDesc, nullptr, &pDepthBufferTex);
        m_pDepthBufferDSV = RefCntAutoPtr<ITextureViewWebGPU>{pDepthBufferTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL), IID_TextureViewWebGPU};
    }
}

void SwapChainWebGPUImpl::ReleaseSwapChainResources()
{
    if (!m_wgpuSurface)
        return;

    m_pBackBufferSRV.Release();
    m_pBackBufferRTV.Release();
    m_pDepthBufferDSV.Release();
}

void SwapChainWebGPUImpl::RecreateSwapChain()
{
    try
    {
        ReleaseSwapChainResources();
        ConfigureSurface();
        CreateBuffersAndViews();
    }
    catch (const std::runtime_error&)
    {
        LOG_ERROR("Failed to recreate the swap chain");
    }
}

} // namespace Diligent
