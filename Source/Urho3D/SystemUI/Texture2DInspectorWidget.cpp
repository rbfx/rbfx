// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/SystemUI/Texture2DInspectorWidget.h"

#include "Urho3D/SystemUI/SystemUI.h"

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

const ea::vector<ResourceInspectorWidget::PropertyDesc> Texture2DInspectorWidget::properties{
    {
        "SRGB",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const Texture2D*>(resource)->GetSRGB()}; },
        [](Resource* resource, const Variant& value) { static_cast<Texture2D*>(resource)->SetSRGB(value.GetBool()); },
        "SRGB",
    },
    {
        "Linear",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const Texture2D*>(resource)->GetLinear()}; },
        [](Resource* resource, const Variant& value) { static_cast<Texture2D*>(resource)->SetLinear(value.GetBool()); },
        "Linear color space",
    },
    {
        "Filter Mode",
        Variant{TextureFilterMode::FILTER_DEFAULT},
        [](const Resource* resource)
        {
            return Variant{static_cast<int>(static_cast<const Texture2D*>(resource)->GetFilterMode())};
        },
        [](Resource* resource, const Variant& value)
        {
            static_cast<Texture2D*>(resource)->SetFilterMode(static_cast<TextureFilterMode>(value.GetInt()));
        },
        "Texture Filter Mode",
        Widgets::EditVariantOptions{}.Enum(textureFilterModes),
    },
    {
        "U Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const Texture2D*>(resource)->GetAddressMode(TextureCoordinate::U))}; },
        [](Resource* resource, const Variant& value)
        { static_cast<Texture2D*>(resource)->SetAddressMode(TextureCoordinate::U, static_cast<TextureAddressMode>(value.GetInt())); },
        "U texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
    {
        "V Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const Texture2D*>(resource)->GetAddressMode(TextureCoordinate::V))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<Texture2D*>(resource)->SetAddressMode(TextureCoordinate::V, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "V texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
};

Texture2DInspectorWidget::Texture2DInspectorWidget(Context* context, const ResourceVector& resources)
    : BaseClassName(context, resources, ea::span(properties.begin(), properties.end()))
{
}

Texture2DInspectorWidget::~Texture2DInspectorWidget()
{
}

} // namespace Urho3D
