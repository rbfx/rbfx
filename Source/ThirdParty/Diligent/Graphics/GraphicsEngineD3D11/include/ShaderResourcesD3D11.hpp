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

/// \file
/// Declaration of Diligent::ShaderResourcesD3D11 class

#include "ShaderResources.hpp"

namespace Diligent
{

/// Diligent::ShaderResourcesD3D11 class
class ShaderResourcesD3D11 : public ShaderResources
{
public:
    // Loads shader resources from the compiled shader bytecode
    ShaderResourcesD3D11(ID3DBlob*         pShaderBytecode,
                         const ShaderDesc& ShdrDesc,
                         const char*       CombinedSamplerSuffix,
                         bool              LoadConstantBufferReflection);
    ~ShaderResourcesD3D11();

    // clang-format off
    ShaderResourcesD3D11             (const ShaderResourcesD3D11&)  = delete;
    ShaderResourcesD3D11             (      ShaderResourcesD3D11&&) = delete;
    ShaderResourcesD3D11& operator = (const ShaderResourcesD3D11&)  = delete;
    ShaderResourcesD3D11& operator = (      ShaderResourcesD3D11&&) = delete;

    __forceinline Int32 GetMaxCBBindPoint()     const { return m_MaxCBBindPoint;      }
    __forceinline Int32 GetMaxSRVBindPoint()    const { return m_MaxSRVBindPoint;     }
    __forceinline Int32 GetMaxSamplerBindPoint()const { return m_MaxSamplerBindPoint; }
    __forceinline Int32 GetMaxUAVBindPoint()    const { return m_MaxUAVBindPoint;     }
    // clang-format on

private:
    using MaxBindPointType = Int8;

    // clang-format off
    MaxBindPointType m_MaxCBBindPoint      = -1; // Max == 13
    MaxBindPointType m_MaxSRVBindPoint     = -1; // Max == 127
    MaxBindPointType m_MaxSamplerBindPoint = -1; // Max == 15
    MaxBindPointType m_MaxUAVBindPoint     = -1; // Max == 7

    static constexpr UINT MaxAllowedBindPoint = std::numeric_limits<MaxBindPointType>::max();
    static_assert(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT-1 <= MaxAllowedBindPoint, "Not enough bits to represent max CB slot" );
    static_assert(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT-1      <= MaxAllowedBindPoint, "Not enough bits to represent max SRV slot");
    static_assert(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT-1             <= MaxAllowedBindPoint, "Not enough bits to represent max Sampler slot");
    static_assert(D3D11_PS_CS_UAV_REGISTER_COUNT-1                    <= MaxAllowedBindPoint, "Not enough bits to represent max UAV slot");
    // clang-format on
};

} // namespace Diligent
