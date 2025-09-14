//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/Resource/Decompress.h"
#include "Urho3D/Resource/ImageDDS.h"

#include <STB/stb_image.h>
#include <STB/stb_image_write.h>

#include <EASTL/utility.h>

#include <SDL_surface.h>
#ifdef URHO3D_WEBP
    #include <webp/decode.h>
    #include <webp/encode.h>
    #include <webp/mux.h>
#endif

#include "Urho3D/DebugNew.h"

STBIWDEF unsigned char* stbi_write_png_to_mem(const unsigned char* pixels, int stride_bytes, int x, int y, int n, int* out_len);

namespace Urho3D
{

namespace
{

static constexpr unsigned FOURCC_DX10 = MakeFourCC('D', 'X', '1', '0');

static const unsigned DDSCAPS_COMPLEX = 0x00000008U;
static const unsigned DDSCAPS_TEXTURE = 0x00001000U;
static const unsigned DDSCAPS_MIPMAP = 0x00400000U;
static const unsigned DDSCAPS2_VOLUME = 0x00200000U;
static const unsigned DDSCAPS2_CUBEMAP = 0x00000200U;

static const unsigned DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
static const unsigned DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
static const unsigned DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

// DX10 flags
static const unsigned DDS_DIMENSION_TEXTURE1D = 2;
static const unsigned DDS_DIMENSION_TEXTURE2D = 3;
static const unsigned DDS_DIMENSION_TEXTURE3D = 4;

static const unsigned DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;

IntVector3 ToIntVector3(unsigned x, unsigned y, unsigned z)
{
    return {static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)};
}

} // namespace

bool CompressedLevel::Decompress(unsigned char* dest) const
{
    if (!data_)
        return false;

    switch (format_)
    {
    case TextureFormat::TEX_FORMAT_RGBA8_UNORM:
        memcpy(dest, data_, width_ * height_ * depth_ * 4);
        return true;

    case TextureFormat::TEX_FORMAT_BC1_UNORM:
    case TextureFormat::TEX_FORMAT_BC2_UNORM:
    case TextureFormat::TEX_FORMAT_BC3_UNORM:
        DecompressImageDXT(dest, data_, width_, height_, depth_, format_);
        return true;

    // ETC2 format is compatible with ETC1, so we just use the same function.
    case TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM:
        DecompressImageETC(dest, data_, width_, height_, false);
        return true;
    case TextureFormat::TEX_FORMAT_ETC2_RGBA8_UNORM:
        DecompressImageETC(dest, data_, width_, height_, true);
        return true;

    case EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_2BPP:
    case EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_2BPP:
    case EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_4BPP:
    case EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_4BPP:
        DecompressImagePVRTC(dest, data_, width_, height_, format_);
        return true;

    default:
        // Unknown format
        return false;
    }
}

Image::Image(Context* context) :
    Resource(context)
{
}

Image::~Image() = default;

void Image::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Image>();
}

bool Image::BeginLoad(Deserializer& source)
{
    // Check for DDS, KTX or PVR compressed format
    ea::string fileID = source.ReadFileID();

    if (fileID == "DDS ")
    {
        // DDS compressed format
        DDSurfaceDesc2 ddsd;        // NOLINT(hicpp-member-init)
        source.Read(&ddsd, sizeof(ddsd));

        // DDS DX10+
        const bool hasDXGI = ddsd.ddpfPixelFormat_.dwFourCC_ == FOURCC_DX10;
        DDSHeader10 dxgiHeader{};
        if (hasDXGI)
            source.Read(&dxgiHeader, sizeof(dxgiHeader));

        const TextureFormat originalTextureFormat = PickTextureFormat(ddsd.ddpfPixelFormat_, dxgiHeader.dxgiFormat);
        if (originalTextureFormat == TextureFormat::TEX_FORMAT_UNKNOWN)
        {
            URHO3D_LOGERROR("Unsupported DDS image format");
            return false;
        }

        compressedFormat_ = SetTextureFormatSRGB(originalTextureFormat, false);
        sRGB_ = compressedFormat_ != originalTextureFormat;

        const auto& formatAttribs = GetTextureFormatInfo(compressedFormat_);
        components_ = formatAttribs.NumComponents;

        // Is it a cube map or texture array? If so determine the size of the image chain.
        cubemap_ = (ddsd.ddsCaps_.dwCaps2_ & DDSCAPS2_CUBEMAP_ALL_FACES) != 0 || (hasDXGI && (dxgiHeader.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0);
        unsigned imageChainCount = 1;
        if (cubemap_)
            imageChainCount = 6;
        else if (hasDXGI && dxgiHeader.arraySize > 1)
        {
            imageChainCount = dxgiHeader.arraySize;
            array_ = true;
        }

        // Calculate the size of the data
        const IntVector3 dimensions = ToIntVector3(ddsd.dwWidth_, ddsd.dwHeight_, ddsd.dwDepth_);
        unsigned dataSize = 0;
        for (unsigned level = 0; level < ddsd.dwMipMapCount_; ++level)
            dataSize += GetMipLevelSizeInBytes(compressedFormat_, dimensions, level);

        // Do not use a shared ptr here, in case nothing is refcounting the image outside this function.
        // A raw pointer is fine as the image chain (if needed) uses shared ptr's properly
        Image* currentImage = this;

        for (unsigned faceIndex = 0; faceIndex < imageChainCount; ++faceIndex)
        {
            currentImage->data_ = new unsigned char[dataSize];
            currentImage->cubemap_ = cubemap_;
            currentImage->array_ = array_;
            currentImage->components_ = components_;
            currentImage->compressedFormat_ = compressedFormat_;
            currentImage->width_ = ddsd.dwWidth_;
            currentImage->height_ = ddsd.dwHeight_;
            currentImage->depth_ = ddsd.dwDepth_;

            currentImage->numCompressedLevels_ = ddsd.dwMipMapCount_;
            if (!currentImage->numCompressedLevels_)
                currentImage->numCompressedLevels_ = 1;

            // Memory use needs to be exact per image as it's used for verifying the data size in GetCompressedLevel()
            // even though it would be more proper for the first image to report the size of all siblings combined
            currentImage->SetMemoryUse(dataSize);

            source.Read(currentImage->data_.get(), dataSize);

            if (faceIndex < imageChainCount - 1)
            {
                // Build the image chain
                SharedPtr<Image> nextImage(MakeShared<Image>(context_));
                currentImage->nextSibling_ = nextImage;
                currentImage = nextImage;
            }
        }

        // If uncompressed DDS, convert the data to 8bit RGBA as the texture classes can not currently use eg. RGB565 format
        if (compressedFormat_ == TextureFormat::TEX_FORMAT_RGBA8_UNORM
            && !AreTextureComponentsMatching(ddsd.ddpfPixelFormat_, compressedFormat_))
        {
            URHO3D_PROFILE("ConvertDDSToRGBA");

            currentImage = this;

            while (currentImage)
            {
                unsigned sourcePixelByteSize = ddsd.ddpfPixelFormat_.dwRGBBitCount_ >> 3;
                unsigned numPixels = dataSize / sourcePixelByteSize;

#define ADJUSTSHIFT(mask, l, r) \
                if ((mask) >= 0x100) \
                { \
                    while (((mask) >> (r)) >= 0x100) \
                    ++(r); \
                } \
                else if ((mask) && (mask) < 0x80) \
                { \
                    while (((mask) << (l)) < 0x80) \
                    ++(l); \
                }

                unsigned rShiftL = 0, gShiftL = 0, bShiftL = 0, aShiftL = 0;
                unsigned rShiftR = 0, gShiftR = 0, bShiftR = 0, aShiftR = 0;
                unsigned rMask = ddsd.ddpfPixelFormat_.dwRBitMask_;
                unsigned gMask = ddsd.ddpfPixelFormat_.dwGBitMask_;
                unsigned bMask = ddsd.ddpfPixelFormat_.dwBBitMask_;
                unsigned aMask = ddsd.ddpfPixelFormat_.dwRGBAlphaBitMask_;
                ADJUSTSHIFT(rMask, rShiftL, rShiftR)
                ADJUSTSHIFT(gMask, gShiftL, gShiftR)
                ADJUSTSHIFT(bMask, bShiftL, bShiftR)
                ADJUSTSHIFT(aMask, aShiftL, aShiftR)

                ea::shared_array<unsigned char> rgbaData(new unsigned char[numPixels * 4]);

                switch (sourcePixelByteSize)
                {
                case 4:
                {
                    auto* src = (unsigned*)currentImage->data_.get();
                    unsigned char* dest = rgbaData.get();

                    while (numPixels--)
                    {
                        unsigned pixels = *src++;
                        *dest++ = ((pixels & rMask) << rShiftL) >> rShiftR;
                        *dest++ = ((pixels & gMask) << gShiftL) >> gShiftR;
                        *dest++ = ((pixels & bMask) << bShiftL) >> bShiftR;
                        *dest++ = ((pixels & aMask) << aShiftL) >> aShiftR;
                    }
                }
                break;

                case 3:
                {
                    unsigned char* src = currentImage->data_.get();
                    unsigned char* dest = rgbaData.get();

                    while (numPixels--)
                    {
                        unsigned pixels = src[0] | (src[1] << 8) | (src[2] << 16);
                        src += 3;
                        *dest++ = ((pixels & rMask) << rShiftL) >> rShiftR;
                        *dest++ = ((pixels & gMask) << gShiftL) >> gShiftR;
                        *dest++ = ((pixels & bMask) << bShiftL) >> bShiftR;
                        *dest++ = ((pixels & aMask) << aShiftL) >> aShiftR;
                    }
                }
                break;

                default:
                {
                    auto* src = (unsigned short*)currentImage->data_.get();
                    unsigned char* dest = rgbaData.get();

                    while (numPixels--)
                    {
                        unsigned short pixels = *src++;
                        *dest++ = ((pixels & rMask) << rShiftL) >> rShiftR;
                        *dest++ = ((pixels & gMask) << gShiftL) >> gShiftR;
                        *dest++ = ((pixels & bMask) << bShiftL) >> bShiftR;
                        *dest++ = ((pixels & aMask) << aShiftL) >> aShiftR;
                    }
                }
                break;
                }

                // Replace with converted data
                currentImage->data_ = rgbaData;
                currentImage->SetMemoryUse(numPixels * 4);
                currentImage = currentImage->GetNextSibling();
            }
        }
    }
    else if (fileID == "\253KTX")
    {
        source.Seek(12);

        unsigned endianness = source.ReadUInt();
        unsigned type = source.ReadUInt();
        /* unsigned typeSize = */ source.ReadUInt();
        unsigned format = source.ReadUInt();
        unsigned internalFormat = source.ReadUInt();
        /* unsigned baseInternalFormat = */ source.ReadUInt();
        unsigned width = source.ReadUInt();
        unsigned height = source.ReadUInt();
        unsigned depth = source.ReadUInt();
        /* unsigned arrayElements = */ source.ReadUInt();
        unsigned faces = source.ReadUInt();
        unsigned mipmaps = source.ReadUInt();
        unsigned keyValueBytes = source.ReadUInt();

        if (endianness != 0x04030201)
        {
            URHO3D_LOGERROR("Big-endian KTX files not supported");
            return false;
        }

        if (type != 0 || format != 0)
        {
            URHO3D_LOGERROR("Uncompressed KTX files not supported");
            return false;
        }

        if (faces > 1 || depth > 1)
        {
            URHO3D_LOGERROR("3D or cube KTX files not supported");
            return false;
        }

        if (mipmaps == 0)
        {
            URHO3D_LOGERROR("KTX files without explicitly specified mipmap count not supported");
            return false;
        }

        switch (internalFormat)
        {
        case 0x83f1:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC1_UNORM;
            components_ = 4;
            break;

        case 0x83f2:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC2_UNORM;
            components_ = 4;
            break;

        case 0x83f3:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC3_UNORM;
            components_ = 4;
            break;

        case 0x8d64:
        case 0x9274:
            compressedFormat_ = TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM;
            components_ = 3;
            break;

        case 0x9278:
            compressedFormat_ = TextureFormat::TEX_FORMAT_ETC2_RGBA8_UNORM;
            components_ = 4;
            break;

        case 0x8c00:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_4BPP;
            components_ = 3;
            break;

        case 0x8c01:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_2BPP;
            components_ = 3;
            break;

        case 0x8c02:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_4BPP;
            components_ = 4;
            break;

        case 0x8c03:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_2BPP;
            components_ = 4;
            break;

        default:
            compressedFormat_ = TextureFormat::TEX_FORMAT_UNKNOWN;
            break;
        }

        if (compressedFormat_ == TextureFormat::TEX_FORMAT_UNKNOWN)
        {
            URHO3D_LOGERROR("Unsupported texture format in KTX file");
            return false;
        }

        source.Seek(source.GetPosition() + keyValueBytes);
        auto dataSize = (unsigned)(source.GetSize() - source.GetPosition() - mipmaps * sizeof(unsigned));

        data_ = new unsigned char[dataSize];
        width_ = width;
        height_ = height;
        numCompressedLevels_ = mipmaps;

        unsigned dataOffset = 0;
        for (unsigned i = 0; i < mipmaps; ++i)
        {
            unsigned levelSize = source.ReadUInt();
            if (levelSize + dataOffset > dataSize)
            {
                URHO3D_LOGERROR("KTX mipmap level data size exceeds file size");
                return false;
            }

            source.Read(&data_[dataOffset], levelSize);
            dataOffset += levelSize;
            if (source.GetPosition() & 3)
                source.Seek((source.GetPosition() + 3) & 0xfffffffc);
        }

        SetMemoryUse(dataSize);
    }
    else if (fileID == "PVR\3")
    {
        /* unsigned flags = */ source.ReadUInt();
        unsigned pixelFormatLo = source.ReadUInt();
        /* unsigned pixelFormatHi = */ source.ReadUInt();
        /* unsigned colourSpace = */ source.ReadUInt();
        /* unsigned channelType = */ source.ReadUInt();
        unsigned height = source.ReadUInt();
        unsigned width = source.ReadUInt();
        unsigned depth = source.ReadUInt();
        /* unsigned numSurfaces = */ source.ReadUInt();
        unsigned numFaces = source.ReadUInt();
        unsigned mipmapCount = source.ReadUInt();
        unsigned metaDataSize = source.ReadUInt();

        if (depth > 1 || numFaces > 1)
        {
            URHO3D_LOGERROR("3D or cube PVR files not supported");
            return false;
        }

        if (mipmapCount == 0)
        {
            URHO3D_LOGERROR("PVR files without explicitly specified mipmap count not supported");
            return false;
        }

        switch (pixelFormatLo)
        {
        case 0:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_2BPP;
            components_ = 3;
            break;

        case 1:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_2BPP;
            components_ = 4;
            break;

        case 2:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGB_4BPP;
            components_ = 3;
            break;

        case 3:
            compressedFormat_ = EmulatedTextureFormat::TEX_FORMAT_PVRTC_RGBA_4BPP;
            components_ = 4;
            break;

        case 6:
            compressedFormat_ = TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM;
            components_ = 3;
            break;

        case 7:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC1_UNORM;
            components_ = 4;
            break;

        case 9:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC2_UNORM;
            components_ = 4;
            break;

        case 11:
            compressedFormat_ = TextureFormat::TEX_FORMAT_BC3_UNORM;
            components_ = 4;
            break;

        // .pvr files also support ETC2 texture format.
        case 22:
            compressedFormat_ = TextureFormat::TEX_FORMAT_ETC2_RGB8_UNORM;
            components_ = 3;
            break;

        case 23:
            compressedFormat_ = TextureFormat::TEX_FORMAT_ETC2_RGBA8_UNORM;
            components_ = 4;
            break;

        default:
            compressedFormat_ = TextureFormat::TEX_FORMAT_UNKNOWN;
            break;
        }

        if (compressedFormat_ == TextureFormat::TEX_FORMAT_UNKNOWN)
        {
            URHO3D_LOGERROR("Unsupported texture format in PVR file");
            return false;
        }

        source.Seek(source.GetPosition() + metaDataSize);
        unsigned dataSize = source.GetSize() - source.GetPosition();

        data_ = new unsigned char[dataSize];
        width_ = width;
        height_ = height;
        numCompressedLevels_ = mipmapCount;

        source.Read(data_.get(), dataSize);
        SetMemoryUse(dataSize);
    }
#ifdef URHO3D_WEBP
    else if (fileID == "RIFF")
    {
        // WebP: https://developers.google.com/speed/webp/docs/api

        // RIFF layout is:
        //   Offset  tag
        //   0...3   "RIFF" 4-byte tag
        //   4...7   size of image data (including metadata) starting at offset 8
        //   8...11  "WEBP"   our form-type signature
        const uint8_t TAG_SIZE(4);

        source.Seek(8);
        uint8_t fourCC[TAG_SIZE];
        memset(&fourCC, 0, sizeof(uint8_t) * TAG_SIZE);

        unsigned bytesRead(source.Read(&fourCC, TAG_SIZE));
        if (bytesRead != TAG_SIZE)
        {
            // Truncated.
            URHO3D_LOGERROR("Truncated RIFF data");
            return false;
        }
        const uint8_t WEBP[TAG_SIZE] = {'W', 'E', 'B', 'P'};
        if (memcmp(fourCC, WEBP, TAG_SIZE) != 0)
        {
            // VP8_STATUS_BITSTREAM_ERROR
            URHO3D_LOGERROR("Invalid header");
            return false;
        }

        // Read the file to buffer.
        size_t dataSize(source.GetSize());
        ea::shared_array<uint8_t> data(new uint8_t[dataSize]);

        memset(data.get(), 0, sizeof(uint8_t) * dataSize);
        source.Seek(0);
        source.Read(data.get(), static_cast<unsigned>(dataSize));

        WebPBitstreamFeatures features;

        if (WebPGetFeatures(data.get(), dataSize, &features) != VP8_STATUS_OK)
        {
            URHO3D_LOGERROR("Error reading WebP image: " + source.GetName());
            return false;
        }

        size_t imgSize = (size_t)features.width * features.height * (features.has_alpha ? 4 : 3);
        ea::shared_array<uint8_t> pixelData(new uint8_t[imgSize]);

        bool decodeError(false);
        if (features.has_alpha)
        {
            decodeError = WebPDecodeRGBAInto(data.get(), dataSize, pixelData.get(), imgSize, 4 * features.width) == nullptr;
        }
        else
        {
            decodeError = WebPDecodeRGBInto(data.get(), dataSize, pixelData.get(), imgSize, 3 * features.width) == nullptr;
        }
        if (decodeError)
        {
            URHO3D_LOGERROR("Error decoding WebP image:" + source.GetName());
            return false;
        }

        SetSize(features.width, features.height, features.has_alpha ? 4 : 3);
        SetData(pixelData.get());
    }
#endif
    else
    {
        // Not DDS, KTX or PVR, use STBImage to load other image formats as uncompressed
        source.Seek(0);
        int width, height;
        unsigned components;
        unsigned char* pixelData = GetImageData(source, width, height, components);
        if (!pixelData)
        {
            URHO3D_LOGERROR("Could not load image " + source.GetName() + ": " + ea::string(stbi_failure_reason()));
            return false;
        }
        SetSize(width, height, components);
        SetData(pixelData);
        FreeImageData(pixelData);
    }

    return true;
}

bool Image::Save(Serializer& dest) const
{
    URHO3D_PROFILE("SaveImage");

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image " + GetName());
        return false;
    }

    if (!data_)
    {
        URHO3D_LOGERROR("Can not save zero-sized image " + GetName());
        return false;
    }

    int len;
    unsigned char* png = stbi_write_png_to_mem(data_.get(), 0, width_, height_, components_, &len);
    bool success = dest.Write(png, (unsigned)len) == (unsigned)len;
    free(png);      // NOLINT(hicpp-no-malloc)
    return success;
}

bool Image::SaveFile(const FileIdentifier& fileName) const
{
    // TODO(vfs): This function can save only to the host filesystem.
    const ea::string& absoluteFileName = fileName.fileName_;
    if (fileName.scheme_ != "file")
    {
        URHO3D_LOGERROR("Can not save image {}", fileName.ToUri());
        return false;
    }

    auto fs = GetSubsystem<FileSystem>();
    if (!fs->CreateDirsRecursive(GetPath(absoluteFileName)))
        return false;

    if (absoluteFileName.ends_with(".dds", false))
        return SaveDDS(absoluteFileName);
    else if (absoluteFileName.ends_with(".bmp", false))
        return SaveBMP(absoluteFileName);
    else if (absoluteFileName.ends_with(".jpg", false) || absoluteFileName.ends_with(".jpeg", false))
        return SaveJPG(absoluteFileName, 100);
    else if (absoluteFileName.ends_with(".tga", false))
        return SaveTGA(absoluteFileName);
#ifdef URHO3D_WEBP
    else if (absoluteFileName.ends_with(".webp", false))
        return SaveWEBP(absoluteFileName, 100.0f);
#endif
    else
        return SavePNG(absoluteFileName);
}

bool Image::SetSize(int width, int height, unsigned components)
{
    return SetSize(width, height, 1, components);
}

bool Image::SetSize(int width, int height, int depth, unsigned components)
{
    if (width == width_ && height == height_ && depth == depth_ && components == components_)
        return true;

    if (width <= 0 || height <= 0 || depth <= 0)
        return false;

    if (components > 4)
    {
        URHO3D_LOGERROR("More than 4 color components are not supported");
        return false;
    }

    data_ = new unsigned char[width * height * depth * components];
    width_ = width;
    height_ = height;
    depth_ = depth;
    components_ = components;
    compressedFormat_ = TextureFormat::TEX_FORMAT_UNKNOWN;
    numCompressedLevels_ = 0;
    nextLevel_.Reset();

    SetMemoryUse(width * height * depth * components);
    return true;
}

void Image::SetPixel(int x, int y, const Color& color)
{
    SetPixelInt(x, y, 0, color.ToUInt());
}

void Image::SetPixel(int x, int y, int z, const Color& color)
{
    SetPixelInt(x, y, z, color.ToUInt());
}

void Image::SetPixelInt(int x, int y, unsigned uintColor)
{
    SetPixelInt(x, y, 0, uintColor);
}

void Image::SetPixelInt(int x, int y, int z, unsigned uintColor)
{
    if (!data_ || x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_ || IsCompressed())
        return;

    unsigned char* dest = data_.get() + (z * width_ * height_ + y * width_ + x) * components_;
    auto* src = (unsigned char*)&uintColor;

    switch (components_)
    {
    case 4:
        dest[3] = src[3];
        // Fall through
    case 3:
        dest[2] = src[2];
        // Fall through
    case 2:
        dest[1] = src[1];
        // Fall through
    default:
        dest[0] = src[0];
        break;
    }
}

void Image::SetData(const unsigned char* pixelData)
{
    if (!data_)
        return;

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not set new pixel data for a compressed image");
        return;
    }

    auto size = (size_t)width_ * height_ * depth_ * components_;
    if (pixelData)
        memcpy(data_.get(), pixelData, size);
    else
        memset(data_.get(), 0, size);
    nextLevel_.Reset();
}

bool Image::LoadColorLUT(Deserializer& source)
{
    ea::string fileID = source.ReadFileID();

    if (fileID == "DDS " || fileID == "\253KTX" || fileID == "PVR\3")
    {
        URHO3D_LOGERROR("Invalid image format, can not load image");
        return false;
    }

    source.Seek(0);
    int width, height;
    unsigned components;
    unsigned char* pixelDataIn = GetImageData(source, width, height, components);
    if (!pixelDataIn)
    {
        URHO3D_LOGERROR("Could not load image " + source.GetName() + ": " + ea::string(stbi_failure_reason()));
        return false;
    }
    if (components != 3)
    {
        URHO3D_LOGERROR("Invalid image format, can not load image");
        return false;
    }

    SetSize(COLOR_LUT_SIZE, COLOR_LUT_SIZE, COLOR_LUT_SIZE, components);
    SetMemoryUse(width_ * height_ * depth_ * components);

    unsigned char* pixelDataOut = GetData();

    for (int z = 0; z < depth_; ++z)
    {
        for (int y = 0; y < height_; ++y)
        {
            unsigned char* in = &pixelDataIn[z * width_ * 3 + y * width * 3];
            unsigned char* out = &pixelDataOut[z * width_ * height_ * 3 + y * width_ * 3];

            for (int x = 0; x < width_ * 3; x += 3)
            {
                out[x] = in[x];
                out[x + 1] = in[x + 1];
                out[x + 2] = in[x + 2];
            }
        }
    }

    FreeImageData(pixelDataIn);

    return true;
}

bool Image::FlipHorizontal()
{
    if (!data_)
        return false;

    if (depth_ > 1)
    {
        URHO3D_LOGERROR("FlipHorizontal not supported for 3D images");
        return false;
    }

    const auto& formatAttribs = GetTextureFormatInfo(compressedFormat_);
    const bool isWidthPowerOf2 = IsPowerOfTwo(width_);
    const bool isSimpleFlip = !IsCompressed() || (formatAttribs.BlockWidth == 1 && formatAttribs.BlockHeight == 1);
    const bool isBlockFlip = !isSimpleFlip && IsFlipBlockImplemented(compressedFormat_) && isWidthPowerOf2;
    if (!isSimpleFlip && !isBlockFlip)
    {
        URHO3D_LOGERROR(
            "FlipHorizontal not implemented for {}{}", compressedFormat_, !isWidthPowerOf2 ? " (NPOT)" : "");
        return false;
    }

    if (!IsCompressed())
    {
        const unsigned rowSize = width_ * components_;
        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_ / 2; ++x)
            {
                unsigned char* first = &data_[y * rowSize + x * components_];
                unsigned char* second = &data_[y * rowSize + (width_ - x - 1) * components_];
                ea::swap_ranges(first, first + components_, second);
            }
        }
    }
    else
    {
        const unsigned blockSize = formatAttribs.GetElementSize();
        ea::vector<unsigned char> tempBlock1(blockSize);
        ea::vector<unsigned char> tempBlock2(blockSize);

        for (unsigned i = 0; i < numCompressedLevels_; ++i)
        {
            CompressedLevel level = GetCompressedLevel(i);
            if (!level.data_)
            {
                URHO3D_LOGERROR("Got compressed level with no data, aborting horizontal flip");
                return false;
            }

            // Bug: Small blocks cannot be flipped in current implementation.
            if (level.width_ < formatAttribs.BlockWidth)
                continue;

            const unsigned numRows = (level.height_ + formatAttribs.BlockHeight - 1) / formatAttribs.BlockHeight;
            const unsigned numBlocksInRow = (level.width_ + formatAttribs.BlockWidth - 1) / formatAttribs.BlockWidth;
            const unsigned rowSize = numBlocksInRow * blockSize;
            for (unsigned y = 0; y < numRows; ++y)
            {
                for (unsigned x = 0; x < numBlocksInRow / 2; ++x)
                {
                    unsigned char* first = level.data_ + y * rowSize + (numBlocksInRow - x - 1) * blockSize;
                    unsigned char* second = level.data_ + y * rowSize + x * blockSize;
                    if (isSimpleFlip)
                        ea::swap_ranges(first, first + blockSize, second);
                    else
                    {
                        FlipBlockHorizontal(tempBlock1.data(), first, compressedFormat_);
                        FlipBlockHorizontal(tempBlock2.data(), second, compressedFormat_);
                        ea::copy(tempBlock1.begin(), tempBlock1.end(), second);
                        ea::copy(tempBlock2.begin(), tempBlock2.end(), first);
                    }
                }
            }
        }
    }

    return true;
}

bool Image::FlipVertical()
{
    if (!data_)
        return false;

    if (depth_ > 1)
    {
        URHO3D_LOGERROR("FlipVertical not supported for 3D images");
        return false;
    }

    const auto& formatAttribs = GetTextureFormatInfo(compressedFormat_);
    const bool isHeightPowerOf2 = IsPowerOfTwo(height_);
    const bool isSimpleFlip = !IsCompressed() || (formatAttribs.BlockWidth == 1 && formatAttribs.BlockHeight == 1);
    const bool isBlockFlip = !isSimpleFlip && IsFlipBlockImplemented(compressedFormat_) && isHeightPowerOf2;
    if (!isSimpleFlip && !isBlockFlip)
    {
        URHO3D_LOGERROR(
            "FlipVertical not implemented for {}{}", compressedFormat_, !isHeightPowerOf2 ? " (NPOT)" : "");
        return false;
    }

    if (!IsCompressed())
    {
        const unsigned rowSize = width_ * components_;
        for (int y = 0; y < height_ / 2; ++y)
        {
            unsigned char* first = &data_[y * rowSize];
            unsigned char* second = &data_[(height_ - y - 1) * rowSize];
            ea::swap_ranges(first, first + rowSize, second);
        }
    }
    else
    {
        const unsigned blockSize = formatAttribs.GetElementSize();
        ea::vector<unsigned char> tempBlock(blockSize);

        for (unsigned i = 0; i < numCompressedLevels_; ++i)
        {
            CompressedLevel level = GetCompressedLevel(i);
            if (!level.data_)
            {
                URHO3D_LOGERROR("Got compressed level with no data, aborting vertical flip");
                return false;
            }

            // Bug: Small blocks cannot be flipped in current implementation.
            if (level.height_ < formatAttribs.BlockHeight)
                continue;

            const unsigned numRows = (level.height_ + formatAttribs.BlockHeight - 1) / formatAttribs.BlockHeight;
            const unsigned numBlocksInRow = (level.width_ + formatAttribs.BlockWidth - 1) / formatAttribs.BlockWidth;
            const unsigned rowSize = numBlocksInRow * blockSize;
            for (unsigned y = 0; y < numRows / 2; ++y)
            {
                unsigned char* first = level.data_ + y * rowSize;
                unsigned char* second = level.data_ + (numRows - y - 1) * rowSize;
                ea::swap_ranges(first, first + rowSize, second);

                if (isSimpleFlip)
                    continue;

                for (unsigned char* data : {first, second})
                {
                    for (unsigned x = 0; x < numBlocksInRow; ++x)
                    {
                        FlipBlockVertical(tempBlock.data(), data + x * blockSize, compressedFormat_);
                        ea::copy(tempBlock.begin(), tempBlock.end(), data + x * blockSize);
                    }
                }
            }
        }
    }

    return true;
}

bool Image::Resize(int width, int height)
{
    URHO3D_PROFILE("ResizeImage");

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Resize not supported for compressed images");
        return false;
    }

    if (depth_ > 1)
    {
        URHO3D_LOGERROR("Resize not supported for 3D images");
        return false;
    }

    if (!data_ || width <= 0 || height <= 0)
        return false;

    /// \todo Reducing image size does not sample all needed pixels
    ea::shared_array<unsigned char> newData(new unsigned char[width * height * components_]);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Calculate float coordinates between 0 - 1 for resampling
            float xF = (width_ > 1) ? (float)x / (float)(width - 1) : 0.0f;
            float yF = (height_ > 1) ? (float)y / (float)(height - 1) : 0.0f;
            unsigned uintColor = GetPixelBilinear(xF, yF).ToUInt();
            unsigned char* dest = newData.get() + (y * width + x) * components_;
            auto* src = (unsigned char*)&uintColor;

            switch (components_)
            {
            case 4:
                dest[3] = src[3];
                // Fall through
            case 3:
                dest[2] = src[2];
                // Fall through
            case 2:
                dest[1] = src[1];
                // Fall through
            default:
                dest[0] = src[0];
                break;
            }
        }
    }

    width_ = width;
    height_ = height;
    data_ = newData;
    SetMemoryUse(width * height * depth_ * components_);
    return true;
}

void Image::Clear(const Color& color)
{
    ClearInt(color.ToUInt());
}

void Image::ClearInt(unsigned uintColor)
{
    URHO3D_PROFILE("ClearImage");

    if (!data_)
        return;

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Clear not supported for compressed images");
        return;
    }

    if (components_ == 4)
    {
        unsigned color = uintColor;
        auto* data = (unsigned*)GetData();
        auto* data_end = (unsigned*)(GetData() + width_ * height_ * depth_ * components_);
        for (; data < data_end; ++data)
            *data = color;
    }
    else
    {
        auto* src = (unsigned char*)&uintColor;
        for (unsigned i = 0; i < width_ * height_ * depth_ * components_; ++i)
            data_[i] = src[i % components_];
    }
}

bool Image::SaveBMP(const ea::string& fileName) const
{
    URHO3D_PROFILE("SaveImageBMP");

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (fileSystem && !fileSystem->CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image to BMP");
        return false;
    }

    if (data_)
        return stbi_write_bmp(fileName.c_str(), width_, height_, components_, data_.get()) != 0;
    else
        return false;
}

bool Image::SavePNG(const ea::string& fileName) const
{
    URHO3D_PROFILE("SaveImagePNG");

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (!fileSystem->DirExists(GetPath(fileName)))
    {
        if (!fileSystem->CreateDir(GetPath(fileName)))
            return false;
    }

    File outFile(context_, fileName, FILE_WRITE);
    if (outFile.IsOpen())
        return Image::Save(outFile); // Save uses PNG format
    else
        return false;
}

bool Image::SaveTGA(const ea::string& fileName) const
{
    URHO3D_PROFILE("SaveImageTGA");

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (fileSystem && !fileSystem->CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image to TGA");
        return false;
    }

    if (data_)
        return stbi_write_tga(GetNativePath(fileName).c_str(), width_, height_, components_, data_.get()) != 0;
    else
        return false;
}

bool Image::SaveJPG(const ea::string& fileName, int quality) const
{
    URHO3D_PROFILE("SaveImageJPG");

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (fileSystem && !fileSystem->CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image to JPG");
        return false;
    }

    if (data_)
        return stbi_write_jpg(GetNativePath(fileName).c_str(), width_, height_, components_, data_.get(), quality) != 0;
    else
        return false;
}

bool Image::SaveDDS(const ea::string& fileName) const
{
    URHO3D_PROFILE("SaveImageDDS");

    File outFile(context_, fileName, FILE_WRITE);
    if (!outFile.IsOpen())
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image to DDS");
        return false;
    }

    if (components_ != 4)
    {
        URHO3D_LOGERRORF("Can not save image with %u components to DDS", components_);
        return false;
    }

    // Write image
    ea::vector<const Image*> levels;
    GetLevels(levels);

    outFile.WriteFileID("DDS ");

    DDSurfaceDesc2 ddsd;        // NOLINT(hicpp-member-init)
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize_ = sizeof(ddsd);
    ddsd.dwFlags_ = 0x00000001l /*DDSD_CAPS*/
        | 0x00000002l /*DDSD_HEIGHT*/ | 0x00000004l /*DDSD_WIDTH*/ | 0x00020000l /*DDSD_MIPMAPCOUNT*/ | 0x00001000l /*DDSD_PIXELFORMAT*/;
    ddsd.dwWidth_ = width_;
    ddsd.dwHeight_ = height_;
    ddsd.dwMipMapCount_ = levels.size();
    ddsd.ddpfPixelFormat_.dwFlags_ = 0x00000040l /*DDPF_RGB*/ | 0x00000001l /*DDPF_ALPHAPIXELS*/;
    ddsd.ddpfPixelFormat_.dwSize_ = sizeof(ddsd.ddpfPixelFormat_);
    ddsd.ddpfPixelFormat_.dwRGBBitCount_ = 32;
    ddsd.ddpfPixelFormat_.dwRBitMask_ = 0x000000ff;
    ddsd.ddpfPixelFormat_.dwGBitMask_ = 0x0000ff00;
    ddsd.ddpfPixelFormat_.dwBBitMask_ = 0x00ff0000;
    ddsd.ddpfPixelFormat_.dwRGBAlphaBitMask_ = 0xff000000;

    outFile.Write(&ddsd, sizeof(ddsd));
    for (unsigned i = 0; i < levels.size(); ++i)
        outFile.Write(levels[i]->GetData(), levels[i]->GetWidth() * levels[i]->GetHeight() * 4);

    return true;
}

bool Image::SaveWEBP(const ea::string& fileName, float compression /* = 0.0f */) const
{
#ifdef URHO3D_WEBP
    URHO3D_PROFILE("SaveImageWEBP");

    auto* fileSystem(GetSubsystem<FileSystem>());
    File outFile(context_, fileName, FILE_WRITE);

    if (fileSystem && !fileSystem->CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not save compressed image to WebP");
        return false;
    }

    if (height_ > WEBP_MAX_DIMENSION || width_ > WEBP_MAX_DIMENSION)
    {
        URHO3D_LOGERROR("Maximum dimension supported by WebP is " + ea::to_string(WEBP_MAX_DIMENSION));
        return false;
    }

    if (components_ != 4 && components_ != 3)
    {
        URHO3D_LOGERRORF("Can not save image with %u components to WebP, which requires 3 or 4; Try ConvertToRGBA first?", components_);
        return false;
    }

    if (!data_)
    {
        URHO3D_LOGERROR("No image data to save");
        return false;
    }

    WebPPicture pic;
    WebPConfig config;
    WebPMemoryWriter wrt;
    int importResult(0);
    size_t encodeResult(0);

    if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT, compression) || !WebPPictureInit(&pic))
    {
        URHO3D_LOGERROR("WebP initialization failed; check installation");
        return false;
    }
    config.lossless = 1;
    config.exact = 1; // Preserve RGB values under transparency, as they may be wanted.

    pic.use_argb = 1;
    pic.width = width_;
    pic.height = height_;
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &wrt;
    WebPMemoryWriterInit(&wrt);

    if (components_ == 4)
        importResult = WebPPictureImportRGBA(&pic, data_.get(), components_ * width_);
    else if (components_ == 3)
        importResult = WebPPictureImportRGB(&pic, data_.get(), components_ * width_);

    if (!importResult)
    {
        URHO3D_LOGERROR("WebP import of image data failed (truncated RGBA/RGB data or memory error?)");
        WebPPictureFree(&pic);
        WebPMemoryWriterClear(&wrt);
        return false;
    }

    encodeResult = WebPEncode(&config, &pic);
    // Check only general failure. WebPEncode() sets pic.error_code with specific error.
    if (!encodeResult)
    {
        URHO3D_LOGERRORF("WebP encoding failed (memory error?). WebPEncodingError = %d", pic.error_code);
        WebPPictureFree(&pic);
        WebPMemoryWriterClear(&wrt);
        return false;
    }

    WebPPictureFree(&pic);
    outFile.Write(wrt.mem, static_cast<unsigned>(wrt.size));
    WebPMemoryWriterClear(&wrt);

    return true;
#else
    URHO3D_LOGERROR("Cannot save in WEBP format, support not compiled in");
    return false;
#endif
}


Color Image::GetPixel(int x, int y) const
{
    return GetPixel(x, y, 0);
}

Color Image::GetPixel(int x, int y, int z) const
{
    if (!data_ || z < 0 || z >= depth_ || IsCompressed())
        return Color::BLACK;
    x = Clamp(x, 0, width_ - 1);
    y = Clamp(y, 0, height_ - 1);

    unsigned char* src = data_.get() + (z * width_ * height_ + y * width_ + x) * components_;
    Color ret;

    switch (components_)
    {
    case 4:
        ret.a_ = (float)src[3] / 255.0f;
        // Fall through
    case 3:
        ret.b_ = (float)src[2] / 255.0f;
        // Fall through
    case 2:
        ret.g_ = (float)src[1] / 255.0f;
        ret.r_ = (float)src[0] / 255.0f;
        break;
    default:
        ret.r_ = ret.g_ = ret.b_ = (float)src[0] / 255.0f;
        break;
    }

    return ret;
}

unsigned Image::GetPixelInt(int x, int y) const
{
    return GetPixelInt(x, y, 0);
}

unsigned Image::GetPixelInt(int x, int y, int z) const
{
    if (!data_ || z < 0 || z >= depth_ || IsCompressed())
        return 0xff000000;
    x = Clamp(x, 0, width_ - 1);
    y = Clamp(y, 0, height_ - 1);

    unsigned char* src = data_.get() + (z * width_ * height_ + y * width_ + x) * components_;
    unsigned ret = 0;
    if (components_ < 4)
        ret |= 0xff000000;

    switch (components_)
    {
    case 4:
        ret |= (unsigned)src[3] << 24;
        // Fall through
    case 3:
        ret |= (unsigned)src[2] << 16;
        // Fall through
    case 2:
        ret |= (unsigned)src[1] << 8;
        ret |= (unsigned)src[0];
        break;
    default:
        ret |= (unsigned)src[0] << 16;
        ret |= (unsigned)src[0] << 8;
        ret |= (unsigned)src[0];
        break;
    }

    return ret;
}

Color Image::GetPixelBilinear(float x, float y) const
{
    x = Clamp(x * width_ - 0.5f, 0.0f, (float)(width_ - 1));
    y = Clamp(y * height_ - 0.5f, 0.0f, (float)(height_ - 1));

    auto xI = (int)x;
    auto yI = (int)y;
    float xF = Fract(x);
    float yF = Fract(y);

    Color topColor = GetPixel(xI, yI).Lerp(GetPixel(xI + 1, yI), xF);
    Color bottomColor = GetPixel(xI, yI + 1).Lerp(GetPixel(xI + 1, yI + 1), xF);
    return topColor.Lerp(bottomColor, yF);
}

Color Image::GetPixelTrilinear(float x, float y, float z) const
{
    if (depth_ < 2)
        return GetPixelBilinear(x, y);

    x = Clamp(x * width_ - 0.5f, 0.0f, (float)(width_ - 1));
    y = Clamp(y * height_ - 0.5f, 0.0f, (float)(height_ - 1));
    z = Clamp(z * depth_ - 0.5f, 0.0f, (float)(depth_ - 1));

    auto xI = (int)x;
    auto yI = (int)y;
    auto zI = (int)z;
    if (zI == depth_ - 1)
        return GetPixelBilinear(x, y);
    float xF = Fract(x);
    float yF = Fract(y);
    float zF = Fract(z);

    Color topColorNear = GetPixel(xI, yI, zI).Lerp(GetPixel(xI + 1, yI, zI), xF);
    Color bottomColorNear = GetPixel(xI, yI + 1, zI).Lerp(GetPixel(xI + 1, yI + 1, zI), xF);
    Color colorNear = topColorNear.Lerp(bottomColorNear, yF);
    Color topColorFar = GetPixel(xI, yI, zI + 1).Lerp(GetPixel(xI + 1, yI, zI + 1), xF);
    Color bottomColorFar = GetPixel(xI, yI + 1, zI + 1).Lerp(GetPixel(xI + 1, yI + 1, zI + 1), xF);
    Color colorFar = topColorFar.Lerp(bottomColorFar, yF);
    return colorNear.Lerp(colorFar, zF);
}

TextureFormat Image::GetGPUFormat() const
{
    static const TextureFormat unormFormats[] = {
        TextureFormat::TEX_FORMAT_UNKNOWN,

        TextureFormat::TEX_FORMAT_R8_UNORM,
        TextureFormat::TEX_FORMAT_RG8_UNORM,
        TextureFormat::TEX_FORMAT_UNKNOWN,
        TextureFormat::TEX_FORMAT_RGBA8_UNORM,
    };

    if (!IsCompressed())
        return unormFormats[GetComponents()];
    else if (GetCompressedFormat() < TextureFormat::TEX_FORMAT_NUM_FORMATS)
        return GetCompressedFormat();
    else
        return TextureFormat::TEX_FORMAT_UNKNOWN;
}

SharedPtr<Image> Image::GetNextLevel() const
{
    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not generate mip level from compressed data");
        return SharedPtr<Image>();
    }
    if (components_ < 1 || components_ > 4)
    {
        URHO3D_LOGERROR("Illegal number of image components for mip level generation");
        return SharedPtr<Image>();
    }

    if (nextLevel_)
        return nextLevel_;

    URHO3D_PROFILE("CalculateImageMipLevel");

    int widthOut = width_ / 2;
    int heightOut = height_ / 2;
    int depthOut = depth_ / 2;

    if (widthOut < 1)
        widthOut = 1;
    if (heightOut < 1)
        heightOut = 1;
    if (depthOut < 1)
        depthOut = 1;

    SharedPtr<Image> mipImage(MakeShared<Image>(context_));

    if (depth_ > 1)
        mipImage->SetSize(widthOut, heightOut, depthOut, components_);
    else
        mipImage->SetSize(widthOut, heightOut, components_);

    const unsigned char* pixelDataIn = data_.get();
    unsigned char* pixelDataOut = mipImage->data_.get();

    // 1D case
    if (depth_ == 1 && (height_ == 1 || width_ == 1))
    {
        // Loop using the larger dimension
        if (widthOut < heightOut)
            widthOut = heightOut;

        switch (components_)
        {
        case 1:
            for (int x = 0; x < widthOut; ++x)
                pixelDataOut[x] = (unsigned char)(((unsigned)pixelDataIn[x * 2] + pixelDataIn[x * 2 + 1]) >> 1);
            break;

        case 2:
            for (int x = 0; x < widthOut * 2; x += 2)
            {
                pixelDataOut[x] = (unsigned char)(((unsigned)pixelDataIn[x * 2] + pixelDataIn[x * 2 + 2]) >> 1);
                pixelDataOut[x + 1] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 1] + pixelDataIn[x * 2 + 3]) >> 1);
            }
            break;

        case 3:
            for (int x = 0; x < widthOut * 3; x += 3)
            {
                pixelDataOut[x] = (unsigned char)(((unsigned)pixelDataIn[x * 2] + pixelDataIn[x * 2 + 3]) >> 1);
                pixelDataOut[x + 1] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 1] + pixelDataIn[x * 2 + 4]) >> 1);
                pixelDataOut[x + 2] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 2] + pixelDataIn[x * 2 + 5]) >> 1);
            }
            break;

        case 4:
            for (int x = 0; x < widthOut * 4; x += 4)
            {
                pixelDataOut[x] = (unsigned char)(((unsigned)pixelDataIn[x * 2] + pixelDataIn[x * 2 + 4]) >> 1);
                pixelDataOut[x + 1] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 1] + pixelDataIn[x * 2 + 5]) >> 1);
                pixelDataOut[x + 2] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 2] + pixelDataIn[x * 2 + 6]) >> 1);
                pixelDataOut[x + 3] = (unsigned char)(((unsigned)pixelDataIn[x * 2 + 3] + pixelDataIn[x * 2 + 7]) >> 1);
            }
            break;

        default:
            assert(false);  // Should never reach here
            break;
        }
    }
    // 2D case
    else if (depth_ == 1)
    {
        switch (components_)
        {
        case 1:
            for (int y = 0; y < heightOut; ++y)
            {
                const unsigned char* inUpper = &pixelDataIn[(y * 2) * width_];
                const unsigned char* inLower = &pixelDataIn[(y * 2 + 1) * width_];
                unsigned char* out = &pixelDataOut[y * widthOut];

                for (int x = 0; x < widthOut; ++x)
                {
                    out[x] = (unsigned char)(((unsigned)inUpper[x * 2] + inUpper[x * 2 + 1] +
                                              inLower[x * 2] + inLower[x * 2 + 1]) >> 2);
                }
            }
            break;

        case 2:
            for (int y = 0; y < heightOut; ++y)
            {
                const unsigned char* inUpper = &pixelDataIn[(y * 2) * width_ * 2];
                const unsigned char* inLower = &pixelDataIn[(y * 2 + 1) * width_ * 2];
                unsigned char* out = &pixelDataOut[y * widthOut * 2];

                for (int x = 0; x < widthOut * 2; x += 2)
                {
                    out[x] = (unsigned char)(((unsigned)inUpper[x * 2] + inUpper[x * 2 + 2] +
                                              inLower[x * 2] + inLower[x * 2 + 2]) >> 2);
                    out[x + 1] = (unsigned char)(((unsigned)inUpper[x * 2 + 1] + inUpper[x * 2 + 3] +
                                                  inLower[x * 2 + 1] + inLower[x * 2 + 3]) >> 2);
                }
            }
            break;

        case 3:
            for (int y = 0; y < heightOut; ++y)
            {
                const unsigned char* inUpper = &pixelDataIn[(y * 2) * width_ * 3];
                const unsigned char* inLower = &pixelDataIn[(y * 2 + 1) * width_ * 3];
                unsigned char* out = &pixelDataOut[y * widthOut * 3];

                for (int x = 0; x < widthOut * 3; x += 3)
                {
                    out[x] = (unsigned char)(((unsigned)inUpper[x * 2] + inUpper[x * 2 + 3] +
                                              inLower[x * 2] + inLower[x * 2 + 3]) >> 2);
                    out[x + 1] = (unsigned char)(((unsigned)inUpper[x * 2 + 1] + inUpper[x * 2 + 4] +
                                                  inLower[x * 2 + 1] + inLower[x * 2 + 4]) >> 2);
                    out[x + 2] = (unsigned char)(((unsigned)inUpper[x * 2 + 2] + inUpper[x * 2 + 5] +
                                                  inLower[x * 2 + 2] + inLower[x * 2 + 5]) >> 2);
                }
            }
            break;

        case 4:
            for (int y = 0; y < heightOut; ++y)
            {
                const unsigned char* inUpper = &pixelDataIn[(y * 2) * width_ * 4];
                const unsigned char* inLower = &pixelDataIn[(y * 2 + 1) * width_ * 4];
                unsigned char* out = &pixelDataOut[y * widthOut * 4];

                for (int x = 0; x < widthOut * 4; x += 4)
                {
                    out[x] = (unsigned char)(((unsigned)inUpper[x * 2] + inUpper[x * 2 + 4] +
                                              inLower[x * 2] + inLower[x * 2 + 4]) >> 2);
                    out[x + 1] = (unsigned char)(((unsigned)inUpper[x * 2 + 1] + inUpper[x * 2 + 5] +
                                                  inLower[x * 2 + 1] + inLower[x * 2 + 5]) >> 2);
                    out[x + 2] = (unsigned char)(((unsigned)inUpper[x * 2 + 2] + inUpper[x * 2 + 6] +
                                                  inLower[x * 2 + 2] + inLower[x * 2 + 6]) >> 2);
                    out[x + 3] = (unsigned char)(((unsigned)inUpper[x * 2 + 3] + inUpper[x * 2 + 7] +
                                                  inLower[x * 2 + 3] + inLower[x * 2 + 7]) >> 2);
                }
            }
            break;

        default:
            assert(false);  // Should never reach here
            break;
        }
    }
    // 3D case
    else
    {
        switch (components_)
        {
        case 1:
            for (int z = 0; z < depthOut; ++z)
            {
                const unsigned char* inOuter = &pixelDataIn[(z * 2) * width_ * height_];
                const unsigned char* inInner = &pixelDataIn[(z * 2 + 1) * width_ * height_];

                for (int y = 0; y < heightOut; ++y)
                {
                    const unsigned char* inOuterUpper = &inOuter[(y * 2) * width_];
                    const unsigned char* inOuterLower = &inOuter[(y * 2 + 1) * width_];
                    const unsigned char* inInnerUpper = &inInner[(y * 2) * width_];
                    const unsigned char* inInnerLower = &inInner[(y * 2 + 1) * width_];
                    unsigned char* out = &pixelDataOut[z * widthOut * heightOut + y * widthOut];

                    for (int x = 0; x < widthOut; ++x)
                    {
                        out[x] = (unsigned char)(((unsigned)inOuterUpper[x * 2] + inOuterUpper[x * 2 + 1] +
                                                  inOuterLower[x * 2] + inOuterLower[x * 2 + 1] +
                                                  inInnerUpper[x * 2] + inInnerUpper[x * 2 + 1] +
                                                  inInnerLower[x * 2] + inInnerLower[x * 2 + 1]) >> 3);
                    }
                }
            }
            break;

        case 2:
            for (int z = 0; z < depthOut; ++z)
            {
                const unsigned char* inOuter = &pixelDataIn[(z * 2) * width_ * height_ * 2];
                const unsigned char* inInner = &pixelDataIn[(z * 2 + 1) * width_ * height_ * 2];

                for (int y = 0; y < heightOut; ++y)
                {
                    const unsigned char* inOuterUpper = &inOuter[(y * 2) * width_ * 2];
                    const unsigned char* inOuterLower = &inOuter[(y * 2 + 1) * width_ * 2];
                    const unsigned char* inInnerUpper = &inInner[(y * 2) * width_ * 2];
                    const unsigned char* inInnerLower = &inInner[(y * 2 + 1) * width_ * 2];
                    unsigned char* out = &pixelDataOut[z * widthOut * heightOut * 2 + y * widthOut * 2];

                    for (int x = 0; x < widthOut * 2; x += 2)
                    {
                        out[x] = (unsigned char)(((unsigned)inOuterUpper[x * 2] + inOuterUpper[x * 2 + 2] +
                                                  inOuterLower[x * 2] + inOuterLower[x * 2 + 2] +
                                                  inInnerUpper[x * 2] + inInnerUpper[x * 2 + 2] +
                                                  inInnerLower[x * 2] + inInnerLower[x * 2 + 2]) >> 3);
                        out[x + 1] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 1] + inOuterUpper[x * 2 + 3] +
                                                      inOuterLower[x * 2 + 1] + inOuterLower[x * 2 + 3] +
                                                      inInnerUpper[x * 2 + 1] + inInnerUpper[x * 2 + 3] +
                                                      inInnerLower[x * 2 + 1] + inInnerLower[x * 2 + 3]) >> 3);
                    }
                }
            }
            break;

        case 3:
            for (int z = 0; z < depthOut; ++z)
            {
                const unsigned char* inOuter = &pixelDataIn[(z * 2) * width_ * height_ * 3];
                const unsigned char* inInner = &pixelDataIn[(z * 2 + 1) * width_ * height_ * 3];

                for (int y = 0; y < heightOut; ++y)
                {
                    const unsigned char* inOuterUpper = &inOuter[(y * 2) * width_ * 3];
                    const unsigned char* inOuterLower = &inOuter[(y * 2 + 1) * width_ * 3];
                    const unsigned char* inInnerUpper = &inInner[(y * 2) * width_ * 3];
                    const unsigned char* inInnerLower = &inInner[(y * 2 + 1) * width_ * 3];
                    unsigned char* out = &pixelDataOut[z * widthOut * heightOut * 3 + y * widthOut * 3];

                    for (int x = 0; x < widthOut * 3; x += 3)
                    {
                        out[x] = (unsigned char)(((unsigned)inOuterUpper[x * 2] + inOuterUpper[x * 2 + 3] +
                                                  inOuterLower[x * 2] + inOuterLower[x * 2 + 3] +
                                                  inInnerUpper[x * 2] + inInnerUpper[x * 2 + 3] +
                                                  inInnerLower[x * 2] + inInnerLower[x * 2 + 3]) >> 3);
                        out[x + 1] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 1] + inOuterUpper[x * 2 + 4] +
                                                      inOuterLower[x * 2 + 1] + inOuterLower[x * 2 + 4] +
                                                      inInnerUpper[x * 2 + 1] + inInnerUpper[x * 2 + 4] +
                                                      inInnerLower[x * 2 + 1] + inInnerLower[x * 2 + 4]) >> 3);
                        out[x + 2] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 2] + inOuterUpper[x * 2 + 5] +
                                                      inOuterLower[x * 2 + 2] + inOuterLower[x * 2 + 5] +
                                                      inInnerUpper[x * 2 + 2] + inInnerUpper[x * 2 + 5] +
                                                      inInnerLower[x * 2 + 2] + inInnerLower[x * 2 + 5]) >> 3);
                    }
                }
            }
            break;

        case 4:
            for (int z = 0; z < depthOut; ++z)
            {
                const unsigned char* inOuter = &pixelDataIn[(z * 2) * width_ * height_ * 4];
                const unsigned char* inInner = &pixelDataIn[(z * 2 + 1) * width_ * height_ * 4];

                for (int y = 0; y < heightOut; ++y)
                {
                    const unsigned char* inOuterUpper = &inOuter[(y * 2) * width_ * 4];
                    const unsigned char* inOuterLower = &inOuter[(y * 2 + 1) * width_ * 4];
                    const unsigned char* inInnerUpper = &inInner[(y * 2) * width_ * 4];
                    const unsigned char* inInnerLower = &inInner[(y * 2 + 1) * width_ * 4];
                    unsigned char* out = &pixelDataOut[z * widthOut * heightOut * 4 + y * widthOut * 4];

                    for (int x = 0; x < widthOut * 4; x += 4)
                    {
                        out[x] = (unsigned char)(((unsigned)inOuterUpper[x * 2] + inOuterUpper[x * 2 + 4] +
                                                  inOuterLower[x * 2] + inOuterLower[x * 2 + 4] +
                                                  inInnerUpper[x * 2] + inInnerUpper[x * 2 + 4] +
                                                  inInnerLower[x * 2] + inInnerLower[x * 2 + 4]) >> 3);
                        out[x + 1] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 1] + inOuterUpper[x * 2 + 5] +
                                                      inOuterLower[x * 2 + 1] + inOuterLower[x * 2 + 5] +
                                                      inInnerUpper[x * 2 + 1] + inInnerUpper[x * 2 + 5] +
                                                      inInnerLower[x * 2 + 1] + inInnerLower[x * 2 + 5]) >> 3);
                        out[x + 2] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 2] + inOuterUpper[x * 2 + 6] +
                                                      inOuterLower[x * 2 + 2] + inOuterLower[x * 2 + 6] +
                                                      inInnerUpper[x * 2 + 2] + inInnerUpper[x * 2 + 6] +
                                                      inInnerLower[x * 2 + 2] + inInnerLower[x * 2 + 6]) >> 3);
                        out[x + 3] = (unsigned char)(((unsigned)inOuterUpper[x * 2 + 3] + inOuterUpper[x * 2 + 7] +
                                                      inOuterLower[x * 2 + 3] + inOuterLower[x * 2 + 7] +
                                                      inInnerUpper[x * 2 + 3] + inInnerUpper[x * 2 + 7] +
                                                      inInnerLower[x * 2 + 3] + inInnerLower[x * 2 + 7]) >> 3);
                    }
                }
            }
            break;

        default:
            assert(false);  // Should never reach here
            break;
        }
    }

    return mipImage;
}

SharedPtr<Image> Image::ConvertToRGBA() const
{
    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not convert compressed image to RGBA");
        return SharedPtr<Image>();
    }
    if (components_ < 1 || components_ > 4)
    {
        URHO3D_LOGERROR("Illegal number of image components for conversion to RGBA");
        return SharedPtr<Image>();
    }
    if (!data_)
    {
        URHO3D_LOGERROR("Can not convert image without data to RGBA");
        return SharedPtr<Image>();
    }

    SharedPtr<Image> ret(MakeShared<Image>(context_));
    ret->SetSize(width_, height_, depth_, 4);

    const unsigned char* src = data_.get();
    unsigned char* dest = ret->GetData();

    switch (components_)
    {
    case 1:
        for (unsigned i = 0; i < static_cast<unsigned>(width_ * height_ * depth_); ++i)
        {
            unsigned char pixel = *src++;
            *dest++ = pixel;
            *dest++ = pixel;
            *dest++ = pixel;
            *dest++ = 255;
        }
        break;

    case 2:
        for (unsigned i = 0; i < static_cast<unsigned>(width_ * height_ * depth_); ++i)
        {
            unsigned char pixel = *src++;
            *dest++ = pixel;
            *dest++ = pixel;
            *dest++ = pixel;
            *dest++ = *src++;
        }
        break;

    case 3:
        for (unsigned i = 0; i < static_cast<unsigned>(width_ * height_ * depth_); ++i)
        {
            *dest++ = *src++;
            *dest++ = *src++;
            *dest++ = *src++;
            *dest++ = 255;
        }
        break;

    case 4:
        memcpy(dest, src, width_ * height_ * depth_ * 4);
        break;

    default:
        assert(false);  // Should never reach nere
        break;
    }

    return ret;
}

CompressedLevel Image::GetCompressedLevel(unsigned index) const
{
    CompressedLevel level;

    if (compressedFormat_ == TextureFormat::TEX_FORMAT_UNKNOWN)
    {
        URHO3D_LOGERROR("Image is not compressed");
        return level;
    }
    if (index >= numCompressedLevels_)
    {
        URHO3D_LOGERROR("Compressed image mip level out of bounds");
        return level;
    }

    level.format_ = compressedFormat_;

    const IntVector3 levelSize = GetMipLevelSize(GetSize(), index);
    level.width_ = levelSize.x_;
    level.height_ = levelSize.y_;
    level.depth_ = levelSize.z_;
    level.dataSize_ = GetMipLevelSizeInBytes(compressedFormat_, GetSize(), index);

    unsigned offset = 0;
    for (unsigned i = 0; i < index; ++i)
        offset += GetMipLevelSizeInBytes(compressedFormat_, GetSize(), i);

    if (offset + level.dataSize_ > GetMemoryUse())
    {
        URHO3D_LOGERROR("Compressed level is outside image data. Offset: {} Size: {} Datasize: {}", offset,
            level.dataSize_, GetMemoryUse());
        return level;
    }

    level.data_ = data_.get() + offset;
    return level;
}

SharedPtr<Image> Image::GetDecompressedImageLevel(unsigned index) const
{
    if (!IsCompressed())
    {
        if (index == 0)
            return ConvertToRGBA();
        else
            return GetNextLevel()->GetDecompressedImageLevel(index - 1);
    }
    else
    {
        const CompressedLevel compressedLevel = GetCompressedLevel(ea::min(index, numCompressedLevels_));

        auto decompressedImage = MakeShared<Image>(context_);
        decompressedImage->SetSize(compressedLevel.width_, compressedLevel.height_, 4);
        if (!compressedLevel.Decompress(decompressedImage->GetData()))
        {
            URHO3D_LOGERROR("Failed to decompress image level");
            return nullptr;
        }

        return decompressedImage;
    }
}

SharedPtr<Image> Image::GetDecompressedImage() const
{
    return GetDecompressedImageLevel(0);
}

SharedPtr<Image> Image::GetSubimage(const IntRect& rect) const
{
    if (!data_)
        return nullptr;

    if (depth_ > 1)
    {
        URHO3D_LOGERROR("Subimage not supported for 3D images");
        return nullptr;
    }

    if (rect.left_ < 0 || rect.top_ < 0 || rect.right_ > width_ || rect.bottom_ > height_ || !rect.Width() || !rect.Height())
    {
        URHO3D_LOGERROR("Can not get subimage from image " + GetName() + " with invalid region");
        return nullptr;
    }

    if (!IsCompressed())
    {
        int x = rect.left_;
        int y = rect.top_;
        int width = rect.Width();
        int height = rect.Height();

        auto image = MakeShared<Image>(context_);
        image->SetSize(width, height, components_);

        unsigned char* dest = image->GetData();
        unsigned char* src = data_.get() + (y * width_ + x) * components_;
        for (int i = 0; i < height; ++i)
        {
            memcpy(dest, src, (size_t)width * components_);
            dest += width * components_;
            src += width_ * components_;
        }

        return image;
    }
    else
    {
        const auto& formatAttribs = GetTextureFormatInfo(compressedFormat_);
        const unsigned blockSize = formatAttribs.GetElementSize();

        // Pad the region to be a multiple of block size
        IntRect paddedRect = rect;
        paddedRect.left_ = (rect.left_ / formatAttribs.BlockWidth) * formatAttribs.BlockWidth;
        paddedRect.right_ = (rect.right_ / formatAttribs.BlockWidth) * formatAttribs.BlockWidth;
        paddedRect.top_ = (rect.top_ / formatAttribs.BlockHeight) * formatAttribs.BlockHeight;
        paddedRect.bottom_ = (rect.bottom_ / formatAttribs.BlockHeight) * formatAttribs.BlockHeight;
        IntRect currentRect = paddedRect;

        ea::vector<unsigned char> subimageData;
        unsigned subimageLevels = 0;

        // Save as many mips as possible until the next mip would cross a block boundary
        for (unsigned i = 0; i < numCompressedLevels_; ++i)
        {
            CompressedLevel level = GetCompressedLevel(i);
            if (!level.data_)
                break;

            // Mips are stored continuously
            const unsigned sourceBlocksInRow = (level.width_ + formatAttribs.BlockWidth - 1) / formatAttribs.BlockWidth;
            const unsigned sourceRowSize = sourceBlocksInRow * blockSize;

            const unsigned destStartOffset = subimageData.size();
            const unsigned destRowSize = currentRect.Width() / formatAttribs.BlockWidth * blockSize;
            const unsigned destSize = currentRect.Height() / formatAttribs.BlockHeight * destRowSize;
            if (!destSize)
                break;

            subimageData.resize(destStartOffset + destSize);
            unsigned char* dest = &subimageData[destStartOffset];

            const unsigned beginX = currentRect.left_ / formatAttribs.BlockWidth;
            const unsigned beginY = currentRect.top_ / formatAttribs.BlockHeight;
            const unsigned endY = currentRect.bottom_ / formatAttribs.BlockHeight;
            for (unsigned y = beginY; y < endY; ++y)
            {
                unsigned char* src = level.data_ + sourceRowSize * y + beginX * blockSize;
                memcpy(dest, src, destRowSize);
                dest += destRowSize;
            }

            ++subimageLevels;
            if ((currentRect.left_ & formatAttribs.BlockWidth) || (currentRect.right_ & formatAttribs.BlockWidth)
                || (currentRect.top_ & formatAttribs.BlockHeight) || (currentRect.bottom_ & formatAttribs.BlockHeight))
                break;

            currentRect.left_ /= 2;
            currentRect.right_ /= 2;
            currentRect.top_ /= 2;
            currentRect.bottom_ /= 2;
        }

        if (!subimageLevels)
        {
            URHO3D_LOGERROR("Subimage region from compressed image " + GetName() + " did not produce any data");
            return nullptr;
        }

        auto image = MakeShared<Image>(context_);
        image->width_ = paddedRect.Width();
        image->height_ = paddedRect.Height();
        image->depth_ = 1;
        image->compressedFormat_ = compressedFormat_;
        image->numCompressedLevels_ = subimageLevels;
        image->components_ = components_;
        image->data_ = new unsigned char[subimageData.size()];
        memcpy(image->data_.get(), &subimageData[0], subimageData.size());
        image->SetMemoryUse(subimageData.size());

        return image;
    }
}

SDL_Surface* Image::GetSDLSurface(const IntRect& rect) const
{
    if (!data_)
        return nullptr;

    if (depth_ > 1)
    {
        URHO3D_LOGERROR("Can not get SDL surface from 3D image");
        return nullptr;
    }

    if (IsCompressed())
    {
        URHO3D_LOGERROR("Can not get SDL surface from compressed image " + GetName());
        return nullptr;
    }

    if (components_ < 3)
    {
        URHO3D_LOGERROR("Can not get SDL surface from image " + GetName() + " with less than 3 components");
        return nullptr;
    }

    IntRect imageRect = rect;
    // Use full image if illegal rect
    if (imageRect.left_ < 0 || imageRect.top_ < 0 || imageRect.right_ > width_ || imageRect.bottom_ > height_ ||
        imageRect.left_ >= imageRect.right_ || imageRect.top_ >= imageRect.bottom_)
    {
        imageRect.left_ = 0;
        imageRect.top_ = 0;
        imageRect.right_ = width_;
        imageRect.bottom_ = height_;
    }

    int imageWidth = width_;
    int width = imageRect.Width();
    int height = imageRect.Height();

    // Assume little-endian for all the supported platforms
    unsigned rMask = 0x000000ff;
    unsigned gMask = 0x0000ff00;
    unsigned bMask = 0x00ff0000;
    unsigned aMask = 0xff000000;

    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, components_ * 8, rMask, gMask, bMask, aMask);
    if (surface)
    {
        SDL_LockSurface(surface);

        auto* destination = reinterpret_cast<unsigned char*>(surface->pixels);
        unsigned char* source = data_.get() + components_ * (imageWidth * imageRect.top_ + imageRect.left_);
        for (int i = 0; i < height; ++i)
        {
            memcpy(destination, source, (size_t)components_ * width);
            destination += surface->pitch;
            source += components_ * imageWidth;
        }

        SDL_UnlockSurface(surface);
    }
    else
        URHO3D_LOGERROR("Failed to create SDL surface from image " + GetName());

    return surface;
}

void Image::PrecalculateLevels()
{
    if (!data_ || IsCompressed())
        return;

    URHO3D_PROFILE("PrecalculateImageMipLevels");

    nextLevel_.Reset();

    if (width_ > 1 || height_ > 1)
    {
        SharedPtr<Image> current = GetNextLevel();
        nextLevel_ = current;
        while (current && (current->width_ > 1 || current->height_ > 1))
        {
            current->nextLevel_ = current->GetNextLevel();
            current = current->nextLevel_;
        }
    }
}

void Image::CleanupLevels()
{
    nextLevel_.Reset();
}

void Image::GetLevels(ea::vector<Image*>& levels)
{
    levels.clear();

    Image* image = this;
    while (image)
    {
        levels.push_back(image);
        image = image->nextLevel_;
    }
}

void Image::GetLevels(ea::vector<const Image*>& levels) const
{
    levels.clear();

    const Image* image = this;
    while (image)
    {
        levels.push_back(image);
        image = image->nextLevel_;
    }
}

unsigned char* Image::GetImageData(Deserializer& source, int& width, int& height, unsigned& components)
{
    unsigned dataSize = source.GetSize();

    ea::shared_array<unsigned char> buffer(new unsigned char[dataSize]);
    source.Read(buffer.get(), dataSize);
    return stbi_load_from_memory(buffer.get(), dataSize, &width, &height, (int*)&components, 0);
}

void Image::FreeImageData(unsigned char* pixelData)
{
    if (!pixelData)
        return;

    stbi_image_free(pixelData);
}

bool Image::HasAlphaChannel() const
{
    return components_ > 3;
}

// Author: Josh Engebretson (AtomicGameEngine)
bool Image::SetSubimage(const Image* image, const IntRect& rect)
{
    if (!data_)
        return false;

    if (depth_ > 1 || IsCompressed())
    {
        URHO3D_LOGERROR("Image::SetSubimage is not supported for compressed or 3D images");
        return false;
    }

    if (components_ != image->components_)
    {
        URHO3D_LOGERROR("Can not set subimage in image " + GetName() + " with different number of components");
        return false;
    }

    if (rect.left_ < 0 || rect.top_ < 0 || rect.right_ > width_ || rect.bottom_ > height_ || !rect.Width() || !rect.Height())
    {
        URHO3D_LOGERROR("Can not set subimage in image " + GetName() + " with invalid region");
        return false;
    }

    const int destWidth = rect.Width();
    const int destHeight = rect.Height();
    if (destWidth == image->GetWidth() && destHeight == image->GetHeight())
    {
        unsigned char* src = image->GetData();
        unsigned char* dest = data_.get() + (rect.top_ * width_ + rect.left_) * components_;
        for (int i = 0; i < destHeight; ++i)
        {
            memcpy(dest, src, (size_t)destWidth * components_);

            src += destWidth * image->components_;
            dest += width_ * components_;
        }
    }
    else
    {
        unsigned char* dest = data_.get() + (rect.top_ * width_ + rect.left_) * components_;
        for (int y = 0; y < destHeight; ++y)
        {
            for (int x = 0; x < destWidth; ++x)
            {
                // Calculate float coordinates between 0 - 1 for resampling
                const float xF = (image->width_ > 1) ? static_cast<float>(x) / (destWidth - 1) : 0.0f;
                const float yF = (image->height_ > 1) ? static_cast<float>(y) / (destHeight - 1) : 0.0f;
                const unsigned uintColor = image->GetPixelBilinear(xF, yF).ToUInt();

                memcpy(dest, reinterpret_cast<const unsigned char*>(&uintColor), components_);

                dest += components_;
            }
            dest += (width_ - destWidth) * components_;
        }
    }

    return true;
}

}
