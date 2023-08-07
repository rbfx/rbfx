//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Graphics/Texture2D.h>

using namespace Urho3D;

TEST_CASE("Serialize SpriteSheet2D")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto spriteSheet = context->GetSubsystem<ResourceCache>()->GetResource<SpriteSheet2D>("Urho2D/Orc/Orc.xml");
    REQUIRE(spriteSheet);
    REQUIRE(!spriteSheet->GetSpriteMapping().empty());

    VectorBuffer data;
    data.SetName(spriteSheet->GetName());
    spriteSheet->Save(data);
    REQUIRE(data.GetSize() != 0);
    data.Seek(0);

    auto spriteSheet2 = MakeShared<SpriteSheet2D>(context);
    spriteSheet2->SetAbsoluteFileName(spriteSheet->GetAbsoluteFileName());
    spriteSheet2->SetName(spriteSheet->GetName());
    spriteSheet2->Load(data);
    for (auto& spriteKV: spriteSheet->GetSpriteMapping())
    {
        auto sprite = spriteSheet2->GetSprite(spriteKV.first);
        REQUIRE(sprite);
        CHECK(sprite->GetRectangle() == spriteKV.second->GetRectangle());
        CHECK(sprite->GetOffset() == spriteKV.second->GetOffset());
        CHECK(sprite->GetHotSpot().Equals(spriteKV.second->GetHotSpot()));
    }
}

TEST_CASE("Hotspot evaluation SpriteSheet2D")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto orcSheet = context->GetSubsystem<ResourceCache>()->GetResource<SpriteSheet2D>("Urho2D/Orc/Orc.xml");

    auto spriteSheet = MakeShared<SpriteSheet2D>(context);
    spriteSheet->SetAbsoluteFileName(orcSheet->GetAbsoluteFileName());
    spriteSheet->SetName(orcSheet->GetName());
    spriteSheet->SetTexture(orcSheet->GetTexture());
    spriteSheet->DefineSprite("bla", IntRect(1, 2, 100, 200), Vector2(0.29798f, 0.10101f), IntVector2(10, 20));
    VectorBuffer data;
    data.SetName(spriteSheet->GetName());
    spriteSheet->Save(data);
    data.Seek(0);
    ea::string xml((const char*)data.GetData(), data.GetSize());

    auto spriteSheet2 = MakeShared<SpriteSheet2D>(context);
    spriteSheet2->SetAbsoluteFileName(spriteSheet->GetAbsoluteFileName());
    spriteSheet2->SetName(spriteSheet->GetName());
    spriteSheet2->Load(data);
    for (auto& spriteKV : spriteSheet->GetSpriteMapping())
    {
        auto sprite = spriteSheet2->GetSprite(spriteKV.first);
        REQUIRE(sprite);
        CHECK(sprite->GetRectangle() == spriteKV.second->GetRectangle());
        CHECK(sprite->GetOffset() == spriteKV.second->GetOffset());
        CHECK(sprite->GetHotSpot().Equals(spriteKV.second->GetHotSpot()));
    }
}
