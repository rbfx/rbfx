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

#include "../../Core/Context.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

namespace Urho3D
{

void Texture2DArray::OnDeviceLost()
{
}

void Texture2DArray::OnDeviceReset()
{
}

void Texture2DArray::Release()
{
    if (graphics_ && object_.idx_ != bgfx::kInvalidHandle)
    {
        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (graphics_->GetTexture(i) == this)
                graphics_->SetTexture(i, nullptr);
        }
    }

    for (unsigned i = 0; i < renderSurfaces_.Size(); ++i)
    {
        if (renderSurfaces_[i])
            renderSurfaces_[i]->Release();
    }

    bgfx::TextureHandle handle;
    handle.idx = object_.idx_;
    bgfx::destroy(handle);
    object_.idx_ = bgfx::kInvalidHandle;

    levelsDirty_ = false;
}

bool Texture2DArray::SetData(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data)
{
    URHO3D_PROFILE(SetTextureData);

    if (object_.idx_ == bgfx::kInvalidHandle)
    {
        URHO3D_LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!data)
    {
        URHO3D_LOGERROR("Null source for setting data");
        return false;
    }

    if (layer >= layers_)
    {
        URHO3D_LOGERROR("Illegal layer for setting data");
        return false;
    }

    if (level >= levels_)
    {
        URHO3D_LOGERROR("Illegal mip level for setting data");
        return false;
    }

    if (IsCompressed())
    {
        x &= ~3;
        y &= ~3;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        URHO3D_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    bgfx::TextureHandle handle;
    handle.idx = object_.idx_;

    bgfx::updateTexture2D(handle, (uint16_t)layer, (uint8_t)level, (uint16_t)x, (uint16_t)y, (uint16_t)width, (uint16_t)height, bgfx::makeRef(data, sizeof(data)));

    return true;
}

bool Texture2DArray::SetData(unsigned layer, Deserializer& source)
{
    SharedPtr<Image> image(new Image(context_));
    if (!image->Load(source))
        return false;

    return SetData(layer, image);
}

bool Texture2DArray::SetData(unsigned layer, Image* image, bool useAlpha)
{
    if (!image)
    {
        URHO3D_LOGERROR("Null image, can not load texture");
        return false;
    }
    if (!layers_)
    {
        URHO3D_LOGERROR("Number of layers in the array must be set first");
        return false;
    }
    if (layer >= layers_)
    {
        URHO3D_LOGERROR("Illegal layer for setting data");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = 0;
    int quality = QUALITY_HIGH;
    Renderer* renderer = GetSubsystem<Renderer>();
    if (renderer)
        quality = renderer->GetTextureQuality();

    if (!image->IsCompressed())
    {
        // Convert unsuitable formats to RGBA
        unsigned components = image->GetComponents();
        if ((components == 1 && !useAlpha) || components == 2 || components == 3)
        {
            mipImage = image->ConvertToRGBA(); image = mipImage;
            if (!image)
                return false;
            components = image->GetComponents();
        }

        unsigned char* levelData = image->GetData();
        int levelWidth = image->GetWidth();
        int levelHeight = image->GetHeight();
        unsigned format = 0;

        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            mipImage = image->GetNextLevel(); image = mipImage;
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
        }

        switch (components)
        {
        case 1:
            format = Graphics::GetAlphaFormat();
            break;

        case 4:
            format = Graphics::GetRGBAFormat();
            break;

        default: break;
        }

        // Create the texture array when layer 0 is being loaded, check that rest of the layers are same size & format
        if (!layer)
        {
            // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
            if (IsCompressed() && requestedLevels_ > 1)
                requestedLevels_ = 0;
            SetSize(0, levelWidth, levelHeight, format);
        }
        else
        {
            if (object_.idx_ == bgfx::kInvalidHandle)
            {
                URHO3D_LOGERROR("Texture array layer 0 must be loaded first");
                return false;
            }
            if (levelWidth != width_ || levelHeight != height_ || format != format_)
            {
                URHO3D_LOGERROR("Texture array layer does not match size or format of layer 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData(layer, i, 0, 0, levelWidth, levelHeight, levelData);
            memoryUse += levelWidth * levelHeight * components;

            if (i < levels_ - 1)
            {
                mipImage = image->GetNextLevel(); image = mipImage;
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
        unsigned levels = image->GetNumCompressedLevels();
        unsigned format = graphics_->GetFormat(image->GetCompressedFormat());
        bool needDecompress = false;

        if (!format)
        {
            format = Graphics::GetRGBAFormat();
            needDecompress = true;
        }

        unsigned mipsToSkip = mipsToSkip_[quality];
        if (mipsToSkip >= levels)
            mipsToSkip = levels - 1;
        while (mipsToSkip && (width / (1 << mipsToSkip) < 4 || height / (1 << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1 << mipsToSkip);
        height /= (1 << mipsToSkip);

        // Create the texture array when layer 0 is being loaded, assume rest of the layers are same size & format
        if (!layer)
        {
            SetNumLevels(Max((levels - mipsToSkip), 1U));
            SetSize(0, width, height, format);
        }
        else
        {
            if (object_.idx_ == bgfx::kInvalidHandle)
            {
                URHO3D_LOGERROR("Texture array layer 0 must be loaded first");
                return false;
            }
            if (width != width_ || height != height_ || format != format_)
            {
                URHO3D_LOGERROR("Texture array layer does not match size or format of layer 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData(layer, i, 0, 0, level.width_, level.height_, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                unsigned char* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData(layer, i, 0, 0, level.width_, level.height_, rgbaData);
                memoryUse += level.width_ * level.height_ * 4;
                delete[] rgbaData;
            }
        }
    }

    layerMemoryUse_[layer] = memoryUse;
    unsigned totalMemoryUse = sizeof(Texture2DArray) + layerMemoryUse_.Capacity() * sizeof(unsigned);
    for (unsigned i = 0; i < layers_; ++i)
        totalMemoryUse += layerMemoryUse_[i];
    SetMemoryUse(totalMemoryUse);

    return true;
}

bool Texture2DArray::GetData(unsigned layer, unsigned level, void* dest) const
{
    return false;
}

bool Texture2DArray::Create()
{
    Release();

    if (!graphics_ || !width_ || !height_ || !layers_)
        return false;

    levels_ = CheckMaxLevels(width_, height_, requestedLevels_);

    bgfx::TextureHandle handle;
    handle = bgfx::createTexture2D((uint16_t)width_, (uint16_t)height_, levels_ > 1 ? true : false, (uint16_t)layers_, (bgfx::TextureFormat::Enum)format_, GetBGFXFlags() /*, mem*/);
    object_.idx_ = handle.idx;

    if (usage_ == TEXTURE_RENDERTARGET)
    {
        if (layers_ == 1)
        {
            bgfx::Attachment attachment;
            attachment.handle = handle;
            attachment.mip = 0;
            attachment.layer = layers_;
            bgfx::FrameBufferHandle fbHandle;
            fbHandle = bgfx::createFrameBuffer(1, &attachment, false);
            renderSurfaces_[0]->idx_ = fbHandle.idx;
        }
        else
        {
            for (unsigned i = 0; i < layers_; ++i)
            {
                bgfx::Attachment attachment;
                attachment.handle = handle;
                attachment.mip = 0;
                attachment.layer = i;
                bgfx::FrameBufferHandle fbHandle;
                fbHandle = bgfx::createFrameBuffer(1, &attachment, false);
                renderSurfaces_[i]->idx_ = fbHandle.idx;
            }
        }
    }

    if (object_.idx_ != bgfx::kInvalidHandle)
        return true;
    else
        return false;
}

RenderSurface* Texture2DArray::GetRenderSurfaceLayer(unsigned layer)
{
    if (layer > renderSurfaces_.Size())
        return nullptr;
    else
        return renderSurfaces_[layer];
}

}
