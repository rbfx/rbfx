// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>

#include <vector>

namespace Urho3D
{

/// SPIR-V shader.
struct SpirVShader
{
    // Use STD vector for easier interop with third parties.
    std::vector<unsigned> bytecode_;
    ea::string compilerOutput_;

    const bool IsValid() const { return !bytecode_.empty(); }
    operator bool() const { return IsValid(); }
};

/// List of supported target shader languages.
enum class TargetShaderLanguage
{
    GLSL_4_1,
    GLSL_ES_3_0,
    HLSL_5_0,
    VULKAN_1_0,
};

/// Array of shader defines: pairs of name and value.
struct URHO3D_API ShaderDefineArray
{
    /// Construct default.
    ShaderDefineArray() = default;

    /// Construct from string.
    explicit ShaderDefineArray(const ea::string& defineString)
    {
        const ea::vector<ea::string> items = defineString.split(' ');
        defines_.reserve(items.size());
        for (const ea::string& item : items)
        {
            const unsigned equalsPos = item.find('=');
            if (equalsPos != ea::string::npos)
                defines_.emplace_back(item.substr(0, equalsPos), item.substr(equalsPos + 1));
            else
                defines_.emplace_back(item, "1");
        }
    }

    /// Append define without value.
    void Append(const ea::string& define) { defines_.emplace_back(define, "1"); }

    /// Append define with value.
    void Append(const ea::string& define, const ea::string& value) { defines_.emplace_back(define, value); }

    /// Return size.
    unsigned Size() const { return defines_.size(); }

    /// Find defines unused in source code.
    ea::vector<ea::string> FindUnused(const ea::string& code) const
    {
        ea::vector<ea::string> unusedDefines;
        for (const auto& define : defines_)
        {
            if (code.find(define.first) == ea::string::npos)
                unusedDefines.push_back(define.first);
        }
        return unusedDefines;
    }

    /// Vector of defines
    ea::vector<ea::pair<ea::string, ea::string>> defines_;
};

/// Return begin iterator of ShaderDefineArray.
inline auto begin(const ShaderDefineArray& defines) { return defines.defines_.begin(); }

/// Return end iterator of ShaderDefineArray.
inline auto end(const ShaderDefineArray& defines) { return defines.defines_.end(); }

}
