// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Container/Ptr.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Texture.h"
#include "../Math/SphericalHarmonics.h"
#include "../Resource/ImageCube.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class Deserializer;
class Image;

/// Cube texture resource.
class URHO3D_API TextureCube : public Texture
{
    URHO3D_OBJECT(TextureCube, Texture);

public:
    /// Construct.
    explicit TextureCube(Context* context);
    /// Destruct.
    ~TextureCube() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Set size, format, usage and multisampling parameter for rendertargets. Note that cube textures always use autoresolve when multisampled due to lacking support (on all APIs) to multisample them in a shader. Return true if successful.
    bool SetSize(int size, TextureFormat format, TextureFlags flags = TextureFlag::None, int multiSample = 1);
    /// Set data either partially or fully on a face's mip level. Return true if successful.
    bool SetData(CubeMapFace face, unsigned level, int x, int y, int width, int height, const void* data);
    /// Set data of one face from a stream. Return true if successful.
    bool SetData(CubeMapFace face, Deserializer& source);
    /// Set data of one face from an image. Return true if successful. Optionally make a single channel image alpha-only.
    bool SetData(CubeMapFace face, Image* image);

    /// Get data from a face's mip level. The destination buffer must be big enough. Return true if successful.
    bool GetData(CubeMapFace face, unsigned level, void* dest);
    /// Get image data from a face's zero mip level. Only RGB and RGBA textures are supported.
    SharedPtr<Image> GetImage(CubeMapFace face);

    /// Return spherical harmonics of the texture, if the source image provided them on load.
    const ea::optional<SphericalHarmonicsDot9>& GetSphericalHarmonics() const { return sphericalHarmonics_; }

    /// Return render surface for one face.
    /// @property{get_renderSurfaces}
    RenderSurface* GetRenderSurface(CubeMapFace face) const { return Texture::GetRenderSurface(face); }

private:
    /// Face image files acquired during BeginLoad.
    SharedPtr<ImageCube> loadImageCube_;
    /// Spherical harmonics, copied from the source image on load.
    ea::optional<SphericalHarmonicsDot9> sphericalHarmonics_;
};

}
