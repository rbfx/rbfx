//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/GraphicsDefs.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/VertexDeclaration.h"
#include "../../Math/Color.h"

#ifdef PLATFORM_MACOS
#include <SDL.h>
#include <SDL_metal.h>
#endif

#if defined(WIN32)
#include <d3d11_1.h>
#include <dxgi1_2.h>
#endif

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Buffer.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ResourceMapping.h>

namespace Urho3D
{

#define URHO3D_SAFE_RELEASE(p) if (p) { ((IUnknown*)p)->Release();  p = 0; }

#define URHO3D_LOGD3DERROR(msg, hr) URHO3D_LOGERRORF("%s (HRESULT %x)", msg, (unsigned)hr)

using ShaderProgramMap = ea::unordered_map<ea::pair<ShaderVariation*, ShaderVariation*>, SharedPtr<ShaderProgram> >;
using VertexDeclarationMap = ea::unordered_map<unsigned long long, SharedPtr<VertexDeclaration> >;
using ConstantBufferMap = ea::unordered_map<unsigned, SharedPtr<ConstantBuffer> >;

class DiligentConstantBufferManager;
class DiligentCommonPipelines;
class DiligentResourceMappingCache;
/// %Graphics implementation. Holds API-specific objects.
class URHO3D_API GraphicsImpl
{
    friend class Graphics;

public:
    /// Construct.
    GraphicsImpl();

    /// Return Diligent device.
    Diligent::IRenderDevice* GetDevice() const { return device_; }

    /// Return Diligent immediate device context.
    Diligent::IDeviceContext* GetDeviceContext() const { return deviceContext_; }

    /// Return swapchain.
    Diligent::ISwapChain* GetSwapChain() const { return swapChain_; }

    /// Return default render target view.
    Diligent::ITextureView* GetDefaultRenderTargetView() const { return swapChain_->GetCurrentBackBufferRTV(); }

    /// Return whether multisampling is supported for a given texture format and sample count.
    bool CheckMultiSampleSupport(Diligent::TEXTURE_FORMAT format, unsigned sampleCount) const;

    /// Return multisample quality level for a given texture format and sample count. The sample count must be supported. On D3D feature level 10.1+, uses the standard level. Below that uses the best quality.
    unsigned GetMultiSampleQuality(Diligent::TEXTURE_FORMAT format, unsigned sampleCount) const;

    /// Mark render targets as dirty. Must be called if render targets were set using DX11 device directly.
    void MarkRenderTargetsDirty() { renderTargetsDirty_ = true; }

    unsigned FindBestAdapter(Diligent::IEngineFactory* engineFactory, Diligent::Version& version);
private:
    /// Graphics device.
    Diligent::IRenderDevice* device_;
    /// Immediate device context.
    Diligent::IDeviceContext* deviceContext_;
    /// Swap chain.
    Diligent::ISwapChain* swapChain_;
    /*/// Default (backbuffer) rendertarget view.
    ID3D11RenderTargetView* defaultRenderTargetView_;
    /// Default depth-stencil texture.
    ID3D11Texture2D* defaultDepthTexture_;
    /// Default depth-stencil view.
    ID3D11DepthStencilView* defaultDepthStencilView_;*/
    /// Current color rendertarget views.
    Diligent::ITextureView* renderTargetViews_[MAX_RENDERTARGETS];
    /// Current depth-stencil view.
    Diligent::ITextureView* depthStencilView_;
    /// Intermediate texture for multisampled screenshots and less than whole viewport multisampled resolve, created on demand.
    Diligent::ITexture* resolveTexture_;
    /// Bound shader resource views.
    Diligent::ITextureView* shaderResourceViews_[MAX_TEXTURE_UNITS];
    /// Bound sampler state objects.
    Diligent::ISampler* samplers_[MAX_TEXTURE_UNITS];
    /// Bound vertex buffers.
    Diligent::IBuffer* vertexBuffers_[MAX_VERTEX_STREAMS];
    /// Bound constant buffers.
    Diligent::IBuffer* constantBuffers_[MAX_SHADER_PARAMETER_GROUPS]{};
    /// Bound constant buffers start slots.
    unsigned constantBuffersStartSlots_[MAX_SHADER_PARAMETER_GROUPS]{};
    /// Bound constant buffers start slots.
    unsigned constantBuffersNumSlots_[MAX_SHADER_PARAMETER_GROUPS]{};
#ifdef URHO3D_DILIGENT
    unsigned long long vertexOffsets_[MAX_VERTEX_STREAMS];
#else
    /// Vertex sizes per buffer.
    unsigned vertexSizes_[MAX_VERTEX_STREAMS];
    /// Vertex stream offsets per buffer.
    unsigned vertexOffsets_[MAX_VERTEX_STREAMS];
#endif
    /// Rendertargets dirty flag.
    bool renderTargetsDirty_;
    /// Viewport dirty flag.
    bool viewportDirty_;
    /// Textures dirty flag.
    bool texturesDirty_;
    /// Vertex declaration dirty flag.
    bool vertexDeclarationDirty_;
    /// Blend state dirty flag.
    bool blendStateDirty_;
    /// Depth state dirty flag.
    bool depthStateDirty_;
    /// Rasterizer state dirty flag.
    bool rasterizerStateDirty_;
    /// Scissor rect dirty flag.
    bool scissorRectDirty_;
    /// Stencil ref dirty flag.
    bool stencilRefDirty_;
    /// Hash of current blend state.
    unsigned blendStateHash_;
    /// Hash of current depth state.
    unsigned depthStateHash_;
    /// Hash of current rasterizer state.
    unsigned rasterizerStateHash_;
    /// First dirtied texture unit.
    unsigned firstDirtyTexture_;
    /// Last dirtied texture unit.
    unsigned lastDirtyTexture_;
    /// First dirtied vertex buffer.
    unsigned firstDirtyVB_;
    /// Last dirtied vertex buffer.
    unsigned lastDirtyVB_;
    /// Vertex declarations.
    VertexDeclarationMap vertexDeclarations_;
    /// Constant buffer search map.
    ConstantBufferMap allConstantBuffers_;
    /// Shader programs.
    ShaderProgramMap shaderPrograms_;
    /// Shader program in use.
    ShaderProgram* shaderProgram_;
    /// Current running backend
    RenderBackend renderBackend_;
    /// Current adapter id
    unsigned adapterId_;
#ifdef PLATFORM_MACOS
    SDL_MetalView metalView_;
#endif
};

}
