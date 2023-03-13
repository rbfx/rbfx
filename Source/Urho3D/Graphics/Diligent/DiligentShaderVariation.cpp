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
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"

#include <Diligent/Graphics/ShaderTools/include/GLSLangUtils.hpp>
#include <Diligent/Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp>
#include <Diligent/Common/interface/DataBlobImpl.hpp>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "../../DebugNew.h"

#include "./DiligentLookupSettings.h"

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

    ea::string binaryShaderName = graphics_->GetShaderCacheDir() + name + "_" + StringHash(defines_).ToString() + extension;

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

    return object_.ptr_ != nullptr;
}

void ShaderVariation::Release()
{
    if (object_.ptr_)
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

        assert(0);
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

bool ShaderVariation::LoadByteCode(const ea::string& binaryShaderName)
{
    ResourceCache* cache = owner_->GetSubsystem<ResourceCache>();
    if (!cache->Exists(binaryShaderName))
        return false;

    const FileSystem* fileSystem = owner_->GetSubsystem<FileSystem>();
    const unsigned sourceTimeStamp = owner_->GetTimeStamp();
    // If source code is loaded from a package, its timestamp will be zero. Else check that binary is not older
    // than source
    if (sourceTimeStamp && fileSystem->GetLastModifiedTime(cache->GetResourceFileName(binaryShaderName)) < sourceTimeStamp)
        return false;

    const AbstractFilePtr file = cache->GetFile(binaryShaderName);
    if (!file || file->ReadFileID() != "UDSHD")
    {
        URHO3D_LOGERROR(binaryShaderName + " is not a valid shader bytecode file");
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
            URHO3D_LOGERROR(binaryShaderName + " has invalid shader type");
            return false;
        }

        CalculateConstantBufferSizes();
        return true;
    }
    else
    {
        URHO3D_LOGERROR(binaryShaderName + " has zero length bytecode");
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
    ShaderMacroHelper macros;


    switch (type_)
    {
    case Urho3D::VS:
    {
        entryPoint = "VS";
        macros.AddShaderMacro("COMPILEVS", true);
        //defines.Append("COMPILEVS");
        shaderType = SHADER_TYPE_VERTEX;
    }
        break;
    case Urho3D::PS:
    {
        entryPoint = "PS";
        macros.AddShaderMacro("COMPILEPS", true);
        //defines.Append("COMPILEPS");
        shaderType = SHADER_TYPE_PIXEL;
    }
        break;
    case Urho3D::GS:
        entryPoint = "GS";
        macros.AddShaderMacro("COMPILEGS", true);
        //defines.Append("COMPILEGS");
        shaderType = SHADER_TYPE_GEOMETRY;
        break;
    case Urho3D::HS:
        entryPoint = "HS";
        macros.AddShaderMacro("COMPILEHS", true);
        //defines.Append("COMPILEHS");
        shaderType = SHADER_TYPE_HULL;
        break;
    case Urho3D::DS:
        entryPoint = "DS";
        macros.AddShaderMacro("COMPILEDS", true);
        //defines.Append("COMPILEDS");
        shaderType = SHADER_TYPE_DOMAIN;
        break;
    case Urho3D::CS:
        entryPoint = "CS";
        macros.AddShaderMacro("COMPILECS", true);
        //defines.Append("COMPILECS");
        shaderType = SHADER_TYPE_COMPUTE;
        break;
    }

    macros.AddShaderMacro("MAXBONES", Graphics::GetMaxBones());
    macros.AddShaderMacro("D3D11", true);
    macros.AddShaderMacro("DILIGENT", true);

    // Include Variant defines into macros
    {
        auto defineParts = defines_.split(' ');
        for (auto part = defineParts.begin(); part != defineParts.end(); ++part) {
            auto define = part->split('=');
            if (define.size() == 2)
                macros.AddShaderMacro(define[0].c_str(), define[1].c_str());
            else
                macros.AddShaderMacro(define[0].c_str(), true);
        }
    }

    // Convert shader source code if GLSL
    static thread_local ea::string convertedShaderSourceCode;
    if (owner_->IsGLSL())
    {
        macros.AddShaderMacro("DESKTOP_GRAPHICS", true);
        macros.AddShaderMacro("GL3", true);

        ShaderDefineArray defines;
        for (const ShaderMacro* macro = macros; macro->Name != nullptr && macro->Definition != nullptr; ++macro)
            defines.Append(macro->Name, macro->Definition);

        const ea::string& universalSourceCode = owner_->GetSourceCode(type_);
        ea::string errorMessage;
        if (!ConvertShaderToHLSL5(type_, universalSourceCode, defines, convertedShaderSourceCode, errorMessage))
        {
            URHO3D_LOGERROR("Failed to convert shader {} from GLSL:\n{}{}", GetFullName(), Shader::GetShaderFileList(), errorMessage);
            return false;
        }

        // In debug mode, check that all defines are referenced by the shader code
#ifdef _DEBUG
        const auto& unusedDefines = defines.FindUnused(universalSourceCode);
        if (!unusedDefines.empty())
            URHO3D_LOGWARNING("Shader {} does not use the define(s): {}", GetFullName(), ea::string::joined(unusedDefines, ", "));
#endif

        sourceCode = convertedShaderSourceCode;
        entryPoint = "main";
    }
    else
    {
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

    // TODO: Refactor this line
    sourceCode.replace("CameraVS", "Camera");
    sourceCode.replace("ObjectVS", "Object");
    sourceCode.replace("CameraPS", "Camera");
    sourceCode.replace("ObjectPS", "Object");

    ShaderCreateInfo shaderCI;
    shaderCI.Desc.Name = name_.c_str();
    shaderCI.Desc.ShaderType = shaderType;
    shaderCI.Desc.UseCombinedTextureSamplers = false;
    shaderCI.EntryPoint = entryPoint;
    shaderCI.Source = sourceCode.c_str();
    shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    shaderCI.Macros = macros;

    auto compilerOutputBlob = DataBlobImpl::Create();
    auto shaderSrcOutput = DataBlobImpl::Create();

    compilerOutputBlob->Resize(sourceCode.size());
    IDataBlob* compilerOutputs[] = {compilerOutputBlob, shaderSrcOutput};
    // Compile Shader Code into SPIR-V
    // This is required because SPIR-V bytecode is
    // used to collect cbuffers and input layout
    auto spirvByteCode = Diligent::GLSLangUtils::HLSLtoSPIRV(shaderCI, GLSLangUtils::SpirvVersion::Vk120, nullptr, compilerOutputs);

    compilerOutput_ = ea::string(static_cast<const char*>(compilerOutputBlob->GetConstDataPtr()));

    if (!compilerOutput_.empty()) {
        URHO3D_LOGERROR("Failed to compile shader " + GetFullName());
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

    ParseParameters(spirvByteCode);
    CalculateConstantBufferSizes();
    GenerateVertexBindings(sourceCode);

    shaderCI.Source = sourceCode.c_str();
#ifndef URHO3D_DEBUG
    // Compile Real Shader code and remove reflection info.
    shaderCI.CompileFlags = SHADER_COMPILE_FLAG_SKIP_REFLECTION;
#endif

    IShader* shader = nullptr;
    graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);

    if (!shader) {
        URHO3D_LOGDEBUG("Error has ocurred at Shader Compilation " + GetFullName());
        return false;
    }

    object_.ptr_ = shader;

    return true;
//
//    D3D_SHADER_MACRO endMacro;
//    endMacro.Name = nullptr;
//    endMacro.Definition = nullptr;
//    macros.emplace_back(endMacro);
//
//    // Compile using D3DCompile
//    ID3DBlob* shaderCode = nullptr;
//    ID3DBlob* errorMsgs = nullptr;
//
//    HRESULT hr = D3DCompile(sourceCode->c_str(), sourceCode->length(), owner_->GetName().c_str(), &macros.front(), nullptr,
//        entryPoint, profile, flags, 0, &shaderCode, &errorMsgs);
//    if (FAILED(hr))
//    {
//        // Do not include end zero unnecessarily
//        compilerOutput_ = ea::string((const char*)errorMsgs->GetBufferPointer(), (unsigned)errorMsgs->GetBufferSize() - 1);
//    }
//    else
//    {
//        if (type_ == VS)
//            URHO3D_LOGDEBUG("Compiled vertex shader " + GetFullName());
//        else if (type_ == CS)
//            URHO3D_LOGDEBUG("Compiled compute shader " + GetFullName());
//        else
//            URHO3D_LOGDEBUG("Compiled pixel shader " + GetFullName());
//
//        unsigned char* bufData = (unsigned char*)shaderCode->GetBufferPointer();
//        unsigned bufSize = (unsigned)shaderCode->GetBufferSize();
//        // Use the original bytecode to reflect the parameters
//        ParseParameters(bufData, bufSize);
//        CalculateConstantBufferSizes();
//
//        // Then strip everything not necessary to use the shader
//        ID3DBlob* strippedCode = nullptr;
//        D3DStripShader(bufData, bufSize,
//            D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, &strippedCode);
//        byteCode_.resize((unsigned)strippedCode->GetBufferSize());
//        memcpy(&byteCode_[0], strippedCode->GetBufferPointer(), byteCode_.size());
//        strippedCode->Release();
//    }
//
//    URHO3D_SAFE_RELEASE(shaderCode);
//    URHO3D_SAFE_RELEASE(errorMsgs);
//
//    return !byteCode_.empty();
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
        sourceCode.replace(semanticNameWithIndex, attribKey);
        // Sometimes semantic doesn't contains a index normally this occurs if semantic doesn't repeats
        // For this case, we replace semantic without index. Ex: SEM_POSITION -> ATTRIB0 or SEM_COLOR -> ATTRIBN
        if (repeatedSemantics[element->semantic_] == 0)
            sourceCode.replace(semanticName, attribKey);
        ++repeatedSemantics[element->semantic_];
        ++attribIdx;
    }
}

void ShaderVariation::SaveByteCode(const ea::string& binaryShaderName)
{
    ResourceCache* cache = owner_->GetSubsystem<ResourceCache>();
    FileSystem* fileSystem = owner_->GetSubsystem<FileSystem>();

    // Filename may or may not be inside the resource system
    ea::string fullName = binaryShaderName;
    if (!IsAbsolutePath(fullName))
    {
        // If not absolute, use the resource dir of the shader
        ea::string shaderFileName = cache->GetResourceFileName(owner_->GetName());
        if (shaderFileName.empty())
            return;
        fullName = shaderFileName.substr(0, shaderFileName.find(owner_->GetName())) + binaryShaderName;
    }
    ea::string path = GetPath(fullName);
    if (!fileSystem->DirExists(path))
        fileSystem->CreateDir(path);

    auto file = MakeShared<File>(owner_->GetContext(), fullName, FILE_WRITE);
    if (!file->IsOpen())
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
