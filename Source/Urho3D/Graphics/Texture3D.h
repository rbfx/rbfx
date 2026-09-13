// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Graphics/RenderSurface.h"
#include "Urho3D/Graphics/Texture.h"
#include "Urho3D/Resource/Image.h"

namespace Urho3D
{

/// 3D texture resource.
class URHO3D_API Texture3D : public Texture
{
    URHO3D_OBJECT(Texture3D, Texture);

public:
    /// Construct.
    explicit Texture3D(Context* context);
    /// Destruct.
    ~Texture3D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Set size, format and usage. Zero size will follow application window size. Return true if successful.
    bool SetSize(int width, int height, int depth, TextureFormat format, TextureFlags flags = TextureFlag::None);
    /// Set data either partially or fully on a mip level. Return true if successful.
    bool SetData(unsigned level, int x, int y, int z, int width, int height, int depth, const void* data);
    /// Set data from an image. Return true if successful. Optionally make a single channel image alpha-only.
    bool SetData(Image* image);

    /// Get data from a mip level. The destination buffer must be big enough. Return true if successful.
    bool GetData(unsigned level, void* dest);

private:
    /// Image file acquired during BeginLoad.
    SharedPtr<Image> loadImage_;
    /// Parameter file acquired during BeginLoad.
    SharedPtr<XMLFile> loadParameters_;
};

}
