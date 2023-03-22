#include "./ShaderCompiler.h"
#include "../IO/Log.h"
#include "./Graphics.h"
#include "./ShaderConverter.h"

#ifdef WIN32
#include <d3dcompiler.h>
#include <comdef.h>
#endif
#include <SPIRV-Reflect/spirv_reflect.h>

#ifdef URHO3D_DILIGENT
#include "../Graphics/Diligent/DiligentLookupSettings.h"
#endif

namespace Urho3D
{
#ifdef URHO3D_SPIRV
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
    static const char* cbufferSuffixes[] = {
        "VS", "PS", "GS", "HS", "DS", "CS"
    };
    static const char* samplerNames[] = {
        "DiffMap",
        "DiffCubeMap",
        "NormalMap",
        "SpecMap",
        "EmissiveMap",
        "EnvMap",
        "EnvCubeMap",
        "LightRampMap",
        "LightSpotMap",
        "LightCubeMap",
        "ShadowMap",
        "VolumeMap",
        "DepthBuffer",
        "ZoneCubeMap",
        "ZoneVolumeMap",
        nullptr
    };

    static void sanitizeCBName(ea::string& cbName) {
        for (unsigned i = 0; i < MAX_SHADER_TYPES; ++i)
            cbName.replace(cbufferSuffixes[i], "");
    }
#endif
    ShaderCompiler::ShaderCompiler(ShaderCompilerDesc desc) : desc_(desc)
    {
        textureSlots_.fill(false);
        constantBufferSlots_.fill(false);
    }
    bool ShaderCompiler::Compile()
    {
        textureSlots_.fill(false);
        constantBufferSlots_.fill(false);
        vertexElements_.clear();
        parameters_.clear();
        byteCode_.clear();
        compilerOutput_.clear();
#ifdef URHO3D_DILIGENT
        inputLayoutMapping_.clear();
#endif
        if (desc_.language_ == ShaderLanguage::HLSL) {
#ifdef WIN32
            return CompileHLSL();
#else
            URHO3D_LOGERROR("Not supported HLSL Compilation.");
            return false;
#endif
        }
        else {
#ifdef URHO3D_SPIRV
            return CompileGLSL();
#else
            URHO3D_LOGERROR("URHO3D_SPIRV macro is disabled! You must compile Urho3D lib with spirv macro enabled.");
            return false;
#endif
        }
    }
#ifdef WIN32
    bool ShaderCompiler::CompileHLSL()
    {
        ea::string sourceCode = desc_.code_;
        ea::string cbufferSuffix = "";
        const char* profile = nullptr;
        unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

#ifdef URHO3D_DEBUG
        flags |= D3DCOMPILE_DEBUG;
#endif

        switch (desc_.type_)
        {
        case VS:
            profile = "vs_4_0";
            cbufferSuffix = "VS";
            break;
        case PS:
        {
            cbufferSuffix = "PS";
            profile = "ps_5_0";
            flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
        }
            break;
        case GS:
            cbufferSuffix = "GS";
            profile = "gs_5_0";
            break;
        case HS:
            cbufferSuffix = "HS";
            profile = "hs_5_0";
            break;
        case DS:
            cbufferSuffix = "DS";
            profile = "ds_5_0";
            break;
        case CS:
            cbufferSuffix = "CS";
            profile = "cs_5_0";
            break;
        }

        ea::vector<D3D_SHADER_MACRO> macros;
        macros.reserve(desc_.defines_.Size());

        for (auto pair = desc_.defines_.defines_.begin(); pair != desc_.defines_.defines_.end(); ++pair)
            macros.push_back({pair->first.c_str(), pair->second.c_str()});
        D3D_SHADER_MACRO endMacro;
        endMacro.Name = nullptr;
        endMacro.Definition = nullptr;
        macros.emplace_back(endMacro);

        ea::vector<ID3DBlob*> destroyableBlobs;
        // Compile using D3DCompile
        ID3DBlob* shaderCode = nullptr;
        ID3DBlob* errorMsgs = nullptr;
#define RELEASE_BLOBS() \
        for(uint8_t i = 0; i < destroyableBlobs.size(); ++i) { \
            if(destroyableBlobs[i]) destroyableBlobs[i]->Release(); \
        }\
        destroyableBlobs.clear();

        {
            ID3DBlob* processedCode = nullptr;
            HRESULT hr = D3DPreprocess(
                sourceCode.data(),
                sourceCode.length(),
                desc_.name_.c_str(),
                &macros.front(),
                nullptr,
                &processedCode,
                &errorMsgs
            );

            if (processedCode) {
                destroyableBlobs.push_back(processedCode);
                sourceCode = ea::string((const char*)processedCode->GetBufferPointer(), (const char*)processedCode->GetBufferPointer() + processedCode->GetBufferSize());
            }
            if (errorMsgs) {
                destroyableBlobs.push_back(errorMsgs);
                compilerOutput_.append((const char*)errorMsgs->GetBufferPointer());
            }

            RELEASE_BLOBS();
            if (FAILED(hr)) {
                URHO3D_LOGERROR("Failed to preprocess shader code " + desc_.name_);
                return false;
            }
        }

        // Remove suffixes from constant buffers
        {
            size_t i = 0;
            do {
                sourceCode.replace(Format("{}{}", shaderParameterGroupNames[i], cbufferSuffix), shaderParameterGroupNames[i]);
            } while (shaderParameterGroupNames[++i] != nullptr);
        }

        HRESULT hr = D3DCompile(
            sourceCode.data(),
            sourceCode.length(),
            desc_.name_.c_str(),
            nullptr,
            nullptr,
            desc_.entryPoint_.c_str(),
            profile,
            flags,
            0,
            &shaderCode,
            &errorMsgs
        );

        destroyableBlobs.push_back(shaderCode);
        destroyableBlobs.push_back(errorMsgs);

        if (errorMsgs) {
            if (!compilerOutput_.empty())
                compilerOutput_.push_back('\n');
            compilerOutput_.append((const char*)errorMsgs->GetBufferPointer());
        }

        if (FAILED(hr)) {
            RELEASE_BLOBS();
            URHO3D_LOGERROR("Failed to compile " + desc_.name_);
            return false;
        }

        uint8_t* byteCode = (uint8_t*)shaderCode->GetBufferPointer();
        size_t byteCodeSize = shaderCode->GetBufferSize();

        switch (desc_.type_) {
        case VS:
            URHO3D_LOGDEBUG("Compiled vertex shader " + desc_.name_);
            break;
        case PS:
            URHO3D_LOGDEBUG("Compiled pixel shader " + desc_.name_);
            break;
        case GS:
            URHO3D_LOGDEBUG("Compiled geometry shader " + desc_.name_);
            break;
        case HS:
            URHO3D_LOGDEBUG("Compiled hull shader " + desc_.name_);
            break;
        case DS:
            URHO3D_LOGDEBUG("Compiled domain shader " + desc_.name_);
            break;
        case CS:
            URHO3D_LOGDEBUG("Compiled compute shader " + desc_.name_);
            break;
        }


        if (!ReflectHLSL(byteCode, byteCodeSize)) {
            RELEASE_BLOBS();
            return false;
        }

#ifdef URHO3D_DILIGENT
        RemapInputLayout(sourceCode);
        {
            RELEASE_BLOBS();
            // Compile again to get final bytecode
            HRESULT hr = D3DCompile(
                sourceCode.data(),
                sourceCode.length(),
                desc_.name_.c_str(),
                nullptr,
                nullptr,
                desc_.entryPoint_.c_str(),
                profile,
                flags,
                0,
                &shaderCode,
                nullptr
            );

            destroyableBlobs.push_back(shaderCode);

            if (FAILED(hr)) {
                RELEASE_BLOBS();
                URHO3D_LOGERROR("Failed to compile " + desc_.name_);
                return false;
            }

            byteCode = static_cast<uint8_t*>(shaderCode->GetBufferPointer());
            byteCodeSize = shaderCode->GetBufferSize();
        }
#endif
//#ifndef URHO3D_DEBUG
//        ID3DBlob* strippedCode = nullptr;
//        D3DStripShader(
//            shaderCode->GetBufferPointer(),
//            shaderCode->GetBufferSize(),
//            D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS,
//            &strippedCode
//        );
//        byteCode = (uint8_t*)strippedCode->GetBufferPointer();
//        byteCodeSize = strippedCode->GetBufferSize();
//        destroyableBlobs.push_back(strippedCode);
//#endif

        byteCode_.resize(byteCodeSize);
        memcpy_s(byteCode_.data(), byteCodeSize, byteCode, byteCodeSize);

        RELEASE_BLOBS();
        return true;
    }
    bool ShaderCompiler::ReflectHLSL(unsigned char* byteCode, size_t byteCodeSize)
    {
        ID3D11ShaderReflection* reflection = nullptr;
        D3D11_SHADER_DESC shaderDesc;

        HRESULT hr = D3DReflect(byteCode, byteCodeSize, IID_ID3D11ShaderReflection, (void**)&reflection);
        if (FAILED(hr) || !reflection) {
            if (reflection)
                reflection->Release();
            URHO3D_LOGERROR("Failed to reflect shader.");
            return false;
        }

        reflection->GetDesc(&shaderDesc);


        // Extract Input Layout
        if (desc_.type_ == VS) {
            auto GetElementType = [](D3D11_SIGNATURE_PARAMETER_DESC& paramDesc) {
                VertexElementType type = MAX_VERTEX_ELEMENT_TYPES;
                uint8_t componentCount = 1;
                for (uint8_t i = 0; i < 4; ++i) {
                    if (paramDesc.Mask & (1 << i))
                        componentCount++;
                }

                switch (paramDesc.ComponentType)
                {
                case D3D_REGISTER_COMPONENT_UINT32:
                {
                    if (componentCount == 4)
                        type = TYPE_UBYTE4;
                }
                    break;
                case D3D_REGISTER_COMPONENT_SINT32:
                {
                    if (componentCount == 1)
                        type = TYPE_INT;
                }
                    break;
                case D3D_REGISTER_COMPONENT_FLOAT32: {
                    if (componentCount == 1)
                        type = TYPE_FLOAT;
                    else if (componentCount == 2)
                        type = TYPE_VECTOR2;
                    else if (componentCount == 3)
                        type = TYPE_VECTOR3;
                    else if (componentCount == 4)
                        type = TYPE_VECTOR4;
                }
                    break;
                }
                return type;
            };

            for (uint8_t i = 0; i < shaderDesc.InputParameters; ++i) {
                D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
                reflection->GetInputParameterDesc((UINT)i, &paramDesc);
                VertexElementSemantic semantic = (VertexElementSemantic)GetStringListIndex(paramDesc.SemanticName, elementSemanticNames, MAX_VERTEX_ELEMENT_SEMANTICS, true);
                if (semantic != MAX_VERTEX_ELEMENT_SEMANTICS) {
                    vertexElements_.push_back(
                        VertexElement(
                            GetElementType(paramDesc),
                            semantic,
                            paramDesc.SemanticIndex
                        )
                    );
#ifdef URHO3D_DILIGENT
                    inputLayoutMapping_.push_back(ea::make_pair(paramDesc.SemanticIndex, semantic));
#endif
                }
            }
        }

        // Extract Constant Buffer Bindings and Textures
        ea::unordered_map<ea::string, unsigned> cbRegisterMap;
        for (unsigned i = 0; i < shaderDesc.BoundResources; ++i) {
            D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
            reflection->GetResourceBindingDesc(i, &resourceDesc);
            ea::string resourceName(resourceDesc.Name);
            if (resourceDesc.Type == D3D_SIT_CBUFFER) {
                auto bufferLookupValue = constantBuffersNamesLookup.find(resourceName);
                if (bufferLookupValue != constantBuffersNamesLookup.end()) {
                    cbRegisterMap[resourceName] = resourceDesc.BindPoint;
                    constantBufferSlots_[bufferLookupValue->second] = true;
                }
                else {
                    URHO3D_LOGWARNING("Invalid Resource Name for "+resourceName);
                    continue;
                }
            }
            else if (resourceDesc.Type == D3D_SIT_SAMPLER && resourceDesc.BindPoint < MAX_TEXTURE_UNITS)
                textureSlots_[resourceDesc.BindPoint] = true;
        }

        for (unsigned i = 0; i < shaderDesc.ConstantBuffers; ++i) {
            ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);
            unsigned cbRegister = cbRegisterMap[ea::string(cbDesc.Name)];

            for (unsigned j = 0; j < cbDesc.Variables; ++j) {
                ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
                D3D11_SHADER_VARIABLE_DESC varDesc;
                var->GetDesc(&varDesc);
                ea::string varName(varDesc.Name);
                const auto nameStart = varName.find('c');
                if (nameStart != ea::string::npos) {
                    varName = varName.substr(nameStart + 1);
                    parameters_[varName] = ShaderParameter{ desc_.type_, varName, varDesc.StartOffset, varDesc.Size, cbRegister };
                }
            }
        }

        reflection->Release();
        return true;
    }
#endif
#ifdef URHO3D_SPIRV
    bool ShaderCompiler::CompileGLSL()
    {
        ea::string sourceCode = desc_.code_;
        ea::vector<unsigned> byteCode;
        if (!CompileGLSLToSpirV(
            desc_.type_,
            desc_.code_,
            desc_.defines_,
            byteCode,
            compilerOutput_
        )) {
            URHO3D_LOGERROR("Failed to compile " + desc_.name_);
            return false;
        }
        size_t byteCodeSize = sizeof(unsigned) * byteCode.size();
        if (!ReflectGLSL(byteCode.data(), byteCodeSize))
            return false;

#ifdef URHO3D_DILIGENT
        // On Diligent Mode, we must convert GLSL to HLSL for better compatibility
        // Of course we can use GLSL only for OpenGL but this won't work if we
        // choose another backend like D3D, Metal(MoltenVK) or Vulkan

        ConvertShaderToHLSL5(byteCode, sourceCode, compilerOutput_);
        RemapSamplers(sourceCode);
        ApplyFixes(sourceCode);
#endif
        // On GLSL, bytecode is HLSL or GLSL code
        byteCode_.resize(sourceCode.length() + 1);
        byteCodeSize = sourceCode.length();
        memcpy_s(byteCode_.data(), byteCodeSize, sourceCode.data(), byteCodeSize);
        return true;
    }
    bool ShaderCompiler::ReflectGLSL(const void* byteCode, size_t byteCodeSize)
    {
        SpvReflectShaderModule module = {};
        SpvReflectResult result = spvReflectCreateShaderModule(byteCodeSize, byteCode, &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            URHO3D_LOGERROR("Failed to reflect SPIR-V code for "+desc_.name_);
            return false;
        }

        if (desc_.type_ == VS) {
            unsigned varCount = 0;
            result = spvReflectEnumerateInputVariables(&module, &varCount, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            ea::vector<SpvReflectInterfaceVariable*> inputVars(varCount);
            result = spvReflectEnumerateInputVariables(&module, &varCount, inputVars.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            auto GetElementType = [](SpvReflectInterfaceVariable* variable) {
                VertexElementType type = MAX_VERTEX_ELEMENT_TYPES;
                switch (variable->format)
                {
                case SPV_REFLECT_FORMAT_R32_UINT:
                case SPV_REFLECT_FORMAT_R32_SINT:
                    type = TYPE_INT;
                    break;
                case SPV_REFLECT_FORMAT_R32_SFLOAT:
                    type = TYPE_FLOAT;
                    break;
                case SPV_REFLECT_FORMAT_R32G32_UINT:
                case SPV_REFLECT_FORMAT_R32G32_SINT:
                case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
                    type = TYPE_VECTOR2;
                    break;
                case SPV_REFLECT_FORMAT_R32G32B32_UINT:
                case SPV_REFLECT_FORMAT_R32G32B32_SINT:
                case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
                    type = TYPE_VECTOR3;
                    break;
                case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
                    type = TYPE_VECTOR4;
                    break;
                case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
                case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
                case SPV_REFLECT_FORMAT_R64_UINT:
                case SPV_REFLECT_FORMAT_R64_SINT:
                    type = TYPE_UBYTE4;
                    break;
                }

                return type;
            };
            for (SpvReflectInterfaceVariable** it = inputVars.begin(); it != inputVars.end(); ++it) {
                // Skip gl_VertexId and gl_InstanceID
                if ((*it)->built_in != -1)
                    continue;
                ea::string inputName((*it)->name == nullptr ? "" : (*it)->name);
                ea::string inputNameWithoutIdx = inputName;

                unsigned inputNameEndIdx = inputName.length() - 1;
                while (isdigit(inputName.at(inputNameEndIdx)))
                    --inputNameEndIdx;
                inputNameWithoutIdx = inputName.substr(0, inputNameEndIdx + 1);

                unsigned slotIdx = inputNameEndIdx == inputName.length() - 1 ? 0 : ToUInt(inputName.substr(inputNameWithoutIdx.length(), inputName.length() - 1));
                auto semanticIt = semanticsMapping.find(inputNameWithoutIdx);
                if (semanticIt == semanticsMapping.end()) {
                    URHO3D_LOGWARNING("Invalid semantic \"{}\" name for {} shader.", inputNameWithoutIdx, desc_.name_);
                    continue;
                }
                VertexElementSemantic semantic = semanticIt->second;

                vertexElements_.push_back(
                    VertexElement(
                        GetElementType(*it),
                        semantic,
                        slotIdx
                    )
                );

            }
        }

        unsigned bindingCount = 0;
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        ea::vector<SpvReflectDescriptorBinding*> descriptorBindings(bindingCount);
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, descriptorBindings.data());

        ea::unordered_map<ea::string, unsigned> cbRegisterMap;
        for (auto descBindingIt = descriptorBindings.begin(); descBindingIt != descriptorBindings.end(); descBindingIt++) {
            SpvReflectDescriptorBinding* binding = *descBindingIt;

            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                assert(binding->type_description->type_name != nullptr);
                ea::string bindingName(binding->type_description->type_name);
                sanitizeCBName(bindingName);

                auto cBufferLookupValue = constantBuffersNamesLookup.find(bindingName);
                if (cBufferLookupValue == constantBuffersNamesLookup.end()) {
                    URHO3D_LOGERRORF("Failed to reflect shader constant buffer for %s shader. Invalid constant buffer name: %s", desc_.name_, bindingName);
                    spvReflectDestroyShaderModule(&module);
                    return false;
                }

                constantBufferSlots_[cBufferLookupValue->second] = true;

                unsigned membersCount = binding->block.member_count;
                // Extract CBuffer variable parameters and build shader parameters
                while (membersCount--) {
                    const SpvReflectBlockVariable& variable = binding->block.members[membersCount];
                    ea::string varName(variable.name);
                    const auto nameStart = varName.find('c');
                    if (nameStart != ea::string::npos)
                        varName = varName.substr(nameStart + 1);
                    parameters_[varName] = ShaderParameter { desc_.type_ ,varName, variable.offset, variable.size, (unsigned)cBufferLookupValue->second };
                }
            }
            else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                assert(binding->name);
                ea::string name(binding->name);
                if (name.at(0) == 's')
                    name = name.substr(1, name.size());
                auto texUnitLookupVal = DiligentTextureUnitLookup.find(name);
                if (texUnitLookupVal == DiligentTextureUnitLookup.end()) {
                    URHO3D_LOGERRORF("Failed to reflect shader texture samplers. Invalid texture sampler name: %s", name);
                    spvReflectDestroyShaderModule(&module);
                    return false;
                }
                textureSlots_[texUnitLookupVal->second] = true;
            }
        }

        return true;
    }
    void ShaderCompiler::RemapSamplers(ea::string& sourceCode)
    {
        // HLSL Conversion change samplers to _sTexMap_sampler and sTexMap
        // We remapping theses names to sTexMap (SamplerState) and tTexMap (Texture Resource)

        unsigned index = 0;
        do {
            ea::string targetTexName = ea::string("> s") + samplerNames[index];
            ea::string targetSamplerName = ea::string("_s") + samplerNames[index];
            ea::string targetSampleRead = Format("s{0}.Sample(s{0}", samplerNames[index]);
            ea::string targetSampleCmpRead = Format("s{0}.SampleCmp(s{0}", samplerNames[index]);
            targetSamplerName.append("_sampler");

            ea::string outputTexName = ea::string("> t") + samplerNames[index];
            ea::string outputSamplerName = ea::string("s") + samplerNames[index];
            ea::string outputSampleRead = Format("t{0}.Sample(s{0}", samplerNames[index]);
            ea::string outputSampleCmpRead = Format("t{0}.SampleCmp(s{0}", samplerNames[index]);

            sourceCode.replace(targetTexName, outputTexName);
            sourceCode.replace(targetSamplerName, outputSamplerName);
            sourceCode.replace(targetSampleRead, outputSampleRead);
            sourceCode.replace(targetSampleCmpRead, outputSampleCmpRead);
        } while (samplerNames[++index] != nullptr);
    }
    void ShaderCompiler::ApplyFixes(ea::string& sourceCode)
    {
        if (desc_.type_ != PS)
            return;
        size_t inputIdx = sourceCode.find("struct SPIRV_Cross_Input");
        if (inputIdx == ea::string::npos)
            return;
        inputIdx = sourceCode.find("}", inputIdx);
        if (inputIdx == ea::string::npos)
            return;
        sourceCode.insert(inputIdx, "    float4 gl_Position : SV_Position;\n");
    }
#endif
#ifdef URHO3D_DILIGENT
    void ShaderCompiler::RemapInputLayout(ea::string& sourceCode)
    {
        unsigned attribCount = 0;
        for (ea::pair<unsigned, VertexElementSemantic>* it = inputLayoutMapping_.begin(); it != inputLayoutMapping_.end(); ++it) {
            ea::string newValue = "ATTRIB";
            ea::string targetValue = elementSemanticNames[it->second];

            newValue.append_sprintf("%d", attribCount++);
            targetValue.append_sprintf("%d", it->first);

            size_t replaceStartIdx = ea::string::npos;
            replaceStartIdx = sourceCode.find(targetValue);
            if (it->first == 0 && replaceStartIdx == ea::string::npos) {
                replaceStartIdx = sourceCode.find(elementSemanticNames[it->second]);
                if (replaceStartIdx != ea::string::npos)
                    targetValue = elementSemanticNames[it->second];
            }

            if (replaceStartIdx != ea::string::npos)
                sourceCode.replace(replaceStartIdx, targetValue.length(), newValue);
            else // This case must never happens
                assert(0);
        }
    }
#endif
}
