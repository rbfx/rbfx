//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../../Graphics/Direct3D11/D3D11ShaderProgram.h"

namespace Urho3D
{

ShaderProgram::ShaderProgram(Graphics* graphics, ShaderVariation* vertexShader, ShaderVariation* pixelShader, ShaderVariation* geometryShader, ShaderVariation* hullShader, ShaderVariation* domainShader)
{
    // Vertex-shader is the authority on constant buffers, HS/DS/GS can add buffers though
    // this is necessary because shader optimization will eliminate unused constant buffers
    ea::vector<unsigned> vertexProcessingBufferSizes(MAX_SHADER_PARAMETER_GROUPS, 0);
    // Create needed constant buffers
    const unsigned* vsBufferSizes = vertexShader->GetConstantBufferSizes();
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    {
        if (vsBufferSizes[i])
            vsConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(VS, i, vsBufferSizes[i]);
        // Initialize buffer sizes
        vertexProcessingBufferSizes[i] = vsBufferSizes[i];
    }

    if (graphics->GetTessellationSupport())
    {
        if (hullShader)
        {
            const unsigned* hullBufferSizes = hullShader->GetConstantBufferSizes();
            for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
            {
                if (hullBufferSizes[i] != 0 && hullBufferSizes[i] != vertexProcessingBufferSizes[i])
                {
                    if (vertexProcessingBufferSizes[i] == 0)
                    {
                        vsConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(VS, i, hullBufferSizes[i]);
                        vertexProcessingBufferSizes[i] = hullBufferSizes[i];
                    }
                    else
                    {
                        URHO3D_LOGERRORF("Hull shader and vertex shader constant buffer mismatch: HS size '%u', VS size '%u' at index %u", hullBufferSizes[i], vsBufferSizes[i], i);
                        URHO3D_LOGINFO("Hull and vertex shaders must use matching constant buffers");
                    }
                }
            }
        }

        if (domainShader)
        {
            const unsigned* domainBufferSizes = domainShader->GetConstantBufferSizes();
            for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
            {
                if (domainBufferSizes[i] != 0 && domainBufferSizes[i] != vertexProcessingBufferSizes[i])
                {
                    if (vertexProcessingBufferSizes[i] == 0)
                    {
                        vsConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(VS, i, domainBufferSizes[i]);
                        vertexProcessingBufferSizes[i] = domainBufferSizes[i];
                    }
                    else
                    {
                        URHO3D_LOGERRORF("Domain shader and vertex shader constant buffer mismatch: DS size '%u', VS size '%u' at index %u", domainBufferSizes[i], vsBufferSizes[i], i);
                        URHO3D_LOGINFO("Domain and vertex shaders must use matching constant buffers");
                    }
                }
            }
        }
    }

    if (geometryShader && graphics->GetGeometryShaderSupport())
    {
        const unsigned* gsBufferSizes = geometryShader->GetConstantBufferSizes();
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        {
            // buffer size can be zero if it unused
            if (gsBufferSizes[i] != vertexProcessingBufferSizes[i] && gsBufferSizes[i] != 0)
            {
                if (vertexProcessingBufferSizes[i] == 0)
                {
                    vsConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(VS, i, gsBufferSizes[i]);
                    vertexProcessingBufferSizes[i] = gsBufferSizes[i];
                }
                else
                {
                    URHO3D_LOGERRORF("Geometry shader and vertex shader constant buffer mismatch: GS size '%u', VS size '%u' at index %u", gsBufferSizes[i], vsBufferSizes[i], i);
                    URHO3D_LOGINFO("Geometry and vertex shaders must use matching constant buffers");
                }
            }
        }
    }

    const unsigned* psBufferSizes = pixelShader->GetConstantBufferSizes();
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    {
        if (psBufferSizes[i])
            psConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(PS, i, psBufferSizes[i]);
    }

    // Copy parameters, add direct links to constant buffers
    const ea::unordered_map<StringHash, ShaderParameter>& vsParams = vertexShader->GetParameters();
    for (const auto& elem : vsParams)
    {
        parameters_[elem.first] = elem.second;
        parameters_[elem.first].bufferPtr_ = vsConstantBuffers_[elem.second.buffer_].Get();
    }

    // Coalesce tessellation stage parameters
    if (hullShader && domainShader && graphics->GetTessellationSupport())
    {
        const auto& hsParams = hullShader->GetParameters();
        for (const auto& elem : hsParams)
        {
            if (!parameters_.contains(elem.first))
            {
                parameters_[elem.first] = elem.second;
                parameters_[elem.first].bufferPtr_ = vsConstantBuffers_[elem.second.buffer_].Get();
            }
        }

        const auto& dsParams = domainShader->GetParameters();
        for (const auto& elem : dsParams)
        {
            if (!parameters_.contains(elem.first))
            {
                parameters_[elem.first] = elem.second;
                parameters_[elem.first].bufferPtr_ = vsConstantBuffers_[elem.second.buffer_].Get();
            }
        }
    }

    // Coalesce geometry shader paramters
    if (geometryShader && graphics->GetGeometryShaderSupport())
    {
        const auto& gsParams = geometryShader->GetParameters();
        for (const auto& elem : gsParams)
        {
            if (!parameters_.contains(elem.first))
            {
                parameters_[elem.first] = elem.second;
                parameters_[elem.first].bufferPtr_ = vsConstantBuffers_[elem.second.buffer_].Get();
            }
        }
    }

    const ea::unordered_map<StringHash, ShaderParameter>& psParams = pixelShader->GetParameters();
    for (const auto& elem : psParams)
    {
        parameters_[elem.first] = elem.second;
        parameters_[elem.first].bufferPtr_ = psConstantBuffers_[elem.second.buffer_].Get();
    }

    // Optimize shader parameter lookup by rehashing to next power of two
    parameters_.rehash(NextPowerOfTwo(parameters_.size()));

}

}