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
/// Definition of the Diligent::IBottomLevelASD3D12 interface

#include "../../GraphicsEngine/interface/BottomLevelAS.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {610228AF-F161-4B12-A00E-71E6E3BB97FE}
static const INTERFACE_ID IID_BottomLevelASD3D12 =
    {0x610228af, 0xf161, 0x4b12, {0xa0, 0xe, 0x71, 0xe6, 0xe3, 0xbb, 0x97, 0xfe}};

#define DILIGENT_INTERFACE_NAME IBottomLevelASD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBottomLevelASD3D12InclusiveMethods \
    IBottomLevelASInclusiveMethods;         \
    IBottomLevelASD3D12Methods BottomLevelASD3D12

// clang-format off

/// Exposes Direct3D12-specific functionality of a bottom-level acceleration structure object.
DILIGENT_BEGIN_INTERFACE(IBottomLevelASD3D12, IBottomLevelAS)
{
    /// Returns ID3D12Resource interface of the internal D3D12 acceleration structure object.

    /// The method does *NOT* increment the reference counter of the returned object,
    /// so Release() must not be called.
    VIRTUAL ID3D12Resource* METHOD(GetD3D12BLAS)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBottomLevelASD3D12_GetD3D12BLAS(This)  CALL_IFACE_METHOD(BottomLevelASD3D12, GetD3D12BLAS, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
