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
/// Definition of the Diligent::IShaderResourceVariableD3D interface

#include "../../GraphicsEngine/interface/ShaderResourceVariable.h"
#include "ShaderD3D.h"

namespace Diligent
{

// {99BCAFBF-E7E1-420A-929B-C862265FD146}
static constexpr INTERFACE_ID IID_ShaderResourceVariableD3D =
    {0x99bcafbf, 0xe7e1, 0x420a, {0x92, 0x9b, 0xc8, 0x62, 0x26, 0x5f, 0xd1, 0x46}};


/// Interface to the Direct3D ShaderResourceVariable resource variable
class IShaderResourceVariableD3D : public IShaderResourceVariable
{
public:
    /// Returns HLSL ShaderResourceVariable resource description
    virtual void DILIGENT_CALL_TYPE GetHLSLResourceDesc(HLSLShaderResourceDesc& HLSLResDesc) const = 0;
};

} // namespace Diligent
