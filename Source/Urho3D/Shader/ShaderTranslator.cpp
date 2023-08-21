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

#include "Urho3D/Shader/ShaderTranslator.h"

#include "Urho3D/Graphics/ShaderVariation.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#ifdef URHO3D_SHADER_TRANSLATOR
    #include <SPIRV/GlslangToSpv.h>
    #include <glslang/Public/ResourceLimits.h>
    #include <glslang/Public/ShaderLang.h>

    #include <spirv_hlsl.hpp>
    #include <spirv_parser.hpp>
#endif

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

ea::optional<ea::pair<unsigned, unsigned>> FindVersionTag(ea::string_view shaderCode)
{
    const unsigned start = shaderCode.find("#version");
    if (start == ea::string::npos)
        return ea::nullopt;

    const unsigned end = shaderCode.find('\n', start);
    if (end == ea::string::npos)
        return ea::make_pair(start, static_cast<unsigned>(shaderCode.size()));
    return ea::make_pair(start, end);
}

#ifdef URHO3D_SHADER_TRANSLATOR

namespace
{

/// Utility to de/initialize glslang library.
struct GlslangGuardian
{
    GlslangGuardian() { glslang::InitializeProcess(); }
    ~GlslangGuardian() { glslang::FinalizeProcess(); }
};
static const GlslangGuardian glslangGuardian;

EShLanguage ConvertShaderType(ShaderType shaderType)
{
    switch (shaderType)
    {
    default:
    case VS: return EShLangVertex;
    case PS: return EShLangFragment;
    case GS: return EShLangGeometry;
    case HS: return EShLangTessControl;
    case DS: return EShLangTessEvaluation;
    case CS: return EShLangCompute;
    }
}

void AppendWithoutVersion(ea::string& dest, ea::string_view source)
{
    const auto versionTag = FindVersionTag(source);
    if (!versionTag)
    {
        dest.append(source.begin(), source.end());
        return;
    }

    const auto versionIter = ea::next(source.begin(), versionTag->first);
    dest.append(source.begin(), versionIter);
    dest.append("//");
    dest.append(versionIter, source.end());
}

/// Compile SpirV shader.
void CompileSpirV(SpirVShader& outputShader, EShLanguage stage, ea::string_view sourceCode,
    const ShaderDefineArray& shaderDefines, TargetShaderLanguage targetLanguage)
{
    outputShader.bytecode_.clear();
    outputShader.compilerOutput_.clear();

    // Prepare defines
    thread_local ea::string shaderCode;
    shaderCode.clear();
    shaderCode += "#version 450\n";
    for (const auto& define : shaderDefines)
        shaderCode += Format("#define {} {}\n", define.first, define.second);
    AppendWithoutVersion(shaderCode, sourceCode);

    const char* inputStrings[] = { shaderCode.data() };
    const int inputLengths[] = { static_cast<int>(shaderCode.length()) };

    // Setup glslang shader
    const auto envClient =
        targetLanguage == TargetShaderLanguage::VULKAN_1_0 ? glslang::EShClientVulkan : glslang::EShClientOpenGL;
    glslang::TShader shader(stage);
    shader.setStringsWithLengths(inputStrings, inputLengths, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, envClient, 100);
    shader.setEnvClient(envClient, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    shader.setAutoMapLocations(true);

    // Parse input shader
    if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault))
    {
        outputShader.compilerOutput_ = shader.getInfoLog();
        return;
    }

    // Link into fake program
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault))
    {
        outputShader.compilerOutput_ = program.getInfoLog();
        return;
    }
    if (!program.mapIO())
    {
        outputShader.compilerOutput_ = program.getInfoLog();
        return;
    }

    // Convert intermediate representation to SPIRV
    const glslang::TIntermediate* im = program.getIntermediate(stage);
    assert(im);
    spv::SpvBuildLogger spvLogger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = true;
    spvOptions.disableOptimizer = true;
    spvOptions.optimizeSize = false;
    glslang::GlslangToSpv(*im, outputShader.bytecode_, &spvLogger, &spvOptions);
    const std::string spirvLog = spvLogger.getAllMessages();
    if (!spirvLog.empty())
    {
        outputShader.bytecode_.clear();
        outputShader.compilerOutput_ = spirvLog.c_str();
    }
}

/// Remapping HLSL compiler.
class RemappingCompilerHLSL : public spirv_cross::CompilerHLSL
{
public:
    RemappingCompilerHLSL(std::vector<unsigned> spirv)
        : spirv_cross::CompilerHLSL(ea::move(spirv))
    {
        RemapInputLayout();
        CollectSamplers();
    }

    std::string compile() override
    {
        std::string result = spirv_cross::CompilerHLSL::compile();
        ReplaceSamplers(result);
        return result;
    }

private:
    void RemapInputLayout()
    {
        const auto& execution = get_entry_point();
        if (execution.model == spv::ExecutionModelVertex)
        {
            // Output Uniform Constants (values, samplers, images, etc).
            ir.for_each_typed_id<spirv_cross::SPIRVariable>([&](uint32_t, spirv_cross::SPIRVariable &var)
            {
                auto& type = this->get<spirv_cross::SPIRType>(var.basetype);
                auto& m = ir.meta[var.self].decoration;
                if (type.storage == spv::StorageClassInput && !is_builtin_variable(var))
                {
                    URHO3D_ASSERT(m.decoration_flags.get(spv::DecorationLocation));

                    const ea::string name = Format("ATTRIB{}", m.location);
                    add_vertex_attribute_remap({ m.location, name.c_str() });
                }
            });
        }
    }

    void CollectSamplers()
    {
        ir.for_each_typed_id<spirv_cross::SPIRVariable>([&](uint32_t, spirv_cross::SPIRVariable &var)
        {
            auto& type = this->get<spirv_cross::SPIRType>(var.basetype);
            auto& m = ir.meta[var.self].decoration;
            if (type.basetype == spirv_cross::SPIRType::SampledImage && type.image.dim != spv::DimBuffer)
            {
                const std::string samplerName = "_" + m.alias + "_sampler";
                samplers_.emplace_back(samplerName);
            }
        });
    }

    void ReplaceSamplers(std::string& hlsl)
    {
        // Use arbitrary non-text symbol to replace leading underscore in sampler name.
        // Then remove it from the string.
        const char placeholder = '\1';

        // TODO: Optimize it?
        for (const std::string& name : samplers_)
        {
            for (size_t pos = hlsl.find(name, 0); pos != std::string::npos; pos = hlsl.find(name, pos))
                hlsl[pos] = placeholder;
        }

        const auto isPlaceholder = [placeholder](char c) { return c == placeholder; };
        hlsl.erase(ea::remove_if(hlsl.begin(), hlsl.end(), isPlaceholder), hlsl.end());
    }

    ea::vector<std::string> samplers_;
};

/// Convert SpirV to HLSL5
void ConvertToHLSL5(TargetShader& output, const SpirVShader& shader)
{
    output.sourceCode_.clear();
    output.compilerOutput_.clear();

    RemappingCompilerHLSL compiler(shader.bytecode_);

    spirv_cross::CompilerGLSL::Options commonOptions;
    commonOptions.emit_line_directives = true;
    compiler.set_common_options(commonOptions);
    spirv_cross::CompilerHLSL::Options hlslOptions;
    hlslOptions.shader_model = 50;
    hlslOptions.point_size_compat = true;
    compiler.set_hlsl_options(hlslOptions);
    const std::string src = compiler.compile();
    if (src.empty())
    {
        output.compilerOutput_ = "Unknown error";
        return;
    }

    output.sourceCode_ = src.c_str();
}

void RemoveClipDistance(ea::string& shaderCode)
{
    const unsigned index = shaderCode.find("out float gl_ClipDistance");
    if (index != ea::string::npos)
        shaderCode.insert(index, "// Workaround for GLSL ES:\n// ");
}

void ConvertToGLSL(TargetShader& output, const SpirVShader& shader, int version, bool es)
{
    output.sourceCode_.clear();
    output.compilerOutput_.clear();

    spirv_cross::CompilerGLSL compiler(shader.bytecode_);
    spirv_cross::CompilerGLSL::Options options;
    options.version = version;
    options.es = es;
    //options.vertex.fixup_clipspace = true;
    //options.vertex.flip_vert_y = true;
    compiler.set_common_options(options);
    const std::string src = compiler.compile();
    if (src.empty())
    {
        output.compilerOutput_ = "Unknown error";
        return;
    }

    output.sourceCode_ = src.c_str();

    if (es)
        RemoveClipDistance(output.sourceCode_);
}

}
#endif

void ParseUniversalShader(SpirVShader& output, ShaderType shaderType, ea::string_view sourceCode,
    const ShaderDefineArray& shaderDefines, TargetShaderLanguage targetLanguage)
{
#ifdef URHO3D_SHADER_TRANSLATOR
    CompileSpirV(output, ConvertShaderType(shaderType), sourceCode, shaderDefines, targetLanguage);
#else
    URHO3D_ASSERTLOG(0, "URHO3D_SHADER_TRANSLATOR should be enabled to use ParseUniversalShader");

    output = {};
#endif
}

void TranslateSpirVShader(TargetShader& output, const SpirVShader& shader, TargetShaderLanguage targetLanguage)
{
#ifdef URHO3D_SHADER_TRANSLATOR
    output.language_ = targetLanguage;
    switch (targetLanguage)
    {
    case TargetShaderLanguage::HLSL_5_0:
        ConvertToHLSL5(output, shader);
        break;

    case TargetShaderLanguage::GLSL_4_1:
        ConvertToGLSL(output, shader, 410, false);
        break;

    case TargetShaderLanguage::GLSL_ES_3_0:
        ConvertToGLSL(output, shader, 300, true);
        break;

    default:
        URHO3D_ASSERTLOG(0, "TODO: Implement");
    }
#else
    URHO3D_ASSERTLOG(0, "URHO3D_SHADER_TRANSLATOR should be enabled to use TranslateSpirVShader");

    output = {};
#endif
}

VertexShaderAttributeVector GetVertexAttributesFromSpirV(const SpirVShader& shader)
{
#ifdef URHO3D_SHADER_TRANSLATOR
    VertexShaderAttributeVector result;

    // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
    spirv_cross::Parser parser{shader.bytecode_};
    parser.parse();
    spirv_cross::Compiler compiler{ea::move(parser.get_parsed_ir())};

    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (const auto& input : resources.stage_inputs)
    {
        if (compiler.has_decoration(input.id, spv::DecorationLocation))
        {
            const auto location = compiler.get_decoration(input.id, spv::DecorationLocation);
            if (const auto vertexInput = ParseVertexAttribute(input.name.c_str()))
            {
                result.push_back(*vertexInput);
                result.back().inputIndex_ = location;
            }
        }
    }
    return result;
#else
    URHO3D_ASSERTLOG(0, "URHO3D_SHADER_TRANSLATOR should be enabled to use GetVertexAttributesFromSpirV");
    return {};
#endif
}

}
