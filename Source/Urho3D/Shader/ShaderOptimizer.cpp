//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
