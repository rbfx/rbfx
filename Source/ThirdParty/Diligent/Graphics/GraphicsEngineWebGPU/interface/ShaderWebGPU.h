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
/// Definition of the Diligent::IShaderWebGPU interface

#include "../../../Primitives/interface/CommonDefinitions.h"
#if DILIGENT_CPP_INTERFACE
#    include <vector>
#endif

#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {902FA43B-AFC6-4103-815F-C97AA31E0C7B}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_ShaderWebGPU =
    {0x902fA43B, 0xAFC6, 0x4103, {0x81, 0x5F, 0xC9, 0x7A, 0xA3, 0x1E, 0x0C, 0x7B}};

#define IShaderWebGPUInclusiveMethods \
    IShaderInclusiveMethods;          \
    IShaderWebGPUMethods ShaderWebGPU

#if DILIGENT_CPP_INTERFACE

/// Exposes WebGPU-specific functionality of a shader object.
class IShaderWebGPU : public IShader
{
public:
    /// Returns WGSL source code
    virtual const std::string& DILIGENT_CALL_TYPE GetWGSL() const = 0;

    /// Returns a suffix to append to the name of emulated array variables to get
    /// the indexed array element name.
    ///
    /// \remarks    This value is defined by ShaderCI.WebGPUEmulatedArrayIndexSuffix,
    ///             see Diligent::ShaderCreateInfo::WebGPUEmulatedArrayIndexSuffix.
    virtual const char* DILIGENT_CALL_TYPE GetEmulatedArrayIndexSuffix() const = 0;
};

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
