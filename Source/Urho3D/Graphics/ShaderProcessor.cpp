#include "./ShaderProcessor.h"
#include "../Graphics/Diligent/DiligentLookupSettings.h"
#include "../Graphics/ShaderConverter.h"
#include <Diligent/Graphics/HLSL2GLSLConverterLib/include/HLSL2GLSLConverterImpl.hpp>
#include <Diligent/Graphics/ShaderTools/include/SPIRVTools.hpp>
#include <Diligent/Graphics/ShaderTools/include/GLSLangUtils.hpp>
#include <Diligent/Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "../Graphics/ShaderMacroExpander.h"
#ifdef WIN32
#include <d3dcompiler.h>
#endif
namespace Urho3D
{
    using namespace Diligent;
    static const ea::unordered_map<ea::string, VertexElementSemantic> sSemanticsMapping = {
        { "iPos", SEM_POSITION },
        { "iNormal", SEM_NORMAL },
        { "iColor", SEM_COLOR },
        { "iTexCoord", SEM_TEXCOORD },
        { "iTangent", SEM_TANGENT },
        { "iBlendWeights", SEM_BLENDWEIGHTS },
        { "iBlendIndices", SEM_BLENDINDICES },
        { "iObjectIndex", SEM_OBJECTINDEX }
    };
    static const char* sCBufferSuffixes[] = {
        "VS", "PS", "GS", "HS", "DS", "CS"
    };
    static const char* sSamplerNames[] = {
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
    static void sSanitizeCBName(ea::string& cbName) {
        for (unsigned i = 0; i < MAX_SHADER_TYPES; ++i)
            cbName.replace(sCBufferSuffixes[i], "");
    }

    ShaderProcessor::ShaderProcessor(ShaderProcessorDesc desc) :
        desc_(desc) {
        textureSlots_.fill(false);
        cbufferSlots_.fill(false);
    }
    bool ShaderProcessor::Execute()
    {
        textureSlots_.fill(false);
        cbufferSlots_.fill(false);
        compilerOutput_.clear();
        outputCode_.clear();
        vertexElements_.clear();
        parameters_.clear();

        if (desc_.language_ == ShaderLang::HLSL)
            return ProcessHLSL();
        else
            return ProcessGLSL();
        return false;
    }
    bool ShaderProcessor::ProcessHLSL()
    {
#ifdef WIN32
        ea::string sourceCode = desc_.sourceCode_;
        ea::string cbufferSuffix = "";
        const char* profile = nullptr;
        unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
        switch (desc_.type_)
        {
        case VS:
            profile = "vs_4_0";
            cbufferSuffix = "VS";
            break;
        case PS:
            profile = "ps_5_0";
            cbufferSuffix = "PS";
            flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
            break;
        case GS:
            profile = "gs_5_0";
            cbufferSuffix = "GS";
            break;
        case HS:
            cbufferSuffix = "HS";
            profile = "hs_5_0";
            break;
        case DS:
            cbufferSuffix = "HS";
            profile = "ds_5_0";
            break;
        case CS:
            cbufferSuffix = "CS";
            profile = "cs_5_0";
            break;
        default:
            URHO3D_LOGERROR("Unsupported ShaderType {}", desc_.type_);
            return false;
        }

        ea::vector<D3D_SHADER_MACRO> macros;
        macros.reserve(desc_.macros_.Size());
        for (auto pair = desc_.macros_.defines_.begin(); pair != desc_.macros_.defines_.end(); ++pair)
            macros.push_back({pair->first.c_str(), pair->second.c_str()});

        D3D_SHADER_MACRO endMacro;
        endMacro.Name = nullptr;
        endMacro.Definition = nullptr;
        macros.emplace_back(endMacro);

        ea::vector<ID3DBlob*> destroyableBlobs;
        #define RELEASE_BLOBS() \
        for(uint8_t i = 0; i < destroyableBlobs.size(); ++i) { \
            if(destroyableBlobs[i]) { \
                destroyableBlobs[i]->Release(); \
                destroyableBlobs[i] = nullptr; \
            }\
        }\
        destroyableBlobs.clear();

        ID3DBlob* processedCode = nullptr;
        ID3DBlob* errorMsg = nullptr;
        HRESULT hr = D3DPreprocess(
            sourceCode.data(),
            sourceCode.length(),
            desc_.name_.c_str(),
            &macros.front(),
            nullptr,
            &processedCode,
            &errorMsg
        );

        if (processedCode) {
            destroyableBlobs.push_back(processedCode);
            sourceCode = ea::string(
                (const char*)processedCode->GetBufferPointer(),
                (const char*)processedCode->GetBufferPointer() + processedCode->GetBufferSize()
            );
        }
        if (errorMsg) {
            destroyableBlobs.push_back(errorMsg);
            ea::string output(
                (const char*)errorMsg->GetBufferPointer(),
                (const char*)errorMsg->GetBufferSize() + errorMsg->GetBufferSize()
            );
            if (!output.empty()) {
                compilerOutput_.append(output);
                compilerOutput_.append("\n");
            }
        }

        RELEASE_BLOBS();
        if (FAILED(hr))
            return false;

        // Remove constant buffer suffixes
        // Old Urho3D constant buffer is named like: ConstantBufferNameTYPE
        // Examples: CameraVS, ObjectPS, ZonePS
        // We normalize buffers to: CameraVS => Camera, ObjectPS => Object, ZonePS => Zone
        {
            size_t i = 0;
            do {
                sourceCode.replace(Format("{}{}", shaderParameterGroupNames[i], cbufferSuffix), shaderParameterGroupNames[i]);
            } while (shaderParameterGroupNames[++i] != nullptr);
        }

        // We must generate shader bytecode to execute reflection
        ID3DBlob* byteCode = nullptr;
        hr = D3DCompile(
            sourceCode.data(),
            sourceCode.length(),
            desc_.name_.c_str(),
            nullptr,
            nullptr,
            desc_.entryPoint_.c_str(),
            profile,
            flags,
            0,
            &byteCode,
            &errorMsg
        );

        destroyableBlobs.push_back(byteCode);
        destroyableBlobs.push_back(errorMsg);

        if (errorMsg) {
            ea::string output(
                (const char*)errorMsg->GetBufferPointer(),
                (const char*)errorMsg->GetBufferSize() + errorMsg->GetBufferSize()
            );
            if (!output.empty())
                compilerOutput_.append(output);
        }

        if (FAILED(hr)) {
            RELEASE_BLOBS();
            return false;
        }

        uint8_t* byteCodeBin = (uint8_t*)byteCode->GetBufferPointer();
        size_t byteCodeLength = byteCode->GetBufferSize();

        StringVector mappedSamplers;
        ea::vector<ea::pair<unsigned, VertexElementSemantic>> inputLayout;
        bool result = ReflectHLSL(byteCodeBin, byteCodeLength, mappedSamplers, inputLayout);

        RELEASE_BLOBS();

        if (result) {
            RemapHLSLInputLayout(sourceCode, inputLayout);
            RemapHLSLSamplers(sourceCode, mappedSamplers);
            outputCode_ = sourceCode;
        }

        return result;
#else
        ea::string sourceCode = desc_.sourceCode_;
        ShaderMacroExpanderCreationDesc ci;
        ci.macros_ = desc_.macros_;
        ci.shaderCode_ = desc_.sourceCode_;

        ea::shared_ptr<ShaderMacroExpander> expander = ea::make_shared<ShaderMacroExpander>(ci);
        expander->Process(sourceCode);
        
        ea::string cbufferSuffix = "";
        switch (desc_.type_) {
            case VS:
                cbufferSuffix = "VS";
                break;
            case PS:
                cbufferSuffix = "PS";
                break;
            case GS:
                cbufferSuffix = "GS";
                break;
            case HS:
                cbufferSuffix = "HS";
                break;
            case DS:
                cbufferSuffix = "DS";
                break;
            case CS:
                cbufferSuffix = "CS";
                break;
            default:
                URHO3D_LOGERROR("Unsupported ShaderType {}", desc_.type_);
                return false;
        }
        
        // Remove constant buffer suffixes
        // Old Urho3D constant buffer is named like: ConstantBufferNameTYPE
        // Examples: CameraVS, ObjectPS, ZonePS
        // We normalize buffers to: CameraVS => Camera, ObjectPS => Object, ZonePS => Zone
        {
            size_t i = 0;
            do {
                sourceCode.replace(Format("{}{}", shaderParameterGroupNames[i], cbufferSuffix), shaderParameterGroupNames[i]);
            } while (shaderParameterGroupNames[++i] != nullptr);
        }
        
        ShaderMacroHelper macros;
        for(auto macroIt = desc_.macros_.defines_.begin(); macroIt != desc_.macros_.defines_.end(); ++macroIt) {
            macros.AddShaderMacro(macroIt->first.c_str(), macroIt->second.c_str());
        }
        
        ShaderCreateInfo shaderCI;
        shaderCI.Desc.Name = desc_.name_.c_str();
        shaderCI.Desc.ShaderType = DiligentShaderType[desc_.type_];
        shaderCI.Macros = macros;
        shaderCI.Source = sourceCode.c_str();
        shaderCI.SourceLength = sourceCode.length();
        shaderCI.EntryPoint = desc_.entryPoint_.c_str();
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        
        std::vector<uint32_t> byteCode = GLSLangUtils::HLSLtoSPIRV(shaderCI, GLSLangUtils::SpirvVersion::Vk100, nullptr, nullptr);
        if(byteCode.size() == 0) {
            return false;
        }
        
        StringVector mappedSamplers;
        ea::vector<ea::pair<unsigned, VertexElementSemantic>> inputLayout;
        if (!ReflectGLSL(byteCode.data(), byteCode.size() * sizeof(unsigned), mappedSamplers, inputLayout))
            return false;
        
        RemapHLSLSamplers(sourceCode,  mappedSamplers);
        RemapHLSLInputLayout(sourceCode, inputLayout);
        outputCode_ = sourceCode;
        return true;
#endif
    }
    bool ShaderProcessor::ProcessGLSL()
    {
        ea::vector<unsigned> byteCode;
        if (!CompileGLSL(byteCode))
            return false;

        size_t byteCodeSize = sizeof(unsigned) * byteCode.size();
        StringVector mappedSamplers;
        ea::vector<ea::pair<unsigned, VertexElementSemantic>> inputLayout;
        if (!ReflectGLSL(byteCode.data(), byteCodeSize, mappedSamplers, inputLayout))
            return false;

        ea::string sourceCode = desc_.sourceCode_;
        if (!ConvertShaderToHLSL5(byteCode, sourceCode, compilerOutput_))
            return false;
        outputCode_ = sourceCode;
        return true;
    }
#ifdef WIN32
    bool ShaderProcessor::ReflectHLSL(uint8_t* byteCode, size_t byteCodeLength,
        StringVector& samplers, ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout)
    {
        ID3D11ShaderReflection* reflection = nullptr;
        D3D11_SHADER_DESC shaderDesc;

        HRESULT hr = D3DReflect(byteCode, byteCodeLength, IID_ID3D11ShaderReflection, (void**)&reflection);
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
                if (semantic == MAX_VERTEX_ELEMENT_SEMANTICS)
                    continue;
                vertexElements_.push_back(
                    VertexElement(
                        GetElementType(paramDesc),
                        semantic,
                        paramDesc.SemanticIndex
                    )
                );
                inputLayout.push_back(ea::make_pair(paramDesc.SemanticIndex, semantic));
            }
        }

        // Extract CBuffer and Textures Bindings
        ea::unordered_map<ea::string, unsigned> cbRegisterMap;
        for (unsigned i = 0; i < shaderDesc.BoundResources; ++i) {
            D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
            reflection->GetResourceBindingDesc(i, &resourceDesc);
            ea::string resourceName(resourceDesc.Name);
            if (resourceDesc.Type == D3D_SIT_CBUFFER) {
                auto bufferLookupValue = constantBuffersNamesLookup.find(resourceName);
                if (bufferLookupValue != constantBuffersNamesLookup.end()) {
                    cbRegisterMap[resourceName] = resourceDesc.BindPoint;
                    cbufferSlots_[bufferLookupValue->second] = true;
                }
                else {
                    URHO3D_LOGWARNING("Invalid Resource Name for "+resourceName);
                    continue;
                }
            }
            else if (resourceDesc.Type == D3D_SIT_SAMPLER && resourceDesc.BindPoint < MAX_TEXTURE_UNITS) {
                samplers.push_back(resourceName);
                textureSlots_[resourceDesc.BindPoint] = true;
            }
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
    bool ShaderProcessor::CompileGLSL(ea::vector<unsigned>& byteCode)
    {
        if (!CompileGLSLToSpirV(
            desc_.type_,
            desc_.sourceCode_,
            desc_.macros_,
            byteCode,
            compilerOutput_
        ))
            return false;
        if (desc_.optimizeCode_) {
            std::vector<unsigned> tmpByteCode = OptimizeSPIRV(std::vector<unsigned>(byteCode.begin(), byteCode.end()), SPV_ENV_OPENGL_4_0, SPIRV_OPTIMIZATION_FLAG_LEGALIZATION);
            if (tmpByteCode.size() == 0)
                return false;

            byteCode.resize(tmpByteCode.size());
            memcpy(byteCode.data(), tmpByteCode.data(), tmpByteCode.size() * sizeof(unsigned));
        }

        return true;
    }
    bool ShaderProcessor::ReflectGLSL(const void* byteCode, size_t byteCodeSize, StringVector& samplers, ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout)
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
                inputNameWithoutIdx.replace("input.", "");

                unsigned slotIdx = inputNameEndIdx == inputName.length() - 1 ? 0 : ToUInt(inputName.substr(inputNameWithoutIdx.length(), inputName.length() - 1));
                auto semanticIt = sSemanticsMapping.find(inputNameWithoutIdx);
                if (semanticIt == sSemanticsMapping.end()) {
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
                vertexElements_[vertexElements_.size() - 1].location_ = (*it)->location;
                // Extract Semantic Index from Semantic Name
                ea::string semanticName = (*it)->semantic == nullptr ? "" : (*it)->semantic;
                unsigned semanticIdx = 0;
                if(semanticName.size() > 0 && IsNum(semanticName[semanticName.size() - 1])) {
                    unsigned semanticCharIdx = semanticName.size() - 1;
                    while(semanticCharIdx > 0) {
                        if(IsNum(semanticName[semanticCharIdx]))
                            --semanticCharIdx;
                        else
                            break;
                    }
                    
                    semanticIdx = atoi(semanticName.substr(semanticCharIdx).c_str());
                }
                inputLayout.push_back(ea::make_pair(semanticIdx, semantic));
            }
        }

        unsigned bindingCount = 0;
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        ea::vector<SpvReflectDescriptorBinding*> descriptorBindings(bindingCount);
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, descriptorBindings.data());

        ea::unordered_map<ea::string, unsigned> cbRegisterMap;
        ea::unordered_map<ea::string, bool> texMap;
        for (auto descBindingIt = descriptorBindings.begin(); descBindingIt != descriptorBindings.end(); descBindingIt++) {
            SpvReflectDescriptorBinding* binding = *descBindingIt;

            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                assert(binding->type_description->type_name != nullptr);
                ea::string bindingName(binding->type_description->type_name);
                sSanitizeCBName(bindingName);

                auto cBufferLookupValue = constantBuffersNamesLookup.find(bindingName);
                if (cBufferLookupValue == constantBuffersNamesLookup.end()) {
                    URHO3D_LOGERROR("Failed to reflect shader constant buffer for {} shader. Invalid constant buffer name: {}", desc_.name_, bindingName);
                    spvReflectDestroyShaderModule(&module);
                    return false;
                }

                cbufferSlots_[cBufferLookupValue->second] = true;

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
            else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER) {
                assert(binding->name);
                ea::string name(binding->name);
                ea::string rawName = name;
                
                if(texMap.contains(rawName))
                    continue;
                
                if (name.at(0) == 's')
                    name = name.substr(1, name.size());
                auto texUnitLookupVal = DiligentTextureUnitLookup.find(name);
                if (texUnitLookupVal == DiligentTextureUnitLookup.end()) {
                    URHO3D_LOGERROR("Failed to reflect shader texture samplers. Invalid texture sampler name: {}", name);
                    spvReflectDestroyShaderModule(&module);
                    return false;
                }
                textureSlots_[texUnitLookupVal->second] = true;
                samplers.push_back(rawName);
                
                texMap[rawName] = true;
            }
        }

        return true;
    }
    void ShaderProcessor::RemapHLSLInputLayout(ea::string& sourceCode, const ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout)
    {
        unsigned attribCount = 0;
        for (auto it = inputLayout.begin(); it != inputLayout.end(); ++it) {
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
    void ShaderProcessor::RemapHLSLSamplers(ea::string& sourceCode, const StringVector& samplers)
    {
        // Append _sampler suffix to samplers
        for (auto sampler = samplers.begin(); sampler != samplers.end(); ++sampler) {
            sourceCode.replace(*sampler, Format("_{}_sampler", *sampler));
        }
        // Rename tTexture to sTexture, like: tDiffMap => sDiffMap
        for (auto sampler = samplers.begin(); sampler != samplers.end(); ++sampler) {
            assert(sampler->length() > 1);
            ea::string targetSampler = *sampler;
            if (targetSampler.at(0) == 's')
                targetSampler.replace(0, 1, "t");

            sourceCode.replace(targetSampler, *sampler);
        }
    }
}
