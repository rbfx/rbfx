//
// Copyright (c) 2017-2020 the Urho3D project.
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

#include "../../Graphics/ComputeBuffer.h"
#ifdef URHO3D_COMPUTE
    #include "../../Graphics/ComputeDevice.h"
#endif
#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/IndexBuffer.h"
#include "../../Graphics/Texture.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Graphics/TextureCube.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/Log.h"

#if defined(URHO3D_COMPUTE)

namespace Urho3D
{

GLuint ComputeDevice_GetImageTextureFormat(Texture* texture)
{
    if (texture->GetComponents() == 4)
    {
        switch (texture->GetFormat())
        {
        case GL_RGBA:
            return GL_RGBA8;
        }
    }
    return texture->GetFormat();
}

void ComputeDevice::Init()
{
    // No work to perform.
}

void ComputeDevice::ReleaseLocalState()
{
    // No work to perform.
}

bool ComputeDevice::IsSupported() const
{
    // Is this reliable?
    return glDispatchCompute != nullptr && glBindImageTexture != nullptr;
}

void ComputeDevice::ApplyBindings()
{
    if (!computeShader_)
        return;

    // Does the shader require compilation?
    if (!computeShader_->GetGPUObjectName())
    {
        if (computeShader_->GetByteCode().empty())
        {
            URHO3D_PROFILE("Compile compute shader");
            bool success = computeShader_->Create();
            if (success)
            {
                URHO3D_LOGDEBUG("Compiled compute shader " + computeShader_->GetFullName());
            }
            else
            {
                URHO3D_LOGERROR("Failed to compile compute shader " + computeShader_->GetFullName() + ":\n" + computeShader_->GetCompilerOutput());
                return;
            }
        }
    }

    // Does the program require linking or bind?
    ea::pair<ShaderVariation*, ShaderVariation*> combo(computeShader_, nullptr);
    auto foundExisting = graphics_->impl_->shaderPrograms_.find(combo);
    if (foundExisting != graphics_->impl_->shaderPrograms_.end())
    {
        if (graphics_->impl_->shaderProgram_ != foundExisting->second)
        {
            glUseProgram(foundExisting->second->GetGPUObjectName());
            graphics_->impl_->shaderProgram_ = foundExisting->second;
        }
    }
    else
    {
        URHO3D_PROFILE("LinkComputeShader");

        SharedPtr<ShaderProgram> newProgram(new ShaderProgram(graphics_, computeShader_));
        if (newProgram->Link())
        {
            URHO3D_LOGDEBUG("Linked compute shader " + computeShader_->GetFullName());
            glUseProgram(newProgram->GetGPUObjectName());
            graphics_->impl_->shaderProgram_ = newProgram;
        }
        else
        {
            URHO3D_LOGERROR("Failed to link compute shader " + computeShader_->GetFullName() + ":\n" + newProgram->GetLinkerOutput());
            glUseProgram(0);
            graphics_->impl_->shaderProgram_ = nullptr;
        }

        graphics_->impl_->shaderPrograms_[combo] = newProgram;
    }

    // VS shader param groups are reused for compute. Check if any need to be bound.
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    {
        if (constantBuffers_[i] && constantBuffers_[i] != graphics_->impl_->constantBuffers_[i])
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, i, constantBuffers_[i]->GetGPUObjectName());
            graphics_->impl_->constantBuffers_[i] = constantBuffers_[i];
        }
    }

    // Read-only textures were already handled through graphics in `ComputeDevice::SetReadTexture(...)`

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        const auto& uav = uavs_[i];
        if (uavs_[i].object_)
        {
            // Bnd as writable.
            glBindImageTexture(i,
                uav.object_->GetGPUObjectName(),
                uav.mipLevel_,
                uav.layerCount_ > 1 ? GL_TRUE : GL_FALSE,
                uav.layer_,
                GL_READ_WRITE,
                ComputeDevice_GetImageTextureFormat(uav.object_));

            // Force it to null so that if the same object is bound later as a texture it'll work, rebinding as an image in a following dispatch is less of a concern.
            graphics_->textures_[i] = nullptr;
        }
    }

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        if (ssbos_[i].dirty_)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbos_[i].object_);
    }

    programDirty_ = false;
    uavsDirty_ = false;
    constantBuffersDirty_ = false;
    texturesDirty_ = false;
}

void ComputeDevice::HandleGPUResourceRelease(StringHash eventID, VariantMap& eventData)
{
    // Never needed
}

bool ComputeDevice::SetReadTexture(Texture* texture, unsigned unit)
{
    graphics_->SetTexture(unit, texture);
    return true;
}

bool ComputeDevice::SetConstantBuffer(ConstantBuffer* buffer, unsigned unit)
{
    if (unit >= MAX_SHADER_PARAMETER_GROUPS)
    {
        URHO3D_LOGERROR("ComputeDevice::SetConstantBuffer, unit {} exceed limit of MAX_SHADER_PARAMETER_GROUPS ({})", unit, MAX_SHADER_PARAMETER_GROUPS);
        return false;
    }

    constantBuffers_[unit] = buffer;
    constantBuffersDirty_ = true;
    return true;
}

bool ComputeDevice::SetWriteTexture(Texture* texture, unsigned unit, unsigned faceIndex, unsigned mipLevel)
{
    if (texture == nullptr)
        return false;

    if (!texture || unit >= MAX_TEXTURE_UNITS)
    {
        URHO3D_LOGERROR("ComputeDevice::SetWriteTexture, attempted to assign write texture to out-of-bounds slot {}", unit);
        return false;
    }

    if (!Texture::IsComputeWriteable(texture->GetFormat()))
    {
        URHO3D_LOGERROR("ComputeDevice::SetWriteTexture, texture format {} is not a compute-writeable format", texture->GetFormat());
        return false;
    }

    uavs_[unit].object_ = texture;
    uavs_[unit].mipLevel_ = mipLevel;
    uavs_[unit].layer_ = faceIndex;
    uavs_[unit].layerCount_ = 1;

    if (auto tex2DArray = dynamic_cast<Texture2DArray*>(texture))
    {
        uavs_[unit].layerCount_ = faceIndex == UINT_MAX ? tex2DArray->GetLayers() : 1;
        if (faceIndex >= tex2DArray->GetLayers())
            uavs_[unit].layer_ = faceIndex != UINT_MAX ? faceIndex : 0;
    }
    else if (auto texCube = dynamic_cast<TextureCube*>(texture))
    {
        uavs_[unit].layerCount_ = faceIndex == UINT_MAX ? 6 : 1;
        if (faceIndex >= 6)
            uavs_[unit].layer_ = faceIndex != UINT_MAX ? faceIndex : 0;
    }

    uavsDirty_ = true;
    return true;
}

bool ComputeDevice::SetWritableBuffer(Object* buffer, unsigned slot)
{
    // easy case
    if (buffer == nullptr)
    {
        if (ssbos_[slot].object_ != 0)
        {
            ssbos_[slot] = { 0, true };
            uavsDirty_ = true;
        }
        return true;
    }

    // Note: that just because we *might* be able to doesn't mean the underlying buffer will map reasonably.
    GLuint objectName = 0;
    if (auto vbo = buffer->Cast<VertexBuffer>())
        objectName = vbo->GetGPUObjectName();
    else if (auto ibo = buffer->Cast<IndexBuffer>())
        objectName = ibo->GetGPUObjectName();
    else if (auto ubo = buffer->Cast<ConstantBuffer>())
        objectName = ubo->GetGPUObjectName();
    else if(auto ssbo = buffer->Cast<ComputeBuffer>())
        objectName = ssbo->GetGPUObjectName();
    else
    {
        URHO3D_LOGERROR("ComputeDevice::SetWritableBuffer, attempted ot use unsupported object type: {}", buffer->GetTypeName());
    }

    if (ssbos_[slot].object_ != objectName)
    {
        ssbos_[slot] = { objectName, true };
        uavsDirty_ = true;
    }

    return objectName != 0;
}

void ComputeDevice::Dispatch(unsigned xDim, unsigned yDim, unsigned zDim)
{
    ApplyBindings();

    if (!computeShader_ || !computeShader_->GetGPUObjectName())
    {
        URHO3D_LOGERROR("ComputeDevice::Dispatch, cannot dispatch without a compute-shader");
        return;
    }

    glDispatchCompute(Max(xDim, 1), Max(yDim, 1), Max(zDim, 1));

    // check if there are any UAVs before deciding to place a barrier on images.
    bool anyUavs = false;
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        if (uavs_[i].object_)
        {
            anyUavs = true;
            break;
        }
    }

    // The real necessity of this barrier depends on usage.
    // Example: if compute is done before render (ie. BeginFrame) and shadowmap reuse disabled
    //          then this barrier doesn't really contribute so long as a CS isn't writing to an 
    //          alpha-texture that a shadow pass is using.
    if (anyUavs)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}



}

#endif

