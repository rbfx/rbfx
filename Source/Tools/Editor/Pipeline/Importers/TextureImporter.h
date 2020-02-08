//
// Copyright (c) 2017-2020 the rbfx project.
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

#include <Urho3D/IO/Log.h>

#include "Pipeline/Importers/AssetImporter.h"

namespace Urho3D
{

class TextureImporter : public AssetImporter
{
    URHO3D_OBJECT(TextureImporter, AssetImporter);
public:
    enum class MipMode
    {
        /// Do not output any mipmaps
        None,
        /// Always generate a new mipmap chain (ignore source mipmaps)
        Generate,
        /// Use source mipmaps if possible, or create new mipmaps.
        UseSourceOrGenerate,
        /// Always use source mipmaps, if any (never generate new mipmaps)
        UseSource,
    };

    enum class MipFilter
    {
        Box,
        Tent,
        Lanczos4,
        Mitchell,
        Kaiser,
    };

    enum class Compressor
    {
        CRN,
        CRNF,
        RYG,
        ATI,
    };

    enum class DxtQuality
    {
        Superfast,
        Fast,
        Normal,
        Better,
        Uber
    };

    enum class PixelFormat
    {
        None,
        DXT1,
        DXT2,
        DXT3,
        DXT4,
        DXT5,
        _3DC,
        DXN,
        DXT5A,
        DXT5_CCxY,
        DXT5_xGxR,
        DXT5_xGBR,
        DXT5_AGBR,
        DXT1A,
        ETC1,
        ETC2,
        ETC2A,
        R8G8B8,
        L8,
        A8,
        A8L8,
        A8R8G8B8,
    };

    static const char* mipModeNames[];
    static const char* mipFilterNames[];
    static const char* compressorNames[];
    static const char* dxtQualityNames[];
    static const char* pixelFormatNames[];

    explicit TextureImporter(Context* context);
    /// Register object with the engine.
    static void RegisterObject(Context* context);
    ///
    bool Accepts(const ea::string& path) const override;
    ///
    bool Execute(Urho3D::Asset* input, const ea::string& outputPath) override;

protected:
    ///
    void ApplyBlurLimit();
    ///
    void ApplyMipsLimits();
    ///
    void ApplyQualityLimits();
    ///
    void ApplyAlphaThresholdLimits();

    /// Always flip texture on Y axis before processing.
    bool yFlip_ = false;
    /// Unflip texture if read from source file as flipped.
    bool unflip_ = false;
    ///
    uint8_t quality_ = 255u;
    ///
    uint8_t bitrate_ = 0u;
    ///
    MipMode mipMode_ = MipMode::Generate;
    ///
    MipFilter mipFilter_ = MipFilter::Kaiser;
    ///
    float gamma_ = 2.2f;
    ///
    float blur_ = 0.9f;
    /// Assume texture is tiled when filtering, default=clamping.
    bool wrap_ = false;
    ///
    bool renormalize_ = false;
    ///
    uint8_t maxMips_ = 16;
    ///
    uint8_t minMipSize_ = 1;
    ///
    uint8_t alphaThreshold_ = 128u;
    ///
    bool uniformMetrics_ = false;
    ///
    bool adaptiveBlocks_ = true;
    ///
    Compressor compressor_ = Compressor::CRN;
    ///
    DxtQuality dxtQuality_ = DxtQuality::Uber;
    /// Don't try reusing previous DXT endpoint solutions.
    bool noEndpointCaching_ = false;
    /// Assume shader will convert fetched results to luma (Y).
    bool grayscaleSampling_ = false;
    /// Only use DXT1 color4 and DXT5 alpha8 block encodings.
    bool forcePrimaryEncoding_ = false;
    ///
    bool useTransparentIndicesForBlack_ = false;
    ///
    PixelFormat pixelFormat_ = PixelFormat::None;
    ///
    Logger logger_ = Log::GetLogger(ClassName::GetTypeNameStatic());
};


}
