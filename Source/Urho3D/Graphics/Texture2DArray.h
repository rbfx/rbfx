// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Graphics/RenderSurface.h"
#include "Urho3D/Graphics/Texture.h"

namespace Urho3D
{

class Deserializer;
class Image;

/// 2D texture array resource.
class URHO3D_API Texture2DArray : public Texture
{
    URHO3D_OBJECT(Texture2DArray, Texture);

public:
    /// Construct.
    explicit Texture2DArray(Context* context);
    /// Destruct.
    ~Texture2DArray() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Set the number of layers in the texture. To be used before SetData.
    /// @property
    void SetLayers(unsigned layers);
    /// Set layers, size, format and usage. Set layers to zero to leave them unchanged. Return true if successful.
    bool SetSize(unsigned layers, int width, int height, TextureFormat format, TextureFlags flags = TextureFlag::None);
    /// Set data either partially or fully on a layer's mip level. Return true if successful.
    bool SetData(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data);
    /// Set data of one layer from a stream. Return true if successful.
    bool SetData(unsigned layer, Deserializer& source);
    /// Set data of one layer from an image. Return true if successful. Optionally make a single channel image alpha-only.
    bool SetData(unsigned layer, Image* image);

    /// Return number of layers in the texture.
    /// @property
    unsigned GetLayers() const { return layers_; }
    /// Get data from a mip level. The destination buffer must be big enough. Return true if successful.
    bool GetData(unsigned layer, unsigned level, void* dest);

private:
    /// Texture array layers number.
    unsigned layers_{};
    /// Layer image files acquired during BeginLoad.
    ea::vector<SharedPtr<Image> > loadImages_;
    /// Parameter file acquired during BeginLoad.
    SharedPtr<XMLFile> loadParameters_;
};

}
