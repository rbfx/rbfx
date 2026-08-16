#include <Urho3D/Resource/PackageBuilder.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

TEST_CASE("Package build profile round trip preserves platform and filters", "[packaging][profile]")
{
    PackageBuildProfile profile;
    profile.version = 3;
    profile.name = "WindowsShipping";
    profile.platform = PackagePlatform::Windows;
    profile.architecture = "x64";
    profile.optimization = PackageOptimization::Shipping;
    profile.outputPath = "dist/windows";
    profile.reproducible = true;
    profile.assetFilters.push_back({"*.rbscene", false});
    profile.assetFilters.push_back({"Generated/*", true});

    const JSONValue json = profile.ToJSON();
    PackageBuildProfile loaded;
    ea::string error;
    REQUIRE(loaded.FromJSON(json, &error));
    CHECK(error.empty());
    CHECK(loaded.version == 3);
    CHECK(loaded.name == "WindowsShipping");
    CHECK(loaded.platform == PackagePlatform::Windows);
    CHECK(loaded.architecture == "x64");
    CHECK(loaded.optimization == PackageOptimization::Shipping);
    CHECK(loaded.outputPath == "dist/windows");
    CHECK(loaded.IncludesAsset("levels/main.rbscene"));
    CHECK_FALSE(loaded.IncludesAsset("Generated/main.rbscene"));
    CHECK_FALSE(loaded.IncludesAsset("textures/albedo.png"));
}

TEST_CASE("Package filters include all assets when no include rule exists", "[packaging][filters]")
{
    PackageBuildProfile profile;
    profile.assetFilters.push_back({"Generated/*", true});
    CHECK(profile.IncludesAsset("Textures/albedo.png"));
    CHECK_FALSE(profile.IncludesAsset("Generated/cache.bin"));
}

TEST_CASE("Package builder applies filters and produces a sorted manifest", "[packaging][manifest]")
{
    PackageBuildProfile profile;
    profile.name = "LinuxDevelopment";
    profile.platform = PackagePlatform::Linux;
    profile.architecture = "x64";
    profile.assetFilters.push_back({"*", false});
    profile.assetFilters.push_back({"*.tmp", true});

    ea::vector<PackageFileEntry> candidates;
    candidates.push_back({"z.rbscene", "Content/z.rbscene", 10, 100});
    candidates.push_back({"cache.tmp", "Content/cache.tmp", 11, 20});
    candidates.push_back({"a.rbscene", "Content/a.rbscene", 12, 200});

    PackageManifest manifest;
    ea::string error;
    REQUIRE(PackageBuilder::BuildManifest(profile, candidates, manifest, &error));
    CHECK(error.empty());
    REQUIRE(manifest.files.size() == 2);
    CHECK(manifest.files[0].sourcePath == "z.rbscene");
    CHECK(manifest.files[1].sourcePath == "a.rbscene");

    const JSONValue json = manifest.ToJSON();
    REQUIRE(json.Contains("files"));
    REQUIRE(json["files"].GetArray().size() == 2);
    CHECK(json["files"][0]["packagePath"].GetString() == "Content/a.rbscene");
    CHECK(json["files"][1]["packagePath"].GetString() == "Content/z.rbscene");

    PackageManifest loaded;
    REQUIRE(loaded.FromJSON(json, &error));
    CHECK(loaded.files.size() == 2);
    CHECK(loaded.files[0].size == 200);
}

TEST_CASE("Package manifest validation rejects duplicate output paths", "[packaging][validation]")
{
    PackageManifest manifest;
    manifest.profileName = "Linux";
    manifest.architecture = "x64";
    manifest.files.push_back({"a.png", "Content/shared.bin", 1, 10});
    manifest.files.push_back({"b.png", "Content/shared.bin", 2, 20});

    const PackageValidationResult validation = PackageBuilder::ValidateManifest(manifest);
    CHECK_FALSE(validation.valid);
    REQUIRE_FALSE(validation.errors.empty());
    CHECK(validation.errors.front().find("Duplicate") != ea::string::npos);
}
