#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/PackageBuilder.h>
#include <Urho3D/Resource/PlatformExportAdapter.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace Urho3D;
namespace fs = std::filesystem;

namespace
{

unsigned HashBytes(const ea::string& data)
{
    unsigned hash = 2166136261u;
    for (unsigned char byte : data)
    {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

bool ReadTextFile(const fs::path& path, ea::string& result)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    std::ostringstream contents;
    contents << stream.rdbuf();
    result = contents.str().c_str();
    return true;
}

bool WriteTextFile(const fs::path& path, const ea::string& contents)
{
    if (path.has_parent_path())
        fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return false;
    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    return stream.good();
}

void PrintUsage()
{
    std::cout << "Usage: PackageBuilder <profile.json> <asset-root> <manifest.json>\n"
                 "       PackageBuilder --help\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        PrintUsage();
        return 0;
    }
    if (argc != 4)
    {
        PrintUsage();
        return 2;
    }

    const fs::path profilePath = fs::u8path(argv[1]);
    const fs::path assetRoot = fs::u8path(argv[2]);
    const fs::path manifestPath = fs::u8path(argv[3]);

    ea::string profileText;
    if (!ReadTextFile(profilePath, profileText))
    {
        std::cerr << "PackageBuilder: unable to read profile '" << profilePath.string() << "'.\n";
        return 3;
    }

    JSONValue profileJson;
    if (!JSONFile::ParseJSON(profileText, profileJson, true))
    {
        std::cerr << "PackageBuilder: invalid profile JSON.\n";
        return 4;
    }

    PackageBuildProfile profile;
    ea::string error;
    if (!profile.FromJSON(profileJson, &error))
    {
        std::cerr << "PackageBuilder: " << error.c_str() << "\n";
        return 5;
    }
    const PlatformExportAdapter* adapter = PlatformExportAdapter::Find(profile.platform);
    if (!adapter)
    {
        std::cerr << "PackageBuilder: no export adapter is registered for platform '"
                  << PackageBuilder::ToString(profile.platform).c_str() << "'.\n";
        return 5;
    }
    if (!adapter->Validate(profile, &error))
    {
        std::cerr << "PackageBuilder: " << error.c_str() << "\n";
        return 5;
    }
    if (!fs::is_directory(assetRoot))
    {
        std::cerr << "PackageBuilder: asset root is not a directory.\n";
        return 6;
    }

    ea::vector<PackageFileEntry> candidates;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(assetRoot))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path relative = fs::relative(entry.path(), assetRoot);
        const ea::string relativePath = relative.generic_string().c_str();
        ea::string bytes;
        if (!ReadTextFile(entry.path(), bytes))
        {
            std::cerr << "PackageBuilder: unable to read asset '" << entry.path().string() << "'.\n";
            return 7;
        }

        PackageFileEntry file;
        file.sourcePath = relativePath;
        file.packagePath = relativePath;
        file.contentHash = HashBytes(bytes);
        file.size = static_cast<unsigned long long>(fs::file_size(entry.path()));
        candidates.push_back(ea::move(file));
    }

    PackageManifest manifest;
    if (!PackageBuilder::BuildManifest(profile, candidates, manifest, &error))
    {
        std::cerr << "PackageBuilder: " << error.c_str() << "\n";
        return 8;
    }

    JSONFile output(nullptr);
    output.GetRoot() = manifest.ToJSON();
    if (!WriteTextFile(manifestPath, output.ToString("  ")))
    {
        std::cerr << "PackageBuilder: unable to write manifest '" << manifestPath.string() << "'.\n";
        return 9;
    }

    std::cout << "Packaged " << manifest.files.size() << " files for " << profile.name.c_str()
              << " (" << PackageBuilder::ToString(profile.platform).c_str() << ").\n";
    return 0;
}
