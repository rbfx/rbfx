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

#include <vector>
#include <atomic>

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"

#include "GPUTestingEnvironment.hpp"

namespace Diligent
{

namespace Testing
{

class ReferenceBuffers
{
public:
    ReferenceBuffers(Uint32           NumBuffers,
                     USAGE            Usage,
                     BIND_FLAGS       BindFlags,
                     BUFFER_VIEW_TYPE ViewType   = BUFFER_VIEW_UNDEFINED,
                     BUFFER_MODE      BufferMode = BUFFER_MODE_UNDEFINED) :
        Buffers(NumBuffers),
        Views(NumBuffers),
        ppBuffObjects(NumBuffers),
        ppViewObjects(NumBuffers),
        UsedValues(NumBuffers),
        Values(NumBuffers)
    {
        auto* pEnv    = GPUTestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        for (Uint32 i = 0; i < NumBuffers; ++i)
        {
            static std::atomic_int Counter{10};

            auto& Value = Values[i];
            float v     = static_cast<float>(Counter.fetch_add(10));
            Value       = float4{v + 1, v + 2, v + 3, v + 4};

            std::vector<float4> InitData(16, Value);

            BufferDesc BuffDesc;

            String Name   = "Reference buffer " + std::to_string(i);
            BuffDesc.Name = Name.c_str();

            BuffDesc.Usage             = Usage;
            BuffDesc.BindFlags         = BindFlags;
            BuffDesc.Mode              = BufferMode;
            BuffDesc.Size              = static_cast<Uint64>(InitData.size() * sizeof(InitData[0]));
            BuffDesc.ElementByteStride = BufferMode != BUFFER_MODE_UNDEFINED ? 16 : 0;

            auto&      pBuffer = Buffers[i];
            BufferData BuffData{InitData.data(), BuffDesc.Size};
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer);
            if (!pBuffer)
            {
                ADD_FAILURE() << "Unable to create buffer " << BuffDesc;
                return;
            }
            ppBuffObjects[i] = pBuffer;

            if (ViewType != BUFFER_VIEW_UNDEFINED)
            {
                auto& pView = Views[i];
                if (BufferMode == BUFFER_MODE_FORMATTED)
                {
                    BufferViewDesc BuffViewDesc;
                    BuffViewDesc.Name                 = "Formatted buffer SRV";
                    BuffViewDesc.ViewType             = ViewType;
                    BuffViewDesc.Format.ValueType     = VT_FLOAT32;
                    BuffViewDesc.Format.NumComponents = 4;
                    BuffViewDesc.Format.IsNormalized  = false;

                    pBuffer->CreateView(BuffViewDesc, &pView);
                }
                else
                {
                    pView = pBuffer->GetDefaultView(ViewType);
                }

                if (!pView)
                {
                    ADD_FAILURE() << "Unable to create buffer view";
                    return;
                }

                ppViewObjects[i] = pView;
            }
        }
    }

    IBuffer*     GetBuffer(size_t i) { return Buffers[i]; };
    IBufferView* GetView(size_t i) { return Views[i]; };

    IDeviceObject** GetBuffObjects(size_t i) { return &ppBuffObjects[i]; };
    IDeviceObject** GetViewObjects(size_t i) { return &ppViewObjects[i]; };

    const float4& GetValue(size_t i)
    {
        VERIFY(!UsedValues[i], "Buffer ", i, " has already been used. Every buffer is expected to be used once.");
        UsedValues[i] = true;
        VERIFY(Values[i] != float4{}, "Value must not be zero");
        return Values[i];
    }

    void ClearUsedValues()
    {
        std::fill(UsedValues.begin(), UsedValues.end(), false);
    }

private:
    std::vector<RefCntAutoPtr<IBuffer>>     Buffers;
    std::vector<RefCntAutoPtr<IBufferView>> Views;

    std::vector<IDeviceObject*> ppBuffObjects;
    std::vector<IDeviceObject*> ppViewObjects;

    std::vector<bool>   UsedValues;
    std::vector<float4> Values;
};

class ReferenceTextures
{
public:
    ReferenceTextures(Uint32            NumTextures,
                      Uint32            Width,
                      Uint32            Height,
                      USAGE             Usage,
                      BIND_FLAGS        BindFlags,
                      TEXTURE_VIEW_TYPE ViewType) :
        Textures(NumTextures),
        ppViewObjects(NumTextures),
        UsedValues(NumTextures),
        Values(NumTextures)
    {
        auto* pEnv = GPUTestingEnvironment::GetInstance();

        for (Uint32 i = 0; i < NumTextures; ++i)
        {
            auto& pTexture = Textures[i];
            auto& Value    = Values[i];

            static std::atomic_int Counter{1};

            int v = (Counter.fetch_add(1) % 15) + 1;
            Value = float4{
                (v & 0x01) ? 1.f : 0.f,
                (v & 0x02) ? 1.f : 0.f,
                (v & 0x04) ? 1.f : 0.f,
                (v & 0x08) ? 1.f : 0.f //
            };

            std::vector<Uint32> TexData(Width * Height, F4Color_To_RGBA8Unorm(Value));

            String Name      = String{"Reference texture "} + std::to_string(i);
            pTexture         = pEnv->CreateTexture(Name.c_str(), TEX_FORMAT_RGBA8_UNORM, BindFlags, Width, Height, TexData.data());
            ppViewObjects[i] = pTexture->GetDefaultView(ViewType);
        }
    }

    ITextureView*   GetView(size_t i) { return ClassPtrCast<ITextureView>(ppViewObjects[i]); };
    IDeviceObject** GetViewObjects(size_t i) { return &ppViewObjects[i]; };

    const float4& GetColor(size_t i)
    {
        VERIFY(!UsedValues[i], "Texture ", i, " has already been used. Every texture is expected to be used once.");
        UsedValues[i] = true;
        VERIFY(Values[i] != float4{}, "Value must not be zero");
        return Values[i];
    }

    void ClearUsedValues()
    {
        std::fill(UsedValues.begin(), UsedValues.end(), false);
    }

    size_t GetTextureCount() const { return Textures.size(); }

private:
    std::vector<RefCntAutoPtr<ITexture>> Textures;

    std::vector<IDeviceObject*> ppViewObjects;

    std::vector<bool>   UsedValues;
    std::vector<float4> Values;
};

void PrintShaderResources(IShader* pShader);
void RenderDrawCommandReference(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
void ComputeShaderReference(ISwapChain* pSwapChain);

} // namespace Testing

} // namespace Diligent
