// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/DeviceObject.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/Texture.h>
#include <Diligent/Graphics/GraphicsEngine/interface/TextureView.h>

#include <EASTL/tuple.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

struct URHO3D_API RawTextureParams
{
    TextureType type_{};
    TextureFormat format_{};
    TextureFlags flags_{};

    /// Size of the topmost mip level of the single 2D texture for 2D and cube textures and arrays.
    /// Size of the topmost mip level for 3D textures.
    IntVector3 size_;
    /// Array size for array types. Ignored for single textures.
    unsigned arraySize_{1};
    /// Number of mip levels. 0 to deduce automatically.
    unsigned numLevels_{};
    /// Number of samples per pixel. 1 to disable multi-sampling.
    unsigned multiSample_{1};
    /// Number of mip levels of the render target. This value is deduced automatically.
    /// If the texture used multi-sampling with automatic resolve and has multiple mip levels, two textures are created:
    /// multi-sampled texture with single mip level (aka `numLevelsRTV_`) used as RTV,
    /// and resolved texture with multiple mip levels (aka `numLevels_`) used as SRV.
    /// Otherwise, `numLevelsRTV_` is equal to `numLevels_`.
    unsigned numLevelsRTV_{};

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const
    {
        return ea::tie(type_, format_, flags_, size_, arraySize_, numLevels_, multiSample_, numLevelsRTV_);
    }
#endif
    bool operator==(const RawTextureParams& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const RawTextureParams& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

struct URHO3D_API RawTextureUAVKey
{
    bool canWrite_{true};
    bool canRead_{true};
    /// The first array slice or cube face to be viewed.
    unsigned firstSlice_{};
    /// The first mip level to be viewed.
    unsigned firstLevel_{};
    /// The number of array slices or cube faces to be viewed. 0 to deduce automatically.
    unsigned numSlices_{};
    /// The number of mip levels to be viewed. 0 to deduce automatically.
    unsigned numLevels_{};

    // clang-format off
    RawTextureUAVKey& ReadOnly() { canWrite_ = false; return *this; }
    RawTextureUAVKey& WriteOnly() { canRead_ = false; return *this; }
    RawTextureUAVKey& FromLevel(unsigned level, unsigned numLevels = 0) { firstLevel_ = level; numLevels_ = numLevels; return *this; }
    RawTextureUAVKey& FromSlice(unsigned slice, unsigned numSlices = 0) { firstSlice_ = slice; numSlices_ = numSlices; return *this; }
    // clang-format on

    /// Operators.
    /// @{
#ifndef SWIG
    auto Tie() const { return ea::tie(canWrite_, canRead_, firstSlice_, firstLevel_, numSlices_, numLevels_); }
#endif
    bool operator==(const RawTextureUAVKey& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const RawTextureUAVKey& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return MakeHash(Tie()); }
    /// @}
};

struct URHO3D_API RawTextureHandles
{
    /// Main texture. If texture is multi-sampled, only one mip level is created.
    Diligent::RefCntAutoPtr<Diligent::ITexture> texture_;
    /// If texture is multi-sampled, resolved texture is created with requested number of mip levels.
    Diligent::RefCntAutoPtr<Diligent::ITexture> resolvedTexture_;
    /// Texture view that can be used as readable shader resource.
    /// If resolved texture is created, it is referenced by this view.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> srv_;
    /// Texture view that can be used as render target.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> rtv_;
    /// Texture view that can be used as depth-stencil buffer.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> dsv_;
    /// Texture view that can be used as read-only depth-stencil buffer.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> dsvReadOnly_;
    /// Texture view that can be attached as unordered access resource.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> uav_;
    /// Array of all 2D render target views for each array slice and for each cube texture face.
    /// Empty for 3D textures.
    ea::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>> renderSurfaces_;
    /// Same as above, but read-only. Valid only for depth-stencil textures.
    ea::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>> renderSurfacesReadOnly_;
    /// All requested UAVs.
    ea::unordered_map<RawTextureUAVKey, Diligent::RefCntAutoPtr<Diligent::ITextureView>> uavs_;

    /// Syntax sugar.
    /// @{
    bool IsValid() const { return texture_ != nullptr; }
    operator bool() const { return IsValid(); }
    /// @}
};

/// Common class for all GPU textures.
/// By default RawTexture loses data on device lost and does not attempt to recover it.
/// This behavior can be changed in derived classes.
/// RawTexture is RAII object that's not supposed to be in invalid state.
/// However, this behavior can also be changed in derived classes.
class URHO3D_API RawTexture : public DeviceObject
{
public:
    RawTexture(Context* context, const RawTextureParams& params);
    ~RawTexture() override;

    /// Create from texture handle.
    bool CreateFromHandle(Diligent::ITexture* texture, TextureFormat format, int msaaLevel);
    /// Create texture from raw ID3D11Texture2D pointer.
    bool CreateFromD3D11Texture2D(void* d3d11Texture2D, TextureFormat format, int msaaLevel);
    /// Create texture from raw ID3D12Resource pointer.
    bool CreateFromD3D12Resource(void* d3d12Resource, TextureFormat format, int msaaLevel);
    /// Create texture from VkImage handle.
    bool CreateFromVulkanImage(uint64_t vkImage, const RawTextureParams& params);
    /// Create texture from raw OpenGL handle.
    bool CreateFromGLTexture(
        unsigned handle, TextureType type, TextureFlags flags, TextureFormat format, unsigned arraySize, int msaaLevel);

    /// Set default sampler to be used for this texture.
    void SetSamplerStateDesc(const SamplerStateDesc& desc) { samplerDesc_ = desc; }

    /// Create UAV for given array slices and mip levels.
    Diligent::ITextureView* CreateUAV(const RawTextureUAVKey& key);
    Diligent::ITextureView* GetUAV(const RawTextureUAVKey& key) const;

    /// Generate mip levels from the topmost level. Avoid calling it during the rendering.
    void GenerateLevels();
    /// Resolve multi-sampled texture to the simple resolved texture.
    void Resolve();
    /// Update texture data. If strides are not specified, they are deduced automatically from `size`.
    void Update(unsigned level, const IntVector3& offset, const IntVector3& size, unsigned arraySlice, const void* data,
        unsigned rowStride = 0, unsigned sliceStride = 0);
    /// Read texture data from GPU. This operation is very slow and shouldn't be used in real time.
    bool Read(unsigned slice, unsigned level, void* buffer, unsigned bufferSize);
    /// For render target and depth-stencil textures, mark shader resource view dirty.
    void MarkDirty();

    /// Evaluate approximate memory footprint of the texture on GPU.
    unsigned long long CalculateMemoryUseGPU() const;

    /// Implement DeviceObject.
    /// @{
    void Invalidate() override;
    void Restore() override;
    void Destroy() override;
    /// @}

    /// Getters.
    /// @{
    const RawTextureParams& GetParams() const { return params_; }
    const SamplerStateDesc& GetSamplerStateDesc() const { return samplerDesc_; }
    const RawTextureHandles& GetHandles() const { return handles_; }
    /// @}

    /// Internal.
    /// @{
    bool GetLevelsDirty() const { return levelsDirty_; }
    bool GetResolveDirty() const { return resolveDirty_; }
    /// @}

protected:
    /// Create empty texture.
    explicit RawTexture(Context* context);
    /// Validate parameters and create GPU texture.
    bool Create(const RawTextureParams& params);

    /// Create GPU texture from current parameters.
    bool CreateGPU();
    /// Destroy all GPU resources.
    void DestroyGPU();

    /// Called when GPU handles are created.
    virtual void OnCreateGPU() {}
    /// Called when GPU handles are destroyed.
    virtual void OnDestroyGPU() {}
    /// Try to recover texture data after device loss.
    virtual bool TryRestore() { return false; }

private:
    bool InitializeDefaultViews(Diligent::BIND_FLAGS bindFlags);
    bool CreateRenderSurfaces(Diligent::ITextureView* defaultView, Diligent::TEXTURE_VIEW_TYPE viewType,
        ea::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>>& renderSurfaces);

    RawTextureParams params_;
    SamplerStateDesc samplerDesc_;

    RawTextureHandles handles_;

    bool levelsDirty_{};
    bool resolveDirty_{};
};

} // namespace Urho3D
