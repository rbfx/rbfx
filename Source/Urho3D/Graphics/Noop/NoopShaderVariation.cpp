//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/File.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"

#include "../../DebugNew.h"

namespace Urho3D
{

    void CopyStrippedCode(PODVector<unsigned char>& byteCode, unsigned char* bufData, unsigned bufSize)
    {

    }

    void ShaderVariation::OnDeviceLost()
    {
        // No-op on Direct3D9, shaders are preserved through a device loss & reset
    }

    bool ShaderVariation::Create()
    {
        return true;
    }

    void ShaderVariation::Release()
    {

    }

    void ShaderVariation::SetDefines(const String& defines)
    {
        defines_ = defines;
    }

    bool ShaderVariation::LoadByteCode(const String& binaryShaderName)
    {
        return true;
    }

    bool ShaderVariation::Compile()
    {
        return true;
    }

    void ShaderVariation::ParseParameters(unsigned char* bufData, unsigned bufSize)
    {

    }

    void ShaderVariation::SaveByteCode(const String& binaryShaderName)
    {

    }

}
