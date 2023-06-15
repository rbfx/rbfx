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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RawShader.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

RawShader::RawShader(Context* context, ShaderType type)
    : Object(context)
    , DeviceObject(context)
{
    bytecode_.type_ = type;
}

RawShader::RawShader(Context* context, ShaderBytecode bytecode)
    : RawShader(context, bytecode.type_)
{
    CreateFromBinary(bytecode);
}

RawShader::~RawShader()
{
    DestroyGPU();
}

void RawShader::Invalidate()
{
    DestroyGPU();
}

void RawShader::Restore()
{
    if (!handle_)
        CreateGPU();
}

void RawShader::Destroy()
{
    DestroyGPU();
}

void RawShader::CreateFromBinary(ShaderBytecode bytecode)
{
    URHO3D_ASSERT(bytecode.type_ == bytecode_.type_);
    bytecode_ = ea::move(bytecode);
    CreateGPU();
    OnReloaded(this);
}

void RawShader::CreateGPU()
{
    DestroyGPU();

    Diligent::ShaderCreateInfo createInfo;
#ifdef URHO3D_DEBUG
    createInfo.Desc.Name = GetDebugName().c_str();
#endif
    createInfo.Desc.ShaderType = ToInternalShaderType(bytecode_.type_);
    createInfo.Desc.UseCombinedTextureSamplers = true;
    createInfo.EntryPoint = "main";
    createInfo.LoadConstantBufferReflection = true;

    switch (renderDevice_->GetBackend())
    {
    case RenderBackend::D3D11:
    case RenderBackend::D3D12:
    {
        createInfo.ByteCode = bytecode_.bytecode_.data();
        createInfo.ByteCodeSize = bytecode_.bytecode_.size();
        break;
    }
    case RenderBackend::OpenGL:
    {
        createInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
        createInfo.Source = reinterpret_cast<const Diligent::Char*>(bytecode_.bytecode_.data());
        createInfo.SourceLength = bytecode_.bytecode_.size();
        break;
    }
    case RenderBackend::Vulkan:
    {
        createInfo.ByteCode = bytecode_.bytecode_.data();
        createInfo.ByteCodeSize = bytecode_.bytecode_.size();
        break;
    }
    default:
    {
        URHO3D_ASSERTLOG(false, "Not implemented");
        return;
    }
    }

    renderDevice_->GetRenderDevice()->CreateShader(createInfo, &handle_);
    if (!handle_)
    {
        URHO3D_LOGERROR("Failed to create shader '{}'", GetDebugName());
        return;
    }
}

void RawShader::DestroyGPU()
{
    handle_ = nullptr;
}

}
