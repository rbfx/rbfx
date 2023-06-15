//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/ShaderProgramReflection.h"

#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace Urho3D
{

void ShaderProgramReflection::AddUniformBuffer(ShaderParameterGroup group, ea::string_view internalName, unsigned size)
{
    uniformBuffers_[group] = UniformBufferReflection{size, 0u, ea::string{internalName}};
}

void ShaderProgramReflection::AddUniform(StringHash name, ShaderParameterGroup group, unsigned offset, unsigned size)
{
    uniforms_.emplace(name, UniformReflection{ group, offset, size });
}

void ShaderProgramReflection::AddShaderResource(StringHash name, ea::string_view internalName)
{
    shaderResources_.emplace(name, ShaderResourceReflection{ea::string{internalName}});
}

void ShaderProgramReflection::RecalculateLayoutHash()
{
    for (UniformBufferReflection& uniformBuffer : uniformBuffers_)
    {
        uniformBuffer.hash_ = 0;
        if (uniformBuffer.size_ != 0)
            CombineHash(uniformBuffer.hash_, uniformBuffer.size_);
    }

    for (const auto& [nameHash, uniform] : uniforms_)
    {
        UniformBufferReflection& uniformBuffer = uniformBuffers_[uniform.group_];
        CombineHash(uniformBuffer.hash_, nameHash.Value());
        CombineHash(uniformBuffer.hash_, uniform.offset_);
        CombineHash(uniformBuffer.hash_, uniform.size_);

        if (uniformBuffer.hash_ == 0)
            uniformBuffer.hash_ = 1;
    }
}

void ShaderProgramReflection::ConnectToShaderVariables(Diligent::IShaderResourceBinding* binding)
{
    // TODO(diligent): Revisit this place? Do we want to reuse it for compute shaders?
    const unsigned maxShaderType = CS;
    for (UniformBufferReflection& uniformBuffer : uniformBuffers_)
    {
        if (uniformBuffer.size_ == 0)
            continue;

        for (unsigned i = 0; i < maxShaderType; ++i)
        {
            const Diligent::SHADER_TYPE shaderType = ToInternalShaderType(static_cast<ShaderType>(i));
            Diligent::IShaderResourceVariable* shaderVariable =
                binding->GetVariableByName(shaderType, uniformBuffer.internalName_.c_str());
            if (shaderVariable)
                uniformBuffer.variables_.push_back(shaderVariable);
        }
    }

    for (auto& [nameHash, resource] : shaderResources_)
    {
        for (unsigned i = 0; i < maxShaderType; ++i)
        {
            const Diligent::SHADER_TYPE shaderType = ToInternalShaderType(static_cast<ShaderType>(i));
            Diligent::IShaderResourceVariable* shaderVariable =
                binding->GetVariableByName(shaderType, resource.internalName_.c_str());
            if (shaderVariable)
            {
                resource.variable_ = shaderVariable;
                break;
            }
        }
    }
}

}
