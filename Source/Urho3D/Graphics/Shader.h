// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
