// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/RenderAPI/RawShader.h"

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
    : public RawShader
{
public:
    ShaderVariation(Shader* owner, ShaderType type, const ea::string& defines);

    /// Return shader name (as used in resources).
    ea::string GetShaderName() const;
    /// Return full shader variation name with defines.
    ea::string GetShaderVariationName() const;
    /// Return defines used to create the shader.
    const ea::string& GetDefines() const { return defines_; }

private:
    ea::string GetCachedVariationName(ea::string_view extension) const;
    bool NeedShaderTranslation() const;
    bool NeedShaderOptimization() const;

    ea::string PrepareGLSLShaderCode(const ea::string& originalShaderCode) const;
    bool ProcessShaderSource(ea::string_view& translatedSource, const SpirVShader*& translatedSpirv,
        ConstByteSpan& translatedBytecode, ea::string_view originalShaderCode);

    void OnReloaded();
    bool Create();
    bool CompileFromSource();
    bool LoadByteCode(const FileIdentifier& binaryShaderName);
    void SaveByteCode(const FileIdentifier& binaryShaderName);

    /// Cached pointer to Graphics subsystem.
    WeakPtr<Graphics> graphics_;
    /// Source shader.
    WeakPtr<Shader> owner_;
    /// Defines to use when compiling the shader.
    ea::string defines_;
};

} // namespace Urho3D
