// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Resource/Image.h"

namespace Urho3D
{

/// Decompress a DXT compressed image to RGBA.
URHO3D_API void
    DecompressImageDXT(unsigned char* rgba, const void* blocks, int width, int height, int depth, TextureFormat format);
/// Decompress an ETC1/ETC2 compressed image to RGBA.
URHO3D_API void DecompressImageETC(unsigned char* dstImage, const void* blocks, int width, int height, bool hasAlpha);
/// Decompress a PVRTC compressed image to RGBA.
URHO3D_API void DecompressImagePVRTC(unsigned char* rgba, const void* blocks, int width, int height, TextureFormat format);

/// Returns whether flipping is implemented for texture format.
URHO3D_API bool IsFlipBlockImplemented(TextureFormat format);
/// Flip a compressed block vertically.
URHO3D_API void FlipBlockVertical(unsigned char* dest, const unsigned char* src, TextureFormat format);
/// Flip a compressed block horizontally.
URHO3D_API void FlipBlockHorizontal(unsigned char* dest, const unsigned char* src, TextureFormat format);

}
