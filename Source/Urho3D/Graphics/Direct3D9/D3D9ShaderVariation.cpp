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
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/File.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"

#include <d3dcompiler.h>
#include <MojoShader/mojoshader.h>

#include "../../DebugNew.h"

namespace Urho3D
{

void CopyStrippedCode(ea::vector<unsigned char>& byteCode, unsigned char* bufData, unsigned bufSize)
{
    unsigned const D3DSIO_COMMENT = 0xFFFE;
    unsigned* srcWords = (unsigned*)bufData;
    unsigned srcWordSize = bufSize >> 2;
    byteCode.clear();

    for (unsigned i = 0; i < srcWordSize; ++i)
    {
        unsigned opcode = srcWords[i] & 0xffff;
        // unsigned paramLength = (srcWords[i] & 0x0f000000) >> 24;
        unsigned commentLength = srcWords[i] >> 16;

        // For now, skip comment only at fixed position to prevent false positives
        if (i == 1 && opcode == D3DSIO_COMMENT)
        {
            // Skip the comment
            i += commentLength;
        }
        else
        {
            // Not a comment, copy the data
            unsigned destPos = byteCode.size();
            byteCode.resize(destPos + sizeof(unsigned));
            unsigned* dest = (unsigned*)&byteCode[destPos];
            *dest = srcWords[i];
        }
    }
}

void ShaderVariation::OnDeviceLost()
{
    // No-op on Direct3D9, shaders are preserved through a device loss & reset
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
    extension = type_ == VS ? ".vs3" : ".ps3";

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

    // Then create shader from the bytecode
    IDirect3DDevice9* device = graphics_->GetImpl()->GetDevice();
    if (type_ == VS)
    {
        HRESULT hr = device->CreateVertexShader(
            (const DWORD*)&byteCode_[0],
            (IDirect3DVertexShader9**)&object_.ptr_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(object_.ptr_);
            compilerOutput_ = "Could not create vertex shader (HRESULT " + ToStringHex((unsigned)hr) + ")";
        }
    }
    else
    {
        HRESULT hr = device->CreatePixelShader(
            (const DWORD*)&byteCode_[0],
            (IDirect3DPixelShader9**)&object_.ptr_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(object_.ptr_);
            compilerOutput_ = "Could not create pixel shader (HRESULT " + ToStringHex((unsigned)hr) + ")";
        }
    }

    // The bytecode is not needed on Direct3D9 after creation, so delete it to save memory
    byteCode_.clear();
    byteCode_.reserve(0);

    return object_.ptr_ != nullptr;
}

void ShaderVariation::Release()
{
    if (object_.ptr_ && graphics_)
    {
        graphics_->CleanupShaderPrograms(this);

        if (type_ == VS)
        {
            if (graphics_->GetVertexShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        else
        {
            if (graphics_->GetPixelShader() == this)
                graphics_->SetShaders(nullptr, nullptr, nullptr, nullptr, nullptr);
        }
    }

    URHO3D_SAFE_RELEASE(object_.ptr_);

    compilerOutput_.clear();

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        useTextureUnits_[i] = false;
    parameters_.clear();
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

    unsigned numParameters = file->ReadUInt();
    for (unsigned i = 0; i < numParameters; ++i)
    {
        ea::string name = file->ReadString();
        unsigned reg = file->ReadUByte();
        unsigned regCount = file->ReadUByte();

        parameters_[StringHash(name)] = ShaderParameter{type_, name, reg, regCount};
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
        else
            URHO3D_LOGDEBUG("Loaded cached pixel shader " + GetFullName());

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

    if (type_ == VS)
    {
        entryPoint = "VS";
        defines.emplace_back("COMPILEVS");
        profile = "vs_3_0";
    }
    else
    {
        entryPoint = "PS";
        defines.emplace_back("COMPILEPS");
        profile = "ps_3_0";
        flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
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
        else
            URHO3D_LOGDEBUG("Compiled pixel shader " + GetFullName());

        // Inspect the produced bytecode using MojoShader, then strip and store it
        unsigned char* bufData = (unsigned char*)shaderCode->GetBufferPointer();
        unsigned bufSize = (unsigned)shaderCode->GetBufferSize();
        ParseParameters(bufData, bufSize);
        CopyStrippedCode(byteCode_, bufData, bufSize);
    }

    URHO3D_SAFE_RELEASE(shaderCode);
    URHO3D_SAFE_RELEASE(errorMsgs);

    return !byteCode_.empty();
}

void ShaderVariation::ParseParameters(unsigned char* bufData, unsigned bufSize)
{
    MOJOSHADER_parseData const* parseData = MOJOSHADER_parse("bytecode", bufData, bufSize, nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr);

    for (int i = 0; i < parseData->symbol_count; i++)
    {
        MOJOSHADER_symbol const& symbol = parseData->symbols[i];

        ea::string name(symbol.name);
        unsigned reg = symbol.register_index;
        unsigned regCount = symbol.register_count;

        // Check if the parameter is a constant or a texture sampler
        bool isSampler = (name[0] == 's');
        name = name.substr(1);

        if (isSampler)
        {
            // Skip if it's a G-buffer sampler, which are aliases for the standard texture units
            if (reg < MAX_TEXTURE_UNITS)
            {
                if (name != "AlbedoBuffer" && name != "NormalBuffer" && name != "DepthBuffer" && name != "LightBuffer")
                    useTextureUnits_[reg] = true;
            }
        }
        else
        {
            parameters_[StringHash(name)] = ShaderParameter{type_, name, reg, regCount};
        }
    }

    MOJOSHADER_freeParseData(parseData);
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
    file->WriteShort(3);

    file->WriteUInt(parameters_.size());
    for (auto i = parameters_.begin(); i != parameters_.end(); ++i)
    {
        file->WriteString(i->second.name_);
        file->WriteUByte((unsigned char)i->second.register_);
        file->WriteUByte((unsigned char)i->second.regCount_);
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

    unsigned dataSize = byteCode_.size();
    file->WriteUInt(dataSize);
    if (dataSize)
        file->Write(&byteCode_[0], dataSize);
}

}
