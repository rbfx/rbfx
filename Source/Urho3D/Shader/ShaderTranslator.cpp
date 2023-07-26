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

#ifdef URHO3D_SHADER_TRANSLATOR
    #include <SPIRV/GlslangToSpv.h>
    #include <StandAlone/ResourceLimits.h>
    #include <glslang/Public/ShaderLang.h>

    #include <spirv_hlsl.hpp>
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

/// Vertex element semantics and index.
using VertexElementSemanticIndex = ea::pair<VertexElementSemantic, unsigned>;

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

/// Return vertex element semantics.
VertexElementSemanticIndex ParseVertexElement(ea::string_view name)
{
    const auto firstDigit = ea::find_if(name.begin(), name.end(), [](char ch) { return isdigit(ch); });
    const unsigned index = firstDigit != name.end() ? ToUInt(ea::string{ firstDigit, name.end() }) : 0;
    static const ea::unordered_map<ea::string, VertexElementSemantic> semanticsMapping = {
        { "iPos", SEM_POSITION },
        { "iNormal", SEM_NORMAL },
        { "iColor", SEM_COLOR },
        { "iTexCoord", SEM_TEXCOORD },
        { "iTangent", SEM_TANGENT },
        { "iBlendWeights", SEM_BLENDWEIGHTS },
        { "iBlendIndices", SEM_BLENDINDICES },
        { "iObjectIndex", SEM_OBJECTINDEX }
    };
    const ea::string_view sematicName = firstDigit != name.end() ? name.substr(0, firstDigit - name.begin()) : name;
    const auto semanticIter = semanticsMapping.find_as(sematicName);
    const VertexElementSemantic semantic = semanticIter != semanticsMapping.end()
        ? semanticIter->second
        : MAX_VERTEX_ELEMENT_SEMANTICS;
    return { semantic , index };
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
void CompileSpirV(
    SpirVShader& outputShader, EShLanguage stage, ea::string_view sourceCode, const ShaderDefineArray& shaderDefines)
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
    glslang::TShader shader(stage);
    shader.setStringsWithLengths(inputStrings, inputLengths, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 100);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    shader.setAutoMapLocations(true);

    // Parse input shader
    if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, EShMsgDefault))
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

    // Parse input layout
    /*program.buildReflection(EShReflectionAllBlockVariables);
    for (int i = 0; i < program.getNumLiveAttributes(); ++i)
    {
        const char* name = program.getAttributeName(i);
        const auto element = ParseVertexElement(name);
        if (element.first == MAX_VERTEX_ELEMENT_SEMANTICS)
        {
            errorMessage = Format("Unknown vertex element '{}'", name);
            return false;
        }
        outputShader.inputLayout_.push_back(element);
    }*/

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
    RemappingCompilerHLSL(std::vector<unsigned> spirv) : spirv_cross::CompilerHLSL(spirv)
    {
    }

    bool RemapInputLayout(ea::string& errorMessage)
    {
        unsigned location = 0;
        unsigned numErrors = 0;

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
                    m.decoration_flags.set(spv::DecorationLocation);
                    m.location = location++;
                    const VertexElementSemanticIndex vertexElement = ParseVertexElement(m.alias.c_str());
                    if (vertexElement.first == MAX_VERTEX_ELEMENT_SEMANTICS)
                    {
                        ++numErrors;
                        errorMessage += Format("Unknown input vertex element: '{}'\n", m.alias.c_str());
                        return;
                    }

                    const VertexElementSemantic semantic = vertexElement.first;
                    const unsigned index = vertexElement.second;
                    const ea::string name = Format("{}{}", ShaderVariation::elementSemanticNames[semantic], index);
                    add_vertex_attribute_remap({ m.location, name.c_str() });
                }
            });
        }

        return numErrors == 0;
    }
};

/// Convert SpirV to HLSL5
void ConvertToHLSL5(TargetShader& output, const SpirVShader& shader)
{
    output.sourceCode_.clear();
    output.compilerOutput_.clear();

    RemappingCompilerHLSL compiler(shader.bytecode_);
    if (!compiler.RemapInputLayout(output.compilerOutput_))
        return;

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
}

}
#endif

void ParseUniversalShader(
    SpirVShader& output, ShaderType shaderType, const ea::string& sourceCode, const ShaderDefineArray& shaderDefines)
{
#ifdef URHO3D_SHADER_TRANSLATOR
    CompileSpirV(output, ConvertShaderType(shaderType), sourceCode, shaderDefines);
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

    case TargetShaderLanguage::GLSL_3_2:
        ConvertToGLSL(output, shader, 150, false);
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

}

#ifdef URHO3D_SHADER_TRANSLATOR
namespace glslang
{

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

}
#endif
