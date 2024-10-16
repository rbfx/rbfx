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
#include <string>
#include <utility>

#include "BasicTypes.h"
#include "GraphicsTypes.h"
#include "Shader.h"

namespace Diligent
{

enum class TargetGLSLCompiler
{
    glslang,
    driver
};

struct IHLSL2GLSLConversionStream;

// If HLSL->GLSL converter is used to convert HLSL shader source to
// GLSL, this member can provide pointer to the conversion stream. It is useful
// when the same file is used to create a number of different shaders. If
// ppConversionStream is null, the converter will parse the same file
// every time new shader is converted. If ppConversionStream is not null,
// the converter will write pointer to the conversion stream to *ppConversionStream
// the first time and will use it in all subsequent times.
// For all subsequent conversions, FilePath member must be the same, or
// new stream will be created and warning message will be displayed.
struct BuildGLSLSourceStringAttribs
{
    const ShaderCreateInfo&       ShaderCI;
    const GraphicsAdapterInfo&    AdapterInfo;
    const DeviceFeatures&         Features;
    RENDER_DEVICE_TYPE            DeviceType         = RENDER_DEVICE_TYPE_UNDEFINED;
    RenderDeviceShaderVersionInfo MaxShaderVersion   = {};
    TargetGLSLCompiler            TargetCompiler     = TargetGLSLCompiler::glslang;
    bool                          ZeroToOneClipZ     = false;
    const char*                   ExtraDefinitions   = nullptr;
    IHLSL2GLSLConversionStream**  ppConversionStream = nullptr;
};

String BuildGLSLSourceString(const BuildGLSLSourceStringAttribs& Attribs) noexcept(false);

void GetGLSLVersion(const ShaderCreateInfo&              ShaderCI,
                    TargetGLSLCompiler                   TargetCompiler,
                    RENDER_DEVICE_TYPE                   DeviceType,
                    const RenderDeviceShaderVersionInfo& MaxShaderVersion,
                    ShaderVersion&                       GLSLVer,
                    bool&                                IsES);

/// Extracts all #extension directives from the GLSL source, returning them as a vector of pairs
/// (extension name, extension behavior). The behavior is the string following the extension name.
std::vector<std::pair<std::string, std::string>> GetGLSLExtensions(const char* Source, size_t SourceLen = 0);

} // namespace Diligent
