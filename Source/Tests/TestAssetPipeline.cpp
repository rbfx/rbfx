#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Resource/AssetPipeline.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>

#include "CommonUtils.h"

using namespace Urho3D;

TEST_CASE("Asset import settings round trip preserves typed scalar properties", "[assets][serialization]")
{
    AssetImportSettings source;
    source.version = 3;
    source.importer = "TextureImporter";
    source.properties["GenerateMipmaps"] = Variant(true);
    source.properties["Quality"] = Variant(0.75f);
    source.properties["Compression"] = Variant(2);
    source.properties["Profile"] = Variant(ea::string("Mobile"));

    const JSONValue json = source.ToJSON();
    AssetImportSettings restored;
    ea::string error;
    REQUIRE(restored.FromJSON(json, &error));
    REQUIRE(error.empty());
    REQUIRE(restored.version == 3);
    REQUIRE(restored.importer == "TextureImporter");
    REQUIRE(restored.properties.at("GenerateMipmaps").GetBool());
    REQUIRE(restored.properties.at("Quality").GetFloat() == Catch::Approx(0.75f));
    REQUIRE(restored.properties.at("Compression").GetInt() == 2);
    REQUIRE(restored.properties.at("Profile").GetString() == "Mobile");
    REQUIRE(restored.CalculateHash() == source.CalculateHash());
}

TEST_CASE("Asset import settings resource saves and loads JSON", "[assets][resource][serialization]")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    AssetImportSettingsResource source(context);
    source.GetSettings().version = 4;
    source.GetSettings().importer = "ModelImporter";
    source.GetSettings().properties["GenerateLODs"] = Variant(true);

    VectorBuffer buffer;
    REQUIRE(source.Save(buffer));
    REQUIRE(buffer.GetSize() > 0);
    buffer.Seek(0);

    AssetImportSettingsResource restored(context);
    REQUIRE(restored.BeginLoad(buffer));
    REQUIRE(restored.GetSettings().version == 4);
    REQUIRE(restored.GetSettings().importer == "ModelImporter");
    REQUIRE(restored.GetSettings().properties.at("GenerateLODs").GetBool());
}

TEST_CASE("Asset importer reuses cache and invalidates on settings changes", "[assets][cache]")
{
    AssetImporter importer;
    unsigned callbackCount = 0;
    importer.RegisterRule({"mesh", "MeshImporter", 7,
        [&](const ea::string&, const ea::string&, const AssetImportSettings&, const ea::string&,
            ea::vector<ea::string>& dependencies, ea::string&)
        {
            ++callbackCount;
            dependencies.push_back("materials/hero.material");
            return true;
        }});

    AssetImportSettings settings;
    settings.importer = "MeshImporter";
    settings.properties["Optimize"] = Variant(true);

    const AssetImportResult first = importer.Import("models/hero.mesh", "source-v1", settings, "Cooked/hero.model");
    REQUIRE(first.success);
    REQUIRE_FALSE(first.fromCache);
    REQUIRE(callbackCount == 1);

    const AssetImportResult second = importer.Import("models/hero.mesh", "source-v1", settings, "Cooked/hero.model");
    REQUIRE(second.success);
    REQUIRE(second.fromCache);
    REQUIRE(callbackCount == 1);

    settings.properties["Optimize"] = Variant(false);
    const AssetImportResult third = importer.Import("models/hero.mesh", "source-v1", settings, "Cooked/hero.model");
    REQUIRE(third.success);
    REQUIRE_FALSE(third.fromCache);
    REQUIRE(callbackCount == 2);
    REQUIRE(importer.GetCache().GetEntryCount() == 2);
}

TEST_CASE("Asset dependency graph propagates dirty state and rejects cycles", "[assets][dependencies]")
{
    AssetDependencyGraph graph;
    ea::string error;
    REQUIRE(graph.SetDependencies("materials/hero.material", {"textures/hero.png"}, &error));
    REQUIRE(graph.SetDependencies("models/hero.mesh", {"materials/hero.material"}, &error));
    REQUIRE(graph.SetDependencies("levels/demo.scene", {"models/hero.mesh"}, &error));

    const ea::vector<ea::string> dirty = graph.CollectDirty({"textures/hero.png"});
    REQUIRE(dirty == ea::vector<ea::string>({"levels/demo.scene", "materials/hero.material", "models/hero.mesh", "textures/hero.png"}));
    REQUIRE(graph.GetDependents("materials/hero.material") == ea::vector<ea::string>({"models/hero.mesh"}));

    REQUIRE_FALSE(graph.SetDependencies("textures/hero.png", {"levels/demo.scene"}, &error));
    REQUIRE_FALSE(error.empty());
    REQUIRE(graph.Validate(&error));
}

TEST_CASE("Asset importer invalidates cached dependents through dependency graph", "[assets][dependencies][cache]")
{
    AssetImporter importer;
    importer.RegisterRule({"mat", "MaterialImporter", 1, {}});
    importer.RegisterRule({"scene", "SceneImporter", 1,
        [](const ea::string&, const ea::string&, const AssetImportSettings&, const ea::string&,
            ea::vector<ea::string>& dependencies, ea::string&)
        {
            dependencies.push_back("materials/hero.mat");
            return true;
        }});

    AssetImportSettings settings;
    REQUIRE(importer.Import("materials/hero.mat", "mat-v1", settings, "Cooked/hero.material").success);
    REQUIRE(importer.Import("levels/demo.scene", "scene-v1", settings, "Cooked/demo.scene").success);
    REQUIRE(importer.GetCache().GetEntryCount() == 2);

    const ea::vector<ea::string> dirty = importer.MarkDirty("materials/hero.mat");
    REQUIRE(dirty == ea::vector<ea::string>({"levels/demo.scene", "materials/hero.mat"}));
    REQUIRE(importer.GetCache().GetEntryCount() == 0);
}
