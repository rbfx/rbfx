//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../SystemUI/TextureCubeInspectorWidget.h"

#include "../Graphics/TextureCube.h"
#include "../SystemUI/SystemUI.h"

namespace Urho3D
{

namespace
{
const StringVector textureFilterModes{
    "NEAREST",
    "BILINEAR",
    "TRILINEAR",
    "ANISOTROPIC",
    "NEAREST_ANISOTROPIC",
    "DEFAULT",
};
const StringVector textureAddressMode{
    "WRAP",
    "MIRROR",
    "CLAMP",
    "BORDER",
};

} // namespace

const ea::vector<ResourceInspectorWidget::PropertyDesc> TextureCubeInspectorWidget::properties{
    {
        "SRGB",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const TextureCube*>(resource)->GetSRGB()}; },
        [](Resource* resource, const Variant& value) { static_cast<TextureCube*>(resource)->SetSRGB(value.GetBool()); },
        "SRGB",
    },
    {
        "Linear",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const TextureCube*>(resource)->GetLinear()}; },
        [](Resource* resource, const Variant& value) { static_cast<TextureCube*>(resource)->SetLinear(value.GetBool()); },
        "Linear color space",
    },
    {
        "Filter Mode",
        Variant{TextureFilterMode::FILTER_DEFAULT},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetFilterMode())}; },
        [](Resource* resource, const Variant& value)
        { static_cast<TextureCube*>(resource)->SetFilterMode(static_cast<TextureFilterMode>(value.GetInt())); },
        "Texture Filter Mode",
        Widgets::EditVariantOptions{}.Enum(textureFilterModes),
    },
    {
        "U Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::U))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(TextureCoordinate::U, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "U texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
    {
        "V Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::V))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(TextureCoordinate::V, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "V texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
    {
        "W Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::W))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(
                TextureCoordinate::W, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "W texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
};

TextureCubeInspectorWidget::TextureCubeInspectorWidget(Context* context, const ResourceVector& resources)
    : BaseClassName(context, resources, ea::span(properties.begin(), properties.end()))
{
}

TextureCubeInspectorWidget::~TextureCubeInspectorWidget()
{
}

} // namespace Urho3D
