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

#pragma once

#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/RenderSurface.h"
#include "../Math/Color.h"
#include "../Resource/Resource.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/Sampler.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Texture.h>
#include <Diligent/Graphics/GraphicsEngine/interface/TextureView.h>

namespace Urho3D
{

static const int MAX_TEXTURE_QUALITY_LEVELS = 3;

class Image;
class XMLElement;
class XMLFile;

/// Base class for texture resources.
class URHO3D_API Texture : public ResourceWithMetadata, public RawTexture
{
    URHO3D_OBJECT(Texture, ResourceWithMetadata);

public:
    /// Construct.
    explicit Texture(Context* context);
    /// Destruct.
    ~Texture() override;

    /// Set default texture sampler.
    /// If this is called for already used texture, send ReloadFinished event from this texture.
    /// @{

    /// Set filtering mode.
    /// @property
    void SetFilterMode(TextureFilterMode mode);
    /// Set addressing mode by texture coordinate.
    /// @property
    void SetAddressMode(TextureCoordinate coord, TextureAddressMode mode);
    /// Set texture max. anisotropy level. No effect if not using anisotropic filtering. Value 0 (default) uses the default setting from Renderer.
    /// @property
    void SetAnisotropy(unsigned level);
    /// Set shadow compare mode.
    void SetShadowCompare(bool enable);

    /// @}

    /// Set texture properties.
    /// Texture should be reloaded after calling this.
    /// @{

    /// Set number of requested mip levels. Needs to be called before setting size.
    /** The default value (0) allocates as many mip levels as necessary to reach 1x1 size. Set value 1 to disable mipmapping.
        Note that rendertargets need to regenerate mips dynamically after rendering, which may cost performance. Screen buffers
        and shadow maps allocated by Renderer will have mipmaps disabled.
     */
    void SetNumLevels(unsigned levels);
    /// Set whether the texture data is in linear color space (instead of gamma space).
    void SetLinear(bool linear);
    /// Set sRGB sampling and writing mode.
    /// @property
    void SetSRGB(bool enable);
    /// Set backup texture to use when rendering to this texture.
    /// @property
    void SetBackupTexture(Texture* texture);
    /// Set mip levels to skip on a quality setting when loading. Ensures higher quality levels do not skip more.
    /// @property
    void SetMipsToSkip(MaterialQuality quality, int toSkip);

    /// @}

    /// Return API-specific texture format.
    /// @property
    TextureFormat GetFormat() const { return GetParams().format_; }

    /// Return whether the texture format is compressed.
    /// @property
    bool IsCompressed() const;

    /// Return number of mip levels.
    /// @property
    unsigned GetLevels() const { return GetParams().numLevels_; }

    /// Return width.
    /// @property
    int GetWidth() const { return GetParams().size_.x_; }

    /// Return height.
    /// @property
    int GetHeight() const { return GetParams().size_.y_; }

    /// Return size.
    IntVector2 GetSize() const { return GetParams().size_.ToIntVector2(); }

    /// Return viewport rectange.
    IntRect GetRect() const { return { 0, 0, GetWidth(), GetHeight() }; }

    /// Return depth.
    int GetDepth() const { return GetParams().size_.z_; }

    /// Return filtering mode.
    /// @property
    TextureFilterMode GetFilterMode() const { return GetSamplerStateDesc().filterMode_; }

    /// Return addressing mode by texture coordinate.
    /// @property
    TextureAddressMode GetAddressMode(TextureCoordinate coord) const { return GetSamplerStateDesc().addressMode_[coord]; }

    /// Return texture max. anisotropy level. Value 0 means to use the default value from Renderer.
    /// @property
    unsigned GetAnisotropy() const { return GetSamplerStateDesc().anisotropy_; }

    /// Return whether shadow compare is enabled.
    bool GetShadowCompare() const { return GetSamplerStateDesc().shadowCompare_; }

    /// Return whether the texture data are in linear space (instead of gamma space).
    bool GetLinear() const { return linear_; }

    /// Return whether is using sRGB sampling and writing.
    /// @property
    bool GetSRGB() const;

    /// Return texture multisampling level (1 = no multisampling).
    /// @property
    int GetMultiSample() const { return GetParams().multiSample_; }

    /// Return texture multisampling autoresolve mode. When true, the texture is resolved before being sampled on SetTexture(). When false, the texture will not be resolved and must be read as individual samples in the shader.
    /// @property
    bool GetAutoResolve() const { return !GetParams().flags_.Test(TextureFlag::NoMultiSampledAutoResolve); }

    /// Return backup texture.
    /// @property
    Texture* GetBackupTexture() const { return backupTexture_; }

    /// Return render surface for given index.
    RenderSurface* GetRenderSurface(unsigned index = 0) const
    {
        return index < renderSurfaces_.size() ? renderSurfaces_[index] : nullptr;
    }

    /// Return mip levels to skip on a quality setting when loading.
    /// @property
    int GetMipsToSkip(MaterialQuality quality) const;
    /// Return mip level width, or 0 if level does not exist.
    /// @property
    int GetLevelWidth(unsigned level) const;
    /// Return mip level width, or 0 if level does not exist.
    /// @property
    int GetLevelHeight(unsigned level) const;
    /// Return mip level depth, or 0 if level does not exist.
    int GetLevelDepth(unsigned level) const;

    /// Return data size in bytes for a rectangular region.
    unsigned GetDataSize(int width, int height) const;
    /// Return data size in bytes for a volume region.
    unsigned GetDataSize(int width, int height, int depth) const;
    /// Return data size in bytes for a pixel or block row.
    unsigned GetRowDataSize(int width) const;
    /// Return number of image components required to receive pixel data from GetData(), or 0 for compressed images.
    /// @property
    unsigned GetComponents() const;

    /// Set additional parameters from an XML file.
    void SetParameters(XMLFile* file);
    /// Set additional parameters from an XML element.
    void SetParameters(const XMLElement& element);

    /// Getters.
    /// @{
    bool IsRenderTarget() const { return GetParams().flags_.Test(TextureFlag::BindRenderTarget); }
    bool IsDepthStencil() const { return GetParams().flags_.Test(TextureFlag::BindDepthStencil); }
    bool IsUnorderedAccess() const { return GetParams().flags_.Test(TextureFlag::BindUnorderedAccess); }
    /// @}

private:
    /// Implement RawTexture.
    /// @{
    void OnCreateGPU() override;
    void OnDestroyGPU() override;
    bool TryRestore() override;
    /// @}

    /// Handle render surface update event.
    void HandleRenderSurfaceUpdate();

protected:
    /// Check whether texture memory budget has been exceeded. Free unused materials in that case to release the texture references.
    void CheckTextureBudget(StringHash type);

    /// Create texture so it can fit the image.
    /// Size and format are deduced from the image. Number of mips is adjusted according to the image.
    bool CreateForImage(const RawTextureParams& baseParams, Image* image);
    /// Set texture data from image.
    bool UpdateFromImage(unsigned arraySlice, Image* image);
    /// Read texture data to image.
    bool ReadToImage(unsigned arraySlice, unsigned level, Image* image);

    /// Requested mip levels.
    unsigned requestedLevels_{};
    /// Whether sRGB sampling and writing is requested.
    bool requestedSRGB_{};
    /// Mip levels to skip when loading per texture quality setting.
    unsigned mipsToSkip_[MAX_TEXTURE_QUALITY_LEVELS]{2, 1, 0};
    /// Whether the texture data is in linear color space (instead of gamma space).
    bool linear_{};
    /// Multisampling resolve needed -flag.
    bool resolveDirty_{};
    /// Mipmap levels regeneration needed -flag.
    bool levelsDirty_{};
    /// Backup texture.
    SharedPtr<Texture> backupTexture_;
    /// Render surface(s).
    ea::vector<SharedPtr<RenderSurface>> renderSurfaces_;
    /// Most detailed mip level currently used.
    unsigned mostDetailedLevel_{};
};

}
