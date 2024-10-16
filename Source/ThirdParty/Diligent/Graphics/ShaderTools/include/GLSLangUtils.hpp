/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
#include "Shader.h"
#include "DataBlob.h"

namespace Diligent
{

namespace GLSLangUtils
{

enum class SpirvVersion
{
    Vk100,         // SPIRV 1.0
    Vk110,         // SPIRV 1.3
    Vk110_Spirv14, // SPIRV 1.4 (extension)
    Vk120,         // SPIRV 1.5

    GL,   // SPIRV 1.0
    GLES, // SPIRV 1.0

    Count
};

void InitializeGlslang();
void FinalizeGlslang();

struct GLSLtoSPIRVAttribs
{
    SHADER_TYPE                      ShaderType    = SHADER_TYPE_UNKNOWN;
    const char*                      ShaderSource  = nullptr;
    int                              SourceCodeLen = 0;
    ShaderMacroArray                 Macros;
    IShaderSourceInputStreamFactory* pShaderSourceStreamFactory = nullptr;
    SpirvVersion                     Version                    = SpirvVersion::Vk100;
    IDataBlob**                      ppCompilerOutput           = nullptr;
    bool                             AssignBindings             = true;
    bool                             UseRowMajorMatrices        = false;
};

std::vector<unsigned int> GLSLtoSPIRV(const GLSLtoSPIRVAttribs& Attribs);

std::vector<unsigned int> HLSLtoSPIRV(const ShaderCreateInfo& ShaderCI,
                                      SpirvVersion            Version,
                                      const char*             ExtraDefinitions,
                                      IDataBlob**             ppCompilerOutput);

} // namespace GLSLangUtils

} // namespace Diligent
