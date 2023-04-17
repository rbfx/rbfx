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

#if defined(URHO3D_DILIGENT) && defined(URHO3D_SPIRV)

#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/ShaderDefineArray.h"
#include "../Graphics/ShaderVariation.h"
#include "../Core/Variant.h"

namespace Urho3D
{
    enum class ShaderLang {
        GLSL=0,
        HLSL
    };
    struct ShaderProcessorDesc {
        ea::string name_;
        ea::string sourceCode_;
        ea::string entryPoint_;
        ShaderType type_;
        ShaderDefineArray macros_;
        ShaderLang language_;
        bool optimizeCode_{ false };
    };

    class URHO3D_API ShaderProcessor {
    public:
        ShaderProcessor(ShaderProcessorDesc desc);
        /// @{
        /// Execute Shader Processor.
        /// if execute returns true, Processed HLSL code, Input Layout, Constant Buffers slots,
        /// textures slots and Shader parameters will be available.
        /// @}
        bool Execute();
        /// @{
        /// returns processed HLSL code.
        /// @}
        const ea::string& GetOutputCode() const { return outputCode_; }
        /// @{
        /// returns compiler output, this can be errors or warning.
        /// @}
        const ea::string& GetCompilerOutput() const { return compilerOutput_; }
        /// @{
        /// check if Constant Buffer is used on this shader code.
        /// @}
        bool IsUsedCBuffer(ShaderParameterGroup grp) { return cbufferSlots_[grp]; }
        /// @{
        /// check if Texture Unit is used on this shader code.
        /// @}
        bool IsUsedTextureUnit(TextureUnit unit) { return textureSlots_[unit]; }
        /// @{
        /// returns used vertex elements, this value will be empty if ShaderType used on
        /// constructor is not VS.
        /// @}
        const ea::vector<VertexElement> GetVertexElements() const { return vertexElements_; }
        /// @{
        /// returns collected parameters from constant buffers.
        /// @}
        const ea::unordered_map<StringHash, ShaderParameter> GetShaderParameters() const { return parameters_; }
    private:
        bool ProcessHLSL();
        bool ProcessGLSL();

#ifdef WIN32
        bool ReflectHLSL(uint8_t* byteCode, size_t byteCodeLength,
            StringVector& samplers, ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout);
#endif
        bool CompileGLSL(ea::vector<unsigned>& byteCode);
        bool ReflectGLSL(const void* byteCode, size_t byteCodeSize,
             StringVector& samplers, ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout);
        void RemapHLSLInputLayout(ea::string& sourceCode, const ea::vector<ea::pair<unsigned, VertexElementSemantic>>& inputLayout);
        void RemapHLSLSamplers(ea::string& sourceCode, const StringVector& samplers);

        ShaderProcessorDesc desc_;
        /// Processed HLSL code. Used by Diligent
        ea::string outputCode_;
        ea::string compilerOutput_;

        /// Used Input Layout
        ea::vector<VertexElement> vertexElements_;

        /// Texture and Constant Buffer used slots.
        ea::array<bool, MAX_TEXTURE_UNITS> textureSlots_;
        ea::array<bool, MAX_SHADER_PARAMETER_GROUPS> cbufferSlots_;

        /// Collected Shader Parameters where key is cbuffer name.
        ea::unordered_map<StringHash, ShaderParameter> parameters_;
    };
}

#else
#ifndef URHO3D_DILIGENT
#error ShaderProcessor is only available on Diligent mode.
#endif
#ifndef URHO3D_SPIRV
#error ShaderProcessor requires URHO3D_SPIRV feature.
#endif
#endif
