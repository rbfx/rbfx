// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

// Geometry comparison for the ufbx test data. See GatherImportedMeshes for the comparison space.

#include "UFBXDataHarness.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Skeleton.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>

#include <ufbx.h>

#include <EASTL/algorithm.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Urho3D
{

namespace UFBXData
{

namespace
{

/// Name of the prefab that FBXImporter creates for the prefix "UFBXData/". See UFBXDataTest.cpp.
const char* const prefabName = "UFBXData/Prefab.prefab";

Vector3 NegateX(const Vector3& value) { return {-value.x_, value.y_, value.z_}; }

/// Space conversion from engine space to the canonical space. See GatherImportedMeshes.
struct EngineToCanonical
{
    Quaternion inverseRotation_;
    float inverseScale_{1.0f};

    Vector3 Position(const Vector3& value) const { return inverseRotation_ * NegateX(value) * inverseScale_; }
    Vector3 Normal(const Vector3& value) const { return (inverseRotation_ * NegateX(value)).Normalized(); }
};

bool ReadConversion(const ea::string& fbxPath, EngineToCanonical& conversion, ea::string& error)
{
    // Use the same space options as GetLoadOptions in FBXImporter.cpp. Skip the data that the conversion does not use.
    ufbx_load_opts options{};
    options.ignore_geometry = true;
    options.ignore_animation = true;
    options.ignore_embedded = true;
    options.ignore_missing_external_files = true;
    options.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
    options.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
    options.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
    options.target_axes = ufbx_axes_right_handed_y_up;
    options.target_unit_meters = 1.0;
    options.node_depth_limit = 1024;

    ufbx_error ufbxError{};
    ufbx_scene* scene = ufbx_load_file(fbxPath.c_str(), &options, &ufbxError);
    if (!scene)
    {
        error = "ufbx cannot load the file for the space conversion";
        return false;
    }

    const ufbx_quat& q = scene->metadata.root_rotation;
    const double scale = scene->metadata.root_scale;
    conversion.inverseRotation_ =
        Quaternion(static_cast<float>(q.w), static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z))
            .Inverse();
    conversion.inverseScale_ = scale != 0.0 ? static_cast<float>(1.0 / scale) : 1.0f;
    ufbx_free_scene(scene);
    return true;
}

/// One polygon of the OBJ baseline.
struct ObjPolygon
{
    ea::vector<Corner> corners_;
    unsigned group_{};
    bool hasNormals_{};
    bool hasUVs_{};
};

struct ObjFile
{
    /// Group names. Index 0 is the default group "" for faces before the first "o" or "g".
    ea::vector<ea::string> groups_{""};
    /// Each group name can hold several names, for example "g a b". Blender writes one name.
    ea::vector<ea::vector<ea::string>> groupNames_{{}};
    ea::vector<ObjPolygon> polygons_;
};

/// Parse one OBJ index. The OBJ format allows negative indices relative to the end.
bool ResolveIndex(long value, size_t count, size_t& index)
{
    if (value > 0 && static_cast<size_t>(value) <= count)
        index = static_cast<size_t>(value - 1);
    else if (value < 0 && static_cast<size_t>(-value) <= count)
        index = count - static_cast<size_t>(-value);
    else
        return false;
    return true;
}

bool ParseObj(const ea::string& path, ObjFile& obj, ea::string& error)
{
    std::ifstream stream(path.c_str(), std::ios::binary);
    if (!stream)
    {
        error = "cannot open the OBJ baseline";
        return false;
    }

    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;
    unsigned group = 0;
    std::unordered_map<std::string, unsigned> groupIndices;

    std::string line;
    while (std::getline(stream, line))
    {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        const char* text = line.c_str();
        while (*text == ' ' || *text == '\t')
            ++text;

        if (std::strncmp(text, "v ", 2) == 0)
        {
            char* end = nullptr;
            Vector3 value;
            value.x_ = std::strtof(text + 2, &end);
            value.y_ = std::strtof(end, &end);
            value.z_ = std::strtof(end, &end);
            positions.push_back(value);
        }
        else if (std::strncmp(text, "vn ", 3) == 0)
        {
            char* end = nullptr;
            Vector3 value;
            value.x_ = std::strtof(text + 3, &end);
            value.y_ = std::strtof(end, &end);
            value.z_ = std::strtof(end, &end);
            normals.push_back(value);
        }
        else if (std::strncmp(text, "vt ", 3) == 0)
        {
            char* end = nullptr;
            Vector2 value;
            value.x_ = std::strtof(text + 3, &end);
            value.y_ = std::strtof(end, &end);
            uvs.push_back(value);
        }
        else if (std::strncmp(text, "o ", 2) == 0 || std::strncmp(text, "g ", 2) == 0)
        {
            const std::string name = text + 2;
            const auto [iter, inserted] = groupIndices.emplace(name, static_cast<unsigned>(obj.groups_.size()));
            if (inserted)
            {
                obj.groups_.push_back(ea::string(name.c_str()));
                ea::vector<ea::string> names;
                size_t pos = 0;
                while (pos < name.size())
                {
                    size_t end = name.find(' ', pos);
                    if (end == std::string::npos)
                        end = name.size();
                    if (end > pos)
                        names.push_back(ea::string(name.substr(pos, end - pos).c_str()));
                    pos = end + 1;
                }
                obj.groupNames_.push_back(ea::move(names));
            }
            group = iter->second;
        }
        else if (std::strncmp(text, "f ", 2) == 0)
        {
            ObjPolygon polygon;
            polygon.group_ = group;
            polygon.hasNormals_ = true;
            polygon.hasUVs_ = true;
            const char* cursor = text + 2;
            bool valid = true;
            while (*cursor)
            {
                while (*cursor == ' ' || *cursor == '\t')
                    ++cursor;
                if (!*cursor)
                    break;

                // Corner syntax: "v", "v/vt", "v//vn" or "v/vt/vn".
                long values[3] = {0, 0, 0};
                for (unsigned i = 0; i < 3; ++i)
                {
                    char* end = nullptr;
                    values[i] = std::strtol(cursor, &end, 10);
                    cursor = end;
                    if (*cursor != '/')
                        break;
                    ++cursor;
                }
                while (*cursor && *cursor != ' ' && *cursor != '\t')
                    ++cursor;

                Corner corner;
                size_t index = 0;
                if (!ResolveIndex(values[0], positions.size(), index))
                {
                    valid = false;
                    break;
                }
                corner.position_ = positions[index];
                if (values[1] != 0 && ResolveIndex(values[1], uvs.size(), index))
                    corner.uv_ = uvs[index];
                else
                    polygon.hasUVs_ = false;
                if (values[2] != 0 && ResolveIndex(values[2], normals.size(), index))
                    corner.normal_ = normals[index];
                else
                    polygon.hasNormals_ = false;
                polygon.corners_.push_back(corner);
            }
            if (!valid)
            {
                error = "OBJ baseline has a face with a bad index";
                return false;
            }
            // Lines and points are not triangles. FBXImporter does not import them as triangles.
            if (polygon.corners_.size() >= 3)
                obj.polygons_.push_back(ea::move(polygon));
        }
    }
    return true;
}

/// Comparison options from the "# ufbx:" flags.
struct CompareOptions
{
    float tolerance_{0.001f};
    float positionScale_{1.0f};
    bool negateXZ_{};
    bool ignoreNormals_{};
    bool ignoreUVs_{};
    bool skipDegenerate_{};
};

/// One triangle with the information that the matching uses.
struct MatchTriangle
{
    Corner corners_[3];
    bool useNormals_{};
    bool useUVs_{};
    /// OBJ: polygon index. Imported: mesh index.
    unsigned owner_{};
};

float CornerDistanceSquared(const Corner& lhs, const Corner& rhs, bool useNormals, bool useUVs)
{
    float result = (lhs.position_ - rhs.position_).LengthSquared();
    if (useNormals)
        result += (lhs.normal_ - rhs.normal_).LengthSquared();
    if (useUVs)
        result += (lhs.uv_ - rhs.uv_).LengthSquared();
    return result;
}

bool IsDegenerate(const Corner corners[3], float tolerance)
{
    const float limit = tolerance * tolerance;
    return (corners[0].position_ - corners[1].position_).LengthSquared() < limit
        || (corners[1].position_ - corners[2].position_).LengthSquared() < limit
        || (corners[2].position_ - corners[0].position_).LengthSquared() < limit;
}

/// Spatial hash of imported triangles by the positions of their corners.
class TriangleGrid
{
public:
    TriangleGrid(const ea::vector<MatchTriangle>& triangles, float cellSize)
        : cellSize_(cellSize)
    {
        for (unsigned index = 0; index < triangles.size(); ++index)
        {
            for (const Corner& corner : triangles[index].corners_)
            {
                std::vector<unsigned>& cell = cells_[Key(Cell(corner.position_))];
                if (cell.empty() || cell.back() != index)
                    cell.push_back(index);
            }
        }
    }

    /// Call the callback for each triangle with a corner near the position. A triangle can repeat.
    template <class T> void ForEachNear(const Vector3& position, T callback) const
    {
        const IntVector3 center = Cell(position);
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dz = -1; dz <= 1; ++dz)
                {
                    const auto iter = cells_.find(Key(center + IntVector3(dx, dy, dz)));
                    if (iter == cells_.end())
                        continue;
                    for (unsigned index : iter->second)
                        callback(index);
                }
            }
        }
    }

private:
    IntVector3 Cell(const Vector3& position) const
    {
        return {static_cast<int>(std::floor(position.x_ / cellSize_)),
            static_cast<int>(std::floor(position.y_ / cellSize_)),
            static_cast<int>(std::floor(position.z_ / cellSize_))};
    }

    static uint64_t Key(const IntVector3& cell)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cell.x_)) * 0x9E3779B97F4A7C15ull)
            ^ (static_cast<uint64_t>(static_cast<uint32_t>(cell.y_)) * 0xC2B2AE3D27D4EB4Full)
            ^ (static_cast<uint64_t>(static_cast<uint32_t>(cell.z_)) * 0x165667B19E3779F9ull);
    }

    float cellSize_{};
    std::unordered_map<uint64_t, std::vector<unsigned>> cells_;
};

/// Find the permutation that matches the corners. Return -1 if none, 0 for the same winding, 1 for reversed winding.
int MatchCorners(const MatchTriangle& obj, const MatchTriangle& imported, float toleranceSquared)
{
    static const unsigned permutations[6][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}, {0, 2, 1}, {2, 1, 0}, {1, 0, 2}};
    const bool useNormals = obj.useNormals_ && imported.useNormals_;
    const bool useUVs = obj.useUVs_;
    for (unsigned p = 0; p < 6; ++p)
    {
        bool matched = true;
        for (unsigned i = 0; i < 3 && matched; ++i)
        {
            matched = CornerDistanceSquared(obj.corners_[i], imported.corners_[permutations[p][i]], useNormals, useUVs)
                < toleranceSquared;
        }
        if (matched)
            return p < 3 ? 0 : 1;
    }
    return -1;
}

ea::string FormatCorner(const Corner& corner)
{
    return Format("p=({:.4f} {:.4f} {:.4f}) n=({:.3f} {:.3f} {:.3f}) uv=({:.4f} {:.4f})", corner.position_.x_,
        corner.position_.y_, corner.position_.z_, corner.normal_.x_, corner.normal_.y_, corner.normal_.z_,
        corner.uv_.x_, corner.uv_.y_);
}

/// Describe the nearest corner by position among the triangles.
ea::string DescribeNearest(const Corner& corner, const ea::vector<MatchTriangle>& triangles)
{
    float best = M_LARGE_VALUE;
    const Corner* bestCorner = nullptr;
    for (const MatchTriangle& triangle : triangles)
    {
        for (const Corner& candidate : triangle.corners_)
        {
            const float distance = (candidate.position_ - corner.position_).LengthSquared();
            if (distance < best)
            {
                best = distance;
                bestCorner = &candidate;
            }
        }
    }
    if (!bestCorner)
        return "nearest: none";
    return Format("nearest {} dp={:.5f}", FormatCorner(*bestCorner), std::sqrt(best));
}

bool HasFlag(const DataPair& pair, const char* name, ea::string* value = nullptr)
{
    for (const BaselineFlag& flag : pair.flags_)
    {
        if (flag.name_ == name)
        {
            if (value)
                *value = flag.value_;
            return true;
        }
    }
    return false;
}

bool IsFrameBaseline(const DataPair& pair)
{
    if (HasFlag(pair, "frame"))
        return true;
    // Without a primary baseline, EnumeratePairs uses the first frame baseline "<stem>_<frame>.obj".
    return !pair.frameObjPaths_.empty() && pair.objPath_ == pair.frameObjPaths_.front();
}

/// Return the index of the imported mesh for the OBJ group, or -1. Use the rules of ufbxt_diff_to_obj in a simple form.
int FindMeshForGroup(const ea::vector<ea::string>& names, const ea::vector<ImportedMesh>& meshes)
{
    for (const ea::string& name : names)
    {
        for (unsigned i = 0; i < meshes.size(); ++i)
        {
            if (meshes[i].nodeName_ == name)
                return static_cast<int>(i);
        }
    }
    // Blender adds "_<material>" to the object name.
    for (const ea::string& name : names)
    {
        const ea::string prefix = name.substr(0, name.find('_'));
        for (unsigned i = 0; i < meshes.size(); ++i)
        {
            if (meshes[i].nodeName_ == prefix)
                return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace

void GatherImportedMeshes(
    Context* context, const ea::string& fbxPath, const CaptureCallback& callback, ImportedGeometry& geometry)
{
    EngineToCanonical conversion;
    if (!ReadConversion(fbxPath, conversion, geometry.gatherError_))
        return;

    auto cache = context->GetSubsystem<ResourceCache>();
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>(prefabName);
    if (!prefab)
    {
        geometry.gatherError_ = "the importer created no prefab";
        return;
    }

    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    if (!root)
    {
        geometry.gatherError_ = "cannot instantiate the prefab";
        return;
    }

    ea::vector<StaticModel*> components;
    root->FindComponents<StaticModel>(
        components, ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);
    for (StaticModel* component : components)
    {
        const Model* model = component->GetModel();
        if (!model)
            continue;
        // The skeleton driver models have no callback. They have no triangles of the source.
        const auto iter = callback.models_.find(model->GetName());
        if (iter == callback.models_.end())
            continue;
        const CapturedModel& captured = iter->second;

        Node* node = component->GetNode();
        const Matrix3x4 world = node->GetWorldTransform();
        const Matrix3 normalMatrix = world.ToMatrix3().Inverse().Transpose();

        ImportedMesh mesh;
        mesh.nodeName_ = node->GetName();
        for (Node* parent = node; parent && parent != scene; parent = parent->GetParent())
            mesh.nodePath_.push_back(parent->GetName());
        mesh.deformed_ = captured.deformed_;
        mesh.hasNormals_ = captured.hasNormals_;
        mesh.hasUVs_ = captured.hasUVs_;

        // Skin matrices as AnimatedModel uses them: bone node world transform * offset matrix.
        ea::vector<Matrix3x4> skinMatrices;
        auto animatedModel = dynamic_cast<AnimatedModel*>(component);
        if (animatedModel && !captured.skin_.empty())
        {
            Skeleton& skeleton = animatedModel->GetSkeleton();
            for (unsigned i = 0; i < skeleton.GetNumBones(); ++i)
            {
                const Bone* bone = skeleton.GetBone(i);
                if (!bone->node_)
                {
                    geometry.gatherError_ = Format("bone '{}' of '{}' has no node", bone->name_, mesh.nodeName_);
                    return;
                }
                skinMatrices.push_back(bone->node_->GetWorldTransform() * bone->offsetMatrix_);
            }
        }

        mesh.triangles_.reserve(captured.triangles_.size());
        for (unsigned t = 0; t < captured.triangles_.size(); ++t)
        {
            const Triangle& source = captured.triangles_[t];
            Triangle triangle;
            for (unsigned i = 0; i < 3; ++i)
            {
                const Corner& corner = source.corners_[i];
                Corner& result = triangle.corners_[i];
                Vector3 position = world * corner.position_;
                Vector3 normal = normalMatrix * corner.normal_;
                if (!skinMatrices.empty())
                {
                    const SkinCorner& skin = captured.skin_[t * 3 + i];
                    position = Vector3::ZERO;
                    normal = Vector3::ZERO;
                    for (unsigned j = 0; j < 4; ++j)
                    {
                        if (skin.weights_[j] == 0.0f || skin.indices_[j] >= skinMatrices.size())
                            continue;
                        const Matrix3x4& skinMatrix = skinMatrices[skin.indices_[j]];
                        position += (skinMatrix * corner.position_) * skin.weights_[j];
                        normal += (skinMatrix.ToMatrix3().Inverse().Transpose() * corner.normal_) * skin.weights_[j];
                    }
                }
                result.position_ = conversion.Position(position);
                result.normal_ = corner.normal_ == Vector3::ZERO ? Vector3::ZERO : conversion.Normal(normal.Normalized());
                result.uv_ = corner.uv_;
            }
            mesh.triangles_.push_back(triangle);
        }
        geometry.meshes_.push_back(ea::move(mesh));
    }
}

ComparisonResult CompareMeshes(const DataPair& pair, const ImportedGeometry& imported)
{
    if (IsFrameBaseline(pair))
        return {ComparisonResult::Status::Skipped, "baseline: frame"};
    if (!imported.gatherError_.empty())
        return {ComparisonResult::Status::Mismatch, "gather: " + imported.gatherError_};

    CompareOptions options;
    ea::string value;
    if (HasFlag(pair, "tolerance", &value))
        options.tolerance_ = static_cast<float>(std::atof(value.c_str()));
    if (HasFlag(pair, "position_scale", &value))
        options.positionScale_ = static_cast<float>(std::atof(value.c_str()));
    options.negateXZ_ = HasFlag(pair, "negate_xz");
    options.ignoreNormals_ = HasFlag(pair, "bad_normals") || IgnoresNormalsWithHelperNodes(pair);
    options.ignoreUVs_ = HasFlag(pair, "bad_uvs") || HasFlag(pair, "missing_uvs");
    options.skipDegenerate_ = HasFlag(pair, "bad_faces");

    ObjFile obj;
    ea::string error;
    if (!ParseObj(pair.objPath_, obj, error))
        return {ComparisonResult::Status::Mismatch, "obj: " + error};
    // ufbxt_diff_to_obj compares only the meshes of the OBJ. An OBJ without faces gives no comparison.
    if (obj.polygons_.empty())
        return {ComparisonResult::Status::Skipped, "baseline: OBJ has no faces"};

    // OBJ side: triangulate each polygon as a fan. Keep the polygon index for the n-gon fallback.
    ea::vector<MatchTriangle> objTriangles;
    ea::vector<unsigned> polygonFirstTriangle;
    for (unsigned polygonIndex = 0; polygonIndex < obj.polygons_.size(); ++polygonIndex)
    {
        ObjPolygon& polygon = obj.polygons_[polygonIndex];
        for (Corner& corner : polygon.corners_)
        {
            if (options.negateXZ_)
            {
                corner.position_.x_ = -corner.position_.x_;
                corner.position_.z_ = -corner.position_.z_;
                corner.normal_.x_ = -corner.normal_.x_;
                corner.normal_.z_ = -corner.normal_.z_;
            }
            corner.position_ *= options.positionScale_;
        }
        polygonFirstTriangle.push_back(objTriangles.size());
        for (unsigned i = 1; i + 1 < polygon.corners_.size(); ++i)
        {
            MatchTriangle triangle;
            triangle.corners_[0] = polygon.corners_[0];
            triangle.corners_[1] = polygon.corners_[i];
            triangle.corners_[2] = polygon.corners_[i + 1];
            triangle.useNormals_ = polygon.hasNormals_ && !options.ignoreNormals_;
            triangle.useUVs_ = polygon.hasUVs_ && !options.ignoreUVs_;
            triangle.owner_ = polygonIndex;
            objTriangles.push_back(triangle);
        }
    }

    // Imported side. ufbx does not compare the normals of deformed meshes. Do the same.
    ea::vector<MatchTriangle> importedTriangles;
    for (unsigned meshIndex = 0; meshIndex < imported.meshes_.size(); ++meshIndex)
    {
        const ImportedMesh& mesh = imported.meshes_[meshIndex];
        for (const Triangle& source : mesh.triangles_)
        {
            MatchTriangle triangle;
            for (unsigned i = 0; i < 3; ++i)
            {
                triangle.corners_[i] = source.corners_[i];
                triangle.corners_[i].position_ *= options.positionScale_;
            }
            if (options.skipDegenerate_ && IsDegenerate(triangle.corners_, options.tolerance_))
                continue;
            triangle.useNormals_ = mesh.hasNormals_ && !mesh.deformed_;
            triangle.useUVs_ = mesh.hasUVs_;
            triangle.owner_ = meshIndex;
            importedTriangles.push_back(triangle);
        }
    }

    const float toleranceSquared = options.tolerance_ * options.tolerance_;
    const TriangleGrid grid(importedTriangles, ea::max(options.tolerance_, 1e-6f));
    ea::vector<bool> used(importedTriangles.size(), false);

    // Try to match the fan triangles of a polygon. Release the matched triangles if one fails.
    unsigned numSameWinding = 0;
    auto matchFan = [&](unsigned polygonIndex)
    {
        const unsigned first = polygonFirstTriangle[polygonIndex];
        const unsigned count = obj.polygons_[polygonIndex].corners_.size() - 2;
        ea::vector<unsigned> matched;
        unsigned sameWinding = 0;
        for (unsigned t = first; t < first + count; ++t)
        {
            const MatchTriangle& objTriangle = objTriangles[t];
            int found = -1;
            int winding = -1;
            grid.ForEachNear(objTriangle.corners_[0].position_,
                [&](unsigned index)
                {
                    if (found >= 0 || used[index])
                        return;
                    winding = MatchCorners(objTriangle, importedTriangles[index], toleranceSquared);
                    if (winding >= 0)
                        found = static_cast<int>(index);
                });
            if (found < 0)
            {
                for (unsigned index : matched)
                    used[index] = false;
                return false;
            }
            used[found] = true;
            matched.push_back(found);
            sameWinding += winding == 0 ? 1 : 0;
        }
        numSameWinding += sameWinding;
        return true;
    };

    // Fallback for n-gons: the importer can use other diagonals than the fan.
    // Accept the polygon if n-2 free imported triangles use only corners of the polygon
    // and together use each corner of the polygon. The second rule is necessary:
    // two polygons can share three corners, and then a triangle of one polygon uses only corners of the other.
    auto matchCover = [&](unsigned polygonIndex)
    {
        const ObjPolygon& polygon = obj.polygons_[polygonIndex];
        const MatchTriangle& reference = objTriangles[polygonFirstTriangle[polygonIndex]];
        const unsigned numCorners = polygon.corners_.size();
        const unsigned needed = numCorners - 2;
        // Collect some more candidates than necessary. Overlapped triangles of other polygons can be candidates.
        const unsigned maxCandidates = needed * 2 + 8;
        // A candidate is an imported triangle and a bit mask of the polygon corners that it uses.
        ea::vector<ea::pair<unsigned, uint64_t>> candidates;
        for (const Corner& corner : polygon.corners_)
        {
            grid.ForEachNear(corner.position_,
                [&](unsigned index)
                {
                    if (used[index] || candidates.size() >= maxCandidates
                        || ea::any_of(candidates.begin(), candidates.end(),
                            [&](const auto& candidate) { return candidate.first == index; }))
                        return;
                    const MatchTriangle& candidate = importedTriangles[index];
                    const bool useNormals = reference.useNormals_ && candidate.useNormals_;
                    uint64_t mask = 0;
                    for (const Corner& candidateCorner : candidate.corners_)
                    {
                        bool inPolygon = false;
                        for (unsigned i = 0; i < numCorners; ++i)
                        {
                            if (CornerDistanceSquared(
                                    polygon.corners_[i], candidateCorner, useNormals, reference.useUVs_)
                                < toleranceSquared)
                            {
                                inPolygon = true;
                                if (i < 64)
                                    mask |= uint64_t{1} << i;
                            }
                        }
                        if (!inPolygon)
                            return;
                    }
                    candidates.emplace_back(index, mask);
                });
        }
        if (candidates.size() < needed)
            return false;

        // Find "needed" candidates that use all corners. The search has a step limit.
        const uint64_t allCorners = numCorners >= 64 ? ~uint64_t{0} : (uint64_t{1} << numCorners) - 1;
        ea::vector<unsigned> chosen;
        unsigned numSteps = 0;
        const auto search = [&](const auto& self, unsigned first, uint64_t covered) -> bool
        {
            if (chosen.size() == needed)
                return covered == allCorners;
            if (++numSteps > 100000)
                return false;
            for (unsigned i = first; i + (needed - chosen.size()) <= candidates.size(); ++i)
            {
                chosen.push_back(candidates[i].first);
                if (self(self, i + 1, covered | candidates[i].second))
                    return true;
                chosen.pop_back();
            }
            return false;
        };
        if (!search(search, 0, 0))
            return false;
        for (unsigned index : chosen)
            used[index] = true;
        return true;
    };

    // Match the triangles first, then the n-gons, so that an n-gon cannot take the triangles of a triangle face.
    ea::vector<bool> polygonMatched(obj.polygons_.size(), false);
    ea::vector<bool> polygonSkipped(obj.polygons_.size(), false);
    for (unsigned pass = 0; pass < 2; ++pass)
    {
        for (unsigned polygonIndex = 0; polygonIndex < obj.polygons_.size(); ++polygonIndex)
        {
            const ObjPolygon& polygon = obj.polygons_[polygonIndex];
            if ((polygon.corners_.size() == 3) != (pass == 0))
                continue;
            if (options.skipDegenerate_)
            {
                bool degenerate = false;
                for (unsigned t = 0; t + 2 < polygon.corners_.size() && !degenerate; ++t)
                    degenerate = IsDegenerate(objTriangles[polygonFirstTriangle[polygonIndex] + t].corners_,
                        options.tolerance_);
                if (degenerate)
                {
                    polygonSkipped[polygonIndex] = true;
                    continue;
                }
            }
            polygonMatched[polygonIndex] =
                matchFan(polygonIndex) || (polygon.corners_.size() > 3 && matchCover(polygonIndex));
        }
    }

    // Count the results by OBJ group and by imported mesh.
    const unsigned numGroups = obj.groups_.size();
    ea::vector<unsigned> groupTriangles(numGroups, 0);
    ea::vector<unsigned> groupUnmatched(numGroups, 0);
    unsigned numObjTriangles = 0;
    unsigned numObjUnmatched = 0;
    int firstUnmatchedPolygon = -1;
    for (unsigned polygonIndex = 0; polygonIndex < obj.polygons_.size(); ++polygonIndex)
    {
        if (polygonSkipped[polygonIndex])
            continue;
        const ObjPolygon& polygon = obj.polygons_[polygonIndex];
        const unsigned count = polygon.corners_.size() - 2;
        numObjTriangles += count;
        groupTriangles[polygon.group_] += count;
        if (!polygonMatched[polygonIndex])
        {
            numObjUnmatched += count;
            groupUnmatched[polygon.group_] += count;
            if (firstUnmatchedPolygon < 0)
                firstUnmatchedPolygon = static_cast<int>(polygonIndex);
        }
    }

    ea::vector<unsigned> meshTriangles(imported.meshes_.size(), 0);
    unsigned numImportedUnmatched = 0;
    int firstUnmatchedImported = -1;
    for (unsigned index = 0; index < importedTriangles.size(); ++index)
    {
        ++meshTriangles[importedTriangles[index].owner_];
        if (!used[index])
        {
            ++numImportedUnmatched;
            if (firstUnmatchedImported < 0)
                firstUnmatchedImported = static_cast<int>(index);
        }
    }

    ea::string message = Format("obj-tris={} imp-tris={} unmatched-obj={} unmatched-imp={}", numObjTriangles,
        importedTriangles.size(), numObjUnmatched, numImportedUnmatched);
    // The expected winding in the canonical space is reversed. See GatherImportedMeshes.
    if (numSameWinding > 0)
        message += Format(" unexpected-winding={}", numSameWinding);
    // Per-mesh counts. The comparison uses all meshes together, so this is information only.
    unsigned numCountMismatches = 0;
    ea::string countDetail;
    for (unsigned group = 0; group < numGroups; ++group)
    {
        if (groupTriangles[group] == 0 || obj.groupNames_[group].empty())
            continue;
        const int meshIndex = FindMeshForGroup(obj.groupNames_[group], imported.meshes_);
        if (meshIndex < 0)
        {
            if (numCountMismatches++ == 0)
                countDetail = Format("group '{}' has no mesh", obj.groups_[group]);
            continue;
        }
        if (meshTriangles[meshIndex] != groupTriangles[group])
        {
            if (numCountMismatches++ == 0)
            {
                countDetail = Format("group '{}' obj={} imp={}", obj.groups_[group], groupTriangles[group],
                    meshTriangles[meshIndex]);
            }
        }
    }
    if (numCountMismatches > 0)
        message += Format(" mesh-count-mismatches={} ({})", numCountMismatches, countDetail);

    if (firstUnmatchedPolygon >= 0)
    {
        const ObjPolygon& polygon = obj.polygons_[firstUnmatchedPolygon];
        // Show the first corner without a match. If each corner has a match, show the first corner.
        const Corner* shown = &polygon.corners_[0];
        const MatchTriangle& reference = objTriangles[polygonFirstTriangle[firstUnmatchedPolygon]];
        for (const Corner& corner : polygon.corners_)
        {
            bool found = false;
            for (const MatchTriangle& candidate : importedTriangles)
            {
                for (const Corner& candidateCorner : candidate.corners_)
                {
                    found = found
                        || CornerDistanceSquared(corner, candidateCorner,
                               reference.useNormals_ && candidate.useNormals_, reference.useUVs_)
                            < toleranceSquared;
                }
                if (found)
                    break;
            }
            if (!found)
            {
                shown = &corner;
                break;
            }
        }
        message += Format("; first obj: group '{}' n={} {}; {}", obj.groups_[polygon.group_],
            polygon.corners_.size(), FormatCorner(*shown), DescribeNearest(*shown, importedTriangles));
    }
    else if (firstUnmatchedImported >= 0)
    {
        const MatchTriangle& triangle = importedTriangles[firstUnmatchedImported];
        message += Format("; first imp: mesh '{}' {}", imported.meshes_[triangle.owner_].nodeName_,
            FormatCorner(triangle.corners_[0]));
    }

    const bool matched = numObjUnmatched == 0 && numImportedUnmatched == 0;
    return {matched ? ComparisonResult::Status::Match : ComparisonResult::Status::Mismatch, message};
}

} // namespace UFBXData

} // namespace Urho3D
