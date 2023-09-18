// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

URHO3D_API bool IsOpenGLESBackend(RenderBackend backend);
URHO3D_API bool IsMetalBackend(RenderBackend backend);

URHO3D_API const ea::string& ToString(RenderBackend backend);
URHO3D_API const ea::string& ToString(ShaderType type);

URHO3D_API ea::optional<VertexShaderAttribute> ParseVertexAttribute(ea::string_view name);
URHO3D_API const ea::string& ToShaderInputName(VertexElementSemantic semantic);
URHO3D_API Diligent::SHADER_TYPE ToInternalShaderType(ShaderType type);

URHO3D_API unsigned GetMipLevelCount(const IntVector3& size);
URHO3D_API IntVector3 GetMipLevelSize(const IntVector3& size, unsigned level);

URHO3D_API bool IsTextureFormatSRGB(TextureFormat format);
URHO3D_API TextureFormat SetTextureFormatSRGB(TextureFormat format, bool sRGB = true);

URHO3D_API bool IsDepthTextureFormat(TextureFormat format);
URHO3D_API bool IsDepthStencilTextureFormat(TextureFormat format);
URHO3D_API bool IsColorTextureFormat(TextureFormat format);

URHO3D_API RenderBackend SelectRenderBackend(ea::optional<RenderBackend> requestedBackend);
URHO3D_API ShaderTranslationPolicy SelectShaderTranslationPolicy(
    RenderBackend backend, ea::optional<ShaderTranslationPolicy> requestedPolicy);

URHO3D_API void SerializeValue(Archive& archive, const char* name, RenderDeviceSettingsVulkan& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, RenderDeviceSettingsD3D12& value);

/// Try to find a suitable texture format for given internal GAPI format.
/// Only a subset of formats is supported.
URHO3D_API TextureFormat GetTextureFormatFromInternal(RenderBackend backend, unsigned internalFormat);

} // namespace Urho3D
