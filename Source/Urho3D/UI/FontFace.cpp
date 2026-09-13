// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Resource/Image.h"
#include "Urho3D/UI/Font.h"
#include "Urho3D/UI/FontFace.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

FontFace::FontFace(Font* font) :
    font_(font)
{
}

FontFace::~FontFace()
{
    if (font_)
    {
        // When a face is unloaded, deduct the used texture data size from the parent font
        unsigned totalTextureSize = 0;
        for (unsigned i = 0; i < textures_.size(); ++i)
            totalTextureSize += textures_[i]->GetWidth() * textures_[i]->GetHeight();
        font_->SetMemoryUse(font_->GetMemoryUse() - totalTextureSize);
    }
}

const FontGlyph* FontFace::GetGlyph(unsigned c)
{
    auto i = glyphMapping_.find(c);
    if (i != glyphMapping_.end())
    {
        FontGlyph& glyph = i->second;
        glyph.used_ = true;
        return &glyph;
    }
    else
        return nullptr;
}

float FontFace::GetKerning(unsigned c, unsigned d) const
{
    if (kerningMapping_.empty())
        return 0;

    if (c == '\n' || d == '\n')
        return 0;

    if (c > 0xffff || d > 0xffff)
        return 0;

    unsigned value = (c << 16u) + d;

    auto i = kerningMapping_.find(value);
    if (i != kerningMapping_.end())
        return i->second;

    return 0;
}

bool FontFace::IsDataLost() const
{
    for (unsigned i = 0; i < textures_.size(); ++i)
    {
        if (textures_[i]->IsDataLost())
            return true;
    }
    return false;
}


SharedPtr<Texture2D> FontFace::CreateFaceTexture()
{
    auto texture = MakeShared<Texture2D>(font_->GetContext());
    texture->SetMipsToSkip(QUALITY_LOW, 0); // No quality reduction
    texture->SetNumLevels(1); // No mipmaps
    texture->SetAddressMode(TextureCoordinate::U, ADDRESS_CLAMP);
    texture->SetAddressMode(TextureCoordinate::V, ADDRESS_CLAMP);
    return texture;
}

SharedPtr<Texture2D> FontFace::LoadFaceTexture(const SharedPtr<Image>& image)
{
    SharedPtr<Texture2D> texture = CreateFaceTexture();
    texture->SetName(Format("{}:{}", image->GetName(), pointSize_));
    if (!texture->SetData(image))
    {
        URHO3D_LOGERROR("Could not load texture from image resource");
        return SharedPtr<Texture2D>();
    }
    return texture;
}

}
