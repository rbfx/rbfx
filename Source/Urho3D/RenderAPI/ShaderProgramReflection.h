// Copyright (c) 2020-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/RefCounted.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>
#include <EASTL/unordered_map.h>

#include <initializer_list>

namespace Diligent
{

struct IShader;
struct IShaderResourceBinding;
struct IShaderResourceVariable;
struct ShaderCodeBufferDesc;
struct ShaderCodeVariableDesc;
struct ShaderResourceDesc;

} // namespace Diligent

namespace Urho3D
{

/// Description of uniform buffer used by the shader program.
struct UniformBufferReflection
{
    unsigned size_{};
    unsigned hash_{};
    ea::string internalName_;
    ea::fixed_vector<Diligent::IShaderResourceVariable*, MAX_SHADER_TYPES> variables_;
};
using UniformBufferReflectionArray = ea::array<UniformBufferReflection, MAX_SHADER_PARAMETER_GROUPS>;

/// Description of specific uniform in the shader program.
struct UniformReflection
{
    ShaderParameterGroup group_{};
    unsigned offset_{};
    unsigned size_{};
};
using ShaderParameterReflectionMap = ea::unordered_map<StringHash, UniformReflection>;

/// Description of resource used by the shader program, excluding uniform buffers and samplers.
struct ShaderResourceReflection
{
    ea::string internalName_;
    Diligent::IShaderResourceVariable* variable_{};
};
using ShaderResourceReflectionMap = ea::unordered_map<StringHash, ShaderResourceReflection>;

/// Description of shader program: uniform buffers, resources, etc.
class URHO3D_API ShaderProgramReflection : public RefCounted
{
public:
    /// Create reflection from shaders.
    /// @note It works only for GAPIs that can provide per-shader reflection data (this is everyone but old OpenGL).
    explicit ShaderProgramReflection(ea::span<Diligent::IShader* const> shaders);
    /// Create reflection from linked OpenGL shader program.
    explicit ShaderProgramReflection(unsigned programObject);

    /// Getters.
    /// @{
    const UniformBufferReflection* GetUniformBuffer(ShaderParameterGroup group) const
    {
        if (group >= MAX_SHADER_PARAMETER_GROUPS || uniformBuffers_[group].size_ == 0)
            return nullptr;
        return &uniformBuffers_[group];
    }

    const UniformReflection* GetUniform(StringHash name) const
    {
        const auto iter = uniforms_.find(name);
        if (iter == uniforms_.end())
            return nullptr;
        return &iter->second;
    }

    const ShaderResourceReflection* GetShaderResource(StringHash name) const
    {
        const auto iter = shaderResources_.find(name);
        if (iter == shaderResources_.end())
            return nullptr;
        return &iter->second;
    }

    const ShaderResourceReflection* GetUnorderedAccessView(StringHash name) const
    {
        const auto iter = unorderedAccessViews_.find(name);
        if (iter == unorderedAccessViews_.end())
            return nullptr;
        return &iter->second;
    }

    const ShaderParameterReflectionMap& GetUniforms() const { return uniforms_; }
    const ShaderResourceReflectionMap& GetShaderResources() const { return shaderResources_; }
    const ShaderResourceReflectionMap& GetUnorderedAccessViews() const { return unorderedAccessViews_; }
    /// @}

    void ConnectToShaderVariables(PipelineStateType pipelineType, Diligent::IShaderResourceBinding* binding);

private:
    void ReflectShader(Diligent::IShader* shader);
    void ReflectUniformBuffer(
        const Diligent::ShaderResourceDesc& resourceDesc, const Diligent::ShaderCodeBufferDesc& bufferDesc);

    void AddUniformBuffer(ShaderParameterGroup group, ea::string_view internalName, unsigned size);
    void AddUniform(ea::string_view name, ShaderParameterGroup group, unsigned offset, unsigned size);
    void AddUniform(ShaderParameterGroup group, const Diligent::ShaderCodeVariableDesc& desc);
    void AddShaderResource(StringHash name, ea::string_view internalName);
    void AddUnorderedAccessView(StringHash name, ea::string_view internalName);
    void RecalculateUniformHash();

private:
    UniformBufferReflectionArray uniformBuffers_;
    ShaderParameterReflectionMap uniforms_;
    ShaderResourceReflectionMap shaderResources_;
    ShaderResourceReflectionMap unorderedAccessViews_;
};

} // namespace Urho3D
