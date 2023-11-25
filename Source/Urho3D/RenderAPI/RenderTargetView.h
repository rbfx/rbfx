// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/Texture.h>
#include <Diligent/Graphics/GraphicsEngine/interface/TextureView.h>

#include <EASTL/optional.h>

namespace Urho3D
{

class RawTexture;
class RenderDevice;

/// Lightweight reference to a render target view in texture or in swap chain. Cannot be null.
class URHO3D_API RenderTargetView
{
public:
    /// Construct.
    /// @{
    static RenderTargetView SwapChainColor(RenderDevice* renderDevice);
    static RenderTargetView SwapChainDepthStencil(RenderDevice* renderDevice);
    static RenderTargetView Texture(RawTexture* texture);
    static RenderTargetView TextureSlice(RawTexture* texture, unsigned slice);
    static RenderTargetView ReadOnlyDepth(RawTexture* texture);
    static RenderTargetView ReadOnlyDepthSlice(RawTexture* texture, unsigned slice);
    /// @}

    /// Mark referenced texture as dirty.
    void MarkDirty() const;
    /// Return effective view handle.
    Diligent::ITextureView* GetView() const;
    /// Return texture format.
    TextureFormat GetFormat() const;
    /// Return multi-sample level.
    int GetMultiSample() const;

    /// Return whether the view belongs to the swap chain.
    bool IsSwapChain() const { return type_ == Type::SwapChainColor || type_ == Type::SwapChainDepthStencil; }

private:
    enum class Type
    {
        Resource,
        ResourceSlice,
        ReadOnlyResource,
        ReadOnlyResourceSlice,
        SwapChainColor,
        SwapChainDepthStencil,
    };

    RenderTargetView(RenderDevice* renderDevice, RawTexture* texture, Type type, unsigned slice);

    RenderDevice* renderDevice_{};
    RawTexture* texture_{};
    Type type_{};
    unsigned slice_{};
};

using OptionalRawTextureRTV = ea::optional<RenderTargetView>;

} // namespace Urho3D
