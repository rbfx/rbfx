// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

// Test harness that checks FBXImporter against the ufbx test data.
// The data is a separate clone of the ufbx repository. Never commit the data.
// Environment variables:
//   UFBX_DATA_DIR      Path to the "data" directory. The test skips when it is not set or missing.
//   UFBX_DATA_FILTER   Substring. The test uses only the FBX files with this substring in the file name.
//   UFBX_DATA_REPORT   Set to 1 to print one line for each file: name, status and reason.

#pragma once

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Utility/FBXImporter.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

namespace UFBXData
{

/// Flag from a "# ufbx:" comment in the header of an OBJ baseline.
struct BaselineFlag
{
    /// Flag name, for example "bad_normals".
    ea::string name_;
    /// Flag value after "=", for example "0.1" for "position_scale=0.1". Empty if the flag has no value.
    ea::string value_;
};

/// One FBX file with its OBJ baseline.
struct DataPair
{
    /// FBX file name without directory.
    ea::string name_;
    ea::string fbxPath_;
    /// Primary OBJ baseline: "<stem>.obj" or "<stem>_0_obj.obj".
    /// If the data has only frame baselines, this is the first frame baseline.
    ea::string objPath_;
    /// Extra OBJ baselines for animation frames: "<stem>_<frame>.obj".
    ea::vector<ea::string> frameObjPaths_;
    /// Flags from the header of the primary OBJ baseline.
    ea::vector<BaselineFlag> flags_;
};

/// Configuration read from the environment.
struct Config
{
    ea::string dataDir_;
    ea::string filter_;
    bool report_{};
    /// True if UFBX_DATA_DIR is set and the directory exists.
    bool hasData_{};
    /// Message that explains why hasData_ is false.
    ea::string skipMessage_;
};

/// Reason prefixes for known issues. Each reason in the known-issue table starts with one of them.
///   unsupported:       The engine does not support this feature. The test does not load the file.
///   flag:              A "# ufbx:" flag makes the baseline not comparable. The test loads the file only.
///   baseline:          The baseline is wrong or not usable. The test loads the file only.
///   ufbx-expect-fail:  The file fails in ufbx too. The test accepts a load failure.
enum class IssueKind
{
    None,
    Unsupported,
    Flag,
    Baseline,
    UfbxExpectFail,
};

struct KnownIssue
{
    /// FBX file name or "*" suffix pattern, for example "*_fuzz.fbx".
    const char* pattern_;
    /// Reason with a prefix from IssueKind.
    const char* reason_;
};

/// Result of the skip policy for one file.
struct SkipDecision
{
    IssueKind kind_{IssueKind::None};
    /// Reason with prefix. Empty if kind_ is None.
    ea::string reason_;
};

/// Result of the mesh comparison.
struct ComparisonResult
{
    enum class Status
    {
        NotImplemented,
        Match,
        Mismatch,
        /// The baseline is not comparable. message_ holds the reason with a prefix from IssueKind.
        Skipped,
    };

    Status status_{Status::NotImplemented};
    ea::string message_;
};

/// One triangle corner.
struct Corner
{
    Vector3 position_;
    /// Zero if the source has no normal.
    Vector3 normal_;
    Vector2 uv_;
};

struct Triangle
{
    Corner corners_[3];
};

/// Bone influences of one triangle corner. Index into ModelView::GetBones().
struct SkinCorner
{
    unsigned indices_[4]{};
    float weights_[4]{};
};

/// Triangles of one model resource in the model space of the engine. The callback captures them.
/// The positions include the morphs with their initial weights.
struct CapturedModel
{
    ea::vector<Triangle> triangles_;
    /// Three items for each triangle. Empty if the model has no bones.
    ea::vector<SkinCorner> skin_;
    /// True if the model has bones or morphs. The comparison then ignores normals, as ufbx does.
    bool deformed_{};
    bool hasNormals_{};
    bool hasUVs_{};
};

/// Triangles of one model component in the canonical comparison space. See GatherImportedMeshes.
struct ImportedMesh
{
    /// Name of the node with the model component.
    ea::string nodeName_;
    /// Names of the node and of all its parents.
    ea::vector<ea::string> nodePath_;
    bool deformed_{};
    bool hasNormals_{};
    bool hasUVs_{};
    ea::vector<Triangle> triangles_;
};

/// Geometry data that the importer captures for one file.
struct ImportedGeometry
{
    unsigned numModels_{};
    unsigned numVertices_{};
    unsigned numIndices_{};
    unsigned numBones_{};
    unsigned numAnimations_{};
    /// Meshes of the instantiated prefab in the canonical space.
    ea::vector<ImportedMesh> meshes_;
    /// Error from GatherImportedMeshes. Empty if the meshes are valid.
    ea::string gatherError_;
};

/// Importer callback that fills ImportedGeometry.
class CaptureCallback : public FBXImporterCallback
{
public:
    ImportedGeometry geometry_;
    /// Captured models by resource name.
    ea::unordered_map<ea::string, CapturedModel> models_;

    void OnModelLoaded(ModelView& modelView) override;
    void OnAnimationLoaded(Animation& animation) override;
};

/// Instantiate the imported prefab and fill geometry.meshes_ with world-space triangles in the canonical space.
/// Call this function while the importer exists, because the importer owns the resources.
///
/// The canonical space is the native space of the FBX file: the axes and the unit of the file,
/// without a conversion. The ufbx tests compare the OBJ baselines in this space (ufbxt_diff_to_obj
/// uses default load options). The importer maps native space to engine space in two steps:
///   1. ufbx converts to right-handed Y-up meters: p1 = R * (s * p), where R and s are
///      ufbx_metadata.root_rotation and root_scale.
///   2. The importer negates X to get the left-handed engine space: p2 = (-p1.x, p1.y, p1.z).
/// This function does the inverse: p = R^-1 * (-p2.x, p2.y, p2.z) / s. Normals use R^-1 only.
/// UVs need no conversion: the importer and the OBJ baseline both use the FBX UV convention.
/// The function reads R and s from a separate ufbx load with the options of the importer.
/// The triangle winding in the canonical space is the reverse of the OBJ winding. This is correct:
/// the engine uses clockwise front faces, and the mirror of X reverses the winding again.
/// For an AnimatedModel, the function skins the vertices on the CPU with the bone nodes of the prefab.
/// The ufbx tests do the same: they compare the skinned positions in the default pose (evaluate_skinning).
void GatherImportedMeshes(
    Context* context, const ea::string& fbxPath, const CaptureCallback& callback, ImportedGeometry& geometry);

/// Read the configuration from the environment.
Config ReadConfig();

/// Find the FBX files in the top level of dataDir that have an OBJ baseline.
/// Apply the filter (substring of the file name) if it is not empty. The result is sorted by file name.
ea::vector<DataPair> EnumeratePairs(const ea::string& dataDir, const ea::string& filter);

/// Parse "# ufbx:" flags from the leading comment lines of an OBJ text.
ea::vector<BaselineFlag> ParseBaselineFlags(const ea::string& objHeader);

/// Read the "# ufbx:" flags from an OBJ file. Return an empty vector if the file is not readable.
ea::vector<BaselineFlag> ReadBaselineFlags(const ea::string& objPath);

/// Return the known issue and the flag policy decision for a pair. Known issues have priority.
SkipDecision GetSkipDecision(const DataPair& pair);

/// Return the kind that matches the prefix of the reason.
IssueKind GetIssueKind(const ea::string& reason);

/// Return true if ufbx ignores the normals of the file in the diff with the helper node handling.
/// The ufbx tests mark these files with UFBXT_FILE_TEST_FLAG_OPT_HANDLING_IGNORE_NORMALS_IN_DIFF.
/// The importer uses the helper node handling for geometry transforms.
bool IgnoresNormalsWithHelperNodes(const DataPair& pair);

/// Compare the imported geometry with the OBJ baseline in the canonical space.
/// The function matches triangles greedily with the tolerance of the baseline (default 0.001).
/// It compares all meshes together. It reports per-group counts when the OBJ has "o" or "g" groups.
ComparisonResult CompareMeshes(const DataPair& pair, const ImportedGeometry& imported);

/// Return the status name for the report.
const char* ToString(ComparisonResult::Status status);

} // namespace UFBXData

} // namespace Urho3D
