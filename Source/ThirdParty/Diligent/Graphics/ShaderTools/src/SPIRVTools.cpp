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

#include "SPIRVTools.hpp"
#include "DebugUtilities.hpp"

#include "spirv-tools/optimizer.hpp"

namespace Diligent
{

namespace
{

void SpvOptimizerMessageConsumer(
    spv_message_level_t level,
    const char* /* source */,
    const spv_position_t& /* position */,
    const char* message)
{
    const char*            LevelText   = "message";
    DEBUG_MESSAGE_SEVERITY MsgSeverity = DEBUG_MESSAGE_SEVERITY_INFO;
    switch (level)
    {
        case SPV_MSG_FATAL:
            // Unrecoverable error due to environment (e.g. out of memory)
            LevelText   = "fatal error";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_FATAL_ERROR;
            break;

        case SPV_MSG_INTERNAL_ERROR:
            // Unrecoverable error due to SPIRV-Tools internals (e.g. unimplemented feature)
            LevelText   = "internal error";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_ERROR;
            break;

        case SPV_MSG_ERROR:
            // Normal error due to user input.
            LevelText   = "error";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_ERROR;
            break;

        case SPV_MSG_WARNING:
            LevelText   = "warning";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_WARNING;
            break;

        case SPV_MSG_INFO:
            LevelText   = "info";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_INFO;
            break;

        case SPV_MSG_DEBUG:
            LevelText   = "debug";
            MsgSeverity = DEBUG_MESSAGE_SEVERITY_INFO;
            break;
    }

    if (level == SPV_MSG_FATAL || level == SPV_MSG_INTERNAL_ERROR || level == SPV_MSG_ERROR || level == SPV_MSG_WARNING)
        LOG_DEBUG_MESSAGE(MsgSeverity, "Spirv optimizer ", LevelText, ": ", message);
}

spv_target_env SpvTargetEnvFromSPIRV(const std::vector<uint32_t>& SPIRV)
{
    if (SPIRV.size() < 2)
    {
        // Invalid SPIRV
        return SPV_ENV_VULKAN_1_0;
    }

#define SPV_SPIRV_VERSION_WORD(MAJOR, MINOR) ((uint32_t(uint8_t(MAJOR)) << 16) | (uint32_t(uint8_t(MINOR)) << 8))
    switch (SPIRV[1])
    {
        case SPV_SPIRV_VERSION_WORD(1, 0): return SPV_ENV_VULKAN_1_0;
        case SPV_SPIRV_VERSION_WORD(1, 1): return SPV_ENV_VULKAN_1_0;
        case SPV_SPIRV_VERSION_WORD(1, 2): return SPV_ENV_VULKAN_1_0;
        case SPV_SPIRV_VERSION_WORD(1, 3): return SPV_ENV_VULKAN_1_1;
        case SPV_SPIRV_VERSION_WORD(1, 4): return SPV_ENV_VULKAN_1_1_SPIRV_1_4;
        case SPV_SPIRV_VERSION_WORD(1, 5): return SPV_ENV_VULKAN_1_2;
        case SPV_SPIRV_VERSION_WORD(1, 6): return SPV_ENV_VULKAN_1_3;
        default: return SPV_ENV_VULKAN_1_3;
    }
}

} // namespace

std::vector<uint32_t> OptimizeSPIRV(const std::vector<uint32_t>& SrcSPIRV, spv_target_env TargetEnv, SPIRV_OPTIMIZATION_FLAGS Passes)
{
    VERIFY_EXPR(Passes != SPIRV_OPTIMIZATION_FLAG_NONE);

    if (TargetEnv == SPV_ENV_MAX)
        TargetEnv = SpvTargetEnvFromSPIRV(SrcSPIRV);

    spvtools::Optimizer SpirvOptimizer(TargetEnv);
    SpirvOptimizer.SetMessageConsumer(SpvOptimizerMessageConsumer);

    // SPIR-V bytecode generated from HLSL must be legalized to
    // turn it into a valid vulkan SPIR-V shader.
    if (Passes & SPIRV_OPTIMIZATION_FLAG_LEGALIZATION)
    {
        SpirvOptimizer.RegisterLegalizationPasses();
    }

    if (Passes & SPIRV_OPTIMIZATION_FLAG_PERFORMANCE)
    {
        SpirvOptimizer.RegisterPerformancePasses();
    }

    if (Passes & SPIRV_OPTIMIZATION_FLAG_STRIP_REFLECTION)
    {
        // Decorations defined in SPV_GOOGLE_hlsl_functionality1 are the only instructions
        // removed by strip-reflect-info pass. SPIRV offsets become INVALID after this operation.
        SpirvOptimizer.RegisterPass(spvtools::CreateStripReflectInfoPass());
    }

    std::vector<uint32_t> OptimizedSPIRV;
    if (!SpirvOptimizer.Run(SrcSPIRV.data(), SrcSPIRV.size(), &OptimizedSPIRV))
        OptimizedSPIRV.clear();

    return OptimizedSPIRV;
}

} // namespace Diligent
