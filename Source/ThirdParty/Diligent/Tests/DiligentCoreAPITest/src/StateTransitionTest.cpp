/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ResourceStateTest, DiscardResource)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    TextureDesc TexDesc;
    TexDesc.Name      = "DiscardResource test texture";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = 256;
    TexDesc.Height    = 256;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
    TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;

    RefCntAutoPtr<ITexture> pTexture;
    pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
    ASSERT_NE(pTexture, nullptr);

    {
        const StateTransitionDesc Barrier{pTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET, STATE_TRANSITION_FLAG_UPDATE_STATE | STATE_TRANSITION_FLAG_DISCARD_CONTENT};
        pContext->TransitionResourceStates(1, &Barrier);
    }

    ITextureView* pRTV = pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    const float ClearColor[] = {0.4f, 0.1f, 0.2f, 1.f};
    pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

    {
        const StateTransitionDesc Barrier{pTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_UNORDERED_ACCESS, STATE_TRANSITION_FLAG_UPDATE_STATE | STATE_TRANSITION_FLAG_DISCARD_CONTENT};
        pContext->TransitionResourceStates(1, &Barrier);
    }

    pContext->Flush();
}

TEST(ResourceStateTest, DiscardSubresource)
{
    auto*       pEnv       = GPUTestingEnvironment::GetInstance();
    auto*       pDevice    = pEnv->GetDevice();
    auto*       pContext   = pEnv->GetDeviceContext();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    if (DeviceInfo.Type != RENDER_DEVICE_TYPE_D3D12 && DeviceInfo.Type != RENDER_DEVICE_TYPE_VULKAN)
        GTEST_SKIP() << "Subresource state transitions are only supported in D3D12 and Vulkan backends";

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    TextureDesc TexDesc;
    TexDesc.Name      = "DiscardSubresource test texture";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    TexDesc.Width     = 256;
    TexDesc.Height    = 256;
    TexDesc.ArraySize = 8;
    TexDesc.MipLevels = 5;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
    TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;

    RefCntAutoPtr<ITexture> pTexture;
    pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
    ASSERT_NE(pTexture, nullptr);

    RefCntAutoPtr<ITextureView> pRTV;
    {
        TextureViewDesc ViewDesc{"Subresource RTV", TEXTURE_VIEW_RENDER_TARGET, RESOURCE_DIM_TEX_2D_ARRAY};
        ViewDesc.FirstArraySlice = 4;
        ViewDesc.NumArraySlices  = 3;
        ViewDesc.MostDetailedMip = 2;
        pTexture->CreateView(ViewDesc, &pRTV);
        ASSERT_NE(pRTV, nullptr);
    }

    StateTransitionDesc Barrier{pTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_RENDER_TARGET, STATE_TRANSITION_FLAG_DISCARD_CONTENT};
    Barrier.FirstArraySlice = 3;
    Barrier.ArraySliceCount = 4;
    Barrier.FirstMipLevel   = 1;
    Barrier.MipLevelsCount  = 3;
    pContext->TransitionResourceStates(1, &Barrier);

    ITextureView* ppRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

    const float ClearColor[] = {0.4f, 0.1f, 0.2f, 1.f};
    pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);

    pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

    pTexture->SetState(RESOURCE_STATE_UNKNOWN);
    Barrier.OldState = RESOURCE_STATE_RENDER_TARGET;
    Barrier.NewState = RESOURCE_STATE_UNORDERED_ACCESS;
    pContext->TransitionResourceStates(1, &Barrier);

    pContext->Flush();
}

} // namespace
