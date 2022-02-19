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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Graphics.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../UI/Font.h"
#include "../UI/FontFaceBitmap.h"
#include "../UI/FontFaceFreeType.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{
    // Convert float to 26.6 fixed-point (as used internally by FreeType)
    inline int FloatToFixed(float value)
    {
        return (int)(value * 64);
    }
}

static const float MIN_POINT_SIZE = 1;
static const float MAX_POINT_SIZE = 96;

Font::Font(Context* context) :
    Resource(context),
    fontDataSize_(0),
    absoluteOffset_(IntVector2::ZERO),
    scaledOffset_(Vector2::ZERO),
    fontType_(FONT_NONE),
    sdfFont_(false)
{
}

Font::~Font()
{
    // To ensure FreeType deallocates properly, first clear all faces, then release the raw font data
    ReleaseFaces();
    fontData_.reset();
}

void Font::RegisterObject(Context* context)
{
    context->RegisterFactory<Font>();
}

bool Font::BeginLoad(Deserializer& source)
{
    // In headless mode, do not actually load, just return success
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return true;

    fontType_ = FONT_NONE;
    faces_.clear();

    fontDataSize_ = source.GetSize();
    if (fontDataSize_)
    {
        fontData_ = new unsigned char[fontDataSize_];
        if (source.Read(&fontData_[0], fontDataSize_) != fontDataSize_)
            return false;
    }
    else
    {
        fontData_.reset();
        return false;
    }

    ea::string ext = GetExtension(GetName());
    if (ext == ".ttf" || ext == ".otf" || ext == ".woff")
    {
        fontType_ = FONT_FREETYPE;
        LoadParameters();
    }
    else if (ext == ".xml" || ext == ".fnt" || ext == ".sdf")
        fontType_ = FONT_BITMAP;

    sdfFont_ = ext == ".sdf";

    SetMemoryUse(fontDataSize_);
    return true;
}

bool Font::SaveXML(Serializer& dest, int pointSize, bool usedGlyphs, const ea::string& indentation)
{
    FontFace* fontFace = GetFace(pointSize);
    if (!fontFace)
        return false;

    URHO3D_PROFILE("FontSaveXML");

    SharedPtr<FontFaceBitmap> packedFontFace(new FontFaceBitmap(this));
    if (!packedFontFace->Load(fontFace, usedGlyphs))
        return false;

    return packedFontFace->Save(dest, pointSize, indentation);
}

void Font::SetAbsoluteGlyphOffset(const IntVector2& offset)
{
    absoluteOffset_ = offset;
}

void Font::SetScaledGlyphOffset(const Vector2& offset)
{
    scaledOffset_ = offset;
}

FontFace* Font::GetFace(float pointSize)
{
    // In headless mode, always return null
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return nullptr;

    // For bitmap font type, always return the same font face provided by the font's bitmap file regardless of the actual requested point size
    if (fontType_ == FONT_BITMAP)
        pointSize = 0;
    else
        pointSize = Clamp(pointSize, MIN_POINT_SIZE, MAX_POINT_SIZE);

    // For outline fonts, we return the nearest size in 1/64th increments, as that's what FreeType supports.
    int key = FloatToFixed(pointSize);
    auto i = faces_.find(key);
    if (i != faces_.end())
    {
        if (!i->second->IsDataLost())
            return i->second;
        else
        {
            // Erase and reload face if texture data lost (OpenGL mode only)
            faces_.erase(i);
        }
    }

    URHO3D_PROFILE("GetFontFace");

    switch (fontType_)
    {
    case FONT_FREETYPE:
        return GetFaceFreeType(pointSize);

    case FONT_BITMAP:
        return GetFaceBitmap(pointSize);

    default:
        return nullptr;
    }
}

IntVector2 Font::GetTotalGlyphOffset(float pointSize) const
{
    Vector2 multipliedOffset = pointSize * scaledOffset_;
    return absoluteOffset_ + IntVector2(RoundToInt(multipliedOffset.x_), RoundToInt(multipliedOffset.y_));
}

void Font::ReleaseFaces()
{
    faces_.clear();
}

void Font::LoadParameters()
{
    auto* cache = GetSubsystem<ResourceCache>();
    ea::string xmlName = ReplaceExtension(GetName(), ".xml");
    SharedPtr<XMLFile> xml = cache->GetTempResource<XMLFile>(xmlName, false);
    if (!xml)
        return;

    XMLElement rootElem = xml->GetRoot();

    XMLElement absoluteElem = rootElem.GetChild("absoluteoffset");
    if (!absoluteElem)
        absoluteElem = rootElem.GetChild("absolute");

    if (absoluteElem)
    {
        absoluteOffset_.x_ = absoluteElem.GetInt("x");
        absoluteOffset_.y_ = absoluteElem.GetInt("y");
    }

    XMLElement scaledElem = rootElem.GetChild("scaledoffset");
    if (!scaledElem)
        scaledElem = rootElem.GetChild("scaled");

    if (scaledElem)
    {
        scaledOffset_.x_ = scaledElem.GetFloat("x");
        scaledOffset_.y_ = scaledElem.GetFloat("y");
    }
}

FontFace* Font::GetFaceFreeType(float pointSize)
{
    SharedPtr<FontFace> newFace(new FontFaceFreeType(this));
    if (!newFace->Load(&fontData_[0], fontDataSize_, pointSize))
        return nullptr;

    int key = FloatToFixed(pointSize);
    faces_[key] = newFace;
    return newFace;
}

FontFace* Font::GetFaceBitmap(float pointSize)
{
    SharedPtr<FontFace> newFace(new FontFaceBitmap(this));
    if (!newFace->Load(&fontData_[0], fontDataSize_, pointSize))
        return nullptr;

    int key = FloatToFixed(pointSize);
    faces_[key] = newFace;
    return newFace;
}

}
