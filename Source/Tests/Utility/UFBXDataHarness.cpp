// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "UFBXDataHarness.h"

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/ModelView.h>

#include <EASTL/algorithm.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>

namespace Urho3D
{

namespace UFBXData
{

namespace
{

namespace fs = std::filesystem;

ea::string GetEnv(const char* name)
{
    const char* value = std::getenv(name);
    return value ? ea::string(value) : ea::string();
}

ea::string ToEa(const std::string& value)
{
    return ea::string(value.c_str(), value.size());
}

ea::string ToLowerAscii(ea::string value)
{
    for (char& c : value)
    {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return value;
}

/// Remove the format suffix "_<version>_<ascii|binary>" from the FBX stem. Return false if the suffix is missing.
bool GetFbxStem(const std::string& fbxStem, std::string& stem)
{
    static const std::regex suffix(R"(^(.+)_[0-9]+_(ascii|binary)$)");
    std::smatch match;
    if (!std::regex_match(fbxStem, match, suffix))
        return false;
    stem = match[1].str();
    return true;
}

} // namespace

void CaptureCallback::OnModelLoaded(ModelView& modelView)
{
    ++geometry_.numModels_;
    geometry_.numBones_ += modelView.GetBones().size();

    CapturedModel& captured = models_[modelView.GetName()];
    captured.deformed_ = !modelView.GetBones().empty() || !modelView.GetMorphs().empty();
    for (GeometryView& geometry : modelView.GetGeometries())
    {
        if (geometry.lods_.empty())
            continue;
        GeometryLODView& lod = geometry.lods_[0];
        geometry_.numVertices_ += lod.vertices_.size();
        geometry_.numIndices_ += lod.indices_.size();
        if (!lod.IsTriangleGeometry())
            continue;

        const bool hasNormals = lod.vertexFormat_.normal_ != ModelVertexFormat::Undefined;
        const bool hasUVs = lod.vertexFormat_.uv_[0] != ModelVertexFormat::Undefined;
        captured.hasNormals_ |= hasNormals;
        captured.hasUVs_ |= hasUVs;
        const bool hasSkin = !modelView.GetBones().empty() && lod.vertexFormat_.blendIndices_ != ModelVertexFormat::Undefined;

        // Apply the morphs with the initial weights, as AnimatedModel does by default.
        ea::vector<Vector3> morphedPositions(lod.vertices_.size());
        ea::vector<Vector3> morphedNormals(lod.vertices_.size());
        for (unsigned i = 0; i < lod.vertices_.size(); ++i)
        {
            morphedPositions[i] = lod.vertices_[i].GetPosition();
            morphedNormals[i] = lod.vertices_[i].GetNormal();
        }
        const auto& morphs = modelView.GetMorphs();
        for (const auto& [morphIndex, morphVertices] : lod.morphs_)
        {
            const float weight = morphIndex < morphs.size() ? morphs[morphIndex].initialWeight_ : 0.0f;
            if (weight == 0.0f)
                continue;
            for (const ModelVertexMorph& morph : morphVertices)
            {
                if (morph.index_ >= lod.vertices_.size())
                    continue;
                morphedPositions[morph.index_] += morph.positionDelta_ * weight;
                morphedNormals[morph.index_] += morph.normalDelta_ * weight;
            }
        }

        lod.ForEachTriangle(
            [&](unsigned i0, unsigned i1, unsigned i2)
            {
                Triangle triangle;
                const unsigned indices[3] = {i0, i1, i2};
                for (unsigned i = 0; i < 3; ++i)
                {
                    const ModelVertex& vertex = lod.vertices_[indices[i]];
                    Corner& corner = triangle.corners_[i];
                    corner.position_ = morphedPositions[indices[i]];
                    corner.normal_ = hasNormals ? morphedNormals[indices[i]].Normalized() : Vector3::ZERO;
                    corner.uv_ = hasUVs ? Vector2(vertex.uv_[0].x_, vertex.uv_[0].y_) : Vector2::ZERO;
                    if (hasSkin)
                    {
                        SkinCorner skin;
                        const ModelVertex::BoneArray bones = vertex.GetBlendIndicesAndWeights();
                        for (unsigned j = 0; j < 4 && j < bones.size(); ++j)
                        {
                            skin.indices_[j] = bones[j].first;
                            skin.weights_[j] = bones[j].second;
                        }
                        captured.skin_.push_back(skin);
                    }
                }
                captured.triangles_.push_back(triangle);
            });
    }
}

void CaptureCallback::OnAnimationLoaded(Animation& animation)
{
    ++geometry_.numAnimations_;
}

Config ReadConfig()
{
    Config config;
    config.dataDir_ = GetEnv("UFBX_DATA_DIR");
    config.filter_ = GetEnv("UFBX_DATA_FILTER");
    config.report_ = GetEnv("UFBX_DATA_REPORT") == "1";

    std::error_code error;
    if (config.dataDir_.empty())
        config.skipMessage_ = "UFBX_DATA_DIR is not set. Set it to the ufbx \"data\" directory to run this test.";
    else if (!fs::is_directory(fs::path(config.dataDir_.c_str()), error))
        config.skipMessage_ = "UFBX_DATA_DIR does not exist: " + config.dataDir_;
    else
        config.hasData_ = true;
    return config;
}

ea::vector<BaselineFlag> ParseBaselineFlags(const ea::string& objHeader)
{
    ea::vector<BaselineFlag> flags;
    const std::string text(objHeader.c_str(), objHeader.size());
    size_t pos = 0;
    while (pos < text.size())
    {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos)
            end = text.size();
        std::string line = text.substr(pos, end - pos);
        pos = end + 1;

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        // Flags are only in the leading comment block.
        if (line.empty() || line[0] != '#')
            break;

        static const std::string prefix = "# ufbx:";
        if (line.compare(0, prefix.size(), prefix) != 0)
            continue;

        // One line can hold several flags separated by spaces.
        size_t tokenPos = prefix.size();
        while (tokenPos < line.size())
        {
            size_t tokenEnd = line.find(' ', tokenPos);
            if (tokenEnd == std::string::npos)
                tokenEnd = line.size();
            const std::string token = line.substr(tokenPos, tokenEnd - tokenPos);
            tokenPos = tokenEnd + 1;
            if (token.empty())
                continue;

            BaselineFlag flag;
            const size_t equals = token.find('=');
            flag.name_ = ToEa(token.substr(0, equals));
            if (equals != std::string::npos)
                flag.value_ = ToEa(token.substr(equals + 1));
            flags.push_back(flag);
        }
    }
    return flags;
}

ea::vector<BaselineFlag> ReadBaselineFlags(const ea::string& objPath)
{
    std::ifstream stream(fs::path(objPath.c_str()), std::ios::binary);
    if (!stream)
        return {};

    // Flags are in the first lines only. Read the leading comment block.
    std::string header;
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] != '#')
            break;
        header += line;
        header += '\n';
    }
    return ParseBaselineFlags(ToEa(header));
}

ea::vector<DataPair> EnumeratePairs(const ea::string& dataDir, const ea::string& filter)
{
    const fs::path root(dataDir.c_str());
    std::error_code error;

    // Collect the names of all OBJ files in the top level directory.
    std::set<std::string> objNames;
    std::vector<std::string> fbxNames;
    for (fs::directory_iterator iter(root, error), end; !error && iter != end; iter.increment(error))
    {
        if (!iter->is_regular_file(error))
            continue;
        const std::string name = iter->path().filename().string();
        const ea::string extension = ToLowerAscii(ToEa(iter->path().extension().string()));
        if (extension == ".obj")
            objNames.insert(name);
        else if (extension == ".fbx")
            fbxNames.push_back(name);
    }
    std::sort(fbxNames.begin(), fbxNames.end());

    static const std::regex frameSuffix(R"(^_[0-9]+\.obj$)");

    ea::vector<DataPair> pairs;
    for (const std::string& fbxName : fbxNames)
    {
        if (!filter.empty() && fbxName.find(filter.c_str()) == std::string::npos)
            continue;

        std::string stem;
        if (!GetFbxStem(fs::path(fbxName).stem().string(), stem))
            continue;

        DataPair pair;
        pair.name_ = ToEa(fbxName);
        pair.fbxPath_ = ToEa((root / fbxName).string());

        std::string primary;
        for (const std::string& candidate : {stem + ".obj", stem + "_0_obj.obj"})
        {
            if (objNames.count(candidate))
            {
                primary = candidate;
                break;
            }
        }

        // Frame baselines are "<stem>_<frame>.obj". The set is sorted by name, so the order is stable.
        for (auto iter = objNames.lower_bound(stem + "_"); iter != objNames.end(); ++iter)
        {
            if (iter->compare(0, stem.size() + 1, stem + "_") != 0)
                break;
            if (std::regex_match(iter->substr(stem.size()), frameSuffix))
                pair.frameObjPaths_.push_back(ToEa((root / *iter).string()));
        }

        if (primary.empty())
        {
            if (pair.frameObjPaths_.empty())
                continue;
            pair.objPath_ = pair.frameObjPaths_.front();
        }
        else
            pair.objPath_ = ToEa((root / primary).string());

        pair.flags_ = ReadBaselineFlags(pair.objPath_);
        pairs.push_back(ea::move(pair));
    }
    return pairs;
}

const char* ToString(ComparisonResult::Status status)
{
    switch (status)
    {
    case ComparisonResult::Status::NotImplemented: return "not-implemented";
    case ComparisonResult::Status::Match: return "match";
    case ComparisonResult::Status::Mismatch: return "mismatch";
    case ComparisonResult::Status::Skipped: return "skipped";
    }
    return "unknown";
}

} // namespace UFBXData

} // namespace Urho3D
