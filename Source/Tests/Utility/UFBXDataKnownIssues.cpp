// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "UFBXDataHarness.h"

#include <cstring>

namespace Urho3D
{

namespace UFBXData
{

namespace
{

// Known issues for single files. The pattern is a file name. A pattern that starts with "*" matches a suffix.
// Each reason starts with one prefix: "unsupported:", "flag:", "baseline:" or "ufbx-expect-fail:".
// Add an entry here when a file fails for a known reason. Do not hide an unknown failure.
const KnownIssue knownIssues[] = {
    // Example: {"some_file_7400_binary.fbx", "baseline: Blender OBJ export drops the second UV set"},

    // ufbxt_diff_to_obj subdivides meshes with the smooth display mode (testing_utils.h).
    // The importer does not subdivide. It imports the cage mesh.
    {"blender_293_ngon_subsurf_7400_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"blender_293_suzanne_subsurf_7400_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"blender_293_suzanne_subsurf_uv_7400_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"blender_293x_subsurf_boundary_7400_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"blender_312x_vertex_crease_7400_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_3x_cube_7500_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_3x_cube_crease_7500_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_cube_7500_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_cube_crease_7500_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_plane_7700_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_subsurf_plane_7700_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_vertex_crease_6100_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_vertex_crease_6100_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_vertex_crease_7700_ascii.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_vertex_crease_7700_binary.fbx", "unsupported: importer imports the cage mesh"},
    {"maya_vertex_crease_single_7700_ascii.fbx", "unsupported: importer imports the cage mesh"},

    // The skin uses dual-quaternion skinning (ufbx_skin_deformer::skinning_method). The engine skins linear.
    {"maya_dual_quaternion_7500_ascii.fbx", "baseline: baseline uses dual-quaternion skinning, the engine skins linear"},
    {"maya_dual_quaternion_scale_7500_ascii.fbx",
        "baseline: baseline uses dual-quaternion skinning, the engine skins linear"},
    {"synthetic_bind_to_root_7700_ascii.fbx", "baseline: baseline uses dual-quaternion skinning, the engine skins linear"},

    // The pair uses "<name>_0_obj.obj", an input of the OBJ loader tests. ufbx diffs only with "<name>.obj".
    {"blender_279_ball_6100_ascii.fbx", "baseline: OBJ loader input, ufbx does not diff this FBX file"},
    {"blender_279_ball_7400_binary.fbx", "baseline: OBJ loader input, ufbx does not diff this FBX file"},
    {"zbrush_polygroup_mess_7500_binary.fbx", "baseline: OBJ loader input, ufbx does not diff this FBX file"},

    // ufbx diffs these files only with other load options. The default load is DIFF_EXPECT_FAIL.
    {"blender_340_y_up_7400_binary.fbx", "baseline: ufbx diffs the baseline in Z-up meters (ufbxt_z_up_meters_*_opts)"},
    {"blender_340_z_up_7400_binary.fbx", "baseline: ufbx diffs the baseline in Y-up meters (ufbxt_y_up_meters_*_opts)"},
    {"revit_wall_square_7700_binary.fbx", "baseline: ufbx diffs the baseline in Y-up centimeters (ufbxt_cm_y_up_opts)"},
    {nullptr, nullptr},
};

// Files with UFBXT_FILE_TEST_FLAG_OPT_HANDLING_IGNORE_NORMALS_IN_DIFF in the ufbx tests (test_transform.h).
const char* const ignoreNormalsWithHelperNodes[] = {
    "synthetic_geometric_squish_7500_ascii.fbx",
    "synthetic_geometric_transform_7500_ascii.fbx",
    nullptr,
};

// Policy for the "# ufbx:" flags. A file with the flag skips the geometry comparison.
// Flags that the comparison handles (for example "position_scale") do not belong here.
struct FlagPolicy
{
    const char* flag_;
    const char* reason_;
};

const FlagPolicy flagPolicies[] = {
    {"line_faces", "flag: baseline stores line elements as faces"},
    {nullptr, nullptr},
};

bool MatchesPattern(const ea::string& name, const char* pattern)
{
    if (pattern[0] == '*')
    {
        const size_t length = std::strlen(pattern + 1);
        return name.size() >= length && std::strcmp(name.c_str() + name.size() - length, pattern + 1) == 0;
    }
    return name == pattern;
}

} // namespace

IssueKind GetIssueKind(const ea::string& reason)
{
    if (reason.starts_with("unsupported:"))
        return IssueKind::Unsupported;
    if (reason.starts_with("flag:"))
        return IssueKind::Flag;
    if (reason.starts_with("baseline:"))
        return IssueKind::Baseline;
    if (reason.starts_with("ufbx-expect-fail:"))
        return IssueKind::UfbxExpectFail;
    return IssueKind::None;
}

bool IgnoresNormalsWithHelperNodes(const DataPair& pair)
{
    for (const char* const* pattern = ignoreNormalsWithHelperNodes; *pattern; ++pattern)
    {
        if (MatchesPattern(pair.name_, *pattern))
            return true;
    }
    return false;
}

SkipDecision GetSkipDecision(const DataPair& pair)
{
    for (const KnownIssue* issue = knownIssues; issue->pattern_; ++issue)
    {
        if (MatchesPattern(pair.name_, issue->pattern_))
            return {GetIssueKind(issue->reason_), issue->reason_};
    }

    for (const FlagPolicy* policy = flagPolicies; policy->flag_; ++policy)
    {
        for (const BaselineFlag& flag : pair.flags_)
        {
            if (flag.name_ == policy->flag_)
                return {GetIssueKind(policy->reason_), policy->reason_};
        }
    }

    return {};
}

} // namespace UFBXData

} // namespace Urho3D
