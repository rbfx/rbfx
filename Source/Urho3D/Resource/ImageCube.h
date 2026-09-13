// Copyright (c) 2020-2020 the Urho3D project.
// Copyright (c) 2020-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Math/SphericalHarmonics.h"
#include "Urho3D/Resource/Image.h"
#include "Urho3D/Resource/Resource.h"

#include <EASTL/optional.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class Deserializer;
class Image;
class XMLFile;

/// Cube texture resource.
class URHO3D_API ImageCube : public Resource
{
    URHO3D_OBJECT(ImageCube, Resource);

public:
    /// Construct.
    explicit ImageCube(Context* context);
    /// Destruct.
    ~ImageCube() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Return face images.
    const ea::vector<SharedPtr<Image>>& GetImages() const { return faceImages_; }
    /// Return parameters XML.
    XMLFile* GetParametersXML() const { return parametersXml_; }
    /// Return image data from a face's zero mip level.
    Image* GetImage(CubeMapFace face) const { return faceImages_[face]; }
    /// Return mip level used for SH calculation.
    unsigned GetSphericalHarmonicsMipLevel() const;
    /// Return decompressed cube image mip level.
    SharedPtr<ImageCube> GetDecompressedImageLevel(unsigned index) const;
    /// Return decompressed cube image.
    SharedPtr<ImageCube> GetDecompressedImage() const;

    /// Return nearest pixel color at given direction.
    Color SampleNearest(const Vector3& direction) const;
    /// Return offset from the center of the unit cube for given texel (assuming zero mip level).
    Vector3 ProjectTexelOnCube(CubeMapFace face, int x, int y) const;
    /// Return offset from the center of the unit cube for given texel.
    Vector3 ProjectTexelOnCubeLevel(CubeMapFace face, int x, int y, unsigned level) const;
    /// Project direction on texel of cubemap face.
    ea::pair<CubeMapFace, IntVector2> ProjectDirectionOnFaceTexel(const Vector3& direction) const;
    /// Calculate spherical harmonics for the cube map.
    SphericalHarmonicsColor9 CalculateSphericalHarmonics() const;
    /// Return cached spherical harmonics or recalculate from image if cache is not available.
    SphericalHarmonicsDot9 GetOrCreateSphericalHarmonics() const;
    /// Return spherical harmonics, if the loaded file provided or requested them.
    const ea::optional<SphericalHarmonicsDot9>& GetSphericalHarmonics() const { return cachedSphericalHarmonics_; }

    /// Project UV onto cube.
    static Vector3 ProjectUVOnCube(CubeMapFace face, const Vector2& uv);
    /// Project direction onto cubemap.
    static ea::pair<CubeMapFace, Vector2> ProjectDirectionOnFace(const Vector3& direction);

private:
    /// Face images.
    ea::vector<SharedPtr<Image>> faceImages_;
    /// Parameter file.
    SharedPtr<XMLFile> parametersXml_;
    /// Cube width.
    int width_{};
    /// Precalculated spherical harmonics.
    ea::optional<SphericalHarmonicsDot9> cachedSphericalHarmonics_;
};

}
