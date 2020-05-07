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

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"

#include <d3dcompiler.h>

#include "../../DebugNew.h"

namespace Urho3D
{

const char* ShaderVariation_shaderExtensions[] = {
    ".vs4",
    ".ps4",
    ".gs4",
    ".hs5",
    ".ds5",
    ".cs5"
};

const char* ShaderVariation::elementSemanticNames[] =
{
    "POSITION",
    "NORMAL",
    "BINORMAL",
    "TANGENT",
    "TEXCOORD",
    "COLOR",
    "BLENDWEIGHT",
    "BLENDINDICES",
    "OBJECTINDEX"
};

void ShaderVariation::OnDeviceLost()
{
    // No-op on Direct3D11
}

bool ShaderVariation::Create()
{
    Release();

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

    if (!LoadByteCode(binaryShaderName))
    {
        // Compile shader if don't have valid bytecode
        if (!Compile())
            return false;
        // Save the bytecode after successful compile, but not if the source is from a package
        if (owner_->GetTimeStamp())
            SaveByteCode(binaryShaderName);
    }

#define CREATE_SHADER(TYPEENUM, SHADERTYPE, PRINTNAME) \
    if (device && byteCode_.size()) { \
        HRESULT hr = device->Create ## SHADERTYPE (&byteCode_[0], byteCode_.size(), nullptr, (ID3D11 ## SHADERTYPE **)&object_.ptr_); \
        if (FAILED(hr)) { \
            URHO3D_SAFE_RELEASE(object_.ptr_); \
            compilerOutput_ = "Could not create " PRINTNAME " (HRESULT " + ToStringHex((unsigned)hr) + ")"; \
        } \
    } \
    else \
        compilerOutput_ = "Could not create " PRINTNAME " , empty bytecode";


    // Then create shader from the bytecode
    ID3D11Device* device = graphics_->GetImpl()->GetDevice();
    if (type_ == VS)
    {
        CREATE_SHADER(VS, VertexShader, "vertex shader")
    }
    else if (type_ == GS)
    {
        CREATE_SHADER(GS, GeometryShader, "geometry shader")
    }
    else if (type_ == HS)
    {
        CREATE_SHADER(HS, HullShader, "hull shader")
    }
    else if (type_ == DS)
    {
        CREATE_SHADER(DS, DomainShader, "domain shader")
    }
    else if (type_ == CS)
    {
        CREATE_SHADER(CS, ComputeShader, "compute shader")
    }
    else if (type_ == PS)
    {
        CREATE_SHADER(PS, PixelShader, "pixel shader")
    }
    else
    {
        assert(0);
    }

#undef CREATE_SHADER

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
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else if (type_ == PS)
        {
            if (graphics_->GetPixelShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else if (type_ == GS)
        {
            if (graphics_->GetGeometryShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else if (type_ == HS)
        {
            if (graphics_->GetHullShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else if (type_ == DS)
        {
            if (graphics_->GetDomainShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else
        {
            // Nothing to-do here yet?
        }

        URHO3D_SAFE_RELEASE(object_.ptr_);
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

    // Internal mechanism for appending the CLIPPLANE define, prevents runtime (every frame) string manipulation
    definesClipPlane_ = defines;
    if (!definesClipPlane_.ends_with(" CLIPPLANE"))
        definesClipPlane_ += " CLIPPLANE";
}

bool ShaderVariation::LoadByteCode(const ea::string& binaryShaderName)
{
    ResourceCache* cache = owner_->GetSubsystem<ResourceCache>();
    if (!cache->Exists(binaryShaderName))
        return false;

    FileSystem* fileSystem = owner_->GetSubsystem<FileSystem>();
    unsigned sourceTimeStamp = owner_->GetTimeStamp();
    // If source code is loaded from a package, its timestamp will be zero. Else check that binary is not older
    // than source
    if (sourceTimeStamp && fileSystem->GetLastModifiedTime(cache->GetResourceFileName(binaryShaderName)) < sourceTimeStamp)
        return false;

    SharedPtr<File> file = cache->GetFile(binaryShaderName);
    if (!file || file->ReadFileID() != "USHD")
    {
        URHO3D_LOGERROR(binaryShaderName + " is not a valid shader bytecode file");
        return false;
    }

    /// \todo Check that shader type and model match
    /*unsigned short shaderType = */file->ReadUShort();
    /*unsigned short shaderModel = */file->ReadUShort();
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
        /*ea::string unitName = */file->ReadString();
        unsigned reg = file->ReadUByte();

        if (reg < MAX_TEXTURE_UNITS)
            useTextureUnits_[reg] = true;
    }

    unsigned byteCodeSize = file->ReadUInt();
    if (byteCodeSize)
    {
        byteCode_.resize(byteCodeSize);
        file->Read(&byteCode_[0], byteCodeSize);

        if (type_ == VS)
            URHO3D_LOGDEBUG("Loaded cached vertex shader " + GetFullName());
        else if (type_ == PS)
            URHO3D_LOGDEBUG("Loaded cached pixel shader " + GetFullName());
        else if (type_ == GS)
            URHO3D_LOGDEBUG("Loaded cached geometry shader " + GetFullName());
        else if (type_ == HS)
            URHO3D_LOGDEBUG("Loaded cached hull shader " + GetFullName());
        else if (type_ == DS)
            URHO3D_LOGDEBUG("Loaded cached domain shader " + GetFullName());
        else
            URHO3D_LOGDEBUG("Loaded cached compute shader " + GetFullName());

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
    const ea::string& sourceCode = owner_->GetSourceCode(type_);
    ea::vector<ea::string> defines = defines_.split(' ');

    // Set the entrypoint, profile and flags according to the shader being compiled
    const char* entryPoint = nullptr;
    const char* profile = nullptr;
    unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

    defines.emplace_back("D3D11");

    if (type_ == VS)
    {
        entryPoint = "VS";
        defines.emplace_back("COMPILEVS");
        profile = "vs_4_0";
    }
    else if (type_ == PS)
    {
        entryPoint = "PS";
        defines.emplace_back("COMPILEPS");
        profile = "ps_4_0";
        flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
    }
    else if (type_ == GS)
    {
        entryPoint = "GS";
        defines.emplace_back("COMPILEGS");
        profile = "gs_4_0";
    }
    else if (type_ == HS)
    {
        entryPoint = "HS";
        defines.emplace_back("COMPILEHS");
        profile = "hs_5_0";
    }
    else if (type_ == DS)
    {
        entryPoint = "DS";
        defines.emplace_back("COMPILEDS");
        profile = "ds_5_0";
    }
    else if (type_ == CS)
    {
        entryPoint = "CS";
        defines.emplace_back("COMPILECS");
        profile = "cs_5_0";
    }
    else
    {
        assert(0);
    }

    defines.emplace_back("MAXBONES=" + ea::to_string(Graphics::GetMaxBones()));

    // Collect defines into macros
    ea::vector<ea::string> defineValues;
    ea::vector<D3D_SHADER_MACRO> macros;

    for (unsigned i = 0; i < defines.size(); ++i)
    {
        unsigned equalsPos = defines[i].find('=');
        if (equalsPos != ea::string::npos)
        {
            defineValues.emplace_back(defines[i].substr(equalsPos + 1));
            defines[i].resize(equalsPos);
        }
        else
            defineValues.emplace_back("1");
    }
    for (unsigned i = 0; i < defines.size(); ++i)
    {
        D3D_SHADER_MACRO macro;
        macro.Name = defines[i].c_str();
        macro.Definition = defineValues[i].c_str();
        macros.emplace_back(macro);

        // In debug mode, check that all defines are referenced by the shader code
#ifdef _DEBUG
        if (sourceCode.find(defines[i]) == ea::string::npos)
            URHO3D_LOGWARNING("Shader " + GetFullName() + " does not use the define " + defines[i]);
#endif
    }

    D3D_SHADER_MACRO endMacro;
    endMacro.Name = nullptr;
    endMacro.Definition = nullptr;
    macros.emplace_back(endMacro);

    // Compile using D3DCompile
    ID3DBlob* shaderCode = nullptr;
    ID3DBlob* errorMsgs = nullptr;

    HRESULT hr = D3DCompile(sourceCode.c_str(), sourceCode.length(), owner_->GetName().c_str(), &macros.front(), nullptr,
        entryPoint, profile, flags, 0, &shaderCode, &errorMsgs);
    if (FAILED(hr))
    {
        // Do not include end zero unnecessarily
        compilerOutput_ = ea::string((const char*)errorMsgs->GetBufferPointer(), (unsigned)errorMsgs->GetBufferSize() - 1);
    }
    else
    {
        if (type_ == VS)
            URHO3D_LOGDEBUG("Compiled vertex shader " + GetFullName());
        else if (type_ == PS)
            URHO3D_LOGDEBUG("Compiled pixel shader " + GetFullName());
        else if (type_ == GS)
            URHO3D_LOGDEBUG("Compiled geometry shader " + GetFullName());
        else if (type_ == HS)
            URHO3D_LOGDEBUG("Compiled hull shader " + GetFullName());
        else if (type_ == DS)
            URHO3D_LOGDEBUG("Compiled domain shader " + GetFullName());
        else if (type_ == CS)
            URHO3D_LOGDEBUG("Compiled compute shader " + GetFullName());
        else
            assert(0);

        unsigned char* bufData = (unsigned char*)shaderCode->GetBufferPointer();
        unsigned bufSize = (unsigned)shaderCode->GetBufferSize();
        // Use the original bytecode to reflect the parameters
        ParseParameters(bufData, bufSize);
        CalculateConstantBufferSizes();

        // Then strip everything not necessary to use the shader
        ID3DBlob* strippedCode = nullptr;
        D3DStripShader(bufData, bufSize,
            D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, &strippedCode);
        byteCode_.resize((unsigned)strippedCode->GetBufferSize());
        memcpy(&byteCode_[0], strippedCode->GetBufferPointer(), byteCode_.size());
        strippedCode->Release();
    }

    URHO3D_SAFE_RELEASE(shaderCode);
    URHO3D_SAFE_RELEASE(errorMsgs);

    return !byteCode_.empty();
}

void ShaderVariation::ParseParameters(unsigned char* bufData, unsigned bufSize)
{
    ID3D11ShaderReflection* reflection = nullptr;
    D3D11_SHADER_DESC shaderDesc;

    HRESULT hr = D3DReflect(bufData, bufSize, IID_ID3D11ShaderReflection, (void**)&reflection);
    if (FAILED(hr) || !reflection)
    {
        URHO3D_SAFE_RELEASE(reflection);
        URHO3D_LOGD3DERROR("Failed to reflect vertex shader's input signature", hr);
        return;
    }

    reflection->GetDesc(&shaderDesc);

    if (type_ == VS)
    {
        unsigned elementHash = 0;
        for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
        {
            D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
            reflection->GetInputParameterDesc((UINT)i, &paramDesc);
            VertexElementSemantic semantic = (VertexElementSemantic)GetStringListIndex(paramDesc.SemanticName, elementSemanticNames, MAX_VERTEX_ELEMENT_SEMANTICS, true);
            if (semantic != MAX_VERTEX_ELEMENT_SEMANTICS)
            {
                CombineHash(elementHash, semantic);
                CombineHash(elementHash, paramDesc.SemanticIndex);
            }
        }
        elementHash_ = elementHash;
        elementHash_ <<= 32;
    }

    ea::unordered_map<ea::string, unsigned> cbRegisterMap;

    for (unsigned i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
        reflection->GetResourceBindingDesc(i, &resourceDesc);
        ea::string resourceName(resourceDesc.Name);
        if (resourceDesc.Type == D3D_SIT_CBUFFER)
            cbRegisterMap[resourceName] = resourceDesc.BindPoint;
        else if (resourceDesc.Type == D3D_SIT_SAMPLER && resourceDesc.BindPoint < MAX_TEXTURE_UNITS)
            useTextureUnits_[resourceDesc.BindPoint] = true;
    }

    for (unsigned i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(i);
        D3D11_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);
        unsigned cbRegister = cbRegisterMap[ea::string(cbDesc.Name)];

        for (unsigned j = 0; j < cbDesc.Variables; ++j)
        {
            ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC varDesc;
            var->GetDesc(&varDesc);
            ea::string varName(varDesc.Name);
            if (varName[0] == 'c')
            {
                varName = varName.substr(1); // Strip the c to follow Urho3D constant naming convention
                parameters_[varName] = ShaderParameter{type_, varName, varDesc.StartOffset, varDesc.Size, cbRegister};
            }
        }
    }

    reflection->Release();
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

    SharedPtr<File> file(new File(owner_->GetContext(), fullName, FILE_WRITE));
    if (!file->IsOpen())
        return;

    file->WriteFileID("USHD");
    file->WriteShort((unsigned short)type_);
    // Shader Model, CS/HS/DS use SM5
    if (type_ == CS)
        file->WriteShort(5);
    else
        file->WriteShort(4);

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
                constantBufferSizes_[i->second.buffer_] = paramEnd;
        }
    }
}

}
