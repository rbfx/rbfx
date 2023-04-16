//
// Copyright (c) 2022-2022 the rbfx project.
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
#include "../Container/Str.h"
#include "../Graphics/ShaderDefineArray.h"

namespace Urho3D
{
    struct ShaderMacroExpanderCreationDesc
    {
        ea::string shaderCode_;
        ShaderDefineArray macros_;
    };
    /// @{
    /// Simple Helper class used to resolve all macros in shader code
    /// Their use is very simple. You just needed to follow the steps below:
    /// 1. Create and Fill ShaderMacroExpanderCreationDesc
    /// 2. Create ShaderMacroExpander object. Like: new ShaderMacroExpander(context_, shaderMacroExpanderDesc);
    /// 3. Call Process(), return of this method will be true if everything is alright. If not, return will false and you can get
    /// output calling GetErrorOutput() method.
    /// @}
    class URHO3D_API ShaderMacroExpander {
    public:
        ShaderMacroExpander(ShaderMacroExpanderCreationDesc& desc);
        bool Process(ea::string& outputShaderCode);
    private:
        ShaderMacroExpanderCreationDesc desc_;
        ea::string errorOutput_;
    };
}
