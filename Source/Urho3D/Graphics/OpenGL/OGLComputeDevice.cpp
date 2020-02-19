#include "../../Precompiled.h"

#include "../../Graphics/ComputeDevice.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Texture.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Graphics/TextureCube.h"
#include "../../IO/Log.h"

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

    // Textures were already handled through graphics in `ComputeDevice::SetReadTexture(...)`

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
        uavs_[unit].layerCount_ = tex2DArray->GetLayers();
        if (faceIndex >= tex2DArray->GetLayers())
            uavs_[unit].layer_ = 0;
    }
    else if (auto texCube = dynamic_cast<TextureCube*>(texture))
    {
        uavs_[unit].layerCount_ = 6;
        if (faceIndex >= 6)
            uavs_[unit].layer_ = 0;
    }

    uavsDirty_ = true;
    return true;
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
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glFlush();
}



}
