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
/// Declaration of Diligent::PipelineLayoutWebGPU class

#include <array>
#include <memory>

#include "Constants.h"
#include "WebGPUObjectWrappers.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

class RenderDeviceWebGPUImpl;
class PipelineResourceSignatureWebGPUImpl;

/// Implementation of the Diligent::PipelineLayoutWebGPU class
class PipelineLayoutWebGPU
{
public:
    PipelineLayoutWebGPU();
    ~PipelineLayoutWebGPU();

    void Create(RenderDeviceWebGPUImpl* pDeviceWebGPU, RefCntAutoPtr<PipelineResourceSignatureWebGPUImpl> ppSignatures[], Uint32 SignatureCount) noexcept(false);

    WGPUPipelineLayout GetWebGPUPipelineLayout();

    // Returns the index of the first bind group used by the resource signature at the given bind index
    Uint32 GetFirstBindGroupIndex(Uint32 Index) const
    {
        VERIFY_EXPR(Index <= m_DbgMaxBindIndex);
        return m_FirstBindGroupIndex[Index];
    }

    Uint32 GetBindGroupCount() const { return m_BindGroupCount; }

private:
    struct WGPUPipelineLayoutCreateInfo;
    std::unique_ptr<WGPUPipelineLayoutCreateInfo> m_PipelineLayoutCreateInfo;

    WebGPUPipelineLayoutWrapper m_wgpuPipelineLayout;

    using FirstBindGroupIndexArrayType = std::array<Uint8, MAX_RESOURCE_SIGNATURES>;
    // Index of the first bind group, for every resource signature.
    FirstBindGroupIndexArrayType m_FirstBindGroupIndex = {};

    // The total number of bind groups used by this pipeline layout
    // (Maximum is MAX_RESOURCE_SIGNATURES * 2)
    Uint8 m_BindGroupCount = 0;

#ifdef DILIGENT_DEBUG
    Uint32 m_DbgMaxBindIndex = 0;
#endif
};

} // namespace Diligent
