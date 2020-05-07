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

#include "../Precompiled.h"

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Shader.h"
#include "../Graphics/ShaderVariation.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"

#include "../DebugNew.h"

namespace Urho3D
{

void CommentOutFunction(ea::string& code, const ea::string& signature)
{
    unsigned startPos = code.find(signature);
    unsigned braceLevel = 0;
    if (startPos == ea::string::npos)
        return;

    code.insert(startPos, "/*");

    for (unsigned i = startPos + 2 + signature.length(); i < code.length(); ++i)
    {
        if (code[i] == '{')
            ++braceLevel;
        else if (code[i] == '}')
        {
            --braceLevel;
            if (braceLevel == 0)
            {
                code.insert(i + 1, "*/");
                return;
            }
        }
    }
}

void ExtractShaderSource(ea::string& code, ShaderType shaderType)
{
    static const ea::pair<ShaderType, const char*> mainNamesMapping[] = {
        { VS, "void VS(" },
        { PS, "void PS(" },
        { GS, "void GS(" },
        { HS, "void HS(" },
        { DS, "void DS(" },
        { CS, "void CS(" }
    };

    for (const auto& item : mainNamesMapping)
    {
        // Comment out the rest of mains
        if (item.first != shaderType)
            CommentOutFunction(code, item.second);
#ifdef URHO3D_OPENGL
        else
            code.replace(item.second, "void main(");
        // On OpenGL, rename main
#endif
    }
}

void FreeVariations(const ea::unordered_map<unsigned, SharedPtr<ShaderVariation>>& variations)
{
    for (const auto& item : variations)
        item.second->Release();
}

Shader::Shader(Context* context) :
    Resource(context),
    timeStamp_(0),
    numVariations_(0)
{
    RefreshMemoryUse();
}

Shader::~Shader()
{
    if (!context_.Expired())
    {
        auto* cache = GetSubsystem<ResourceCache>();
        if (cache)
            cache->ResetDependencies(this);
    }
}

void Shader::RegisterObject(Context* context)
{
    context->RegisterFactory<Shader>();
}

bool Shader::BeginLoad(Deserializer& source)
{
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return false;

    // Load the shader source code and resolve any includes
    timeStamp_ = 0;
    ea::string shaderCode;
    if (!ProcessSource(shaderCode, source))
        return false;

    // Comment out the unneeded shader function
    vertexShader_.sourceCode_ = shaderCode;
    pixelShader_.sourceCode_ = shaderCode;

    ExtractShaderSource(vertexShader_.sourceCode_, VS);
    ExtractShaderSource(pixelShader_.sourceCode_, PS);

#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    geometryShader_.sourceCode_ = shaderCode;
    hullShader_.sourceCode_ = shaderCode;
    domainShader_.sourceCode_ = shaderCode;
    computeShader_.sourceCode_ = shaderCode;

    ExtractShaderSource(geometryShader_.sourceCode_, GS);
    ExtractShaderSource(hullShader_.sourceCode_, HS);
    ExtractShaderSource(domainShader_.sourceCode_, DS);
    ExtractShaderSource(computeShader_.sourceCode_, VS);
#endif

    RefreshMemoryUse();
    return true;
}

bool Shader::EndLoad()
{
    FreeVariations(vertexShader_.variations_);
    FreeVariations(pixelShader_.variations_);

#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    FreeVariations(geometryShader_.variations_);
    FreeVariations(hullShader_.variations_);
    FreeVariations(domainShader_.variations_);
    FreeVariations(computeShader_.variations_);
#endif

    return true;
}

ShaderVariation* Shader::GetVariation(ShaderType type, const ea::string& defines)
{
    return GetVariation(type, defines.c_str());
}

const ea::string& Shader::GetSourceCode(ShaderType type) const
{
    switch (type)
{
    case VS:
        return vertexShader_.sourceCode_;
    case PS:
        return pixelShader_.sourceCode_;
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    case GS:
        return geometryShader_.sourceCode_;
    case HS:
        return hullShader_.sourceCode_;
    case DS:
        return domainShader_.sourceCode_;
    case CS:
        return computeShader_.sourceCode_;
#endif
    }
    return vertexShader_.sourceCode_;
}

ea::unordered_map<unsigned, SharedPtr<ShaderVariation> >& Shader::GetVariations(ShaderType type)
{
    switch (type)
    {
    case VS:
        return vertexShader_.variations_;
    case PS:
        return pixelShader_.variations_;
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    case GS:
        return geometryShader_.variations_;
    case HS:
        return hullShader_.variations_;
    case DS:
        return domainShader_.variations_;
    case CS:
        return computeShader_.variations_;
#endif
    }
    return vertexShader_.variations_;
    }

ShaderVariation* Shader::GetVariation(ShaderType type, const char* defines)
{
    const unsigned definesHash = GetShaderDefinesHash(defines);
    ea::unordered_map<unsigned, SharedPtr<ShaderVariation> >& variations = GetVariations(type);

    auto i = variations.find(definesHash);
    if (i == variations.end())
    {
        // If shader not found, normalize the defines (to prevent duplicates) and check again. In that case make an alias
        // so that further queries are faster
        const ea::string normalizedDefines = NormalizeDefines(defines);
        const unsigned normalizedHash = GetShaderDefinesHash(normalizedDefines.c_str());

        i = variations.find(normalizedHash);
        if (i != variations.end())
            variations.insert(ea::make_pair(definesHash, i->second));
        else
        {
            // No shader variation found. Create new
            i = variations.insert(ea::make_pair(normalizedHash, SharedPtr<ShaderVariation>(new ShaderVariation(this, type)))).first;
            if (definesHash != normalizedHash)
                variations.insert(ea::make_pair(definesHash, i->second));

            Graphics* graphics = context_->GetGraphics();
            i->second->SetName(GetFileName(GetName()));
            i->second->SetDefines(graphics->GetGlobalShaderDefines() + " " + normalizedDefines);
            ++numVariations_;
            RefreshMemoryUse();
        }
    }

    return i->second;
}

unsigned Shader::GetShaderDefinesHash(const char* defines) const
{
    Graphics* graphics = context_->GetGraphics();
    unsigned definesHash = StringHash(defines).Value();
    CombineHash(definesHash, graphics->GetGlobalShaderDefinesHash().Value());
    return definesHash;
}

bool Shader::ProcessSource(ea::string& code, Deserializer& source)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // If the source if a non-packaged file, store the timestamp
    auto* file = dynamic_cast<File*>(&source);
    if (file && !file->IsPackaged())
    {
        auto* fileSystem = GetSubsystem<FileSystem>();
        ea::string fullName = cache->GetResourceFileName(file->GetName());
        unsigned fileTimeStamp = fileSystem->GetLastModifiedTime(fullName);
        if (fileTimeStamp > timeStamp_)
            timeStamp_ = fileTimeStamp;
    }

    // Store resource dependencies for includes so that we know to reload if any of them changes
    if (source.GetName() != GetName())
        cache->StoreResourceDependency(this, source.GetName());

    while (!source.IsEof())
    {
        ea::string line = source.ReadLine();

        if (line.starts_with("#include"))
        {
            ea::string includeFileName = GetPath(source.GetName()) + line.substr(9).replaced("\"", "").trimmed();

            SharedPtr<File> includeFile = cache->GetFile(includeFileName);
            if (!includeFile)
                return false;

            // Add the include file into the current code recursively
            if (!ProcessSource(code, *includeFile))
                return false;
        }
        else
        {
            code += line;
            code += "\n";
        }
    }

    // Finally insert an empty line to mark the space between files
    code += "\n";

    return true;
}

ea::string Shader::NormalizeDefines(const ea::string& defines)
{
    ea::vector<ea::string> definesVec = defines.to_upper().split(' ');
    ea::quick_sort(definesVec.begin(), definesVec.end());
    return ea::string::joined(definesVec, " ");
}

void Shader::RefreshMemoryUse()
{
    unsigned sourcesSize = vertexShader_.sourceCode_.length() + pixelShader_.sourceCode_.length();
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    sourcesSize += geometryShader_.sourceCode_.length();
    sourcesSize += hullShader_.sourceCode_.length() + domainShader_.sourceCode_.length();
#endif
    SetMemoryUse((unsigned)(sizeof(Shader) + sourcesSize + numVariations_ * sizeof(ShaderVariation)));
}

}
