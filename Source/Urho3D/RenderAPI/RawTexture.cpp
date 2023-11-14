// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RawTexture.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

#if D3D11_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h>
#endif
#if D3D12_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h>
#endif
#if VULKAN_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h>
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/RenderDeviceGL.h>
#endif

namespace Urho3D
{

namespace
{

static const EnumArray<Diligent::RESOURCE_DIMENSION, TextureType> textureTypeToDimensions{{
    Diligent::RESOURCE_DIM_TEX_2D,
    Diligent::RESOURCE_DIM_TEX_CUBE,
    Diligent::RESOURCE_DIM_TEX_3D,
    Diligent::RESOURCE_DIM_TEX_2D_ARRAY,
}};

static const EnumArray<Diligent::RESOURCE_DIMENSION, TextureType> textureTypeToViewDimensions{{
    Diligent::RESOURCE_DIM_TEX_2D,
    Diligent::RESOURCE_DIM_TEX_2D_ARRAY,
    Diligent::RESOURCE_DIM_TEX_3D,
    Diligent::RESOURCE_DIM_TEX_2D_ARRAY,
}};

static const EnumArray<Diligent::RESOURCE_DIMENSION, TextureType> textureTypeToStagingDimensions{{
    Diligent::RESOURCE_DIM_TEX_2D,
    Diligent::RESOURCE_DIM_TEX_2D,
    Diligent::RESOURCE_DIM_TEX_3D,
    Diligent::RESOURCE_DIM_TEX_2D,
}};

bool AllLess(const IntVector3& lhs, const IntVector3& rhs)
{
    return lhs.x_ < rhs.x_ && lhs.y_ < rhs.y_ && lhs.z_ < rhs.z_;
}

bool AllNotLess(const IntVector3& lhs, const IntVector3& rhs)
{
    return lhs.x_ >= rhs.x_ && lhs.y_ >= rhs.y_ && lhs.z_ >= rhs.z_;
}

bool IsAligned(const IntVector2& lhs, const IntVector2& rhs)
{
    return (lhs.x_ % rhs.x_ == 0) && (lhs.y_ % rhs.y_ == 0);
}

bool ValidateDimensions(RawTextureParams& params)
{
    switch (params.type_)
    {
    case TextureType::Texture2D:
        if (params.size_.x_ <= 0 || params.size_.y_ <= 0)
        {
            URHO3D_ASSERTLOG(false, "Zero or negative texture dimensions");
            return false;
        }

        params.size_.z_ = 1;
        params.arraySize_ = 1;
        return true;

    case TextureType::TextureCube:
        if (params.size_.x_ <= 0)
        {
            URHO3D_ASSERTLOG(false, "Zero or negative texture dimensions");
            return false;
        }

        params.size_.y_ = params.size_.x_;
        params.size_.z_ = 1;
        params.arraySize_ = 6;
        return true;

    case TextureType::Texture3D:
        if (params.size_.x_ <= 0 || params.size_.y_ <= 0 || params.size_.z_ <= 0)
        {
            URHO3D_ASSERTLOG(false, "Zero or negative texture dimensions");
            return false;
        }

        params.arraySize_ = 1;
        return true;

    case TextureType::Texture2DArray:
        if (params.size_.x_ <= 0 || params.size_.y_ <= 0 || params.arraySize_ == 0)
        {
            URHO3D_ASSERTLOG(false, "Zero or negative texture dimensions");
            return false;
        }

        params.size_.z_ = 1;
        return true;

    default:
    {
        URHO3D_ASSERT(false);
        return false;
    }
    }
}

bool ValidateBindings(RawTextureParams& params)
{
    const bool isRenderTarget = params.flags_.Test(TextureFlag::BindRenderTarget);
    const bool isDepthStencil = params.flags_.Test(TextureFlag::BindDepthStencil);
    if (isRenderTarget && isDepthStencil)
    {
        URHO3D_ASSERTLOG(false, "Texture cannot be both render target and depth-stencil");
        return false;
    }

    return true;
}

bool ValidateMultiSample(RawTextureParams& params)
{
    params.multiSample_ = Clamp(params.multiSample_, 1u, 16u);
    if (params.multiSample_ == 1)
        params.flags_.Set(TextureFlag::NoMultiSampledAutoResolve);

    const bool isRenderTarget = params.flags_.Test(TextureFlag::BindRenderTarget);
    const bool isDepthStencil = params.flags_.Test(TextureFlag::BindDepthStencil);
    if (params.multiSample_ != 1 && !(isRenderTarget || isDepthStencil))
    {
        URHO3D_ASSERTLOG(false, "Multi-sampling is only supported for render target or depth-stencil textures");
        return false;
    }

    return true;
}

bool ValidateLevels(RawTextureParams& params)
{
    const unsigned maxLevels = GetMipLevelCount(params.size_);
    params.numLevels_ = params.numLevels_ != 0 ? ea::min(params.numLevels_, maxLevels) : maxLevels;

    if (params.numLevels_ > 1 && params.flags_.Test(TextureFlag::BindDepthStencil))
    {
        URHO3D_LOGWARNING("Depth-stencil texture cannot have mipmaps.");
        params.numLevels_ = 1;
    }

    if (params.numLevels_ > 1 && params.multiSample_ != 1 && params.flags_.Test(TextureFlag::NoMultiSampledAutoResolve))
    {
        URHO3D_LOGWARNING("Multi-sampled texture cannot have mipmaps.");
        params.numLevels_ = 1;
    }

    params.numLevelsRTV_ = params.multiSample_ != 1 ? 1 : params.numLevels_;

    return true;
}

bool ValidateCaps(RawTextureParams& params, RenderDevice* renderDevice)
{
    if (!renderDevice)
        return true;

    Diligent::IRenderDevice* device = renderDevice->GetRenderDevice();

    // Attempt to fall back from D24S8 to D32S8 if the former is not supported
    if (params.format_ == TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT
        && !(device->GetTextureFormatInfoExt(params.format_).BindFlags & Diligent::BIND_DEPTH_STENCIL))
    {
        params.format_ = TextureFormat::TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    }

    const Diligent::TextureFormatInfoExt& formatInfo = device->GetTextureFormatInfoExt(params.format_);
    const Diligent::BIND_FLAGS allowedFlags = formatInfo.BindFlags;

    if (params.multiSample_ != 1 && !(formatInfo.SampleCounts & params.multiSample_))
    {
        URHO3D_LOGWARNING("Multi-sampling is not supported for this texture format, demoting to simple texture.");

        params.multiSample_ = 1;
        params.flags_.Set(TextureFlag::NoMultiSampledAutoResolve);
    }

    if (params.flags_.Test(TextureFlag::BindRenderTarget) && !(allowedFlags & Diligent::BIND_RENDER_TARGET))
    {
        URHO3D_LOGWARNING("Render target binding is not supported for this texture format.");
        return false;
    }

    if (params.flags_.Test(TextureFlag::BindDepthStencil) && !(allowedFlags & Diligent::BIND_DEPTH_STENCIL))
    {
        URHO3D_LOGWARNING("Depth-stencil binding is not supported for this texture format.");
        return false;
    }

    if (params.flags_.Test(TextureFlag::BindUnorderedAccess) && !(allowedFlags & Diligent::BIND_UNORDERED_ACCESS))
    {
        URHO3D_LOGWARNING("Unordered access binding is not supported for this texture format.");
        return false;
    }

    return true;
}

ea::string ToString(ea::string_view baseName, const RawTextureUAVKey& key)
{
    return Format("{}:{}-{}:{}-{}:{}{}", baseName, key.firstSlice_, key.firstSlice_ + key.numSlices_,
        key.firstLevel_, key.firstLevel_ + key.numLevels_, key.canRead_ ? 'r' : ' ', key.canWrite_ ? 'w' : ' ');
}

bool ValidateKey(RawTextureUAVKey& key, const RawTextureParams& params)
{
    if (!key.canRead_ && !key.canWrite_)
    {
        URHO3D_ASSERTLOG("UAV must have at least one access flag set");
        return false;
    }
    if (key.firstSlice_ >= params.arraySize_)
    {
        URHO3D_ASSERTLOG("UAV first slice is out of range");
        return false;
    }
    if (key.firstLevel_ >= params.numLevelsRTV_)
    {
        URHO3D_ASSERTLOG("UAV first level is out of range");
        return false;
    }

    if (!key.numLevels_)
        key.numLevels_ = params.numLevelsRTV_ - key.firstLevel_;
    if (!key.numSlices_)
        key.numSlices_ = params.arraySize_ - key.firstSlice_;

    if (key.firstSlice_ + key.numSlices_ > params.arraySize_)
    {
        URHO3D_ASSERTLOG("UAV slice count is out of range");
        return false;
    }
    if (key.firstLevel_ + key.numLevels_ > params.numLevelsRTV_)
    {
        URHO3D_ASSERTLOG("UAV level count is out of range");
        return false;
    }

    return true;
}

IntVector3 GetSizeInBlocks(const IntVector3& size, TextureFormat format)
{
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(format);
    return {
        (size.x_ + formatInfo.BlockWidth - 1) / formatInfo.BlockWidth,
        (size.y_ + formatInfo.BlockHeight - 1) / formatInfo.BlockHeight, //
        size.z_ //
    };
}

unsigned GetBlockSize(TextureFormat format)
{
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(format);
    return formatInfo.GetElementSize();
}

unsigned long long GetMipLevelSizeInBytes(const IntVector3& size, unsigned level, TextureFormat format)
{
    const IntVector3 sizeInTexels = GetMipLevelSize(size, level);
    const IntVector3 sizeInBlocks = GetSizeInBlocks(sizeInTexels, format);
    return sizeInBlocks.x_ * sizeInBlocks.y_ * sizeInBlocks.z_ * GetBlockSize(format);
}

Diligent::RefCntAutoPtr<Diligent::ITextureView> GetDefaultView(
    Diligent::ITexture* texture, Diligent::TEXTURE_VIEW_TYPE viewType, TextureFormat format)
{
    Diligent::RefCntAutoPtr<Diligent::ITextureView> view;
    view = texture->GetDefaultView(viewType);

    if (!view)
    {
        Diligent::TextureViewDesc viewDesc;
        viewDesc.ViewType = viewType;
        viewDesc.Format = format;
        switch (viewType)
        {
        case Diligent::TEXTURE_VIEW_SHADER_RESOURCE:
            if ((texture->GetDesc().MiscFlags & Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0)
                viewDesc.Flags |= Diligent::TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;
            break;

        case Diligent::TEXTURE_VIEW_UNORDERED_ACCESS:
            viewDesc.AccessFlags |= Diligent::UAV_ACCESS_FLAG_READ_WRITE;
            break;

        default: break;
        }

        texture->CreateView(viewDesc, &view);
    }

    return view;
}

} // namespace

RawTexture::RawTexture(Context* context)
    : DeviceObject(context)
{
}

RawTexture::RawTexture(Context* context, const RawTextureParams& params)
    : DeviceObject(context)
{
    Create(params);
}

RawTexture::~RawTexture()
{
    RawTexture::DestroyGPU();
}

void RawTexture::Invalidate()
{
    DestroyGPU();
}

void RawTexture::Restore()
{
    if (params_.size_ == IntVector3::ZERO)
    {
        dataLost_ = false;
    }
    else if (TryRestore())
    {
        dataLost_ = false;
    }
    else
    {
        CreateGPU();
        dataLost_ = true;
    }
}

void RawTexture::Destroy()
{
    DestroyGPU();
}

Diligent::ITextureView* RawTexture::CreateUAV(const RawTextureUAVKey& key)
{
    if (!handles_)
    {
        URHO3D_ASSERTLOG(false, "RawTexture::CreateUAV is ignored for uninitialized texture");
        return nullptr;
    }

    if (Diligent::ITextureView* view = GetUAV(key))
        return view;

    RawTextureUAVKey effectiveKey = key;
    if (!ValidateKey(effectiveKey, params_))
        return nullptr;

    if (Diligent::ITextureView* view = GetUAV(effectiveKey))
    {
        handles_.uavs_[key] = view;
        return view;
    }

    Diligent::TextureViewDesc viewDesc = {};
    const ea::string name = ToString(GetDebugName(), effectiveKey);
    viewDesc.Name = name.c_str();
    viewDesc.ViewType = Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
    viewDesc.TextureDim = textureTypeToViewDimensions[params_.type_];
    viewDesc.Format = params_.format_;
    viewDesc.MostDetailedMip = effectiveKey.firstLevel_;
    viewDesc.NumMipLevels = effectiveKey.numLevels_;
    if (params_.type_ == TextureType::Texture3D)
    {
        viewDesc.FirstDepthSlice = effectiveKey.firstSlice_;
        viewDesc.NumDepthSlices = effectiveKey.numSlices_;
    }
    else
    {
        viewDesc.FirstArraySlice = effectiveKey.firstSlice_;
        viewDesc.NumArraySlices = effectiveKey.numSlices_;
    }

    if (effectiveKey.canRead_)
        viewDesc.AccessFlags |= Diligent::UAV_ACCESS_FLAG_READ;
    if (effectiveKey.canWrite_)
        viewDesc.AccessFlags |= Diligent::UAV_ACCESS_FLAG_WRITE;

    Diligent::RefCntAutoPtr<Diligent::ITextureView> view;
    handles_.texture_->CreateView(viewDesc, &view);
    if (!view)
    {
        URHO3D_LOGERROR("Failed to create UAV for texture");
        return nullptr;
    }

    handles_.uavs_[key] = view;
    handles_.uavs_[effectiveKey] = view;
    return view;
}

Diligent::ITextureView* RawTexture::GetUAV(const RawTextureUAVKey& key) const
{
    const auto iter = handles_.uavs_.find(key);
    return iter != handles_.uavs_.end() ? iter->second.RawPtr() : nullptr;
}

bool RawTexture::Create(const RawTextureParams& params)
{
    // Optimize repeated calls.
    if (params == params_ && handles_)
        return true;

    Destroy();

    params_ = params;

    if (!ValidateBindings(params_))
        return false;
    if (!ValidateDimensions(params_))
        return false;
    if (!ValidateMultiSample(params_))
        return false;
    if (!ValidateLevels(params_))
        return false;
    if (!ValidateCaps(params_, renderDevice_))
        return false;

    if (!renderDevice_)
        return true;

    if (!CreateGPU())
    {
        handles_ = {};
        return false;
    }

    return true;
}

bool RawTexture::CreateFromHandle(Diligent::ITexture* texture, TextureFormat format, int msaaLevel)
{
    DestroyGPU();

    const Diligent::TextureDesc& textureDesc = texture->GetDesc();

    switch (textureDesc.Type)
    {
    case Diligent::RESOURCE_DIM_TEX_2D: params_.type_ = TextureType::Texture2D; break;
    case Diligent::RESOURCE_DIM_TEX_CUBE: params_.type_ = TextureType::TextureCube; break;
    case Diligent::RESOURCE_DIM_TEX_2D_ARRAY: params_.type_ = TextureType::Texture2DArray; break;
    case Diligent::RESOURCE_DIM_TEX_3D: params_.type_ = TextureType::Texture3D; break;
    default:
        URHO3D_LOGERROR("Unsupported texture type '{}'", Diligent::GetResourceDimString(textureDesc.Type));
        return false;
    }

    params_.format_ = format != TextureFormat::TEX_FORMAT_UNKNOWN ? format : textureDesc.Format;

    // TODO: Revisit this flag
    params_.flags_ |= TextureFlag::NoMultiSampledAutoResolve;
    if (textureDesc.BindFlags & Diligent::BIND_RENDER_TARGET)
        params_.flags_ |= TextureFlag::BindRenderTarget;
    if (textureDesc.BindFlags & Diligent::BIND_DEPTH_STENCIL)
        params_.flags_ |= TextureFlag::BindDepthStencil;
    if (textureDesc.BindFlags & Diligent::BIND_UNORDERED_ACCESS)
        params_.flags_ |= TextureFlag::BindUnorderedAccess;

    params_.size_.x_ = static_cast<int>(textureDesc.GetWidth());
    params_.size_.y_ = static_cast<int>(textureDesc.GetHeight());
    params_.size_.z_ = static_cast<int>(textureDesc.GetDepth());
    params_.arraySize_ = textureDesc.GetArraySize();
    params_.numLevels_ = textureDesc.MipLevels;
    params_.multiSample_ = ea::max<unsigned>(textureDesc.SampleCount, msaaLevel);
    params_.numLevelsRTV_ = textureDesc.MipLevels;

    handles_.texture_ = texture;

    return InitializeDefaultViews(textureDesc.BindFlags);
}

bool RawTexture::CreateFromD3D11Texture2D(void* d3d11Texture2D, TextureFormat format, int msaaLevel)
{
#if D3D11_SUPPORTED
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::D3D11)
    {
        auto deviceD3D11 = static_cast<Diligent::IRenderDeviceD3D11*>(renderDevice_->GetRenderDevice());
        Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        deviceD3D11->CreateTexture2DFromD3DResource(
            reinterpret_cast<ID3D11Texture2D*>(d3d11Texture2D), Diligent::RESOURCE_STATE_UNKNOWN, &texture);
        if (!texture)
        {
            URHO3D_LOGERROR("Failed to create texture from existing ID3D11Texture2D pointer");
            return false;
        }

        return CreateFromHandle(texture, format, msaaLevel);
    }
#endif

    URHO3D_ASSERT(false, "RawTexture::CreateFromD3D11Texture2D is not supported on this platform");
    return false;
}

bool RawTexture::CreateFromD3D12Resource(void* d3d12Resource, TextureFormat format, int msaaLevel)
{
#if D3D12_SUPPORTED
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::D3D12)
    {
        auto deviceD3D12 = static_cast<Diligent::IRenderDeviceD3D12*>(renderDevice_->GetRenderDevice());
        Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        deviceD3D12->CreateTextureFromD3DResource(
            reinterpret_cast<ID3D12Resource*>(d3d12Resource), Diligent::RESOURCE_STATE_UNKNOWN, &texture);
        if (!texture)
        {
            URHO3D_LOGERROR("Failed to create texture from existing ID3D12Resource pointer");
            return false;
        }

        return CreateFromHandle(texture, format, msaaLevel);
    }
#endif

    URHO3D_ASSERT(false, "RawTexture::CreateFromD3D12Resource is not supported on this platform");
    return false;
}

bool RawTexture::CreateFromVulkanImage(uint64_t vkImage, const RawTextureParams& params)
{
#if VULKAN_SUPPORTED
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::Vulkan)
    {
        Diligent::TextureDesc textureDesc;
        textureDesc.Name = "Texture from external resource";
        textureDesc.Type = textureTypeToDimensions[params.type_];
        textureDesc.Usage = Diligent::USAGE_DEFAULT;
        textureDesc.Format = params.format_;
        textureDesc.Width = params.size_.x_;
        textureDesc.Height = params.size_.y_;
        if (params.type_ == TextureType::Texture3D)
            textureDesc.Depth = params.size_.z_;
        else
            textureDesc.ArraySize = params.arraySize_;

        if (params.flags_.Test(TextureFlag::BindRenderTarget))
            textureDesc.BindFlags |= Diligent::BIND_RENDER_TARGET;
        if (params.flags_.Test(TextureFlag::BindDepthStencil))
            textureDesc.BindFlags |= Diligent::BIND_DEPTH_STENCIL;
        if (params.flags_.Test(TextureFlag::BindUnorderedAccess))
            textureDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

        textureDesc.MipLevels = params.numLevels_;
        textureDesc.SampleCount = params.multiSample_;

        auto deviceVk = static_cast<Diligent::IRenderDeviceVk*>(renderDevice_->GetRenderDevice());
        Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        deviceVk->CreateTextureFromVulkanImage(
            (VkImage)vkImage, textureDesc, Diligent::RESOURCE_STATE_UNKNOWN, &texture);
        if (!texture)
        {
            URHO3D_LOGERROR("Failed to create texture from existing VkImage pointer");
            return false;
        }

        return CreateFromHandle(texture, params.format_, params.multiSample_);
    }
#endif

    URHO3D_ASSERT(false, "RawTexture::CreateFromVulkanImage is not supported on this platform");
    return false;
}

bool RawTexture::CreateFromGLTexture(
    unsigned handle, TextureType type, TextureFlags flags, TextureFormat format, unsigned arraySize, int msaaLevel)
{
#if GL_SUPPORTED || GLES_SUPPORTED
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::OpenGL)
    {
        Diligent::TextureDesc textureDesc;
        textureDesc.Name = "Texture from external resource";
        textureDesc.Type = textureTypeToDimensions[type];
        textureDesc.Usage = Diligent::USAGE_DEFAULT;
        textureDesc.Format = format;
        if (type == TextureType::Texture2DArray)
            textureDesc.ArraySize = arraySize;

        if (flags.Test(TextureFlag::BindRenderTarget))
            textureDesc.BindFlags |= Diligent::BIND_RENDER_TARGET;
        if (flags.Test(TextureFlag::BindDepthStencil))
            textureDesc.BindFlags |= Diligent::BIND_DEPTH_STENCIL;
        if (flags.Test(TextureFlag::BindUnorderedAccess))
            textureDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

        auto deviceGL = static_cast<Diligent::IRenderDeviceGL*>(renderDevice_->GetRenderDevice());
        Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        deviceGL->CreateTextureFromGLHandle(
            handle, 0, textureDesc, Diligent::RESOURCE_STATE_UNKNOWN, &texture);
        if (!texture)
        {
            URHO3D_LOGERROR("Failed to create texture from existing GL texture handle");
            return false;
        }

        return CreateFromHandle(texture, format, msaaLevel);
    }
#endif

    URHO3D_ASSERT(false, "RawTexture::CreateFromGLTexture is not supported on this platform");
    return false;
}

bool RawTexture::CreateGPU()
{
    const bool isSRV = true; // flags_.Test(TextureFlag::BindShaderResource);
    const bool isRTV = params_.flags_.Test(TextureFlag::BindRenderTarget);
    const bool isDSV = params_.flags_.Test(TextureFlag::BindDepthStencil);
    const bool isUAV = params_.flags_.Test(TextureFlag::BindUnorderedAccess);

    Diligent::TextureDesc textureDesc;
    textureDesc.Type = textureTypeToDimensions[params_.type_];
    textureDesc.Name = GetDebugName().c_str();
    textureDesc.Usage = Diligent::USAGE_DEFAULT;
    textureDesc.Format = params_.format_;
    textureDesc.Width = params_.size_.x_;
    textureDesc.Height = params_.size_.y_;
    if (params_.type_ == TextureType::Texture3D)
        textureDesc.Depth = params_.size_.z_;
    else
        textureDesc.ArraySize = params_.arraySize_;

    textureDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE;
    if (isRTV)
        textureDesc.BindFlags |= Diligent::BIND_RENDER_TARGET;
    if (isDSV)
        textureDesc.BindFlags |= Diligent::BIND_DEPTH_STENCIL;
    if (isUAV)
        textureDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

    // Create main texture.
    // It is used as render target for auto-resolved multi-sampled texture.
    textureDesc.MipLevels = params_.numLevelsRTV_;
    if (params_.multiSample_ == 1)
    {
        textureDesc.SampleCount = 1;
        if (isRTV && params_.numLevelsRTV_ != 1)
            textureDesc.MiscFlags |= Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;
    }
    else
    {
        textureDesc.SampleCount = params_.multiSample_;
    }

    Diligent::IRenderDevice* device = renderDevice_->GetRenderDevice();
    device->CreateTexture(textureDesc, nullptr, &handles_.texture_);
    if (!handles_.texture_)
    {
        URHO3D_LOGERROR(
            "Failed to create texture: "
            "type={} format={} size={}x{}x{} arraySize={} numLevels={} multiSample={} flags=0b{:b}",
            params_.type_, Diligent::GetTextureFormatAttribs(params_.format_).Name, params_.size_.x_, params_.size_.y_,
            params_.size_.z_, params_.arraySize_, params_.numLevelsRTV_, params_.multiSample_,
            params_.flags_.AsInteger());

        return false;
    }

    // Create resolve texture if necessary.
    // It is used as shader resource for auto-resolved multi-sampled texture.
    if (params_.multiSample_ != 1 && !params_.flags_.Test(TextureFlag::NoMultiSampledAutoResolve))
    {
        textureDesc.MipLevels = params_.numLevels_;
        textureDesc.SampleCount = 1;
        if (params_.numLevels_ != 1)
            textureDesc.MiscFlags |= Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;

        device->CreateTexture(textureDesc, nullptr, &handles_.resolvedTexture_);
        if (!handles_.resolvedTexture_)
        {
            URHO3D_LOGERROR(
                "Failed to create resolve texture: "
                "type={} format={} size={}x{}x{} arraySize={} numLevels={} flags=0b{:b}",
                params_.type_, Diligent::GetTextureFormatAttribs(params_.format_).Name, params_.size_.x_,
                params_.size_.y_, params_.size_.z_, params_.arraySize_, params_.numLevels_, params_.flags_.AsInteger());

            return false;
        }
    }

    return InitializeDefaultViews(textureDesc.BindFlags);
}

bool RawTexture::InitializeDefaultViews(Diligent::BIND_FLAGS bindFlags)
{
    if (bindFlags & Diligent::BIND_SHADER_RESOURCE)
    {
        Diligent::ITexture* texture = handles_.resolvedTexture_ ? handles_.resolvedTexture_ : handles_.texture_;
        handles_.srv_ = GetDefaultView(texture, Diligent::TEXTURE_VIEW_SHADER_RESOURCE, params_.format_);

        if (!handles_.srv_)
        {
            URHO3D_LOGERROR("Failed to create shader resource view for texture");
            return false;
        }
    }

    if (bindFlags & Diligent::BIND_RENDER_TARGET)
    {
        handles_.rtv_ = GetDefaultView(handles_.texture_, Diligent::TEXTURE_VIEW_RENDER_TARGET, params_.format_);

        if (!handles_.rtv_)
        {
            URHO3D_LOGERROR("Failed to create render target view for texture");
            return false;
        }

        if (!CreateRenderSurfaces(handles_.rtv_, Diligent::TEXTURE_VIEW_RENDER_TARGET, handles_.renderSurfaces_))
        {
            URHO3D_LOGERROR("Failed to create render surfaces for texture");
            return false;
        }
    }

    if (bindFlags & Diligent::BIND_DEPTH_STENCIL)
    {
        handles_.dsv_ = GetDefaultView(handles_.texture_, Diligent::TEXTURE_VIEW_DEPTH_STENCIL, params_.format_);

        if (!handles_.dsv_)
        {
            URHO3D_LOGERROR("Failed to create depth-stencil view for texture");
            return false;
        }

        if (!CreateRenderSurfaces(handles_.dsv_, Diligent::TEXTURE_VIEW_DEPTH_STENCIL, handles_.renderSurfaces_))
        {
            URHO3D_LOGERROR("Failed to create render surfaces for texture");
            return false;
        }

        Diligent::TextureViewDesc dsvReadOnlyDesc = handles_.dsv_->GetDesc();
        dsvReadOnlyDesc.ViewType = Diligent::TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL;
        handles_.texture_->CreateView(dsvReadOnlyDesc, &handles_.dsvReadOnly_);

        if (!handles_.dsvReadOnly_)
        {
            URHO3D_LOGERROR("Failed to create read-only depth-stencil view for texture");
            return false;
        }

        if (!CreateRenderSurfaces(handles_.dsvReadOnly_, Diligent::TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL,
                handles_.renderSurfacesReadOnly_))
        {
            URHO3D_LOGERROR("Failed to create read-only render surfaces for texture");
            return false;
        }
    }

    if (bindFlags & Diligent::BIND_UNORDERED_ACCESS)
    {
        handles_.uav_ = GetDefaultView(handles_.texture_, Diligent::TEXTURE_VIEW_UNORDERED_ACCESS, params_.format_);

        if (!handles_.uav_)
        {
            URHO3D_LOGERROR("Failed to create unordered access view for texture");
            return false;
        }

        handles_.uavs_[RawTextureUAVKey{}] = handles_.uav_;
    }

    OnCreateGPU();
    return true;
}

void RawTexture::DestroyGPU()
{
    OnDestroyGPU();
    handles_ = {};
}

bool RawTexture::CreateRenderSurfaces(Diligent::ITextureView* defaultView, Diligent::TEXTURE_VIEW_TYPE viewType,
    ea::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>>& renderSurfaces)
{
    if (params_.type_ == TextureType::Texture2D)
    {
        renderSurfaces.emplace_back(defaultView);
    }
    else if (params_.type_ == TextureType::TextureCube || params_.type_ == TextureType::Texture2DArray)
    {
        for (unsigned i = 0; i < params_.arraySize_; ++i)
        {
            Diligent::TextureViewDesc viewDesc;
            viewDesc.ViewType = viewType;
            viewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
            viewDesc.FirstArraySlice = i;
            viewDesc.NumArraySlices = 1;

            Diligent::RefCntAutoPtr<Diligent::ITextureView> view;
            handles_.texture_->CreateView(viewDesc, &view);
            if (!view)
            {
                URHO3D_LOGERROR("Failed to create texture view for render surface");
                return false;
            }

            renderSurfaces.push_back(view);
        }
    }

    return true;
}

void RawTexture::GenerateLevels()
{
    if (params_.numLevels_ > 1)
    {
        if (!handles_.srv_)
        {
            URHO3D_LOGWARNING("RawTexture::GenerateMips is ignored for uninitialized texture");
            return;
        }

        Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
        immediateContext->GenerateMips(handles_.srv_);
    }

    levelsDirty_ = false;
}

void RawTexture::Resolve()
{
    if (handles_.resolvedTexture_)
    {
        Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
        Diligent::ResolveTextureSubresourceAttribs attribs;
        attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        for (unsigned slice = 0; slice < params_.arraySize_; ++slice)
        {
            attribs.SrcSlice = slice;
            attribs.DstSlice = slice;
            immediateContext->ResolveTextureSubresource(handles_.texture_, handles_.resolvedTexture_, attribs);
        }

        MarkDirty();
    }

    resolveDirty_ = false;
}

void RawTexture::Update(unsigned level, const IntVector3& offset, const IntVector3& size, unsigned arraySlice,
    const void* data, unsigned rowStride, unsigned sliceStride)
{
    URHO3D_PROFILE("RawTexture::Update");

    URHO3D_ASSERT(data, "Null data pointer");
    URHO3D_ASSERT(level < params_.numLevelsRTV_, "Invalid mip level");
    URHO3D_ASSERT(arraySlice < params_.arraySize_, "Invalid array slice");
    URHO3D_ASSERT(AllNotLess(offset, IntVector3::ZERO), "Negative update region offset");
    URHO3D_ASSERT(AllLess(IntVector3::ZERO, size), "Negative or zero update region size");
    URHO3D_ASSERT(AllNotLess(GetMipLevelSize(params_.size_, level), offset + size), "Invalid update region");

    const auto& formatInfo = Diligent::GetTextureFormatAttribs(params_.format_);
    if (params_.type_ != TextureType::Texture3D)
    {
        const IntVector2 blockSize{formatInfo.BlockWidth, formatInfo.BlockHeight};
        URHO3D_ASSERT(IsAligned(offset.ToIntVector2(), blockSize), "Unaligned update region offset");
    }

    if (!renderDevice_)
        return;

    if (!handles_)
    {
        URHO3D_LOGWARNING("RawTexture::Update is ignored for uninitialized texture");
        return;
    }

    const unsigned widthInBlocks = (size.x_ + formatInfo.BlockWidth - 1) / formatInfo.BlockWidth;
    const unsigned heightInBlocks = (size.y_ + formatInfo.BlockHeight - 1) / formatInfo.BlockHeight;

    Diligent::Box destBox;
    destBox.MinX = offset.x_;
    destBox.MaxX = offset.x_ + size.x_;
    destBox.MinY = offset.y_;
    destBox.MaxY = offset.y_ + size.y_;
    destBox.MinZ = offset.z_;
    destBox.MaxZ = offset.z_ + size.z_;

    Diligent::TextureSubResData resourceData;
    resourceData.pData = data;
    resourceData.Stride = rowStride ? rowStride : widthInBlocks * formatInfo.GetElementSize();
    resourceData.DepthStride = sliceStride ? sliceStride : heightInBlocks * widthInBlocks * formatInfo.GetElementSize();

    static constexpr unsigned alignment = 4;
    const bool isAligned = (resourceData.Stride % alignment == 0) && (resourceData.DepthStride % alignment == 0);
    if (!isAligned)
    {
        URHO3D_LOGWARNING(
            "RawTexture::Update is called with unaligned data with stride {} and depth stride {}."
            "The data is being repacked. Consider aligning the data rows to {} bytes.",
            resourceData.Stride, resourceData.DepthStride, alignment);

        const auto newStride = (resourceData.Stride + alignment - 1) / alignment * alignment;
        const auto newDepthStride = heightInBlocks * newStride;

        static thread_local ByteVector dataCopy;
        dataCopy.resize(newDepthStride * size.z_);

        auto dest = dataCopy.data();
        auto src = static_cast<const unsigned char*>(data);
        for (unsigned z = 0; z < size.z_; ++z)
        {
            for (unsigned y = 0; y < heightInBlocks; ++y)
            {
                memcpy(dest, src, resourceData.Stride);
                dest += newStride;
                src += resourceData.Stride;
            }
        }

        resourceData.pData = dataCopy.data();
        resourceData.Stride = newStride;
        resourceData.DepthStride = newDepthStride;
    }

    Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
    immediateContext->UpdateTexture(handles_.texture_, level, arraySlice, destBox, resourceData,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

bool RawTexture::Read(unsigned slice, unsigned level, void* buffer, unsigned bufferSize)
{
    if (!handles_)
    {
        URHO3D_LOGWARNING("RawTexture::Read is ignored for uninitialized texture");
        return false;
    }

    if (level >= params_.numLevels_)
    {
        URHO3D_ASSERTLOG(false, "Trying to read invalid mip level {}", level);
        return false;
    }
    if (slice >= params_.arraySize_)
    {
        URHO3D_ASSERTLOG(false, "Trying to read invalid array slice {}", slice);
        return false;
    }
    const auto sizeInBytes = GetMipLevelSizeInBytes(params_.size_, level, params_.format_);
    if (sizeInBytes > bufferSize)
    {
        URHO3D_ASSERTLOG(false, "Trying to read {} bytes of texture to the buffer of size {}", sizeInBytes, bufferSize);
        return false;
    }

    if (GetResolveDirty())
        Resolve();
    if (GetLevelsDirty())
        GenerateLevels();

    Diligent::IRenderDevice* device = renderDevice_->GetRenderDevice();
    Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
    const IntVector3 sizeInTexels = GetMipLevelSize(params_.size_, level);

    Diligent::TextureDesc textureDesc;
    textureDesc.Type = textureTypeToStagingDimensions[params_.type_];
    textureDesc.Name = "RawTexture::Read staging texture";
    textureDesc.Usage = Diligent::USAGE_STAGING;
    textureDesc.CPUAccessFlags = Diligent::CPU_ACCESS_READ;
    textureDesc.Format = params_.format_;
    textureDesc.Width = sizeInTexels.x_;
    textureDesc.Height = sizeInTexels.y_;
    textureDesc.Depth = sizeInTexels.z_;

    Diligent::RefCntAutoPtr<Diligent::ITexture> stagingTexture;
    device->CreateTexture(textureDesc, nullptr, &stagingTexture);
    if (!stagingTexture)
    {
        URHO3D_LOGERROR("Failed to create staging texture for RawTexture::Read");
        return false;
    }

    Diligent::CopyTextureAttribs attribs;
    attribs.pSrcTexture = handles_.resolvedTexture_ ? handles_.resolvedTexture_ : handles_.texture_;
    attribs.SrcMipLevel = level;
    attribs.SrcSlice = slice;
    attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    attribs.pDstTexture = stagingTexture;
    attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    immediateContext->CopyTexture(attribs);

    Diligent::MappedTextureSubresource mappedData;
    immediateContext->WaitForIdle();
    immediateContext->MapTextureSubresource(
        stagingTexture, 0, 0, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, nullptr, mappedData);

    if (!mappedData.pData)
    {
        URHO3D_LOGERROR("Failed to map staging texture for RawTexture::Read");
        return false;
    }

    const auto& formatInfo = Diligent::GetTextureFormatAttribs(params_.format_);

    const IntVector3 sizeInBlocks = GetSizeInBlocks(params_.size_, params_.format_);
    const unsigned rowSize = GetBlockSize(params_.format_) * sizeInBlocks.x_;

    const auto srcBuffer = reinterpret_cast<const unsigned char*>(mappedData.pData);
    auto destBuffer = reinterpret_cast<unsigned char*>(buffer);
    for (unsigned depth = 0; depth < sizeInBlocks.z_; ++depth)
    {
        for (unsigned row = 0; row < sizeInBlocks.y_; ++row)
        {
            memcpy(destBuffer, srcBuffer + depth * mappedData.DepthStride + row * mappedData.Stride, rowSize);
            destBuffer += rowSize;
        }
    }

    immediateContext->UnmapTextureSubresource(stagingTexture, 0, 0);
    return true;
}

void RawTexture::MarkDirty()
{
    if (params_.numLevels_ > 1)
        levelsDirty_ = true;
    if (params_.multiSample_ > 1 && !params_.flags_.Test(TextureFlag::NoMultiSampledAutoResolve))
        resolveDirty_ = true;
}

unsigned long long RawTexture::CalculateMemoryUseGPU() const
{
    if (!handles_)
        return 0;

    unsigned long long sliceMemory = 0;

    // If resolve texture is present, count the only MSAA slice of original texture
    if (handles_.resolvedTexture_)
        sliceMemory += params_.multiSample_ * GetMipLevelSizeInBytes(params_.size_, 0, params_.format_);

    // Count non-multisampled mip levels
    for (unsigned level = 0; level < params_.numLevels_; ++level)
        sliceMemory += GetMipLevelSizeInBytes(params_.size_, level, params_.format_);

    return params_.arraySize_ * sliceMemory;
}

} // namespace Urho3D
