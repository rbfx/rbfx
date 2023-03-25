//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderDefineArray.h"
#include "../../Graphics/ShaderConverter.h"
#include "../../Graphics/ShaderCompiler.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "Urho3D/IO/VirtualFileSystem.h"


#include <Diligent/Graphics/ShaderTools/include/GLSLangUtils.hpp>
#include <Diligent/Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp>
#include <Diligent/Common/interface/DataBlobImpl.hpp>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "../../DebugNew.h"

#include "./DiligentLookupSettings.h"

#include "../../Graphics/ShaderCompiler.h"

namespace Urho3D
{

const char* ShaderVariation_shaderExtensions[] = {
    ".vsbin",
    ".psbin",
    ".gsbin",
    ".hsbin",
    ".dsbin",
    ".csbin"
};

const char* ShaderVariation::elementSemanticNames[] =
{
    "SEM_POSITION",
    "SEM_NORMAL",
    "SEM_BINORMAL",
    "SEM_TANGENT",
    "SEM_TEXCOORD",
    "SEM_COLOR",
    "SEM_BLENDWEIGHT",
    "SEM_BLENDINDICES",
    "SEM_OBJECTINDEX"
};

static ea::unordered_map<ea::string, ShaderParameterGroup> sConstantBuffersLookup = {
    { "Frame", ShaderParameterGroup::SP_FRAME },
    { "Camera", ShaderParameterGroup::SP_CAMERA },
    { "Zone", ShaderParameterGroup::SP_ZONE },
    { "Light", ShaderParameterGroup::SP_MATERIAL },
    { "Object", ShaderParameterGroup::SP_OBJECT },
    { "Custom", ShaderParameterGroup::SP_CUSTOM }
};
static ea::string sConstantBufferSuffixes[] = {
    "VS", "PS", "GS", "HS", "DS", "CS"
};
// Remove Shader Type Suffixes from Constant Buffer Name
static void ShaderVariation_SanitizeCBName(ea::string& cbName) {
    for (unsigned i = 0; i < MAX_SHADER_TYPES; ++i)
        cbName.replace(sConstantBufferSuffixes[i], "");
}

void ShaderVariation::OnDeviceLost()
{
    // No-op on Direct3D11
}

bool ShaderVariation::Create()
{
    if (!graphics_)
        return false;

    if (!owner_)
    {
        compilerOutput_ = "Owner shader has expired";
        return false;
    }

    // Check for up-to-date bytecode on disk
    ea::string path, name, extension;
    SplitPath(owner_->GetName(), path, name, extension);
    extension = ShaderVariation_shaderExtensions[type_];

    const FileIdentifier& cacheDir = graphics_->GetShaderCacheDir();
    const FileIdentifier binaryShaderName = cacheDir + (name + "_" + StringHash(defines_).ToString() + extension);

    // TODO: Fix Bytecode generation
    //if (!LoadByteCode(binaryShaderName))
    //{
    //    // Compile shader if don't have valid bytecode
    //    if (!Compile())
    //        return false;
    //    // Save the bytecode after successful compile, but not if the source is from a package
    //    if (owner_->GetTimeStamp())
    //        SaveByteCode(binaryShaderName);
    //}

    if (!Compile())
        return false;

    return object_ != nullptr;
}

void ShaderVariation::Release()
{
    if (object_)
    {
        if (!graphics_)
            return;

        graphics_->CleanupShaderPrograms(this);

        if (type_ == VS)
        {
            if (graphics_->GetVertexShader() == this)
                graphics_->SetShaders(nullptr, nullptr);
        }
        else if (type_ == CS)
        {
            // Nothing to-do here yet?
        }
        else
        {
            if (graphics_->GetPixelShader() == this)
                graphics_->SetShaders(nullptr, nullptr);
        }
        /*URHO3D_SAFE_RELEASE(object_.ptr_);*/
    }

    compilerOutput_.clear();

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        useTextureUnits_[i] = false;
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        constantBufferSizes_[i] = 0;
    parameters_.clear();
    byteCode_.clear();
    elementHash_ = 0;
}

void ShaderVariation::SetDefines(const ea::string& defines)
{
    defines_ = defines;
}

bool ShaderVariation::LoadByteCode(const FileIdentifier& binaryShaderName)
{
    const VirtualFileSystem* vfs = owner_->GetSubsystem<VirtualFileSystem>();
    if (!vfs->Exists(binaryShaderName))
        return false;

    const FileTime sourceTimeStamp = owner_->GetTimeStamp();
    // If source code is loaded from a package, its timestamp will be zero. Else check that binary is not older
    // than source
    if (sourceTimeStamp)
    {
        const FileTime bytecodeTimeStamp = vfs->GetLastModifiedTime(binaryShaderName, false);
        if (bytecodeTimeStamp && bytecodeTimeStamp < sourceTimeStamp)
            return false;
    }

    const AbstractFilePtr file = vfs->OpenFile(binaryShaderName, FILE_READ);
    if (!file || file->ReadFileID() != "USHD")
    {
        URHO3D_LOGERROR("{} is not a valid shader bytecode file", binaryShaderName.ToUri());
        return false;
    }

    ShaderType shaderType = (ShaderType)file->ReadUShort();
    assert(shaderType == type_);

    elementHash_ = file->ReadUInt();
    elementHash_ <<= 32;

    unsigned numParameters = file->ReadUInt();
    for (unsigned i = 0; i < numParameters; ++i)
    {
        ea::string name = file->ReadString();
        unsigned buffer = file->ReadUByte();
        unsigned offset = file->ReadUInt();
        unsigned size = file->ReadUInt();

        parameters_[StringHash(name)] = ShaderParameter{type_, name, offset, size, buffer};
    }

    unsigned numTextureUnits = file->ReadUInt();
    for (unsigned i = 0; i < numTextureUnits; ++i)
    {
        /*String unitName = */file->ReadString();
        unsigned reg = file->ReadUByte();

        if (reg < MAX_TEXTURE_UNITS)
            useTextureUnits_[reg] = true;
    }

    unsigned byteCodeSize = file->ReadUInt();
    if (byteCodeSize)
    {
        byteCode_.resize(byteCodeSize);
        file->Read(&byteCode_[0], byteCodeSize);

        switch (type_)
        {
        case Urho3D::VS:
            URHO3D_LOGDEBUG("Loaded cached vertex shader " + GetFullName());
            break;
        case Urho3D::PS:
            URHO3D_LOGDEBUG("Loaded cached pixel shader " + GetFullName());
            break;
        case Urho3D::GS:
            URHO3D_LOGDEBUG("Loaded cached geometry shader " + GetFullName());
            break;
        case Urho3D::HS:
            URHO3D_LOGDEBUG("Loaded cached hull shader " + GetFullName());
            break;
        case Urho3D::DS:
            URHO3D_LOGDEBUG("Loaded cached domain shader " + GetFullName());
            break;
        case Urho3D::CS:
            URHO3D_LOGDEBUG("Loaded cached compute shader " + GetFullName());
            break;
        default:
            URHO3D_LOGERROR("{} has invalid shader type", binaryShaderName.ToUri());
            return false;
        }

        CalculateConstantBufferSizes();
        return true;
    }
    else
    {
        URHO3D_LOGERROR("{} has zero length bytecode", binaryShaderName.ToUri());
        return false;
    }
}

bool ShaderVariation::Compile()
{
    using namespace Diligent;
    // Set the code, defines, entrypoint, profile and flags according to the shader being compiled
    ea::string sourceCode;
    const char* entryPoint = nullptr;
    SHADER_TYPE shaderType = SHADER_TYPE_UNKNOWN;
    //const char* profile = nullptr;
    //unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    //ShaderDefineArray defines{ defines_ };
    ShaderCompilerDesc compilerDesc = {};
    compilerDesc.defines_ = ShaderDefineArray{ defines_ };
    compilerDesc.name_ = name_;
    //ShaderMacroHelper macros;


    compilerDesc.type_ = type_;
    switch (type_)
    {
    case Urho3D::VS:
    {
        compilerDesc.entryPoint_ = "VS";
        compilerDesc.defines_.Append("COMPILEVS");
        shaderType = SHADER_TYPE_VERTEX;
    }
        break;
    case Urho3D::PS:
    {
        compilerDesc.entryPoint_ = "PS";
        compilerDesc.defines_.Append("COMPILEPS");
        shaderType = SHADER_TYPE_PIXEL;
    }
        break;
    case Urho3D::GS:
        compilerDesc.entryPoint_ = "GS";
        compilerDesc.defines_.Append("COMPILEGS");
        shaderType = SHADER_TYPE_GEOMETRY;
        break;
    case Urho3D::HS:
        compilerDesc.entryPoint_ = "HS";
        compilerDesc.defines_.Append("COMPILEHS");
        shaderType = SHADER_TYPE_HULL;
        break;
    case Urho3D::DS:
        compilerDesc.entryPoint_ = "DS";
        compilerDesc.defines_.Append("COMPILEDS");
        shaderType = SHADER_TYPE_DOMAIN;
        break;
    case Urho3D::CS:
        compilerDesc.entryPoint_ = "CS";
        compilerDesc.defines_.Append("COMPILECS");
        shaderType = SHADER_TYPE_COMPUTE;
        break;
    }

    compilerDesc.defines_.Append("MAXBONES", Format("{}", Graphics::GetMaxBones()));
    compilerDesc.defines_.Append("D3D11");
    compilerDesc.defines_.Append("DILIGENT");

    // Include Variant defines into macros
    /*{
        auto defineParts = defines_.split(' ');
        for (auto part = defineParts.begin(); part != defineParts.end(); ++part) {
            auto define = part->split('=');
            if (define.size() == 2)
                macros.AddShaderMacro(define[0].c_str(), define[1].c_str());
            else
                macros.AddShaderMacro(define[0].c_str(), true);
        }
    }*/


    // Convert shader source code if GLSL
    static thread_local ea::string convertedShaderSourceCode;
    if (owner_->IsGLSL())
    {
        compilerDesc.defines_.Append("DESKTOP_GRAPHICS");
        compilerDesc.defines_.Append("GL3");
        compilerDesc.language_ = ShaderLanguage::GLSL;
        compilerDesc.entryPoint_ = "main";

        /*ShaderDefineArray defines;
        for (const ShaderMacro* macro = macros; macro->Name != nullptr && macro->Definition != nullptr; ++macro)
            defines.Append(macro->Name, macro->Definition);*/

//        const ea::string& universalSourceCode = owner_->GetSourceCode(type_);
//        ea::string errorMessage;
//        if (!ConvertShaderToHLSL5(type_, universalSourceCode, defines, convertedShaderSourceCode, errorMessage))
//        {
//            URHO3D_LOGERROR("Failed to convert shader {} from GLSL:\n{}{}", GetFullName(), Shader::GetShaderFileList(), errorMessage);
//            return false;
//        }
//
//        // In debug mode, check that all defines are referenced by the shader code
//#ifdef _DEBUG
//        const auto& unusedDefines = defines.FindUnused(universalSourceCode);
//        if (!unusedDefines.empty())
//            URHO3D_LOGWARNING("Shader {} does not use the define(s): {}", GetFullName(), ea::string::joined(unusedDefines, ", "));
//#endif

        /*sourceCode = convertedShaderSourceCode;
        entryPoint = "main";*/
    }
    else
    {
        compilerDesc.language_ = ShaderLanguage::HLSL;
        sourceCode = owner_->GetSourceCode(type_);

//        for (const auto& define : defines)
//        {
//            if (define.second.empty())
//                macros.AddShaderMacro(define.first.c_str(), true);
//            else
//                macros.AddShaderMacro(define.first.c_str(), define.second.c_str());
//
//            // In debug mode, check that all defines are referenced by the shader code
//#ifdef _DEBUG
//            const auto& unusedDefines = defines.FindUnused(nativeSourceCode);
//            if (!unusedDefines.empty())
//                URHO3D_LOGWARNING("Shader {} does not use the define(s): {}", GetFullName(), ea::string::joined(unusedDefines, ", "));
//#endif
//        }
    }

    compilerDesc.code_ = owner_->GetSourceCode(type_);

    ShaderCompiler compiler(compilerDesc);
    if (!compiler.Compile()) {
        URHO3D_LOGERROR("Failed to compile shader " + GetFullName());
        URHO3D_LOGERROR(compiler.GetCompilerOutput());
        return false;
    }

    switch (type_)
    {
    case Urho3D::VS:
        URHO3D_LOGDEBUG("Compiled vertex shader " + GetFullName());
        break;
    case Urho3D::PS:
        URHO3D_LOGDEBUG("Compiled pixel shader " + GetFullName());
        break;
    case Urho3D::GS:
        URHO3D_LOGDEBUG("Compiled geometry shader " + GetFullName());
        break;
    case Urho3D::HS:
        URHO3D_LOGDEBUG("Compiled hull shader " + GetFullName());
        break;
    case Urho3D::DS:
        URHO3D_LOGDEBUG("Compiled domain shader " + GetFullName());
        break;
    case Urho3D::CS:
        URHO3D_LOGDEBUG("Compiled compute shader " + GetFullName());
        break;
    }

    ShaderCreateInfo shaderCI;
#ifdef URHO3D_DEBUG
    shaderCI.Desc.Name = name_.c_str();
#endif
    shaderCI.Desc.ShaderType = shaderType;
    shaderCI.Desc.UseCombinedTextureSamplers = false;
    shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.EntryPoint = compilerDesc.entryPoint_.c_str();

    if (compilerDesc.language_ == ShaderLanguage::HLSL) {
        shaderCI.ByteCode = compiler.GetByteCode().data();
        shaderCI.ByteCodeSize = compiler.GetByteCode().size();
    }
    else {
        shaderCI.Source = (const char*)compiler.GetByteCode().data();
        shaderCI.SourceLength = compiler.GetByteCode().size();
    }


    vertexElements_ = compiler.GetVertexElements();
    parameters_ = compiler.GetShaderParams();

    for (uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i)
        useTextureUnits_[i] = compiler.IsUsedTextureSlot((TextureUnit)i);

    //ParseParameters(spirvByteCode);
    CalculateConstantBufferSizes();
    //GenerateVertexBindings(sourceCode);

    RefCntAutoPtr<IShader> shader;
    graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);

    assert(shader);
    if (!shader) {
        URHO3D_LOGDEBUG("Error has ocurred at Shader Object creation " + GetFullName());
        return false;
    }

    object_ = shader;

    return true;
}

void ShaderVariation::CollectVertexElements(const ea::string& sourceCode) {
    if (type_ != VS)
        return;
    ea::vector<VertexElement> vertexElements;
    ea::array<uint8_t, MAX_VERTEX_ELEMENT_SEMANTICS> repeatedSemantics;

    size_t nextIdx = sourceCode.find(elementSemanticNames[0]);
    while (nextIdx != ea::string::npos) {
        
    }
}

void ShaderVariation::ParseParameters(std::vector<unsigned>& byteCode)
{
    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(byteCode.size() * sizeof(byteCode[0]), byteCode.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Collect Vertex Elements from Shader Reflection and create hash
    if (type_ == VS) {
        vertexElements_ = ea::vector<VertexElement>();
           
        unsigned varCount = 0;
        result = spvReflectEnumerateInputVariables(&module, &varCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        ea::vector<SpvReflectInterfaceVariable*> inputVars(varCount);
        result = spvReflectEnumerateInputVariables(&module, &varCount, inputVars.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        unsigned elementHash = 0;

        ea::array<unsigned, MAX_VERTEX_ELEMENT_SEMANTICS> repeatedSemantics;
        repeatedSemantics.fill(0);

        for (auto it = inputVars.begin(); it != inputVars.end(); ++it) {
            ea::string semanticName((*it)->semantic);
            // Remove Digits from Semantic Name
            while (isdigit(semanticName.at(semanticName.size() - 1)))
                semanticName = semanticName.substr(0, semanticName.size() - 1);
            VertexElementSemantic semantic = (VertexElementSemantic)GetStringListIndex(semanticName.c_str(), elementSemanticNames, MAX_VERTEX_ELEMENT_SEMANTICS, true);
            if (semantic != MAX_VERTEX_ELEMENT_SEMANTICS) {
                CombineHash(elementHash, semantic);
                CombineHash(elementHash, (*it)->location);
                vertexElements_.push_back(VertexElement(MAX_VERTEX_ELEMENT_TYPES, semantic, repeatedSemantics[semantic]));
                ++repeatedSemantics[semantic];
            }
        }
        elementHash_ = elementHash;
        elementHash_ <<= 32;
    }

    {
        // Collect used Constant Buffers
        // This step is too simple, we just follow rules above
        // 1. Collect Constant Buffers
        // 1.1 Lookup Shader Parameter Group by the Constant Buffer Name
        // 1.2 Extract Struct Members, Size and Offsets to Build Shader Parameters, and use ShaderParameterGroup to build ShaderParameter
        // 2. Collect Samplers(Textures)
        // 2.1 Normalize Sampler name by removing your suffixes from parameter.
        // 2.2 After a full sampler name normalization, then we lookup TextureUnit by the sampler name and turn on flag
        unsigned bindingCount = 0;
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        ea::vector<SpvReflectDescriptorBinding*> descriptorBindings(bindingCount);
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, descriptorBindings.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        ea::unordered_map<ea::string, unsigned> cbRegisterMap;

        for (auto descBindingIt = descriptorBindings.begin(); descBindingIt != descriptorBindings.end(); descBindingIt++) {
            SpvReflectDescriptorBinding* binding = *descBindingIt;

            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                assert(binding->type_description->type_name != nullptr);
                ea::string bindingName(binding->type_description->type_name);
                ShaderVariation_SanitizeCBName(bindingName);

                auto cBufferLookupValue = sConstantBuffersLookup.find(bindingName);
                if (cBufferLookupValue == sConstantBuffersLookup.end()) {
                    spvReflectDestroyShaderModule(&module);
                    URHO3D_LOGERRORF("Failed to reflect shader constant buffer. Invalid constant buffer name: %s", bindingName);
                    return;
                }

                unsigned membersCount = binding->block.member_count;
                // Extract CBuffer variable parameters and build shader parameters
                while (membersCount--) {
                    const SpvReflectBlockVariable& variable = binding->block.members[membersCount];
                    ea::string varName(variable.name);
                    const auto nameStart = varName.find('c');
                    if (nameStart != ea::string::npos)
                        varName = varName.substr(nameStart + 1);
                    parameters_[varName] = ShaderParameter { type_ ,varName, variable.offset, variable.size, (unsigned)cBufferLookupValue->second };
                }
            }
            else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                assert(binding->name);
                ea::string name(binding->name);
                if (name.at(0) == 't')
                    name = name.substr(1, name.size());
                auto texUnitLookupVal = DiligentTextureUnitLookup.find(name);
                if (texUnitLookupVal == DiligentTextureUnitLookup.end()) {
                    URHO3D_LOGERRORF("Failed to reflect shader texture samplers. Invalid texture sampler name: %s", name);
                    spvReflectDestroyShaderModule(&module);
                    return;
                }
                useTextureUnits_[texUnitLookupVal->second] = true;
            }
        }
    }

    spvReflectDestroyShaderModule(&module);
}

void ShaderVariation::GenerateVertexBindings(ea::string& sourceCode) {
    if (type_ != VS)
        return;

    byte attribIdx = 0;
    ea::string attribKey = "";
    ea::string semanticName = "";
    ea::string semanticNameWithIndex = "";
    ea::array<byte, MAX_VERTEX_ELEMENT_SEMANTICS> repeatedSemantics;
    repeatedSemantics.fill(0);
    for (const VertexElement* element = vertexElements_.begin(); element != vertexElements_.end(); ++element) {
        semanticName = elementSemanticNames[element->semantic_];
        semanticNameWithIndex = semanticName;
        semanticNameWithIndex.append_sprintf("%d", repeatedSemantics[element->semantic_]);
        attribKey = "ATTRIB";
        attribKey.append_sprintf("%d", attribIdx);

        // Replace semantic with index. Ex: SEM_TEXCOORD0 -> ATTRIBN
        if (sourceCode.find(semanticNameWithIndex) != ea::string::npos)
            sourceCode.replace(semanticNameWithIndex, attribKey);
        else if(repeatedSemantics[element->semantic_] == 0) {
            // Sometimes semantic doesn't contains a index normally this occurs if semantic doesn't repeats
            // For this case, we replace semantic without index. Ex: SEM_POSITION -> ATTRIB0 or SEM_COLOR -> ATTRIBN
            sourceCode.replace(semanticName, attribKey);
        }
        ++repeatedSemantics[element->semantic_];
        ++attribIdx;
    }
}

void ShaderVariation::SaveByteCode(const FileIdentifier& binaryShaderName)
{
   VirtualFileSystem* vfs = owner_->GetSubsystem<VirtualFileSystem>();

    auto file = vfs->OpenFile(binaryShaderName, FILE_WRITE);
    if (!file)
        return;

    // FileID: Urho Diligent Shader
    file->WriteFileID("UDSHD");
    file->WriteUShort((unsigned short)type_);

    file->WriteUInt(elementHash_ >> 32);

    file->WriteUInt(parameters_.size());
    for (auto i = parameters_.begin(); i != parameters_.end(); ++i)
    {
        file->WriteString(i->second.name_);
        file->WriteUByte((unsigned char)i->second.buffer_);
        file->WriteUInt(i->second.offset_);
        file->WriteUInt(i->second.size_);
    }

    unsigned usedTextureUnits = 0;
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        if (useTextureUnits_[i])
            ++usedTextureUnits;
    }
    file->WriteUInt(usedTextureUnits);
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        if (useTextureUnits_[i])
        {
            file->WriteString(graphics_->GetTextureUnitName((TextureUnit)i));
            file->WriteUByte((unsigned char)i);
        }
    }

    file->WriteUInt(byteCode_.size());
    if (byteCode_.size())
        file->Write(&byteCode_[0], byteCode_.size());
}

void ShaderVariation::CalculateConstantBufferSizes()
{
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        constantBufferSizes_[i] = 0;

    for (auto i = parameters_.begin(); i != parameters_.end(); ++i)
    {
        if (i->second.buffer_ < MAX_SHADER_PARAMETER_GROUPS)
        {
            unsigned oldSize = constantBufferSizes_[i->second.buffer_];
            unsigned paramEnd = i->second.offset_ + i->second.size_;
            if (paramEnd > oldSize)
                constantBufferSizes_[i->second.buffer_] = (paramEnd + 15) / 16 * 16;
        }
    }
}

}
