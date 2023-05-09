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

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Graphics/GPUObject.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/RenderAPI/CompiledShaderVariation.h"

#include <EASTL/unordered_map.h>

namespace Diligent
{
struct IShader;
}

namespace Urho3D
{

class Shader;
struct FileIdentifier;
struct SpirVShader;

/// Vertex or pixel shader on the GPU.
class URHO3D_API ShaderVariation
    : public RefCounted
    , public GPUObject
{
public:
    /// Construct.
    ShaderVariation(Shader* owner, ShaderType type);
    /// Destruct.
    ~ShaderVariation() override;

    /// Mark the GPU resource destroyed on graphics context destruction.
    void OnDeviceLost() override;
    /// Release the shader.
    void Release() override;

    /// Compile the shader. Return true if successful.
    bool Create();
    /// Set name.
    void SetName(const ea::string& name);
    /// Set defines.
    void SetDefines(const ea::string& defines);

    /// Return the owner resource.
    Shader* GetOwner() const;

    /// Return shader type.
    ShaderType GetShaderType() const { return type_; }

    /// Return shader name.
    const ea::string& GetName() const { return name_; }

    /// Return full shader name.
    ea::string GetFullName() const { return name_ + "(" + defines_ + ")"; }

    /// Return defines.
    const ea::string& GetDefines() const { return defines_; }

    /// Return compile error/warning string.
    /// TODO(diligent): Revisit this getter
    const ea::string& GetCompilerOutput() const { return compilerOutput_; }

    const VertexShaderAttributeVector& GetVertexShaderAttributes() const { return compiled_.vertexAttributes_; }

private:
    ea::string GetCachedVariationName(ea::string_view extension) const;
    bool NeedShaderTranslation() const;
    bool NeedShaderOptimization() const;

    ea::string PrepareGLSLShaderCode(const ea::string& originalShaderCode) const;
    bool ProcessShaderSource(ea::string_view& translatedSource, const SpirVShader*& translatedSpirv,
        ConstByteSpan& translatedBytecode, ea::string_view originalShaderCode);
    Diligent::IShader* CreateShader(const CompiledShaderVariation& compiledShader) const;

    bool Compile();
    bool LoadByteCode(const FileIdentifier& binaryShaderName);
    void SaveByteCode(const FileIdentifier& binaryShaderName);

    /// Shader this variation belongs to.
    WeakPtr<Shader> owner_;
    /// Shader type.
    ShaderType type_{};

    /// Shader name.
    ea::string name_;
    /// Defines to use in compiling.
    ea::string defines_;
    /// Shader compile error string.
    ea::string compilerOutput_;

    /// Compiled shader info.
    CompiledShaderVariation compiled_;
};

} // namespace Urho3D
