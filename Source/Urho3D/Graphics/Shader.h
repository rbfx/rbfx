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

#include "../Graphics/GraphicsDefs.h"
#include "../IO/ScanFlags.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

class ShaderVariation;

/// %Shader resource consisting of several shader variations.
class URHO3D_API Shader : public Resource
{
    URHO3D_OBJECT(Shader, Resource);

public:
    /// Signals that shader source code was reloaded.
    Signal<void()> OnReloaded;

    /// Construct.
    explicit Shader(Context* context);
    /// Destruct.
    ~Shader() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Return a variation with defines. Separate multiple defines with spaces.
    ShaderVariation* GetVariation(ShaderType type, ea::string_view defines);

    /// Return shader name.
    ea::string GetShaderName() const;
    /// Return either vertex or pixel shader source code.
    const ea::string& GetSourceCode() const { return sourceCode_; }
    /// Return the latest timestamp of the shader code and its includes.
    FileTime GetTimeStamp() const { return timeStamp_; }

    /// Return global list of shader files.
    static ea::string GetShaderFileList();

private:
    using ShaderVariationKey = ea::pair<ShaderType, StringHash>;

    /// Process source code and include files. Return true if successful.
    void ProcessSource(ea::string& code, FileTime& timeStamp, Deserializer& source);
    /// Recalculate the memory used by the shader.
    void RefreshMemoryUse();

    /// Shader source code.
    ea::string sourceCode_;
    /// Timestamp of source code file(s).
    FileTime timeStamp_{};
    /// Shader variations.
    ea::unordered_map<ShaderVariationKey, SharedPtr<ShaderVariation>> variations_;
    /// Number of unique variations so far.
    unsigned numVariations_{};
    /// Mapping of shader files for error reporting.
    static ea::unordered_map<ea::string, unsigned> fileToIndexMapping;
};

}
