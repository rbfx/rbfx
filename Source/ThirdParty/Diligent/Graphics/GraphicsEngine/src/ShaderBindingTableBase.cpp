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

#include "ShaderBindingTableBase.hpp"

namespace Diligent
{

void ValidateShaderBindingTableDesc(const ShaderBindingTableDesc& Desc, Uint32 ShaderGroupHandleSize, Uint32 MaxShaderRecordStride) noexcept(false)
{
#define LOG_SBT_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of Shader binding table '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)

    if (Desc.pPSO == nullptr)
    {
        LOG_SBT_ERROR_AND_THROW("pPSO must not be null.");
    }

    if (Desc.pPSO->GetDesc().PipelineType != PIPELINE_TYPE_RAY_TRACING)
    {
        LOG_SBT_ERROR_AND_THROW("pPSO must be ray tracing pipeline.");
    }


    const auto ShaderRecordSize   = Desc.pPSO->GetRayTracingPipelineDesc().ShaderRecordSize;
    const auto ShaderRecordStride = ShaderRecordSize + ShaderGroupHandleSize;

    if (ShaderRecordStride > MaxShaderRecordStride)
    {
        LOG_SBT_ERROR_AND_THROW("ShaderRecordSize(", ShaderRecordSize, ") is too big, max size is: ", MaxShaderRecordStride - ShaderGroupHandleSize);
    }

    if (ShaderRecordStride % ShaderGroupHandleSize != 0)
    {
        LOG_SBT_ERROR_AND_THROW("ShaderRecordSize (", ShaderRecordSize, ") plus ShaderGroupHandleSize (", ShaderGroupHandleSize,
                                ") must be a multiple of ", ShaderGroupHandleSize);
    }
#undef LOG_SBT_ERROR_AND_THROW
}

} // namespace Diligent
