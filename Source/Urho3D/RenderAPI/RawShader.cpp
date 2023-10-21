// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

    if (bytecode_.IsEmpty())
        return;

    Diligent::ShaderCreateInfo createInfo;
    createInfo.Desc.Name = GetDebugName().c_str();
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
