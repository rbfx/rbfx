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

#pragma once

#include "../Resource/Resource.h"

namespace Urho3D
{

class PListFile;
class Sprite2D;
class Texture2D;
class XMLFile;
class JSONFile;

/// Sprite sheet.
class URHO3D_API SpriteSheet2D : public Resource
{
    URHO3D_OBJECT(SpriteSheet2D, Resource);

public:
    /// Construct.
    explicit SpriteSheet2D(Context* context);
    /// Destruct.
    ~SpriteSheet2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Set texture.
    /// @property
    void SetTexture(Texture2D* texture);
    /// Define sprite.
    void DefineSprite(const ea::string& name, const IntRect& rectangle, const Vector2& hotSpot = Vector2(0.5f, 0.5f),
        const IntVector2& offset = IntVector2::ZERO);

    /// Return texture.
    /// @property
    Texture2D* GetTexture() const { return texture_; }
    /// Return sprite.
    Sprite2D* GetSprite(const ea::string& name) const;

    /// Return sprite mapping.
    const ea::unordered_map<ea::string, SharedPtr<Sprite2D> >& GetSpriteMapping() const { return spriteMapping_; }

private:
    /// Begin load from PList file.
    bool BeginLoadFromPListFile(Deserializer& source);
    /// End load from PList file.
    bool EndLoadFromPListFile();

    /// Begin load from XML file.
    bool BeginLoadFromXMLFile(Deserializer& source);
    /// End load from XML file.
    bool EndLoadFromXMLFile();
    /// Begin load from JSON file.
    bool BeginLoadFromJSONFile(Deserializer& source);
    /// End load from JSON file.
    bool EndLoadFromJSONFile();

    /// Texture.
    SharedPtr<Texture2D> texture_;
    /// Sprite mapping.
    ea::unordered_map<ea::string, SharedPtr<Sprite2D> > spriteMapping_;
    /// PList file used while loading.
    SharedPtr<PListFile> loadPListFile_;
    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;
    /// JSON file used while loading.
    SharedPtr<JSONFile> loadJSONFile_;
    /// Texture name used while loading.
    ea::string loadTextureName_;
};

}
