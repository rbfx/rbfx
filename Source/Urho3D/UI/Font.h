// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/shared_array.h>

#include "Urho3D/Resource/Resource.h"

namespace Urho3D
{

class FontFace;

static const int FONT_TEXTURE_MIN_SIZE = 128;
static const int FONT_DPI = 96;

/// %Font file type.
enum FontType
{
    FONT_NONE = 0,
    FONT_FREETYPE,
    FONT_BITMAP,
    MAX_FONT_TYPES
};

/// %Font resource.
class URHO3D_API Font : public Resource
{
    URHO3D_OBJECT(Font, Resource);

public:
    /// Construct.
    explicit Font(Context* context);
    /// Destruct.
    ~Font() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource as a new bitmap font type in XML format. Return true if successful.
    bool SaveXML(Serializer& dest, int pointSize, bool usedGlyphs = false, const ea::string& indentation = "\t");
    /// Set absolute (in pixels) position adjustment for glyphs.
    /// @property
    void SetAbsoluteGlyphOffset(const IntVector2& offset);
    /// Set point size scaled position adjustment for glyphs.
    /// @property
    void SetScaledGlyphOffset(const Vector2& offset);

    /// Return font face. Pack and render to a texture if not rendered yet. Return null on error.
    FontFace* GetFace(float pointSize);

    /// Return font type.
    /// @property
    FontType GetFontType() const { return fontType_; }

    /// Is signed distance field font.
    bool IsSDFFont() const { return sdfFont_; }

    /// Return absolute position adjustment for glyphs.
    /// @property
    const IntVector2& GetAbsoluteGlyphOffset() const { return absoluteOffset_; }

    /// Return point size scaled position adjustment for glyphs.
    /// @property
    const Vector2& GetScaledGlyphOffset() const { return scaledOffset_; }

    /// Return the total effective offset for a point size.
    IntVector2 GetTotalGlyphOffset(float pointSize) const;

    /// Release font faces and recreate them next time when requested. Called when font textures lost or global font properties change.
    void ReleaseFaces();

private:
    /// Load font glyph offset parameters from an optional XML file. Called internally when loading TrueType fonts.
    void LoadParameters();
    /// Return font face using FreeType. Called internally. Return null on error.
    FontFace* GetFaceFreeType(float pointSize);
    /// Return bitmap font face. Called internally. Return null on error.
    FontFace* GetFaceBitmap(float pointSize);

    /// Created faces.
    ea::unordered_map<int, SharedPtr<FontFace> > faces_;
    /// Font data.
    ea::shared_array<unsigned char> fontData_;
    /// Size of font data.
    unsigned fontDataSize_;
    /// Absolute position adjustment for glyphs.
    IntVector2 absoluteOffset_;
    /// Point size scaled position adjustment for glyphs.
    Vector2 scaledOffset_;
    /// Font type.
    FontType fontType_;
    /// Signed distance field font flag.
    bool sdfFont_;
};

}
