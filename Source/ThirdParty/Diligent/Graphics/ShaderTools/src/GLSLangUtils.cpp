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

#include "GLSLangUtils.hpp"

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <array>

#ifdef VK_USE_PLATFORM_METAL_EXT
#    include <MoltenGLSLToSPIRVConverter/GLSLToSPIRVConverter.h>
#else
#    ifndef ENABLE_HLSL
#        define ENABLE_HLSL
#    endif
#    include "SPIRV/GlslangToSpv.h"
#endif

#include "glslang/Public/ShaderLang.h"

#include "DebugUtilities.hpp"
#include "DataBlobImpl.hpp"
#include "RefCntAutoPtr.hpp"
#include "ShaderToolsCommon.hpp"
#ifdef USE_SPIRV_TOOLS
#    include "SPIRVTools.hpp"
#endif

// clang-format off
static constexpr char g_HLSLDefinitions[] =
{
#include "HLSLDefinitions_inc.fxh"
};
// clang-format on

namespace Diligent
{

namespace GLSLangUtils
{

void InitializeGlslang()
{
    ::glslang::InitializeProcess();
}

void FinalizeGlslang()
{
    ::glslang::FinalizeProcess();
}

namespace
{

EShLanguage ShaderTypeToShLanguage(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return EShLangVertex;
        case SHADER_TYPE_HULL:             return EShLangTessControl;
        case SHADER_TYPE_DOMAIN:           return EShLangTessEvaluation;
        case SHADER_TYPE_GEOMETRY:         return EShLangGeometry;
        case SHADER_TYPE_PIXEL:            return EShLangFragment;
        case SHADER_TYPE_COMPUTE:          return EShLangCompute;
        case SHADER_TYPE_AMPLIFICATION:    return EShLangTaskNV;
        case SHADER_TYPE_MESH:             return EShLangMeshNV;
        case SHADER_TYPE_RAY_GEN:          return EShLangRayGen;
        case SHADER_TYPE_RAY_MISS:         return EShLangMiss;
        case SHADER_TYPE_RAY_CLOSEST_HIT:  return EShLangClosestHit;
        case SHADER_TYPE_RAY_ANY_HIT:      return EShLangAnyHit;
        case SHADER_TYPE_RAY_INTERSECTION: return EShLangIntersect;
        case SHADER_TYPE_CALLABLE:         return EShLangCallable;
        // clang-format on
        case SHADER_TYPE_TILE:
            UNEXPECTED("Unsupported shader type");
            return EShLangCount;
        default:
            UNEXPECTED("Unexpected shader type");
            return EShLangCount;
    }
}

TBuiltInResource InitResources()
{
    TBuiltInResource Resources = {};

    Resources.maxLights                                 = 32;
    Resources.maxClipPlanes                             = 6;
    Resources.maxTextureUnits                           = 32;
    Resources.maxTextureCoords                          = 32;
    Resources.maxVertexAttribs                          = 64;
    Resources.maxVertexUniformComponents                = 4096;
    Resources.maxVaryingFloats                          = 64;
    Resources.maxVertexTextureImageUnits                = 32;
    Resources.maxCombinedTextureImageUnits              = 80;
    Resources.maxTextureImageUnits                      = 32;
    Resources.maxFragmentUniformComponents              = 4096;
    Resources.maxDrawBuffers                            = 32;
    Resources.maxVertexUniformVectors                   = 128;
    Resources.maxVaryingVectors                         = 8;
    Resources.maxFragmentUniformVectors                 = 16;
    Resources.maxVertexOutputVectors                    = 16;
    Resources.maxFragmentInputVectors                   = 15;
    Resources.minProgramTexelOffset                     = -8;
    Resources.maxProgramTexelOffset                     = 7;
    Resources.maxClipDistances                          = 8;
    Resources.maxComputeWorkGroupCountX                 = 65535;
    Resources.maxComputeWorkGroupCountY                 = 65535;
    Resources.maxComputeWorkGroupCountZ                 = 65535;
    Resources.maxComputeWorkGroupSizeX                  = 1024;
    Resources.maxComputeWorkGroupSizeY                  = 1024;
    Resources.maxComputeWorkGroupSizeZ                  = 64;
    Resources.maxComputeUniformComponents               = 1024;
    Resources.maxComputeTextureImageUnits               = 16;
    Resources.maxComputeImageUniforms                   = 8;
    Resources.maxComputeAtomicCounters                  = 8;
    Resources.maxComputeAtomicCounterBuffers            = 1;
    Resources.maxVaryingComponents                      = 60;
    Resources.maxVertexOutputComponents                 = 64;
    Resources.maxGeometryInputComponents                = 64;
    Resources.maxGeometryOutputComponents               = 128;
    Resources.maxFragmentInputComponents                = 128;
    Resources.maxImageUnits                             = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    Resources.maxCombinedShaderOutputResources          = 8;
    Resources.maxImageSamples                           = 0;
    Resources.maxVertexImageUniforms                    = 0;
    Resources.maxTessControlImageUniforms               = 0;
    Resources.maxTessEvaluationImageUniforms            = 0;
    Resources.maxGeometryImageUniforms                  = 0;
    Resources.maxFragmentImageUniforms                  = 8;
    Resources.maxCombinedImageUniforms                  = 8;
    Resources.maxGeometryTextureImageUnits              = 16;
    Resources.maxGeometryOutputVertices                 = 256;
    Resources.maxGeometryTotalOutputComponents          = 1024;
    Resources.maxGeometryUniformComponents              = 1024;
    Resources.maxGeometryVaryingComponents              = 64;
    Resources.maxTessControlInputComponents             = 128;
    Resources.maxTessControlOutputComponents            = 128;
    Resources.maxTessControlTextureImageUnits           = 16;
    Resources.maxTessControlUniformComponents           = 1024;
    Resources.maxTessControlTotalOutputComponents       = 4096;
    Resources.maxTessEvaluationInputComponents          = 128;
    Resources.maxTessEvaluationOutputComponents         = 128;
    Resources.maxTessEvaluationTextureImageUnits        = 16;
    Resources.maxTessEvaluationUniformComponents        = 1024;
    Resources.maxTessPatchComponents                    = 120;
    Resources.maxPatchVertices                          = 32;
    Resources.maxTessGenLevel                           = 64;
    Resources.maxViewports                              = 16;
    Resources.maxVertexAtomicCounters                   = 0;
    Resources.maxTessControlAtomicCounters              = 0;
    Resources.maxTessEvaluationAtomicCounters           = 0;
    Resources.maxGeometryAtomicCounters                 = 0;
    Resources.maxFragmentAtomicCounters                 = 8;
    Resources.maxCombinedAtomicCounters                 = 8;
    Resources.maxAtomicCounterBindings                  = 1;
    Resources.maxVertexAtomicCounterBuffers             = 0;
    Resources.maxTessControlAtomicCounterBuffers        = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    Resources.maxGeometryAtomicCounterBuffers           = 0;
    Resources.maxFragmentAtomicCounterBuffers           = 1;
    Resources.maxCombinedAtomicCounterBuffers           = 1;
    Resources.maxAtomicCounterBufferSize                = 16384;
    Resources.maxTransformFeedbackBuffers               = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances                          = 8;
    Resources.maxCombinedClipAndCullDistances           = 8;
    Resources.maxSamples                                = 4;
    Resources.maxMeshOutputVerticesNV                   = 256;
    Resources.maxMeshOutputPrimitivesNV                 = 512;
    Resources.maxMeshWorkGroupSizeX_NV                  = 32;
    Resources.maxMeshWorkGroupSizeY_NV                  = 1;
    Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    Resources.maxTaskWorkGroupSizeX_NV                  = 32;
    Resources.maxTaskWorkGroupSizeY_NV                  = 1;
    Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    Resources.maxMeshViewCountNV                        = 4;
    Resources.maxMeshOutputVerticesEXT                  = 256;
    Resources.maxMeshOutputPrimitivesEXT                = 256;
    Resources.maxMeshWorkGroupSizeX_EXT                 = 128;
    Resources.maxMeshWorkGroupSizeY_EXT                 = 128;
    Resources.maxMeshWorkGroupSizeZ_EXT                 = 128;
    Resources.maxTaskWorkGroupSizeX_EXT                 = 128;
    Resources.maxTaskWorkGroupSizeY_EXT                 = 128;
    Resources.maxTaskWorkGroupSizeZ_EXT                 = 128;
    Resources.maxMeshViewCountEXT                       = 4;
    Resources.maxDualSourceDrawBuffersEXT               = 1;
    ASSERT_SIZEOF(Resources, 420, "Please initialize new members of Resources struct. Use glslang-default-resource-limits when this is tirggered.");

    Resources.limits.nonInductiveForLoops                 = true;
    Resources.limits.whileLoops                           = true;
    Resources.limits.doWhileLoops                         = true;
    Resources.limits.generalUniformIndexing               = true;
    Resources.limits.generalAttributeMatrixVectorIndexing = true;
    Resources.limits.generalVaryingIndexing               = true;
    Resources.limits.generalSamplerIndexing               = true;
    Resources.limits.generalVariableIndexing              = true;
    Resources.limits.generalConstantMatrixVectorIndexing  = true;
    ASSERT_SIZEOF(Resources.limits, 9, "Please initialize new members of Resources.limits struct. Use glslang-default-resource-limits when this is tirggered.");

    return Resources;
}

void LogCompilerError(const char* DebugOutputMessage,
                      const char* InfoLog,
                      const char* InfoDebugLog,
                      const char* ShaderSource,
                      size_t      SourceCodeLen,
                      IDataBlob** ppCompilerOutput)
{
    std::string ErrorLog(InfoLog);
    if (*InfoDebugLog != '\0')
    {
        ErrorLog.push_back('\n');
        ErrorLog.append(InfoDebugLog);
    }
    LOG_ERROR_MESSAGE(DebugOutputMessage, ErrorLog);

    if (ppCompilerOutput != nullptr)
    {
        auto  pOutputDataBlob = DataBlobImpl::Create(SourceCodeLen + 1 + ErrorLog.length() + 1);
        char* DataPtr         = reinterpret_cast<char*>(pOutputDataBlob->GetDataPtr());
        memcpy(DataPtr, ErrorLog.data(), ErrorLog.length() + 1);
        memcpy(DataPtr + ErrorLog.length() + 1, ShaderSource, SourceCodeLen + 1);
        pOutputDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppCompilerOutput));
    }
}

std::vector<unsigned int> CompileShaderInternal(::glslang::TShader&           Shader,
                                                EShMessages                   messages,
                                                ::glslang::TShader::Includer* pIncluder,
                                                const char*                   ShaderSource,
                                                size_t                        SourceCodeLen,
                                                bool                          AssignBindings,
                                                ::EProfile                    shProfile,
                                                IDataBlob**                   ppCompilerOutput)
{
    Shader.setAutoMapBindings(true);
    Shader.setAutoMapLocations(true);
    TBuiltInResource Resources = InitResources();

    auto ParseResult = pIncluder != nullptr ?
        Shader.parse(&Resources, 100, shProfile, false, false, messages, *pIncluder) :
        Shader.parse(&Resources, 100, shProfile, false, false, messages);
    if (!ParseResult)
    {
        LogCompilerError("Failed to parse shader source: \n", Shader.getInfoLog(), Shader.getInfoDebugLog(), ShaderSource, SourceCodeLen, ppCompilerOutput);
        return {};
    }

    ::glslang::TProgram Program;
    Program.addShader(&Shader);
    if (!Program.link(messages))
    {
        LogCompilerError("Failed to link program: \n", Program.getInfoLog(), Program.getInfoDebugLog(), ShaderSource, SourceCodeLen, ppCompilerOutput);
        return {};
    }

    // This step is essential to set bindings and descriptor sets
    if (AssignBindings)
        Program.mapIO();

    std::vector<unsigned int> spirv;
    ::glslang::GlslangToSpv(*Program.getIntermediate(Shader.getStage()), spirv);

    return spirv;
}


class IncluderImpl : public ::glslang::TShader::Includer
{
public:
    IncluderImpl(IShaderSourceInputStreamFactory* pInputStreamFactory) :
        m_pInputStreamFactory(pInputStreamFactory)
    {}

    // For the "system" or <>-style includes; search the "system" paths.
    virtual IncludeResult* includeSystem(const char* headerName,
                                         const char* /*includerName*/,
                                         size_t /*inclusionDepth*/)
    {
        DEV_CHECK_ERR(m_pInputStreamFactory != nullptr, "The shader source contains #include directives, but no input stream factory was provided");
        RefCntAutoPtr<IFileStream> pSourceStream;
        m_pInputStreamFactory->CreateInputStream(headerName, &pSourceStream);
        if (pSourceStream == nullptr)
        {
            LOG_ERROR("Failed to open shader include file '", headerName, "'. Check that the file exists");
            return nullptr;
        }

        auto pFileData = DataBlobImpl::Create();
        pSourceStream->ReadBlob(pFileData);
        auto* pNewInclude =
            new IncludeResult{
                headerName,
                reinterpret_cast<const char*>(pFileData->GetDataPtr()),
                pFileData->GetSize(),
                nullptr};

        m_IncludeRes.emplace(pNewInclude);
        m_DataBlobs.emplace(pNewInclude, std::move(pFileData));
        return pNewInclude;
    }

    // For the "local"-only aspect of a "" include. Should not search in the
    // "system" paths, because on returning a failure, the parser will
    // call includeSystem() to look in the "system" locations.
    virtual IncludeResult* includeLocal(const char* headerName,
                                        const char* includerName,
                                        size_t      inclusionDepth)
    {
        return nullptr;
    }

    // Signals that the parser will no longer use the contents of the
    // specified IncludeResult.
    virtual void releaseInclude(IncludeResult* IncldRes)
    {
        m_DataBlobs.erase(IncldRes);
    }

private:
    IShaderSourceInputStreamFactory* const                       m_pInputStreamFactory;
    std::unordered_set<std::unique_ptr<IncludeResult>>           m_IncludeRes;
    std::unordered_map<IncludeResult*, RefCntAutoPtr<IDataBlob>> m_DataBlobs;
};

void SetupWithSpirvVersion(::glslang::TShader&  Shader,
                           ::EProfile&          shProfile,
                           EShLanguage          ShLang,
                           SpirvVersion         Version,
                           ::glslang::EShSource ShSource)
{
    static_assert(static_cast<int>(SpirvVersion::Count) == 6, "Did you add a new member to SpirvVersion? You may need to handle it here.");

    shProfile = EProfile::ENoProfile;
    switch (Version)
    {
        case SpirvVersion::Vk100:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientVulkan, 100);
            Shader.setEnvClient(::glslang::EShClientVulkan, ::glslang::EShTargetVulkan_1_0);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_0);
            break;
        case SpirvVersion::Vk110:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientVulkan, 110);
            Shader.setEnvClient(::glslang::EShClientVulkan, ::glslang::EShTargetVulkan_1_1);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_3);
            break;
        case SpirvVersion::Vk110_Spirv14:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientVulkan, 110);
            Shader.setEnvClient(::glslang::EShClientVulkan, ::glslang::EShTargetVulkan_1_1);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_4);
            break;
        case SpirvVersion::Vk120:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientVulkan, 120);
            Shader.setEnvClient(::glslang::EShClientVulkan, ::glslang::EShTargetVulkan_1_2);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_5);
            break;

        case SpirvVersion::GL:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientOpenGL, 450);
            Shader.setEnvClient(::glslang::EShClientOpenGL, ::glslang::EShTargetOpenGL_450);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_0);
            shProfile = EProfile::ECoreProfile;
            break;
        case SpirvVersion::GLES:
            Shader.setEnvInput(ShSource, ShLang, ::glslang::EShClientOpenGL, 450);
            Shader.setEnvClient(::glslang::EShClientOpenGL, ::glslang::EShTargetOpenGL_450);
            Shader.setEnvTarget(::glslang::EShTargetSpv, ::glslang::EShTargetSpv_1_0);
            shProfile = EProfile::EEsProfile;
            break;

        default:
            UNEXPECTED("Unknown SPIRV version");
    }
}

#ifdef USE_SPIRV_TOOLS
spv_target_env SpirvVersionToSpvTargetEnv(SpirvVersion Version)
{
    static_assert(static_cast<int>(SpirvVersion::Count) == 6, "Did you add a new member to SpirvVersion? You may need to handle it here.");
    switch (Version)
    {
        // clang-format off
        case SpirvVersion::Vk100:         return SPV_ENV_VULKAN_1_0;
        case SpirvVersion::Vk110:         return SPV_ENV_VULKAN_1_1;
        case SpirvVersion::Vk110_Spirv14: return SPV_ENV_VULKAN_1_1_SPIRV_1_4;
        case SpirvVersion::Vk120:         return SPV_ENV_VULKAN_1_2;
        case SpirvVersion::GL:            return SPV_ENV_OPENGL_4_5;
        case SpirvVersion::GLES:          return SPV_ENV_OPENGL_4_5;
        // clang-format on
        default:
            UNEXPECTED("Unknown SPIRV version");
            return SPV_ENV_VULKAN_1_0;
    }
}
#endif

} // namespace

std::vector<unsigned int> HLSLtoSPIRV(const ShaderCreateInfo& ShaderCI,
                                      SpirvVersion            Version,
                                      const char*             ExtraDefinitions,
                                      IDataBlob**             ppCompilerOutput)
{
    EShLanguage        ShLang = ShaderTypeToShLanguage(ShaderCI.Desc.ShaderType);
    ::glslang::TShader Shader{ShLang};
    EShMessages        messages  = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslLegalization);
    ::EProfile         shProfile = EProfile::ENoProfile;

    SetupWithSpirvVersion(Shader, shProfile, ShLang, Version, ::glslang::EShSourceHlsl);

    VERIFY_EXPR(ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL);

    VERIFY(ShLang != EShLangRayGen && ShLang != EShLangIntersect && ShLang != EShLangAnyHit && ShLang != EShLangClosestHit && ShLang != EShLangMiss && ShLang != EShLangCallable,
           "Ray tracing shaders are not supported, use DXCompiler to build SPIRV from HLSL");
    VERIFY(ShLang != EShLangTaskNV && ShLang != EShLangMeshNV,
           "Mesh shaders are not supported, use DXCompiler to build SPIRV from HLSL");

    Shader.setHlslIoMapping(true);
    Shader.setEntryPoint(ShaderCI.EntryPoint);
    Shader.setEnvTargetHlslFunctionality1();

    const auto SourceData = ReadShaderSourceFile(ShaderCI);

    std::string Preamble;
    if ((ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR) != 0)
        Preamble += "#pragma pack_matrix(row_major)\n\n";
    Preamble.append("#define GLSLANG\n\n");
    Preamble.append(g_HLSLDefinitions);
    AppendShaderTypeDefinitions(Preamble, ShaderCI.Desc.ShaderType);

    if (ExtraDefinitions != nullptr)
        Preamble += ExtraDefinitions;

    if (ShaderCI.Macros)
    {
        Preamble += '\n';
        AppendShaderMacros(Preamble, ShaderCI.Macros);
    }

    Shader.setPreamble(Preamble.c_str());

    const char* ShaderStrings[]       = {SourceData.Source};
    const int   ShaderStringLengths[] = {static_cast<int>(SourceData.SourceLength)};
    const char* Names[]               = {ShaderCI.FilePath != nullptr ? ShaderCI.FilePath : ""};
    Shader.setStringsWithLengthsAndNames(ShaderStrings, ShaderStringLengths, Names, 1);

    // By default, PSInput.SV_Position.w == 1 / VSOutput.SV_Position.w.
    // Make the behavior consistent with DX:
    Shader.setDxPositionW(true);

    IncluderImpl Includer{ShaderCI.pShaderSourceStreamFactory};

    auto SPIRV = CompileShaderInternal(Shader, messages, &Includer, SourceData.Source, SourceData.SourceLength, true, shProfile, ppCompilerOutput);
    if (SPIRV.empty())
        return SPIRV;

#ifdef USE_SPIRV_TOOLS
    // SPIR-V bytecode generated from HLSL must be legalized to
    // turn it into a valid vulkan SPIR-V shader.
    auto LegalizedSPIRV = OptimizeSPIRV(SPIRV, SpirvVersionToSpvTargetEnv(Version), SPIRV_OPTIMIZATION_FLAG_LEGALIZATION | SPIRV_OPTIMIZATION_FLAG_PERFORMANCE);
    if (!LegalizedSPIRV.empty())
    {
        return LegalizedSPIRV;
    }
    else
    {
        LOG_ERROR("Failed to legalize SPIR-V shader generated by HLSL front-end. This may result in undefined behavior.");
    }
#endif
    return SPIRV;
}

std::vector<unsigned int> GLSLtoSPIRV(const GLSLtoSPIRVAttribs& Attribs)
{
    VERIFY_EXPR(Attribs.ShaderSource != nullptr && Attribs.SourceCodeLen > 0);

    const EShLanguage  ShLang = ShaderTypeToShLanguage(Attribs.ShaderType);
    ::glslang::TShader Shader(ShLang);
    ::EProfile         shProfile = EProfile::ENoProfile;

    SetupWithSpirvVersion(Shader, shProfile, ShLang, Attribs.Version, ::glslang::EShSourceGlsl);

    EShMessages messages = EShMsgSpvRules;
    static_assert(static_cast<int>(SpirvVersion::Count) == 6, "Did you add a new member to SpirvVersion? You may need to handle it here.");
    if (Attribs.Version != SpirvVersion::GL && Attribs.Version != SpirvVersion::GLES)
        messages = static_cast<EShMessages>(messages | EShMsgVulkanRules);

    const char* ShaderStrings[] = {Attribs.ShaderSource};
    int         Lengths[]       = {Attribs.SourceCodeLen};
    Shader.setStringsWithLengths(ShaderStrings, Lengths, 1);

    std::string Preamble;
    if (Attribs.UseRowMajorMatrices)
        Preamble += "layout(row_major) uniform;\n\n";
    Preamble.append("#define GLSLANG\n\n");
    if (Attribs.Macros)
        AppendShaderMacros(Preamble, Attribs.Macros);
    Shader.setPreamble(Preamble.c_str());

    IncluderImpl Includer{Attribs.pShaderSourceStreamFactory};

    auto SPIRV = CompileShaderInternal(Shader, messages, &Includer, Attribs.ShaderSource, Attribs.SourceCodeLen, Attribs.AssignBindings, shProfile, Attribs.ppCompilerOutput);
    if (SPIRV.empty())
        return SPIRV;

#ifdef USE_SPIRV_TOOLS
    auto OptimizedSPIRV = OptimizeSPIRV(SPIRV, SpirvVersionToSpvTargetEnv(Attribs.Version), SPIRV_OPTIMIZATION_FLAG_PERFORMANCE);
    if (!OptimizedSPIRV.empty())
    {
        return OptimizedSPIRV;
    }
    else
    {
        LOG_ERROR("Failed to optimize SPIR-V.");
    }
#endif
    return SPIRV;
}

} // namespace GLSLangUtils

} // namespace Diligent
