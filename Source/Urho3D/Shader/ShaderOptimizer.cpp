// Copyright (c) 2008-2020 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"
#include "Urho3D/Core/Assert.h"

#include "Urho3D/Shader/ShaderOptimizer.h"

#include "Urho3D/Core/Format.h"

#ifdef URHO3D_SHADER_OPTIMIZER
    #include <spirv-tools/optimizer.hpp>
#endif

namespace Urho3D
{

#ifdef URHO3D_SHADER_OPTIMIZER
namespace
{

spv_target_env GetTarget(TargetShaderLanguage targetLanguage)
{
    switch (targetLanguage)
    {
    case TargetShaderLanguage::VULKAN_1_0: return SPV_ENV_VULKAN_1_0;
    default: return SPV_ENV_UNIVERSAL_1_6;
    }
}

auto AppendToString(ea::string& result)
{
    const auto callback =
        [&](spv_message_level_t level, const char* /*source*/, const spv_position_t& /*position*/, const char* message)
    {
        if (level <= SPV_MSG_ERROR)
            result += Format("Failed to optimize SPIR-V shader: {}\n", message);
    };
    return callback;
}

} // namespace
#endif

bool OptimizeSpirVShader(SpirVShader& shader, ea::string& optimizerOutput, TargetShaderLanguage targetLanguage)
{
#ifdef URHO3D_SHADER_OPTIMIZER
    const bool isVulkan = targetLanguage == TargetShaderLanguage::VULKAN_1_0;
    const auto targetEnv = GetTarget(targetLanguage);

    spvtools::Optimizer spirvOptimizer(targetEnv);
    spirvOptimizer.SetMessageConsumer(AppendToString(optimizerOutput));

    if (isVulkan)
        spirvOptimizer.RegisterLegalizationPasses();

    spirvOptimizer.RegisterPerformancePasses(true);

    spvtools::OptimizerOptions optimizerOptions;
    optimizerOptions.set_run_validator(!isVulkan);

    std::vector<uint32_t> optimizedSPIRV;
    if (!spirvOptimizer.Run(shader.bytecode_.data(), shader.bytecode_.size(), &optimizedSPIRV, optimizerOptions))
    {
        // Vulkan requires optimization, other backends don't.
        return !isVulkan;
    }

    if (isVulkan)
    {
        spvtools::ValidatorOptions validatorOptions;
        spvtools::SpirvTools tools(targetEnv);
        tools.SetMessageConsumer(AppendToString(optimizerOutput));
        if (!tools.Validate(optimizedSPIRV.data(), optimizedSPIRV.size(), validatorOptions))
        {
            return false;
        }
    }

    shader.bytecode_ = ea::move(optimizedSPIRV);
    return true;
#else
    URHO3D_ASSERTLOG(0, "URHO3D_SHADER_OPTIMIZER should be enabled to use OptimizeSpirVShader");
    return false;
#endif
}

} // namespace Urho3D
