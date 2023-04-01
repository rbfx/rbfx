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


#include "../../DebugNew.h"

#include "./DiligentLookupSettings.h"

#include "../../Graphics/ShaderCompiler.h"
#include "../../Graphics/ShaderProcessor.h"

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
static const char* sShaderFileID = "UDSH"; // UDSHD => Urho Diligent Shader

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

    if (!LoadByteCode(binaryShaderName))
    {
        // Compile shader if don't have valid bytecode
        if (!Compile())
            return false;
        // Save the bytecode after successful compile, but not if the source is from a package
        if (owner_->GetTimeStamp())
            SaveByteCode(binaryShaderName);
    }

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
}

void ShaderVariation::SetDefines(const ea::string& defines)
{
    defines_ = defines;
}

ea::string ShaderVariation::GetEntryPoint() {
    const char* entryPoint = nullptr;
    if (owner_->IsGLSL()) {
        entryPoint = "main";
    }
    else {
        switch (type_)
        {
        case Urho3D::VS:
            entryPoint = "VS";
            break;
        case Urho3D::PS:
            entryPoint = "PS";
            break;
        case Urho3D::GS:
            entryPoint = "GS";
            break;
        case Urho3D::HS:
            entryPoint = "HS";
            break;
        case Urho3D::DS:
            entryPoint = "DS";
            break;
        case Urho3D::CS:
            entryPoint = "CS";
            break;
        default:
            URHO3D_LOGERROR("Unsupported Shader Type {}", type_);
            assert(0);
            break;
        }
    }
    return ea::string(entryPoint);
}
bool ShaderVariation::Compile()
{
    using namespace Diligent;
    RenderBackend renderBackend = GetGraphics()->GetRenderBackend();
    // Set the code, defines, entrypoint, profile and flags according to the shader being compiled
    ea::string sourceCode;
    const char* entryPoint = nullptr;
    SHADER_TYPE shaderType = SHADER_TYPE_UNKNOWN;
    ShaderProcessorDesc processorDesc = {};
    processorDesc.macros_ = ShaderDefineArray{ defines_ };
    processorDesc.name_ = name_;
    processorDesc.entryPoint_ = GetEntryPoint();

    processorDesc.type_ = type_;
    switch (type_)
    {
    case Urho3D::VS:
    {
        processorDesc.macros_.Append("COMPILEVS");
        shaderType = SHADER_TYPE_VERTEX;
    }
        break;
    case Urho3D::PS:
    {
        processorDesc.macros_.Append("COMPILEPS");
        shaderType = SHADER_TYPE_PIXEL;
    }
        break;
    case Urho3D::GS:
        processorDesc.macros_.Append("COMPILEGS");
        shaderType = SHADER_TYPE_GEOMETRY;
        break;
    case Urho3D::HS:
        processorDesc.macros_.Append("COMPILEHS");
        shaderType = SHADER_TYPE_HULL;
        break;
    case Urho3D::DS:
        processorDesc.macros_.Append("COMPILEDS");
        shaderType = SHADER_TYPE_DOMAIN;
        break;
    case Urho3D::CS:
        processorDesc.macros_.Append("COMPILECS");
        shaderType = SHADER_TYPE_COMPUTE;
        break;
    }

    processorDesc.macros_.Append("MAXBONES", Format("{}", Graphics::GetMaxBones()));
    processorDesc.macros_.Append("D3D11");
    processorDesc.macros_.Append("DILIGENT");


    // Convert shader source code if GLSL
    static thread_local ea::string convertedShaderSourceCode;
    if (owner_->IsGLSL())
    {
        processorDesc.macros_.Append("DESKTOP_GRAPHICS");
        processorDesc.macros_.Append("GL3");
        processorDesc.language_ = ShaderLang::GLSL;
        processorDesc.entryPoint_ = "main";
        if (renderBackend == RENDER_GL)
            processorDesc.optimizeCode_ = true;
    }
    else
    {
        processorDesc.language_ = ShaderLang::HLSL;
        sourceCode = owner_->GetSourceCode(type_);
    }

    processorDesc.sourceCode_ = owner_->GetSourceCode(type_);

    ShaderProcessor processor(processorDesc);
    if (!processor.Execute()) {
        URHO3D_LOGERROR("Failed to pre-process shader " + GetFullName());
        URHO3D_LOGERROR(processor.GetCompilerOutput());
        return false;
    }

    ShaderCreateInfo shaderCI;
#ifdef URHO3D_DEBUG
    shaderCI.Desc.Name = name_.c_str();
#endif
    shaderCI.Desc.ShaderType = shaderType;
    shaderCI.Desc.UseCombinedTextureSamplers = false;
    shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    shaderCI.EntryPoint = processorDesc.entryPoint_.c_str();

    ShaderCompilerDesc compilerDesc;
    compilerDesc.entryPoint_ = processorDesc.entryPoint_;
    compilerDesc.name_ = name_;
    compilerDesc.sourceCode_ = processor.GetOutputCode();
    compilerDesc.type_ = type_;

    RefCntAutoPtr<IShader> shader;

    switch (renderBackend)
    {
    case Urho3D::RENDER_D3D11:
    case Urho3D::RENDER_D3D12:
    {
        compilerDesc.output_ = ShaderCompilerOutput::DXC;
        ShaderCompiler compiler(compilerDesc);
        if (!compiler.Compile()) {
            URHO3D_LOGERROR("Failed to compile shader " + GetFullName());
            URHO3D_LOGERROR(compiler.GetCompilerOutput());
            return false;
        }

        byteCode_ = compiler.GetByteCode();
        byteCodeType_ = ShaderByteCodeType::HLSL;
        shaderCI.ByteCode = byteCode_.data();
        shaderCI.ByteCodeSize = byteCode_.size();
        if (!compiler.GetCompilerOutput().empty())
            URHO3D_LOGWARNING(compiler.GetCompilerOutput());
        graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);
    }
        break;
    case Urho3D::RENDER_GL:
    {
        size_t versionIdx = compilerDesc.sourceCode_.find("#version 450");
        // Include GL version if not exists
        if (versionIdx == ea::string::npos)
            compilerDesc.sourceCode_ = "#version 450\n" + compilerDesc.sourceCode_;
        compilerDesc.output_ = ShaderCompilerOutput::GLSL;
        ShaderCompiler compiler(compilerDesc);
        if (!compiler.Compile()) {
            URHO3D_LOGERROR("Failed to compile shader " + GetFullName());
            return false;
        }
        byteCode_ = compiler.GetByteCode();
        byteCodeType_ = ShaderByteCodeType::RAW;
        const char* sourceCode = reinterpret_cast<const char*>(byteCode_.data());
        shaderCI.Source = sourceCode;
        shaderCI.SourceLength = byteCode_.size();
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;

        graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);
    }
        break;
    case Urho3D::RENDER_VULKAN:
    {
        compilerDesc.output_ = ShaderCompilerOutput::SPIRV;
        ShaderCompiler compiler(compilerDesc);
        if (!compiler.Compile()) {
            URHO3D_LOGERROR("Failed to compile shader " + GetFullName());
            return false;
        }
        byteCodeType_ = ShaderByteCodeType::SPIRV;

        byteCode_ = compiler.GetByteCode();
        shaderCI.ByteCode = byteCode_.data();
        shaderCI.ByteCodeSize = byteCode_.size();

        if (!compiler.GetCompilerOutput().empty())
            URHO3D_LOGWARNING(compiler.GetCompilerOutput());
        graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);
    }
        break;
    case Urho3D::RENDER_METAL:
        // TODO: Implement Metal Backend
        URHO3D_LOGERROR("Not supported metal backend.");
        assert(0);
        break;
    }

    assert(shader);
    if (!shader) {
        URHO3D_LOGDEBUG("Error has ocurred at Shader Object creation " + GetFullName());
        return false;
    }
    object_ = shader;

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

    for (uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i)
        useTextureUnits_[i] = processor.IsUsedTextureUnit((TextureUnit)i);

    parameters_ = processor.GetShaderParameters();
    vertexElements_ = processor.GetVertexElements();

    CalculateConstantBufferSizes();

    return true;
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
    if (!file || file->ReadFileID() != ea::string(sShaderFileID))
    {
        URHO3D_LOGERROR("{} is not a valid shader bytecode file", binaryShaderName.ToUri());
        return false;
    }

    RenderBackend renderBackend = GetGraphics()->GetRenderBackend();


    ShaderType shaderType = (ShaderType)file->ReadUShort();
    assert(shaderType == type_);
    byteCodeType_ = (ShaderByteCodeType)file->ReadUByte();

    switch (renderBackend)
    {
    case Urho3D::RENDER_D3D11:
    case Urho3D::RENDER_D3D12:
    case Urho3D::RENDER_VULKAN:
        if (byteCodeType_ == ShaderByteCodeType::RAW)
            return false;
        break;
    case Urho3D::RENDER_GL:
        if (byteCodeType_ != ShaderByteCodeType::RAW)
            return false;
        break;
        break;
    case Urho3D::RENDER_METAL:
        URHO3D_LOGWARNING("Metal backend shader is not implemented.");
        break;
    }

    unsigned vertexElements = file->ReadUInt();
    vertexElements_.resize(vertexElements);
    for (unsigned i = 0; i < vertexElements; ++i) {
        vertexElements_[i].type_ = (VertexElementType)file->ReadUByte();
        vertexElements_[i].semantic_ = (VertexElementSemantic)file->ReadUByte();
        vertexElements_[i].index_ = file->ReadUByte();
        vertexElements_[i].location_ = file->ReadUByte();
        vertexElements_[i].offset_ = file->ReadUInt();
    }

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

        ea::string entryPoint = GetEntryPoint();
        ShaderCreateInfo shaderCI;
#ifdef URHO3D_DEBUG
        shaderCI.Desc.Name = name_.c_str();
#endif
        shaderCI.Desc.UseCombinedTextureSamplers = false;
        shaderCI.Desc.ShaderType = DiligentShaderType[type_];
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        shaderCI.EntryPoint = entryPoint.c_str();

        if (byteCodeType_ == ShaderByteCodeType::RAW) {
            shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
            shaderCI.Source = (const char*)byteCode_.data();
            shaderCI.SourceLength = byteCode_.size();
        }
        else {
            shaderCI.ByteCode = byteCode_.data();
            shaderCI.ByteCodeSize = byteCode_.size();
            if (byteCodeType_ == ShaderByteCodeType::HLSL)
                shaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
        }

        RefCntAutoPtr<IShader> shader;
        graphics_->GetImpl()->GetDevice()->CreateShader(shaderCI, &shader);
        if (!shader) {
            URHO3D_LOGERROR("Failed to create shader object for {}", GetFullName());
            return false;
        }

        object_ = shader;

        CalculateConstantBufferSizes();
        return true;
    }
    else
    {
        URHO3D_LOGERROR("{} has zero length bytecode", binaryShaderName.ToUri());
        return false;
    }
}

void ShaderVariation::SaveByteCode(const FileIdentifier& binaryShaderName)
{
   VirtualFileSystem* vfs = owner_->GetSubsystem<VirtualFileSystem>();

    auto file = vfs->OpenFile(binaryShaderName, FILE_WRITE);
    if (!file)
        return;

    // UDSHD: Urho Diligent Shader
    file->WriteFileID(sShaderFileID);
    file->WriteUShort((unsigned short)type_);
    file->WriteUByte((unsigned char)byteCodeType_);

    file->WriteUInt(vertexElements_.size());
    for (auto i = vertexElements_.begin(); i != vertexElements_.end(); ++i) {
        file->WriteUByte(i->type_);
        file->WriteUByte(i->semantic_);
        file->WriteUByte(i->index_);
        file->WriteUByte(i->location_);
        file->WriteUInt(i->offset_);
    }

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
