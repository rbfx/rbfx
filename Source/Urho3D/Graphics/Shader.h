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

#pragma once

#include "../Graphics/GraphicsDefs.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

class ShaderVariation;

/// %Shader resource consisting of several shader variations.
class URHO3D_API Shader : public Resource
{
    URHO3D_OBJECT(Shader, Resource);

public:
    /// Construct.
    explicit Shader(Context* context);
    /// Destruct.
    ~Shader() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Return a variation with defines. Separate multiple defines with spaces.
    ShaderVariation* GetVariation(ShaderType type, const ea::string& defines);
    /// Return a variation with defines. Separate multiple defines with spaces.
    ShaderVariation* GetVariation(ShaderType type, const char* defines);

    /// Return either vertex or pixel shader source code.
    const ea::string& GetSourceCode(ShaderType type) const;

    /// Return the latest timestamp of the shader code and its includes.
    unsigned GetTimeStamp() const { return timeStamp_; }

private:
    /// Return hash for given shader defines and current global shader defines.
    unsigned GetShaderDefinesHash(const char* defines) const;
    /// Return the table of shader variations for a specific shader stage.
    ea::unordered_map<unsigned, SharedPtr<ShaderVariation> >& GetVariations(ShaderType type);
    /// Process source code and include files. Return true if successful.
    bool ProcessSource(ea::string& code, Deserializer& source);
    /// Sort the defines and strip extra spaces to prevent creation of unnecessary duplicate shader variations.
    ea::string NormalizeDefines(const ea::string& defines);
    /// Recalculate the memory used by the shader.
    void RefreshMemoryUse();

    /// Encapsulates information for a specific shader stage.
    struct ShaderData
    {
        /// Source code adapted for the shader.
        ea::string sourceCode_;
        /// Shader variations.
        ea::unordered_map<unsigned, SharedPtr<ShaderVariation> > variations_;
    };

    /// Vertex shader source and variations.
    ShaderData vertexShader_;
    /// Pixel shader source and variations.
    ShaderData pixelShader_;
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    /// Geometry shader source and variations.
    ShaderData geometryShader_;
    /// TCS shader source and variations.
    ShaderData hullShader_;
    /// TES shader source and variations.
    ShaderData domainShader_;
    /// Compute shader source and variations.
    ShaderData computeShader_;
#endif
    /// Source code timestamp.
    unsigned timeStamp_;
    /// Number of unique variations so far.
    unsigned numVariations_;
};

}
