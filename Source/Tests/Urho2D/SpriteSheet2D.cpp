// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
