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

#include "D3DShaderResourceValidation.hpp"
#include "ShaderResources.hpp"

namespace Diligent
{

void VerifyD3DResourceMerge(const char*                     PSOName,
                            const D3DShaderResourceAttribs& ExistingRes,
                            const D3DShaderResourceAttribs& NewResAttribs) noexcept(false)
{
#define LOG_RESOURCE_MERGE_ERROR_AND_THROW(PropertyName)                                                  \
    LOG_ERROR_AND_THROW("Shader variable '", NewResAttribs.Name,                                          \
                        "' is shared between multiple shaders in pipeline '", (PSOName ? PSOName : ""),   \
                        "', but its " PropertyName " varies. A variable shared between multiple shaders " \
                        "must be defined identically in all shaders. Either use separate variables for "  \
                        "different shader stages, change resource name or make sure that " PropertyName " is consistent.");

    if (ExistingRes.GetInputType() != NewResAttribs.GetInputType())
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("input type");

    if (ExistingRes.GetSRVDimension() != NewResAttribs.GetSRVDimension())
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("resource dimension");

    if (ExistingRes.BindCount != NewResAttribs.BindCount)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("array size");

    if (ExistingRes.IsMultisample() != NewResAttribs.IsMultisample())
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("multisample state");
#undef LOG_RESOURCE_MERGE_ERROR_AND_THROW
}

void ValidateShaderResourceBindings(const char*                  PSOName,
                                    const ShaderResources&       Resources,
                                    const ResourceBinding::TMap& BindingsMap) noexcept(false)
{
    VERIFY_EXPR(PSOName != nullptr);
    Resources.ProcessResources(
        [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
        {
            auto it = BindingsMap.find(Attribs.Name);
            if (it == BindingsMap.end())
            {
                LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", Resources.GetShaderName(), "' of PSO '", PSOName,
                                    "' is not present in the resource bindings map.");
            }

            const auto& Bindings = it->second;
            if (Bindings.BindPoint != Attribs.BindPoint)
            {
                LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", Resources.GetShaderName(), "' of PSO '", PSOName,
                                    "' is mapped to register ", Attribs.BindPoint, " in the shader, but the PSO expects it to be mapped to register ", Bindings.BindPoint);
            }

            if (Bindings.Space != Attribs.Space)
            {
                LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", Resources.GetShaderName(), "' of PSO '", PSOName,
                                    "' is mapped to space ", Attribs.Space, " in the shader, but the PSO expects it to be mapped to space ", Bindings.Space);
            }
        } //
    );
}

} // namespace Diligent
