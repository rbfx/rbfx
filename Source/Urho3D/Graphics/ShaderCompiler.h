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
#ifdef URHO3D_DILIGENT
#include "../Graphics/GraphicsDefs.h"

namespace Urho3D
{
    enum class ShaderCompilerOutput {
        HLSL=0,
        SPIRV
    };
    struct ShaderCompilerDesc {
        ea::string name_;
        ea::string sourceCode_;
        ea::string entryPoint_;
        ShaderType type_;
        ShaderCompilerOutput output_;
    };
    class URHO3D_API ShaderCompiler {
    public:
        ShaderCompiler(ShaderCompilerDesc desc);
        bool Compile();
        const ea::vector<uint8_t>& GetByteCode() const { return byteCode_; }
        const ea::string& GetCompilerOutput() const { return compilerOutput_; }
    private:
#ifdef WIN32
        bool CompileHLSL();
#endif
        bool CompileSPIRV();

        ShaderCompilerDesc desc_;

        ea::string compilerOutput_;
        ea::vector<uint8_t> byteCode_;
    };
}

#else
#error ShaderCompiler is only available on Diligent mode.
#endif