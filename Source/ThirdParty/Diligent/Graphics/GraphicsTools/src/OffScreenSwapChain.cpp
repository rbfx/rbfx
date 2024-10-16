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


#include "../../../Common/interface/ObjectBase.hpp"
#include "../../../Graphics/GraphicsEngine/include/SwapChainBase.hpp"
#include "../../../Graphics/GraphicsEngine/include/RenderDeviceBase.hpp"
#include "../../../Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"

#include "OffScreenSwapChain.hpp"

namespace Diligent
{

class OffScreenSwapChain : public SwapChainBase<ISwapChain>
{
public:
    using TSwapChainBase = SwapChainBase;

    OffScreenSwapChain(IReferenceCounters*  pRefCounters,
                       IRenderDevice*       pDevice,
                       IDeviceContext*      pContext,
                       const SwapChainDesc& SCDesc) :
        SwapChainBase{pRefCounters, pDevice, pContext, SCDesc}
    {

        if (m_DesiredPreTransform != SURFACE_TRANSFORM_OPTIMAL && m_DesiredPreTransform != SURFACE_TRANSFORM_IDENTITY)
        {
            LOG_WARNING_MESSAGE(GetSurfaceTransformString(m_DesiredPreTransform),
                                " is not an allowed pre transform because off-screen swap chains only support identity transform. "
                                "Use SURFACE_TRANSFORM_OPTIMAL (recommended) or SURFACE_TRANSFORM_IDENTITY.");
        }

        m_DesiredPreTransform        = SURFACE_TRANSFORM_OPTIMAL;
        m_SwapChainDesc.PreTransform = SURFACE_TRANSFORM_IDENTITY;

        Resize(m_SwapChainDesc.Width, m_SwapChainDesc.Height, m_SwapChainDesc.PreTransform);
    }

    virtual void DILIGENT_CALL_TYPE Present(Uint32 SyncInterval) override
    {
        auto pDeviceContext = m_wpDeviceContext.Lock();
        if (!pDeviceContext)
        {
            LOG_ERROR_MESSAGE("Immediate context has been released");
            return;
        }

        pDeviceContext->Flush();

        if (m_SwapChainDesc.IsPrimary)
        {
            pDeviceContext->FinishFrame();
            m_pRenderDevice->ReleaseStaleResources();
        }
    }

    virtual void DILIGENT_CALL_TYPE Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform) override final
    {
        if (TSwapChainBase::Resize(NewWidth, NewHeight, NewPreTransform))
        {
            m_pRTV.Release();
            m_pDSV.Release();
            m_pRenderTarget.Release();
            m_pDepthBuffer.Release();

            {
                TextureDesc RenderTargetDesc;
                RenderTargetDesc.Name        = "Off screen color buffer";
                RenderTargetDesc.Type        = RESOURCE_DIM_TEX_2D;
                RenderTargetDesc.Width       = m_SwapChainDesc.Width;
                RenderTargetDesc.Height      = m_SwapChainDesc.Height;
                RenderTargetDesc.Format      = m_SwapChainDesc.ColorBufferFormat;
                RenderTargetDesc.SampleCount = 1;
                RenderTargetDesc.Usage       = USAGE_DEFAULT;
                RenderTargetDesc.BindFlags   = BIND_RENDER_TARGET;
                m_pRenderDevice->CreateTexture(RenderTargetDesc, nullptr, &m_pRenderTarget);
                VERIFY_EXPR(m_pRenderTarget != nullptr);
                m_pRTV = m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
                VERIFY_EXPR(m_pRTV != nullptr);
            }

            if (m_SwapChainDesc.DepthBufferFormat != TEX_FORMAT_UNKNOWN)
            {
                TextureDesc DepthBufferDesc;
                DepthBufferDesc.Name        = "Off screen depth buffer";
                DepthBufferDesc.Type        = RESOURCE_DIM_TEX_2D;
                DepthBufferDesc.Width       = m_SwapChainDesc.Width;
                DepthBufferDesc.Height      = m_SwapChainDesc.Height;
                DepthBufferDesc.Format      = m_SwapChainDesc.DepthBufferFormat;
                DepthBufferDesc.SampleCount = 1;
                DepthBufferDesc.Usage       = USAGE_DEFAULT;
                DepthBufferDesc.BindFlags   = BIND_DEPTH_STENCIL;

                DepthBufferDesc.ClearValue.Format               = DepthBufferDesc.Format;
                DepthBufferDesc.ClearValue.DepthStencil.Depth   = m_SwapChainDesc.DefaultDepthValue;
                DepthBufferDesc.ClearValue.DepthStencil.Stencil = m_SwapChainDesc.DefaultStencilValue;

                m_pRenderDevice->CreateTexture(DepthBufferDesc, nullptr, &m_pDepthBuffer);
                VERIFY_EXPR(m_pDepthBuffer != nullptr);
                m_pDSV = m_pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
                VERIFY_EXPR(m_pDSV != nullptr);
            }
        }
    }

    virtual void DILIGENT_CALL_TYPE SetFullscreenMode(const DisplayModeAttribs& DisplayMode) override final
    {
        UNEXPECTED("Off-screen swap chain can't go into full screen mode");
    }

    virtual void DILIGENT_CALL_TYPE SetWindowedMode() override final
    {
        UNEXPECTED("Off-screen swap chain can't switch between windowed and full screen modes");
    }

    virtual void DILIGENT_CALL_TYPE SetMaximumFrameLatency(Uint32 MaxLatency) override final
    {
        UNEXPECTED("Off-screen swap chain can't set the maximum frame latency");
    }

    virtual ITextureView* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override final
    {
        return m_pRTV;
    }

    virtual ITextureView* DILIGENT_CALL_TYPE GetDepthBufferDSV() override final
    {
        return m_pDSV;
    }

protected:
    RefCntAutoPtr<ITexture>     m_pRenderTarget;
    RefCntAutoPtr<ITexture>     m_pDepthBuffer;
    RefCntAutoPtr<ITextureView> m_pRTV;
    RefCntAutoPtr<ITextureView> m_pDSV;
};


void CreateOffScreenSwapChain(IRenderDevice* pDevice, IDeviceContext* pContext, const SwapChainDesc& SCDesc, ISwapChain** ppSwapChain)
{
    try
    {
        RefCntAutoPtr<ISwapChain> pSwapChain{MakeNewRCObj<OffScreenSwapChain>()(pDevice, pContext, SCDesc)};
        if (pSwapChain)
            pSwapChain->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
    }
    catch (...)
    {
        LOG_ERROR("Failed to create off-screen swap chain");
    }
}

} // namespace Diligent
