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

#include "../../Precompiled.h"

#include "../../Core/Context.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/TextureCube.h"
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

void TextureCube::OnDeviceLost()
{
    // No-op on Direct3D11
}

void TextureCube::OnDeviceReset()
{
    // No-op on Direct3D11
}

void TextureCube::Release()
{
    if (graphics_)
    {
        VariantMap& eventData = GetEventDataMap();
        eventData[GPUResourceReleased::P_OBJECT] = this;
        SendEvent(E_GPURESOURCERELEASED, eventData);

        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (graphics_->GetTexture(i) == this)
                graphics_->SetTexture(i, nullptr);
        }
    }

    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        if (renderSurfaces_[i])
            renderSurfaces_[i]->Release();
    }

    sampler_ = nullptr;
    resolveTexture_ = nullptr;
    shaderResourceView_ = nullptr;
    object_ = nullptr;
}

bool TextureCube::SetData(CubeMapFace face, unsigned level, int x, int y, int width, int height, const void* data)
{
    using namespace Diligent;
    URHO3D_PROFILE("SetTextureData");

    if (!object_)
    {
        URHO3D_LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!data)
    {
        URHO3D_LOGERROR("Null source for setting data");
        return false;
    }

    if (level >= levels_)
    {
        URHO3D_LOGERROR("Illegal mip level for setting data");
        return false;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        URHO3D_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    // If compressed, align the update region on a block
    if (IsCompressed())
    {
        x &= ~3;
        y &= ~3;
        width += 3;
        width &= 0xfffffffc;
        height += 3;
        height &= 0xfffffffc;
    }

    unsigned char* src = (unsigned char*)data;
    unsigned rowSize = GetRowDataSize(width);
    unsigned rowStart = GetRowDataSize(x);
    unsigned subResource = level + (face * levels_);

    Box destBox;
    destBox.MinX = x;
    destBox.MaxX = x + width;
    destBox.MinY = y;
    destBox.MaxY = y + height;
    destBox.MinZ = 0;
    destBox.MaxZ = 1;
    if (usage_ == TEXTURE_DYNAMIC)
    {
        if (IsCompressed())
        {
            height = (height + 3) >> 2;
            y >>= 2;
        }

        MappedTextureSubresource mappedData;
        graphics_->GetImpl()->GetDeviceContext()->MapTextureSubresource(
            object_.Cast<ITexture>(IID_Texture),
            subResource,
            0,
            MAP_WRITE,
            MAP_FLAG_DISCARD,
            &destBox,
            mappedData
        );
        for (int row = 0; row < height; ++row)
                memcpy((unsigned char*)mappedData.pData + (row + y) * mappedData.Stride + rowStart, src + row * rowSize, rowSize);
        graphics_->GetImpl()->GetDeviceContext()->UnmapTextureSubresource(object_.Cast<ITexture>(IID_Texture), subResource, 0);
    }
    else
    {
        TextureSubResData resourceData;
        resourceData.pData = data;
        resourceData.Stride = rowSize;
        graphics_->GetImpl()->GetDeviceContext()->UpdateTexture(
            object_.Cast<ITexture>(IID_Texture),
            level,
            face,
            destBox,
            resourceData,
            RESOURCE_STATE_TRANSITION_MODE_NONE,
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION
        );
    }

    return true;
}

bool TextureCube::SetData(CubeMapFace face, Deserializer& source)
{
    SharedPtr<Image> image(MakeShared<Image>(context_));
    if (!image->Load(source))
        return false;

    return SetData(face, image);
}

bool TextureCube::SetData(CubeMapFace face, Image* image, bool useAlpha)
{
    if (!image)
    {
        URHO3D_LOGERROR("Null image, can not load texture");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = 0;
    MaterialQuality quality = QUALITY_HIGH;
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

        if (levelWidth != levelHeight)
        {
            URHO3D_LOGERROR("Cube texture width not equal to height");
            return false;
        }

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

        // Create the texture when face 0 is being loaded, check that rest of the faces are same size & format
        if (!face)
        {
            // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
            if (IsCompressed() && requestedLevels_ > 1)
                requestedLevels_ = 0;
            SetSize(levelWidth, format);
        }
        else
        {
            if (!object_)
            {
                URHO3D_LOGERROR("Cube texture face 0 must be loaded first");
                return false;
            }
            if (levelWidth != width_ || format != format_)
            {
                URHO3D_LOGERROR("Cube texture face does not match size or format of face 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData(face, i, 0, 0, levelWidth, levelHeight, levelData);
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

        if (width != height)
        {
            URHO3D_LOGERROR("Cube texture width not equal to height");
            return false;
        }

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

        // Create the texture when face 0 is being loaded, assume rest of the faces are same size & format
        if (!face)
        {
            SetNumLevels(Max((levels - mipsToSkip), 1U));
            SetSize(width, format);
        }
        else
        {
            if (!object_)
            {
                URHO3D_LOGERROR("Cube texture face 0 must be loaded first");
                return false;
            }
            if (width != width_ || format != format_)
            {
                URHO3D_LOGERROR("Cube texture face does not match size or format of face 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData(face, i, 0, 0, level.width_, level.height_, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                unsigned char* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData(face, i, 0, 0, level.width_, level.height_, rgbaData);
                memoryUse += level.width_ * level.height_ * 4;
                delete[] rgbaData;
            }
        }
    }

    faceMemoryUse_[face] = memoryUse;
    unsigned totalMemoryUse = sizeof(TextureCube);
    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
        totalMemoryUse += faceMemoryUse_[i];
    SetMemoryUse(totalMemoryUse);

    return true;
}

bool TextureCube::GetData(CubeMapFace face, unsigned level, void* dest) const
{
    assert(0);
    return false;
   /* if (!object_.ptr_)
    {
        URHO3D_LOGERROR("No texture created, can not get data");
        return false;
    }

    if (!dest)
    {
        URHO3D_LOGERROR("Null destination for getting data");
        return false;
    }

    if (level >= levels_)
    {
        URHO3D_LOGERROR("Illegal mip level for getting data");
        return false;
    }

    if (multiSample_ > 1 && !autoResolve_)
    {
        URHO3D_LOGERROR("Can not get data from multisampled texture without autoresolve");
        return false;
    }

    if (resolveDirty_)
        graphics_->ResolveToTexture(const_cast<TextureCube*>(this));

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);

    D3D11_TEXTURE2D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)levelWidth;
    textureDesc.Height = (UINT)levelHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = (DXGI_FORMAT)format_;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    HRESULT hr = graphics_->GetImpl()->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
    if (FAILED(hr))
    {
        URHO3D_LOGD3DERROR("Failed to create staging texture for GetData", hr);
        URHO3D_SAFE_RELEASE(stagingTexture);
        return false;
    }

    ID3D11Resource* srcResource = (ID3D11Resource*)(resolveTexture_ ? resolveTexture_ : object_.ptr_);
    unsigned srcSubResource = D3D11CalcSubresource(level, face, levels_);

    D3D11_BOX srcBox;
    srcBox.left = 0;
    srcBox.right = (UINT)levelWidth;
    srcBox.top = 0;
    srcBox.bottom = (UINT)levelHeight;
    srcBox.front = 0;
    srcBox.back = 1;
    graphics_->GetImpl()->GetDeviceContext()->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, srcResource,
        srcSubResource, &srcBox);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    mappedData.pData = nullptr;
    unsigned rowSize = GetRowDataSize(levelWidth);
    unsigned numRows = (unsigned)(IsCompressed() ? (levelHeight + 3) >> 2 : levelHeight);

    hr = graphics_->GetImpl()->GetDeviceContext()->Map((ID3D11Resource*)stagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
    if (FAILED(hr) || !mappedData.pData)
    {
        URHO3D_LOGD3DERROR("Failed to map staging texture for GetData", hr);
        URHO3D_SAFE_RELEASE(stagingTexture);
        return false;
    }
    else
    {
        for (unsigned row = 0; row < numRows; ++row)
            memcpy((unsigned char*)dest + row * rowSize, (unsigned char*)mappedData.pData + row * mappedData.RowPitch, rowSize);
        graphics_->GetImpl()->GetDeviceContext()->Unmap((ID3D11Resource*)stagingTexture, 0);
        URHO3D_SAFE_RELEASE(stagingTexture);
        return true;
    }*/
}

bool TextureCube::Create()
{
    using namespace Diligent;
    Release();

    if (!graphics_ || !width_ || !height_)
        return false;

    levels_ = CheckMaxLevels(width_, height_, requestedLevels_);

    TextureDesc textureDesc;
    textureDesc.Type = RESOURCE_DIM_TEX_CUBE;
    textureDesc.Format = (TEXTURE_FORMAT)(sRGB_ ? GetSRGBFormat(format_) : format_);

    // Disable multisampling if not supported
    if (multiSample_ > 1 && !graphics_->GetImpl()->CheckMultiSampleSupport(textureDesc.Format, multiSample_))
    {
        multiSample_ = 1;
        autoResolve_ = false;
    }

    // Set mipmapping
    if (usage_ == TEXTURE_RENDERTARGET && levels_ != 1 && multiSample_ == 1)
        textureDesc.MiscFlags |= MISC_TEXTURE_FLAG_GENERATE_MIPS;

    textureDesc.Width = width_;
    textureDesc.Height = height_;
    // Disable mip levels from the multisample texture. Rather create them to the resolve texture
    textureDesc.MipLevels = (multiSample_ == 1 && usage_ != TEXTURE_DYNAMIC) ? levels_ : 1;
    textureDesc.ArraySize = MAX_CUBEMAP_FACES;
    textureDesc.SampleCount = (unsigned)multiSample_;
    textureDesc.Usage = usage_ == TEXTURE_DYNAMIC ? USAGE_DYNAMIC : USAGE_DEFAULT;
    textureDesc.BindFlags = BIND_SHADER_RESOURCE;

    // Is this format supported by compute?
    if (IsUnorderedAccessSupported() && graphics_->GetComputeSupport())
        textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;

    if (usage_ == TEXTURE_RENDERTARGET)
        textureDesc.BindFlags |= BIND_RENDER_TARGET;
    else if (usage_ == TEXTURE_DEPTHSTENCIL)
        textureDesc.BindFlags |= BIND_DEPTH_STENCIL;
    textureDesc.CPUAccessFlags = usage_ == TEXTURE_DYNAMIC ? CPU_ACCESS_WRITE : CPU_ACCESS_NONE;

    RefCntAutoPtr<ITexture> texture;
    graphics_->GetImpl()->GetDevice()->CreateTexture(textureDesc, nullptr, &texture);
    if (texture == nullptr) {
        URHO3D_LOGERROR("Failed to create texture");
        return false;
    }
    object_ = texture;

    // Create resolve texture for multisampling
    if (multiSample_ > 1)
    {
        textureDesc.SampleCount = 1;
        if (levels_ != 1)
            textureDesc.MiscFlags |= MISC_TEXTURE_FLAG_GENERATE_MIPS;

        graphics_->GetImpl()->GetDevice()->CreateTexture(textureDesc, nullptr, (ITexture**)&resolveTexture_);
        if (resolveTexture_ == nullptr) {
            URHO3D_LOGERROR("Failed to create resolve texture");
            return false;
        }
    }


    shaderResourceView_ = object_.Cast<ITexture>(IID_Texture)->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    if (shaderResourceView_ == nullptr) {
        URHO3D_LOGERROR("Failed to create shader resource view for texture.");
        return false;
    }


    if (usage_ == TEXTURE_RENDERTARGET)
    {
        for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
        {
            TextureViewDesc renderTargetViewDesc;
            renderTargetViewDesc.Format = textureDesc.Format;
            renderTargetViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
            renderTargetViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;

            if (multiSample_ > 1) {
                renderTargetViewDesc.NumArraySlices = 1;
                renderTargetViewDesc.FirstArraySlice = i;
            }
            else {
                renderTargetViewDesc.NumArraySlices = 1;
                renderTargetViewDesc.FirstArraySlice = i;
                renderTargetViewDesc.NumMipLevels = 0;
            }

            object_.Cast<ITexture>(IID_Texture)->CreateView(renderTargetViewDesc, &renderSurfaces_[i]->renderTargetView_);
            if (renderSurfaces_[i]->renderTargetView_ == nullptr) {
                URHO3D_LOGERROR("Failed to create rendertarget view for texture");
                return false;
            }
        }
    }

    return true;
}

}
