// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/FBXImporter.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Exception.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/ModelView.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/Image.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/PrefabResource.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Utility/AnimationMetadata.h"
#include "Urho3D/Utility/ModelImporterUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <ufbx.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

using namespace ModelImporterUtils;

std::string ToString(const ufbx_string& value)
{
    return value.data ? std::string{value.data, value.length} : std::string{};
}

Vector2 ToVector2(const ufbx_vec2& value)
{
    return {static_cast<float>(value.x), static_cast<float>(value.y)};
}

Vector3 ToVector3(const ufbx_vec3& value)
{
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}

Vector4 ToVector4(const ufbx_vec4& value)
{
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z),
        static_cast<float>(value.w)};
}

Quaternion ToQuaternion(const ufbx_quat& value)
{
    return {static_cast<float>(value.w), static_cast<float>(value.x), static_cast<float>(value.y),
        static_cast<float>(value.z)};
}

Matrix3x4 ToMatrix3x4(const ufbx_matrix& value)
{
    return {static_cast<float>(value.m00), static_cast<float>(value.m01), static_cast<float>(value.m02),
        static_cast<float>(value.m03), static_cast<float>(value.m10), static_cast<float>(value.m11),
        static_cast<float>(value.m12), static_cast<float>(value.m13), static_cast<float>(value.m20),
        static_cast<float>(value.m21), static_cast<float>(value.m22), static_cast<float>(value.m23)};
}

/// Maximum number of segments for each NURBS surface span.
const uint32_t MaxNurbsSpanSubdivision = 16;

/// Replace the non-finite components of the vector with zero. Set `replaced` if a component changes.
template <class T> T ReplaceNonFinite(T value, bool& replaced)
{
    float* data = &value.x_;
    for (unsigned i = 0; i < sizeof(T) / sizeof(float); ++i)
    {
        if (!std::isfinite(data[i]))
        {
            data[i] = 0.0f;
            replaced = true;
        }
    }
    return value;
}

/// Return true if ufbx can evaluate the NURBS surface without an out-of-range access.
/// ufbx_evaluate_nurbs_surface() reads `control_points[v * num_control_points_u + u]` with no bounds check.
bool IsValidNurbsSurface(const ufbx_nurbs_surface& surface)
{
    const size_t numU = surface.num_control_points_u;
    const size_t numV = surface.num_control_points_v;
    if (!surface.basis_u.valid || !surface.basis_v.valid || numU == 0 || numV == 0)
        return false;
    // The product must not overflow, and the control point array must hold the full grid.
    if (numU > UINT32_MAX / numV)
        return false;
    return surface.control_points.data && surface.control_points.count == numU * numV;
}

ea::string FormatUFBXError(const ufbx_error& error)
{
    char buffer[2048]{};
    ufbx_format_error(buffer, sizeof(buffer), &error);
    return buffer;
}

/// Minimum memory limit of ufbx for one file load, for each of the temporary and the result allocators.
const size_t MinLoadMemoryLimit = size_t{1} << 30;
/// Memory limit of ufbx for each byte of the source file.
const size_t LoadMemoryLimitPerSourceByte = 64;

/// Return the load options. `sourceSize` is the size of the source file in bytes, or zero if it is not known.
ufbx_load_opts GetLoadOptions(size_t sourceSize)
{
    ufbx_load_opts options{};
    // A corrupt file can claim a huge array size (for example a DEFLATE stream of many gigabytes).
    // The limit makes ufbx fail with UFBX_ERROR_MEMORY_LIMIT before the process runs out of memory.
    const size_t memoryLimit = sourceSize <= SIZE_MAX / LoadMemoryLimitPerSourceByte
        ? ea::max(MinLoadMemoryLimit, sourceSize * LoadMemoryLimitPerSourceByte)
        : SIZE_MAX;
    options.temp_allocator.memory_limit = memoryLimit;
    options.result_allocator.memory_limit = memoryLimit;
    options.generate_missing_normals = true;
    options.clean_skin_weights = true;
    options.use_blender_pbr_material = true;
    options.ignore_missing_external_files = true;
    options.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
    options.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
    options.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
    options.target_axes = ufbx_axes_right_handed_y_up;
    options.target_unit_meters = 1.0;
    options.node_depth_limit = 1024;
    return options;
}

/// Return the size of the file in bytes, or zero on error.
size_t GetSourceFileSize(const ea::string& fileName)
{
    std::error_code error;
    const auto size = std::filesystem::file_size(std::filesystem::u8path(fileName.c_str()), error);
    return error ? 0 : static_cast<size_t>(ea::min<uintmax_t>(size, SIZE_MAX));
}

struct SceneDeleter
{
    void operator()(ufbx_scene* scene) const { ufbx_free_scene(scene); }
};

struct BakedAnimationDeleter
{
    void operator()(ufbx_baked_anim* animation) const { ufbx_free_baked_anim(animation); }
};

struct LineCurveDeleter
{
    void operator()(ufbx_line_curve* curve) const { ufbx_free_line_curve(curve); }
};

struct MeshDeleter
{
    void operator()(ufbx_mesh* mesh) const { ufbx_free_mesh(mesh); }
};

using ScenePtr = std::unique_ptr<ufbx_scene, SceneDeleter>;
using BakedAnimationPtr = std::unique_ptr<ufbx_baked_anim, BakedAnimationDeleter>;
using LineCurvePtr = std::unique_ptr<ufbx_line_curve, LineCurveDeleter>;
using MeshPtr = std::unique_ptr<ufbx_mesh, MeshDeleter>;

struct FBXSource
{
    ScenePtr scene_;
    ea::string sourceDirectory_;
    ea::string animationNameOverride_;
};

struct DecodedImage
{
    /// The image in the RGBA format.
    SharedPtr<Image> image_;
    bool twoChannel_{};
    std::string name_;
};

struct MorphTarget
{
    const ufbx_blend_channel* channel_{};
    unsigned keyframeIndex_{};
    const ufbx_blend_shape* shape_{};
};

struct SkinDeformerInfo
{
    const ufbx_skin_deformer* skin_{};
    std::vector<int> clusterToBone_;
};

struct SkinInfo
{
    const ufbx_node* root_{};
    std::vector<SkinDeformerInfo> deformers_;
    std::vector<const ufbx_node*> boneNodes_;
    ea::vector<BoneView> bones_;
    /// Bone of the mesh node. A vertex with no weights uses it. -1 if each vertex has weights.
    int unweightedBone_{-1};

    bool IsSkinned() const { return root_ && !bones_.empty(); }
};

struct ImportedTexture
{
    SharedPtr<Image> image_;
    SharedPtr<Texture2D> texture_;
    SharedPtr<XMLFile> sampler_;
};

struct ImportedMaterial
{
    SharedPtr<Material> material_;
    StandardMaterialTechniques techniques_;
};

/// The distance between LOD levels whose names carry level indices, in bounding sphere radii of the first level.
/// With a 45 degree field of view, the sphere fills about one half of the view height at 5 radii.
/// Thus level 1 starts at 5 radii, level 2 at 10 radii, and so on.
static constexpr float LOD_INDEX_STEP = 5.0f;

struct ImportedModel
{
    const ufbx_node* sourceNode_{};
    ea::string meshName_;
    ea::string baseMeshName_;
    ea::optional<float> lodDistance_;
    ea::optional<unsigned> lodOrder_;
    /// The number from a name like "Mesh_LOD2" when the number is an integer. It can be a level index.
    ea::optional<unsigned> lodNameNumber_;
    bool absoluteLODDistance_{};
    SkinInfo skin_;
    std::vector<MorphTarget> morphTargets_;
    ea::vector<float> morphWeights_;
    SharedPtr<ModelView> modelView_;
    SharedPtr<Model> model_;
    ea::vector<SharedPtr<Material>> materials_;
    ea::string componentPath_;
    unsigned componentIndex_{};
    bool processedAsLOD_{};
    bool skipComponent_{};
};

class FBXProcessor : public NonCopyable
{
public:
    FBXProcessor(Context* context, const FBXImporterSettings& settings, std::vector<FBXSource>& sources,
        const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback,
        Material* defaultMaterial)
        : context_(context)
        , defaultMaterial_(defaultMaterial)
        , settings_(settings)
        , sources_(sources)
        , callback_(callback)
        , transform_{!settings.mirrorX_, settings.scale_}
        , resources_(context, outputPath, resourceNamePrefix)
    {
        if (sources_.empty() || !sources_[0].scene_)
            throw RuntimeException("Source FBX model is not loaded");
    }

    void Prepare()
    {
        if (prepared_)
            throw RuntimeException("FBX resources are already prepared");
        InitializeNodeNames();
        ImportModels();
        decodedTextures_.clear();
        InitializeComponentBindings();
        ImportAnimations();
        prepared_ = true;
    }

    void Finalize()
    {
        if (!prepared_)
            throw RuntimeException("FBX resources aren't prepared");
        if (finalized_)
            throw RuntimeException("FBX resources are already finalized");

        if (settings_.gpuResources_)
        {
            for (const ImportedTexture& texture : textures_)
            {
                if (texture.texture_ && texture.image_)
                    texture.texture_->SetData(texture.image_);
            }
        }
        FinalizeMaterials();
        CommitResources();
        CookModels();
        for (const SharedPtr<Animation>& animation : animations_)
        {
            callback_->OnAnimationLoaded(*animation);
            AddToResourceCache(animation);
        }
        CommitResources();
        CreatePrefab();
        CommitResources();
        finalized_ = true;
    }

    void SaveResources()
    {
        for (const ImportedTexture& texture : textures_)
        {
            SaveResource(texture.image_);
            if (texture.sampler_)
                texture.sampler_->SaveFile(texture.sampler_->GetAbsoluteFileName());
        }
        for (const auto& [_, variants] : materials_)
        {
            for (const SharedPtr<Material>& material : variants)
            {
                if (material)
                    SaveResource(material);
            }
        }
        for (const SharedPtr<Model>& model : modelsToSave_)
            SaveResource(model);
        for (const SharedPtr<Animation>& animation : animations_)
            SaveResource(animation);
        SaveResource(prefab_);
    }

    const FBXImporter::ResourceToFileNameMap& GetResourceNames() const { return resources_.GetResourceNames(); }
    const StringVector& GetMissingTextures() const { return missingTextures_; }

private:
    ea::string CreateResourceName(
        const ea::string& nameHint, const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        return resources_.CreateResourceName(nameHint, prefix, defaultName, suffix);
    }

    const ea::string& GetAbsoluteFileName(const ea::string& resourceName) const
    {
        return resources_.GetAbsoluteFileName(resourceName);
    }

    void AddToResourceCache(Resource* resource) { resources_.AddResource(resource); }
    void CommitResources() { resources_.CommitResources(); }
    void SaveResource(Resource* resource) const { resources_.SaveResource(resource); }

    void FinalizeMaterials()
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        for (const ImportedMaterial& imported : importedMaterials_)
            SetStandardMaterialTechniques(*imported.material_, imported.techniques_, cache);
    }

    ea::string GetConfiguredNodeName(const ufbx_node& node) const
    {
        ea::string name = ToString(node.name).c_str();
        if (const auto iter = settings_.nodeRenames_.find(name); iter != settings_.nodeRenames_.end())
            name = iter->second;
        return name.empty() ? Format("Node_{}", node.typed_id) : name;
    }

    void InitializeNodeNames()
    {
        ea::unordered_set<ea::string> usedNames;
        const ufbx_scene& scene = *sources_[0].scene_;
        for (const ufbx_node* node : scene.nodes)
        {
            const ea::string baseName = GetConfiguredNodeName(*node);
            const ea::string uniqueName =
                GetUniqueName(baseName, [&](const ea::string& candidate) { return usedNames.contains(candidate); });
            usedNames.insert(uniqueName);
            nodeNames_[node] = uniqueName;
            primaryNodesByConfiguredName_.emplace(baseName.c_str(), node);
        }
    }

    Vector3 ConvertPosition(const ufbx_vec3& value) const { return transform_.ApplyToPosition(ToVector3(value)); }

    Quaternion ConvertRotation(const ufbx_quat& value) const { return transform_.ApplyToRotation(ToQuaternion(value)); }

    Matrix3x4 ConvertOffsetMatrix(const ufbx_matrix& value) const
    {
        return transform_.ApplyToOffsetMatrix(ToMatrix3x4(value));
    }

    static const ufbx_node* FindCommonAncestor(const ufbx_node* lhs, const ufbx_node* rhs)
    {
        for (const ufbx_node* candidate = lhs; candidate; candidate = candidate->parent)
        {
            for (const ufbx_node* node = rhs; node; node = node->parent)
            {
                if (node == candidate)
                    return candidate;
            }
        }
        return nullptr;
    }

    void ImportModels()
    {
        const ufbx_scene& scene = *sources_[0].scene_;
        models_.reserve(scene.nodes.count);
        bool hasSourceSkins = false;
        for (const ufbx_node* node : scene.nodes)
        {
            if (!node->mesh && !ufbx_as_line_curve(node->attrib) && !ufbx_as_nurbs_curve(node->attrib)
                && !ufbx_as_nurbs_surface(node->attrib))
                continue;

            const unsigned index = models_.size();
            modelIndexByNode_[node] = index;
            ImportedModel& importedModel = models_.emplace_back();
            importedModel.sourceNode_ = node;
            InitializeLOD(importedModel);
            importedModel.skin_ = ImportSkin(*node);
            hasSourceSkins |= importedModel.skin_.IsSkinned();
            if (node->mesh)
                ImportMorphTargets(importedModel);
        }

        if (!hasSourceSkins)
            ImportArtificialSkins();
        CleanupBoneNames();
        InitializeAnimationGroups();

        for (ImportedModel& importedModel : models_)
        {
            RefreshBoneNames(importedModel.skin_);
            importedModel.modelView_ = ImportModelView(importedModel);
        }
        ReuseEquivalentModels();
        if (settings_.combineLODs_)
            CombineLODs();
        AssignModelNames();
    }

    void AssignModelNames()
    {
        std::unordered_set<ModelView*> namedViews;
        for (ImportedModel& model : models_)
        {
            if (!namedViews.emplace(model.modelView_.Get()).second)
                continue;
            model.modelView_->SetName(CreateResourceName(model.baseMeshName_, "Models/", "Model", ".mdl"));
        }
    }

    void InitializeAnimationGroups()
    {
        for (const ImportedModel& model : models_)
        {
            if (!model.skin_.IsSkinned())
                continue;
            auto [iter, inserted] =
                skeletonBoneNodes_.emplace(model.skin_.root_, std::unordered_set<const ufbx_node*>{});
            if (inserted)
                skeletonRoots_.push_back(model.skin_.root_);
            iter->second.insert(model.skin_.boneNodes_.begin(), model.skin_.boneNodes_.end());
        }
    }

    static bool IsRecursivelyEmpty(const ufbx_node& node)
    {
        return !node.mesh && !ufbx_as_line_curve(node.attrib) && !ufbx_as_nurbs_curve(node.attrib)
            && !ufbx_as_nurbs_surface(node.attrib)
            && ea::all_of(node.children.begin(), node.children.end(),
                [](const ufbx_node* child) { return IsRecursivelyEmpty(*child); });
    }

    static void IncludeSubtree(std::unordered_set<const ufbx_node*>& included, const ufbx_node& node)
    {
        included.emplace(&node);
        for (const ufbx_node* child : node.children)
            IncludeSubtree(included, *child);
    }

    static const ufbx_node* GetSkinRoot(const ufbx_mesh& mesh)
    {
        const ufbx_node* root = nullptr;
        for (const ufbx_skin_deformer* skin : mesh.skin_deformers)
        {
            for (const ufbx_skin_cluster* cluster : skin->clusters)
            {
                if (cluster->bone_node)
                    root = root ? FindCommonAncestor(root, cluster->bone_node) : cluster->bone_node;
            }
        }
        return root;
    }

    static bool HasPositiveWeights(const ufbx_skin_cluster& cluster)
    {
        return ea::any_of(
            cluster.weights.begin(), cluster.weights.end(), [](ufbx_real weight) { return weight > 0.0; });
    }

    /// Return true if the mesh has a vertex with no positive weight on a bone.
    bool HasUnweightedVertices(const ufbx_mesh& mesh)
    {
        const auto [iter, inserted] = hasUnweightedVertices_.try_emplace(&mesh, false);
        if (!inserted)
            return iter->second;

        for (size_t vertex = 0; vertex < mesh.num_vertices && !iter->second; ++vertex)
        {
            bool weighted = false;
            for (const ufbx_skin_deformer* skin : mesh.skin_deformers)
            {
                if (vertex >= skin->vertices.count)
                    continue;
                const ufbx_skin_vertex& skinVertex = skin->vertices.data[vertex];
                for (size_t i = 0; i < skinVertex.num_weights && !weighted; ++i)
                {
                    const ufbx_skin_weight& weight = skin->weights.data[skinVertex.weight_begin + i];
                    weighted = weight.weight > 0.0 && weight.cluster_index < skin->clusters.count
                        && skin->clusters.data[weight.cluster_index]->bone_node;
                }
            }
            iter->second = !weighted;
        }
        return iter->second;
    }

    /// Return the skeleton root of the skinned mesh node.
    /// ufbx skins a vertex with no weights with the geometry-to-world matrix of the mesh node.
    /// To do the same, the skeleton then contains the mesh node as a bone.
    const ufbx_node* GetSkinRoot(const ufbx_node& meshNode)
    {
        if (!meshNode.mesh)
            return nullptr;
        const ufbx_node* root = GetSkinRoot(*meshNode.mesh);
        if (root && HasUnweightedVertices(*meshNode.mesh))
            root = FindCommonAncestor(root, &meshNode);
        return root;
    }

    std::vector<const ufbx_node*> CollectSkinBoneNodes(const ufbx_node& root, const ufbx_node* sourceMeshNode)
    {
        std::unordered_set<const ufbx_node*> included{&root};
        const auto includePath = [&](const ufbx_node* node)
        {
            for (const ufbx_node* current = node; current && current != &root; current = current->parent)
                included.emplace(current);
        };
        const auto includeMesh = [&](const ufbx_node& meshNode)
        {
            const ufbx_mesh& mesh = *meshNode.mesh;
            for (const ufbx_skin_deformer* skin : mesh.skin_deformers)
            {
                for (const ufbx_skin_cluster* cluster : skin->clusters)
                {
                    if (cluster->bone_node && HasPositiveWeights(*cluster))
                        includePath(cluster->bone_node);
                }
            }
            if (HasUnweightedVertices(mesh))
                includePath(&meshNode);
        };

        if (sourceMeshNode)
            includeMesh(*sourceMeshNode);
        else
        {
            for (const ufbx_node* node : sources_[0].scene_->nodes)
            {
                if (node->mesh && GetSkinRoot(*node) == &root)
                    includeMesh(*node);
            }
        }
        if (settings_.addEmptyNodesToSkeleton_)
        {
            for (const ufbx_node* node : sources_[0].scene_->nodes)
            {
                if (included.find(node) == included.end())
                    continue;
                for (const ufbx_node* child : node->children)
                {
                    if (included.find(child) == included.end() && IsRecursivelyEmpty(*child))
                        IncludeSubtree(included, *child);
                }
            }
        }

        std::vector<const ufbx_node*> result;
        const auto enumerate = [&](const auto& self, const ufbx_node* node) -> void
        {
            if (included.find(node) != included.end())
                result.push_back(node);
            for (const ufbx_node* child : node->children)
                self(self, child);
        };
        enumerate(enumerate, &root);
        return result;
    }

    const std::vector<const ufbx_node*>& GetSkinBoneNodes(const ufbx_node& root)
    {
        auto [resultIter, inserted] = skinBoneNodesByRoot_.try_emplace(&root);
        if (inserted)
            resultIter->second = CollectSkinBoneNodes(root, nullptr);
        return resultIter->second;
    }

    SkinInfo ImportSkin(const ufbx_node& meshNode)
    {
        SkinInfo result;
        if (!meshNode.mesh || meshNode.mesh->skin_deformers.count == 0)
            return result;

        const ufbx_node* root = GetSkinRoot(meshNode);
        if (!root)
            return result;
        result.root_ = root;
        result.boneNodes_ = CollectSkinBoneNodes(*root, &meshNode);

        std::unordered_map<const ufbx_node*, unsigned> nodeToBone;
        for (unsigned i = 0; i < result.boneNodes_.size(); ++i)
            nodeToBone[result.boneNodes_[i]] = i;
        if (HasUnweightedVertices(*meshNode.mesh))
            result.unweightedBone_ = static_cast<int>(nodeToBone.at(&meshNode));

        std::unordered_map<const ufbx_node*, const ufbx_skin_cluster*> nodeToCluster;
        for (const ufbx_skin_deformer* skin : meshNode.mesh->skin_deformers)
        {
            SkinDeformerInfo& deformer = result.deformers_.emplace_back();
            deformer.skin_ = skin;
            deformer.clusterToBone_.resize(skin->clusters.count, -1);
            for (unsigned i = 0; i < skin->clusters.count; ++i)
            {
                const ufbx_skin_cluster* cluster = skin->clusters.data[i];
                if (!cluster->bone_node)
                    continue;
                const auto iter = nodeToBone.find(cluster->bone_node);
                if (iter != nodeToBone.end())
                {
                    deformer.clusterToBone_[i] = static_cast<int>(iter->second);
                    nodeToCluster.emplace(cluster->bone_node, cluster);
                }
            }
        }

        result.bones_.resize(result.boneNodes_.size());
        for (unsigned i = 0; i < result.boneNodes_.size(); ++i)
        {
            const ufbx_node& sourceBone = *result.boneNodes_[i];
            BoneView& bone = result.bones_[i];
            bone.name_ = nodeNames_.at(&sourceBone);
            if (&sourceBone != root)
            {
                const ufbx_node* parent = sourceBone.parent;
                while (parent && nodeToBone.find(parent) == nodeToBone.end())
                    parent = parent->parent;
                if (parent)
                    bone.parentIndex_ = nodeToBone.at(parent);
            }
            bone.SetInitialTransform(ConvertPosition(sourceBone.local_transform.translation),
                ConvertRotation(sourceBone.local_transform.rotation), ToVector3(sourceBone.local_transform.scale));

            if (const auto iter = nodeToCluster.find(&sourceBone); iter != nodeToCluster.end())
                bone.offsetMatrix_ = ConvertOffsetMatrix(iter->second->geometry_to_bone);
            else
            {
                const ufbx_matrix worldToBone = ufbx_matrix_invert(&sourceBone.node_to_world);
                const ufbx_matrix geometryToBone = ufbx_matrix_mul(&worldToBone, &meshNode.geometry_to_world);
                bone.offsetMatrix_ = ConvertOffsetMatrix(geometryToBone);
            }
            primaryBoneNodes_.emplace(&sourceBone);
        }
        return result;
    }

    SkinInfo ImportArtificialSkin(const ufbx_node& root)
    {
        SkinInfo result;
        result.root_ = &root;
        const auto enumerate = [&](const auto& self, const ufbx_node& node) -> void
        {
            result.boneNodes_.push_back(&node);
            for (const ufbx_node* child : node.children)
                self(self, *child);
        };
        enumerate(enumerate, root);

        std::unordered_map<const ufbx_node*, unsigned> nodeToBone;
        for (unsigned i = 0; i < result.boneNodes_.size(); ++i)
            nodeToBone[result.boneNodes_[i]] = i;

        result.bones_.resize(result.boneNodes_.size());
        for (unsigned i = 0; i < result.boneNodes_.size(); ++i)
        {
            const ufbx_node& sourceBone = *result.boneNodes_[i];
            BoneView& bone = result.bones_[i];
            bone.name_ = nodeNames_.at(&sourceBone);
            if (&sourceBone != &root)
                bone.parentIndex_ = nodeToBone.at(sourceBone.parent);
            bone.SetInitialTransform(ConvertPosition(sourceBone.local_transform.translation),
                ConvertRotation(sourceBone.local_transform.rotation), ToVector3(sourceBone.local_transform.scale));
            const ufbx_matrix worldToBone = ufbx_matrix_invert(&sourceBone.node_to_world);
            const ufbx_matrix geometryToBone = ufbx_matrix_mul(&worldToBone, &root.geometry_to_world);
            bone.offsetMatrix_ = ConvertOffsetMatrix(geometryToBone);
            primaryBoneNodes_.emplace(&sourceBone);
        }
        return result;
    }

    void ImportArtificialSkins()
    {
        const ea::vector<ea::string> names = callback_->GetArtificialSkinNodes();
        if (names.empty())
            return;

        const ea::unordered_set<ea::string> requestedNames{names.begin(), names.end()};
        for (const ufbx_node* node : sources_[0].scene_->nodes)
        {
            if (!requestedNames.contains(GetConfiguredNodeName(*node)))
                continue;

            if (const auto iter = modelIndexByNode_.find(node); iter != modelIndexByNode_.end())
            {
                ImportedModel& model = models_[iter->second];
                if (!model.skin_.IsSkinned())
                    model.skin_ = ImportArtificialSkin(*node);
            }
            else
            {
                modelIndexByNode_[node] = models_.size();
                ImportedModel& model = models_.emplace_back();
                model.sourceNode_ = node;
                InitializeLOD(model);
                model.skin_ = ImportArtificialSkin(*node);
            }
        }
    }

    void CleanupBoneNames()
    {
        if (!settings_.cleanupBoneNames_)
            return;

        std::unordered_map<const ufbx_node*, std::vector<const ufbx_node*>> skeletonNodes;
        for (const ImportedModel& model : models_)
        {
            if (model.skin_.IsSkinned())
                skeletonNodes[model.skin_.root_].insert(skeletonNodes[model.skin_.root_].end(),
                    model.skin_.boneNodes_.begin(), model.skin_.boneNodes_.end());
        }
        for (auto& [_, nodes] : skeletonNodes)
        {
            std::sort(nodes.begin(), nodes.end());
            nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
            StringVector names;
            for (const ufbx_node* node : nodes)
                names.push_back(nodeNames_.at(node));
            const unsigned prefixLength = FindCommonPrefixWord(names);
            for (const ufbx_node* node : nodes)
                nodeNames_[node] = nodeNames_.at(node).substr(prefixLength);
        }
    }

    void RefreshBoneNames(SkinInfo& skin)
    {
        for (unsigned i = 0; i < skin.boneNodes_.size(); ++i)
            skin.bones_[i].name_ = nodeNames_.at(skin.boneNodes_[i]);
    }

    void InitializeLOD(ImportedModel& model)
    {
        const ufbx_node& node = *model.sourceNode_;
        model.meshName_ =
            node.attrib && node.attrib->name.length ? ToString(node.attrib->name).c_str() : nodeNames_.at(&node);
        model.baseMeshName_ = model.meshName_;

        // Match the mesh name first. Then match the node name, because exporters may name only the node.
        for (const ea::string& name : {model.meshName_, GetConfiguredNodeName(node)})
        {
            LodName lodName = ParseLodName(name);
            if (lodName.distance_)
            {
                model.baseMeshName_ = ea::move(lodName.baseName_);
                model.lodDistance_ = lodName.distance_;
                model.lodNameNumber_ = lodName.index_;
                return;
            }
        }

        const ufbx_node* parent = node.parent;
        const ufbx_lod_group* lodGroup = parent ? ufbx_as_lod_group(parent->attrib) : nullptr;
        if (!lodGroup)
            return;
        for (unsigned i = 0; i < parent->children.count && i < lodGroup->lod_levels.count; ++i)
        {
            if (parent->children.data[i] == &node)
            {
                model.baseMeshName_ = nodeNames_.at(parent);
                const float distance = static_cast<float>(lodGroup->lod_levels.data[i].distance);
                model.lodDistance_ = i == 0        ? 0.0f
                    : lodGroup->relative_distances ? 100.0f / ea::max(distance, M_EPSILON)
                                                   : distance;
                model.lodOrder_ = i;
                model.absoluteLODDistance_ = !lodGroup->relative_distances;
                return;
            }
        }
    }

    bool AreSkinsEquivalent(const SkinInfo& lhs, const SkinInfo& rhs) const
    {
        if (lhs.IsSkinned() != rhs.IsSkinned())
            return false;
        if (!lhs.IsSkinned())
            return true;
        return lhs.root_ == rhs.root_ && AreBonesEquivalent(lhs.bones_, rhs.bones_, settings_.offsetMatrixError_);
    }

    void ReuseEquivalentModels()
    {
        for (unsigned i = 0; i < models_.size(); ++i)
        {
            ImportedModel& model = models_[i];
            for (unsigned j = 0; j < i; ++j)
            {
                const ImportedModel& previous = models_[j];
                if (model.sourceNode_->attrib == previous.sourceNode_->attrib
                    && AreSkinsEquivalent(model.skin_, previous.skin_))
                {
                    model.modelView_ = previous.modelView_;
                    break;
                }
            }
        }
    }

    /// Return true when the LOD names of the sorted group carry the level indices 0..N-1 or 1..N.
    /// Other numbers, for example "_LOD10" and "_LOD25.5", are LOD distances.
    static bool IsLodIndexSequence(const std::vector<ImportedModel*>& lods)
    {
        if (lods.size() < 2 || !lods.front()->lodNameNumber_ || *lods.front()->lodNameNumber_ > 1)
            return false;
        const unsigned first = *lods.front()->lodNameNumber_;
        for (unsigned lodIndex = 0; lodIndex < lods.size(); ++lodIndex)
        {
            const ImportedModel& lod = *lods[lodIndex];
            if (lod.lodOrder_ || !lod.lodNameNumber_ || *lod.lodNameNumber_ != first + lodIndex)
                return false;
        }
        return true;
    }

    void CombineLODs()
    {
        for (ImportedModel& model : models_)
        {
            if (!model.lodDistance_ || model.processedAsLOD_)
                continue;

            std::vector<ImportedModel*> lods;
            for (ImportedModel& candidate : models_)
            {
                if (candidate.lodDistance_ && candidate.baseMeshName_ == model.baseMeshName_
                    && AreSkinsEquivalent(candidate.skin_, model.skin_))
                    lods.push_back(&candidate);
            }
            std::sort(lods.begin(), lods.end(), [](const ImportedModel* lhs, const ImportedModel* rhs)
            {
                if (lhs->lodOrder_ && rhs->lodOrder_)
                    return *lhs->lodOrder_ < *rhs->lodOrder_;
                return *lhs->lodDistance_ < *rhs->lodDistance_;
            });
            if (lods.empty())
                continue;

            SharedPtr<ModelView> finalView = lods.front()->modelView_;
            const BoundingBox modelBox = finalView->CalculateBoundingBox();
            const float modelScale = ea::max(modelBox.Size().DotProduct(DOT_SCALE), M_EPSILON);

            // The engine compares a geometry LOD distance with the camera distance divided by the model size.
            // The model size is the mean edge of the bounding box (see StaticModel::UpdateBatches).
            ea::vector<float> lodDistances(lods.size());
            if (IsLodIndexSequence(lods))
            {
                // The names are level indices, not distances. Level i starts at i * LOD_INDEX_STEP sphere radii.
                const float radiusInModelSizes = modelBox.Defined() ? modelBox.HalfSize().Length() / modelScale : 1.0f;
                for (unsigned lodIndex = 0; lodIndex < lods.size(); ++lodIndex)
                    lodDistances[lodIndex] = lodIndex * LOD_INDEX_STEP * radiusInModelSizes;
            }
            else
            {
                for (unsigned lodIndex = 0; lodIndex < lods.size(); ++lodIndex)
                {
                    const ImportedModel& lod = *lods[lodIndex];
                    lodDistances[lodIndex] = lod.absoluteLODDistance_
                        ? *lod.lodDistance_ * Abs(settings_.scale_) / modelScale
                        : *lod.lodDistance_;
                }
            }
            ea::vector<ModelView*> lodViews;
            for (const ImportedModel* lod : lods)
                lodViews.push_back(lod->modelView_);
            CombineModelViewLODs(lodViews, lodDistances);
            const unsigned primaryModelIndex = modelIndexByNode_.at(lods.front()->sourceNode_);
            for (unsigned lodIndex = 0; lodIndex < lods.size(); ++lodIndex)
            {
                ImportedModel* lod = lods[lodIndex];
                lod->modelView_ = finalView;
                lod->materials_ = lods.front()->materials_;
                lod->processedAsLOD_ = true;
                lod->skipComponent_ = lodIndex != 0;
                modelIndexByNode_[lod->sourceNode_] = primaryModelIndex;
            }
        }
    }

    void CookModels()
    {
        std::unordered_map<ModelView*, SharedPtr<Model>> cookedViews;
        for (ImportedModel& model : models_)
        {
            if (const auto iter = cookedViews.find(model.modelView_.Get()); iter != cookedViews.end())
            {
                model.model_ = iter->second;
                continue;
            }
            ExportedModel exported = ExportModelView(*model.modelView_, *callback_);
            const ea::vector<unsigned>& materialMapping = exported.materialMapping_;
            if (!materialMapping.empty())
            {
                for (ImportedModel& candidate : models_)
                {
                    if (candidate.modelView_ != model.modelView_)
                        continue;
                    const ea::vector<SharedPtr<Material>> sourceMaterials = ea::move(candidate.materials_);
                    candidate.materials_.reserve(materialMapping.size());
                    for (const unsigned sourceIndex : materialMapping)
                    {
                        candidate.materials_.push_back(
                            sourceIndex < sourceMaterials.size() ? sourceMaterials[sourceIndex] : nullptr);
                    }
                }
            }
            model.model_ = ea::move(exported.model_);
            AddToResourceCache(model.model_);
            modelsToSave_.push_back(model.model_);
            cookedViews[model.modelView_.Get()] = model.model_;
        }
        CookSkeletonDrivers();
    }

    void CookSkeletonDrivers()
    {
        // Keep renderable models compact while one geometry-less master drives every bone shared by the meshes.
        for (const ufbx_node* root : skeletonRoots_)
        {
            if (!NeedsSkeletonDriver(root))
                continue;

            const std::vector<const ufbx_node*>& boneNodes = GetSkinBoneNodes(*root);
            std::unordered_map<const ufbx_node*, unsigned> nodeToBone;
            for (unsigned i = 0; i < boneNodes.size(); ++i)
                nodeToBone[boneNodes[i]] = i;

            ea::vector<BoneView> bones(boneNodes.size());
            for (unsigned i = 0; i < boneNodes.size(); ++i)
            {
                const ufbx_node& sourceBone = *boneNodes[i];
                BoneView& bone = bones[i];
                bone.name_ = nodeNames_.at(&sourceBone);
                if (&sourceBone != root)
                {
                    const ufbx_node* parent = sourceBone.parent;
                    while (parent && nodeToBone.find(parent) == nodeToBone.end())
                        parent = parent->parent;
                    if (parent)
                        bone.parentIndex_ = nodeToBone.at(parent);
                }
                bone.SetInitialTransform(ConvertPosition(sourceBone.local_transform.translation),
                    ConvertRotation(sourceBone.local_transform.rotation), ToVector3(sourceBone.local_transform.scale));
                bone.offsetMatrix_ = Matrix3x4::IDENTITY;
            }

            auto modelView = MakeShared<ModelView>(context_);
            modelView->SetBones(ea::move(bones));
            const ea::string modelName =
                CreateResourceName(nodeNames_.at(root) + "_Skeleton", "Models/", "Skeleton", ".mdl");
            modelView->SetName(modelName);
            SharedPtr<Model> model = modelView->ExportModel();
            AddToResourceCache(model);
            modelsToSave_.push_back(model);
            skeletonDriverModels_[root] = model;
        }
    }

    void ImportMorphTargets(ImportedModel& importedModel)
    {
        const ufbx_mesh& mesh = *importedModel.sourceNode_->mesh;
        for (const ufbx_blend_deformer* deformer : mesh.blend_deformers)
        {
            for (const ufbx_blend_channel* channel : deformer->channels)
            {
                for (unsigned keyframeIndex = 0; keyframeIndex < channel->keyframes.count; ++keyframeIndex)
                {
                    if (const ufbx_blend_shape* shape = channel->keyframes.data[keyframeIndex].shape)
                        importedModel.morphTargets_.push_back({channel, keyframeIndex, shape});
                }
            }
        }

        const auto weights = EvaluateMorphWeights(importedModel.morphTargets_,
            [](const ufbx_blend_channel& channel) { return static_cast<float>(channel.weight); });
        importedModel.morphWeights_.assign(weights.begin(), weights.end());
    }

    SharedPtr<ModelView> ImportModelView(ImportedModel& importedModel)
    {
        auto modelView = MakeShared<ModelView>(context_);
        modelView->SetBones(importedModel.skin_.bones_);
        for (unsigned i = 0; i < importedModel.morphTargets_.size(); ++i)
        {
            const MorphTarget& target = importedModel.morphTargets_[i];
            ea::string name = ToString(target.shape_->name).c_str();
            if (name.empty())
                name = ToString(target.channel_->name).c_str();
            if (name.empty())
                name = Format("Morph_{}", i);
            modelView->SetMorph(i, {name, importedModel.morphWeights_[i]});
        }

        const ufbx_node& node = *importedModel.sourceNode_;
        if (node.mesh)
            ImportMesh(*modelView, importedModel, *node.mesh);
        else if (const ufbx_line_curve* line = ufbx_as_line_curve(node.attrib))
            ImportLineCurve(*modelView, importedModel, *line);
        else if (const ufbx_nurbs_curve* curve = ufbx_as_nurbs_curve(node.attrib))
        {
            // ufbx loads a curve with a bad parametrization, but it cannot tessellate it.
            if (!curve->basis.valid || curve->control_points.count == 0)
            {
                URHO3D_LOGWARNING(
                    "FBX NURBS curve '{}' has an invalid basis; it is not imported", ToString(curve->name).c_str());
                modelView->Normalize();
                return modelView;
            }

            ufbx_tessellate_curve_opts options{};
            options.span_subdivision = 4;
            ufbx_error error{};
            LineCurvePtr line{ufbx_tessellate_nurbs_curve(curve, &options, &error)};
            if (!line)
                throw RuntimeException("Cannot tessellate FBX NURBS curve '{}': {}", ToString(curve->name).c_str(),
                    FormatUFBXError(error));
            ImportLineCurve(*modelView, importedModel, *line);
        }
        else if (const ufbx_nurbs_surface* surface = ufbx_as_nurbs_surface(node.attrib))
        {
            if (!IsValidNurbsSurface(*surface))
            {
                URHO3D_LOGWARNING("FBX NURBS surface '{}' has invalid control points; it is not imported",
                    ToString(surface->name).c_str());
                modelView->Normalize();
                return modelView;
            }

            // Use the display subdivision of the file, as ufbx does. Limit it to keep the triangle count bounded.
            ufbx_tessellate_surface_opts options{};
            options.span_subdivision_u = ea::clamp(surface->span_subdivision_u, 1u, MaxNurbsSpanSubdivision);
            options.span_subdivision_v = ea::clamp(surface->span_subdivision_v, 1u, MaxNurbsSpanSubdivision);
            ufbx_error error{};
            MeshPtr mesh{ufbx_tessellate_nurbs_surface(surface, &options, &error)};
            if (!mesh)
                throw RuntimeException("Cannot tessellate FBX NURBS surface '{}': {}", ToString(surface->name).c_str(),
                    FormatUFBXError(error));
            ImportMesh(*modelView, importedModel, *mesh);
        }
        else
        {
            modelView->Normalize();
            return modelView;
        }

        FinalizeModelView(*modelView, transform_);
        return modelView;
    }

    void ImportMesh(ModelView& modelView, ImportedModel& importedModel, const ufbx_mesh& mesh)
    {
        if (mesh.material_parts.count > 0)
        {
            for (const ufbx_mesh_part& part : mesh.material_parts)
            {
                ImportMeshPart(modelView, importedModel, mesh, part.face_indices.data, part.num_faces, part.index);
            }
        }
        else
        {
            std::vector<uint32_t> faceIndices(mesh.faces.count);
            for (uint32_t i = 0; i < faceIndices.size(); ++i)
                faceIndices[i] = i;
            ImportMeshPart(modelView, importedModel, mesh, faceIndices.data(), faceIndices.size(), 0);
        }
    }

    void ImportLineCurve(ModelView& modelView, ImportedModel& importedModel, const ufbx_line_curve& line)
    {
        const auto importSegment = [&](size_t indexBegin, size_t numIndices, bool usePointIndices)
        {
            if (numIndices == 0)
                return;
            GeometryView geometry;
            GeometryLODView& lod = geometry.lods_.emplace_back();
            lod.primitiveType_ = numIndices == 1 ? POINT_LIST : LINE_STRIP;
            lod.vertexFormat_.position_ = TYPE_VECTOR3;
            lod.vertexFormat_.color_[0] = TYPE_VECTOR4;
            lod.vertices_.reserve(numIndices);
            lod.indices_.reserve(numIndices);
            bool nonFinite = false;
            for (size_t i = 0; i < numIndices; ++i)
            {
                const size_t pointIndex = usePointIndices ? line.point_indices.data[indexBegin + i] : indexBegin + i;
                if (pointIndex >= line.control_points.count)
                    throw RuntimeException(
                        "FBX line curve '{}' has an invalid point index", ToString(line.name).c_str());
                ModelVertex& vertex = lod.vertices_.emplace_back();
                vertex.SetPosition(ReplaceNonFinite(ToVector3(line.control_points.data[pointIndex]), nonFinite));
                vertex.color_[0] = Vector4{ToVector3(line.color), 1.0f};
                lod.indices_.push_back(static_cast<unsigned>(i));
            }
            if (nonFinite)
            {
                URHO3D_LOGWARNING(
                    "FBX line curve '{}' has non-finite positions; they are set to zero", ToString(line.name).c_str());
            }

            const ufbx_node& node = *importedModel.sourceNode_;
            const ufbx_material* sourceMaterial = node.materials.count > 0 ? node.materials.data[0] : nullptr;
            if (SharedPtr<Material> material = GetMaterial(sourceMaterial, UnlitMaterial))
            {
                geometry.material_ = material->GetName();
                importedModel.materials_.push_back(material);
            }
            else
                importedModel.materials_.push_back(nullptr);
            modelView.GetGeometries().push_back(ea::move(geometry));
        };

        if (line.segments.count > 0)
        {
            for (const ufbx_line_segment& segment : line.segments)
            {
                if (segment.index_begin + segment.num_indices > line.point_indices.count)
                    throw RuntimeException("FBX line curve '{}' has an invalid segment", ToString(line.name).c_str());
                importSegment(segment.index_begin, segment.num_indices, true);
            }
        }
        else if (line.point_indices.count > 0)
            importSegment(0, line.point_indices.count, true);
        else
            importSegment(0, line.control_points.count, false);
    }

    void ImportMeshPart(ModelView& modelView, ImportedModel& importedModel, const ufbx_mesh& mesh,
        const uint32_t* faceIndices, size_t numFaces, unsigned materialSlot)
    {
        if (mesh.num_triangles > 0)
            ImportMeshPrimitive(modelView, importedModel, mesh, faceIndices, numFaces, materialSlot, TRIANGLE_LIST);
        if (mesh.num_line_faces > 0)
            ImportMeshPrimitive(modelView, importedModel, mesh, faceIndices, numFaces, materialSlot, LINE_LIST);
        if (mesh.num_point_faces > 0)
            ImportMeshPrimitive(modelView, importedModel, mesh, faceIndices, numFaces, materialSlot, POINT_LIST);
    }

    void ImportMeshPrimitive(ModelView& modelView, ImportedModel& importedModel, const ufbx_mesh& mesh,
        const uint32_t* faceIndices, size_t numFaces, unsigned materialSlot, PrimitiveType primitiveType)
    {
        const ufbx_node& node = *importedModel.sourceNode_;
        GeometryView geometry;
        GeometryLODView& lod = geometry.lods_.emplace_back();
        lod.primitiveType_ = primitiveType;
        lod.vertexFormat_.position_ = TYPE_VECTOR3;

        const bool hasNormals = mesh.vertex_normal.exists;
        const bool hasTangents = mesh.vertex_tangent.exists && mesh.vertex_bitangent.exists;
        const unsigned numUVs = static_cast<unsigned>(std::min<size_t>(mesh.uv_sets.count, ModelVertex::MaxUVs));
        const unsigned numColors =
            static_cast<unsigned>(std::min<size_t>(mesh.color_sets.count, ModelVertex::MaxColors));
        const bool hasSkin = importedModel.skin_.IsSkinned();
        // Corrupt files can contain NaN or infinity. Such values break the bounding boxes and the rendering.
        bool nonFinite = false;
        if (hasNormals)
            lod.vertexFormat_.normal_ = TYPE_VECTOR3;
        if (hasTangents)
            lod.vertexFormat_.tangent_ = TYPE_VECTOR4;
        for (unsigned i = 0; i < numUVs; ++i)
            lod.vertexFormat_.uv_[i] = TYPE_VECTOR2;
        for (unsigned i = 0; i < numColors; ++i)
            lod.vertexFormat_.color_[i] = TYPE_VECTOR4;
        if (hasSkin)
        {
            lod.vertexFormat_.blendIndices_ = TYPE_UBYTE4;
            lod.vertexFormat_.blendWeights_ = TYPE_UBYTE4_NORM;
        }

        std::unordered_map<uint32_t, std::vector<unsigned>> sourceToTargetVertices;
        std::vector<uint32_t> triangleIndices(std::max<size_t>(mesh.max_face_triangles * 3, 3));
        for (size_t faceListIndex = 0; faceListIndex < numFaces; ++faceListIndex)
        {
            const uint32_t faceIndex = faceIndices[faceListIndex];
            if (faceIndex >= mesh.faces.count)
                throw RuntimeException("FBX mesh '{}' has an invalid face index", ToString(mesh.name).c_str());
            if (mesh.face_hole.count > faceIndex && mesh.face_hole.data[faceIndex])
                continue;

            const ufbx_face face = mesh.faces.data[faceIndex];
            const bool isRequestedPrimitive = primitiveType == POINT_LIST ? face.num_indices == 1
                : primitiveType == LINE_LIST                              ? face.num_indices == 2
                                                                          : face.num_indices >= 3;
            if (!isRequestedPrimitive)
                continue;

            size_t numPrimitiveIndices = face.num_indices;
            if (primitiveType == TRIANGLE_LIST)
            {
                const size_t numTriangles =
                    ufbx_triangulate_face(triangleIndices.data(), triangleIndices.size(), &mesh, face);
                numPrimitiveIndices = numTriangles * 3;
            }
            else
            {
                for (size_t i = 0; i < face.num_indices; ++i)
                    triangleIndices[i] = face.index_begin + static_cast<uint32_t>(i);
            }
            for (size_t primitiveIndex = 0; primitiveIndex < numPrimitiveIndices; ++primitiveIndex)
            {
                const uint32_t index = triangleIndices[primitiveIndex];
                const uint32_t sourceVertex = mesh.vertex_indices.data[index];
                ModelVertex vertex;
                vertex.SetPosition(
                    ReplaceNonFinite(ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_position, index)), nonFinite));

                Vector3 normal;
                if (hasNormals)
                {
                    normal = ReplaceNonFinite(ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_normal, index)), nonFinite)
                                 .Normalized();
                    vertex.SetNormal(normal);
                }
                if (hasTangents)
                {
                    const Vector3 tangent =
                        ReplaceNonFinite(ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_tangent, index)), nonFinite)
                            .Normalized();
                    const Vector3 bitangent =
                        ReplaceNonFinite(ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_bitangent, index)), nonFinite)
                            .Normalized();
                    vertex.tangent_ = {
                        tangent, normal.CrossProduct(tangent).DotProduct(bitangent) < 0.0f ? -1.0f : 1.0f};
                }
                for (unsigned uvIndex = 0; uvIndex < numUVs; ++uvIndex)
                    vertex.uv_[uvIndex] = {ReplaceNonFinite(ToVector2(ufbx_get_vertex_vec2(
                                                                &mesh.uv_sets.data[uvIndex].vertex_uv, index)),
                                               nonFinite),
                        Vector2::ZERO};
                for (unsigned colorIndex = 0; colorIndex < numColors; ++colorIndex)
                    vertex.color_[colorIndex] = ReplaceNonFinite(
                        ToVector4(ufbx_get_vertex_vec4(&mesh.color_sets.data[colorIndex].vertex_color, index)),
                        nonFinite);
                if (hasSkin)
                    AppendSkinVertex(vertex, importedModel.skin_, sourceVertex);

                std::vector<ModelVertexMorph> vertexMorphs(importedModel.morphTargets_.size());
                for (unsigned morphIndex = 0; morphIndex < importedModel.morphTargets_.size(); ++morphIndex)
                {
                    const ufbx_blend_shape& shape = *importedModel.morphTargets_[morphIndex].shape_;
                    ModelVertexMorph& morph = vertexMorphs[morphIndex];
                    const uint32_t offsetIndex = ufbx_get_blend_shape_offset_index(&shape, sourceVertex);
                    if (offsetIndex != UFBX_NO_INDEX)
                    {
                        const float offsetWeight = offsetIndex < shape.offset_weights.count
                            ? static_cast<float>(shape.offset_weights.data[offsetIndex])
                            : 1.0f;
                        morph.positionDelta_ = ReplaceNonFinite(
                            ToVector3(shape.position_offsets.data[offsetIndex]) * offsetWeight, nonFinite);
                        if (offsetIndex < shape.normal_offsets.count)
                            morph.normalDelta_ = ReplaceNonFinite(
                                ToVector3(shape.normal_offsets.data[offsetIndex]) * offsetWeight, nonFinite);
                    }
                }

                ea::optional<unsigned> targetVertex;
                for (const unsigned candidate : sourceToTargetVertices[sourceVertex])
                {
                    if (lod.vertices_[candidate] != vertex)
                        continue;
                    bool morphsMatch = true;
                    for (unsigned morphIndex = 0; morphIndex < vertexMorphs.size(); ++morphIndex)
                        morphsMatch &= lod.morphs_[morphIndex][candidate] == vertexMorphs[morphIndex];
                    if (morphsMatch)
                    {
                        targetVertex = candidate;
                        break;
                    }
                }
                if (!targetVertex)
                {
                    targetVertex = lod.vertices_.size();
                    lod.vertices_.push_back(vertex);
                    sourceToTargetVertices[sourceVertex].push_back(*targetVertex);
                    for (unsigned morphIndex = 0; morphIndex < vertexMorphs.size(); ++morphIndex)
                    {
                        vertexMorphs[morphIndex].index_ = *targetVertex;
                        lod.morphs_[morphIndex].push_back(vertexMorphs[morphIndex]);
                    }
                }
                lod.indices_.push_back(*targetVertex);
            }
        }

        if (nonFinite && nonFiniteMeshes_.insert(&mesh).second)
        {
            URHO3D_LOGWARNING("FBX mesh '{}' has non-finite vertex data; the bad values are set to zero",
                ToString(mesh.name).c_str());
        }

        if (lod.vertices_.empty())
            return;

        const ufbx_material* sourceMaterial = nullptr;
        if (materialSlot < node.materials.count)
            sourceMaterial = node.materials.data[materialSlot];
        else if (materialSlot < mesh.materials.count)
            sourceMaterial = mesh.materials.data[materialSlot];
        ValidateMaterialUVs(sourceMaterial, mesh);
        if (SharedPtr<Material> material = GetMaterial(sourceMaterial, LitNormalMapMaterial))
        {
            geometry.material_ = material->GetName();
            importedModel.materials_.push_back(material);
        }
        else
            importedModel.materials_.push_back(nullptr);
        modelView.GetGeometries().push_back(ea::move(geometry));
    }

    static void ValidateMaterialUVs(const ufbx_material* material, const ufbx_mesh& mesh)
    {
        if (!material)
            return;
        const ufbx_material_pbr_maps& pbr = material->pbr;
        const ufbx_material_map* maps[] = {&pbr.base_color, &pbr.opacity, &GetRoughnessMap(*material), &pbr.metalness,
            &pbr.ambient_occlusion, &pbr.normal_map, &pbr.emission_color};
        const char* mapNames[] = {"base color", "opacity", "roughness", "metalness", "occlusion", "normal", "emission"};
        const std::string primaryUV = mesh.uv_sets.count > 0 ? ToString(mesh.uv_sets.data[0].name) : std::string{};
        for (unsigned i = 0; i < ea::size(maps); ++i)
        {
            const ufbx_material_map& map = *maps[i];
            if (!map.texture_enabled || !GetFileTexture(map.texture))
                continue;
            const std::string uvSet = ToString(map.texture->uv_set);
            if (mesh.uv_sets.count == 0)
            {
                URHO3D_LOGWARNING("Material '{}' uses a {} texture but mesh '{}' has no UV coordinates",
                    ToString(material->name).c_str(), mapNames[i], ToString(mesh.name).c_str());
            }
            else if (!uvSet.empty() && uvSet != primaryUV)
            {
                URHO3D_LOGWARNING(
                    "Material '{}' uses UV set '{}' for its {} texture, but standard materials use "
                    "the primary UV set '{}'",
                    ToString(material->name).c_str(), uvSet, mapNames[i], primaryUV);
            }
        }
    }

    static const ufbx_material_map& GetRoughnessMap(const ufbx_material& material)
    {
        return material.features.roughness_as_glossiness.enabled ? material.pbr.glossiness : material.pbr.roughness;
    }

    static void AppendSkinVertex(ModelVertex& vertex, const SkinInfo& skinInfo, unsigned sourceVertex)
    {
        std::vector<std::pair<unsigned, float>> influences;
        for (const SkinDeformerInfo& deformer : skinInfo.deformers_)
        {
            const ufbx_skin_deformer& skin = *deformer.skin_;
            if (sourceVertex >= skin.vertices.count)
                continue;
            const ufbx_skin_vertex& skinVertex = skin.vertices.data[sourceVertex];
            for (size_t i = 0; i < skinVertex.num_weights; ++i)
            {
                const ufbx_skin_weight& weight = skin.weights.data[skinVertex.weight_begin + i];
                if (weight.cluster_index >= deformer.clusterToBone_.size()
                    || deformer.clusterToBone_[weight.cluster_index] < 0)
                    continue;
                const float weightValue = static_cast<float>(weight.weight);
                // A weight above FLT_MAX or an infinite weight would make the blend weights NaN.
                if (!std::isfinite(weightValue))
                    continue;
                const unsigned boneIndex = deformer.clusterToBone_[weight.cluster_index];
                const auto iter = std::find_if(influences.begin(), influences.end(),
                    [=](const auto& influence) { return influence.first == boneIndex; });
                if (iter != influences.end())
                    iter->second += weightValue;
                else
                    influences.emplace_back(boneIndex, weightValue);
            }
        }
        std::sort(influences.begin(), influences.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

        float totalWeight = 0.0f;
        const unsigned numWeights = ea::min<unsigned>(influences.size(), ModelVertex::MaxBones);
        for (unsigned i = 0; i < numWeights; ++i)
        {
            (&vertex.blendIndices_.x_)[i] = static_cast<float>(influences[i].first);
            (&vertex.blendWeights_.x_)[i] = influences[i].second;
            totalWeight += influences[i].second;
        }
        if (totalWeight > 0.0f)
            vertex.blendWeights_ /= totalWeight;
        else
        {
            if (skinInfo.unweightedBone_ >= 0)
                vertex.blendIndices_.x_ = static_cast<float>(skinInfo.unweightedBone_);
            vertex.blendWeights_.x_ = 1.0f;
        }
    }

    SharedPtr<Material> GetMaterial(const ufbx_material* sourceMaterial, MaterialVariant requestedVariant)
    {
        if (!sourceMaterial)
            return nullptr;

        const ufbx_material_pbr_maps& pbr = sourceMaterial->pbr;
        const bool unlit = sourceMaterial->features.unlit.enabled;
        const bool hasNormalMap = !unlit && requestedVariant == LitNormalMapMaterial && pbr.normal_map.texture_enabled
            && DecodeTexture(GetFileTexture(pbr.normal_map.texture));
        const MaterialVariant variant = unlit || requestedVariant == UnlitMaterial ? UnlitMaterial
            : hasNormalMap                                                         ? LitNormalMapMaterial
                                                                                   : LitMaterial;
        if (const auto iter = materials_.find(sourceMaterial); iter != materials_.end() && iter->second[variant])
            return iter->second[variant];

        const float baseFactor = pbr.base_factor.has_value ? static_cast<float>(pbr.base_factor.value_real) : 1.0f;
        const float opacity = pbr.opacity.has_value ? static_cast<float>(pbr.opacity.value_real) : 1.0f;
        const Color baseColor{static_cast<float>(pbr.base_color.value_vec4.x) * baseFactor,
            static_cast<float>(pbr.base_color.value_vec4.y) * baseFactor,
            static_cast<float>(pbr.base_color.value_vec4.z) * baseFactor,
            static_cast<float>(pbr.base_color.value_vec4.w) * opacity};
        const bool transparent =
            baseColor.a_ < 0.999f || (pbr.opacity.texture_enabled && GetFileTexture(pbr.opacity.texture));

        // Material::ResetToDefaults is a no-op on worker threads, so clone the prototype made on the main thread.
        SharedPtr<Material> material = defaultMaterial_->Clone();
        // Techniques are loaded later on the main thread.
        importedMaterials_.push_back(
            {material, GetStandardMaterialTechniques(variant, transparent, settings_.fadeTransparency_)});
        SetStandardMaterialFlags(*material, variant, sourceMaterial->features.double_sided.enabled);

        StandardMaterialParameters parameters;
        parameters.baseColor_ = baseColor;
        parameters.albedoTexture_ = GetBaseColorTexture(*sourceMaterial);
        if (variant != UnlitMaterial)
        {
            parameters.metallic_ = pbr.metalness.has_value ? static_cast<float>(pbr.metalness.value_real) : 0.0f;
            const ufbx_material_map& roughnessMap = GetRoughnessMap(*sourceMaterial);
            parameters.emissionTexture_ = GetTexture(pbr.emission_color);
            bool hasRoughnessTexture = false;
            parameters.propertiesTexture_ = GetPropertiesTexture(*sourceMaterial, hasRoughnessTexture);
            const bool glossinessTextureConverted =
                sourceMaterial->features.roughness_as_glossiness.enabled && hasRoughnessTexture;
            parameters.roughness_ = roughnessMap.has_value
                ? static_cast<float>(glossinessTextureConverted                      ? 1.0
                          : sourceMaterial->features.roughness_as_glossiness.enabled ? 1.0 - roughnessMap.value_real
                                                                                     : roughnessMap.value_real)
                : 1.0f;
            const float emissionFactor =
                pbr.emission_factor.has_value ? static_cast<float>(pbr.emission_factor.value_real) : 1.0f;
            parameters.emissiveColor_ = Color{static_cast<float>(pbr.emission_color.value_vec3.x) * emissionFactor,
                static_cast<float>(pbr.emission_color.value_vec3.y) * emissionFactor,
                static_cast<float>(pbr.emission_color.value_vec3.z) * emissionFactor};
            parameters.occlusionStrength_ =
                pbr.ambient_occlusion.has_value ? static_cast<float>(pbr.ambient_occlusion.value_real) : 1.0f;
        }
        if (variant == LitNormalMapMaterial)
        {
            parameters.normalScale_ = pbr.normal_map.has_value ? static_cast<float>(pbr.normal_map.value_real) : 1.0f;
            parameters.normalTexture_ = GetTexture(pbr.normal_map, true);
        }
        SetStandardMaterialParameters(*material, variant, parameters);

        if (!parameters.albedoTexture_ && settings_.highlightMissingTextures_ && pbr.base_color.texture_enabled
            && GetFileTexture(pbr.base_color.texture) && !DecodeTexture(pbr.base_color.texture))
            material->SetShaderParameter(ShaderConsts::Material_MatDiffColor, Vector4(1.0f, 0.1f, 0.9f, baseColor.a_));
        ApplyTextureTransform(*material, *sourceMaterial, variant);

        const ea::string materialName = CreateResourceName(
            ToString(sourceMaterial->name).c_str(), "Materials/", "Material", GetMaterialVariantSuffix(variant) + ".xml");
        material->SetName(materialName);
        AddToResourceCache(material);
        materials_[sourceMaterial][variant] = material;
        return material;
    }

    SharedPtr<Texture2D> GetTexture(const ufbx_material_map& materialMap, bool normalMap = false)
    {
        return materialMap.texture_enabled ? GetTexture(materialMap.texture, normalMap) : nullptr;
    }

    SharedPtr<Texture2D> GetTexture(const ufbx_texture* sourceTexture, bool normalMap = false)
    {
        const ufbx_texture* textureMapping = sourceTexture;
        const ufbx_texture* fileTexture = GetFileTexture(textureMapping);
        if (!fileTexture)
            return nullptr;
        if (const auto iter = sourceTextures_.find(textureMapping); iter != sourceTextures_.end())
            return iter->second;

        const std::shared_ptr<DecodedImage> decoded = DecodeTexture(fileTexture);
        if (!decoded)
            return nullptr;
        // Copy the image, because the decoded image is shared.
        const SharedPtr<Image> image = decoded->image_->ConvertToRGBA();
        if (normalMap && decoded->twoChannel_)
            ReconstructNormalZ(*image);
        const SharedPtr<Texture2D> texture = AddTexture(image, decoded->name_, textureMapping);
        sourceTextures_[textureMapping] = texture;
        return texture;
    }

    static bool AreTextureMappingsCompatible(const ufbx_texture* lhs, const ufbx_texture* rhs)
    {
        if (!lhs || !rhs || lhs == rhs)
            return true;
        if (ToString(lhs->uv_set) != ToString(rhs->uv_set) || lhs->wrap_u != rhs->wrap_u || lhs->wrap_v != rhs->wrap_v
            || lhs->has_uv_transform != rhs->has_uv_transform)
            return false;
        if (lhs->has_uv_transform)
        {
            for (unsigned i = 0; i < 12; ++i)
            {
                if (lhs->texture_to_uv.v[i] != rhs->texture_to_uv.v[i])
                    return false;
            }
        }
        return true;
    }

    static void ApplyTextureTransform(Material& material, const ufbx_material& sourceMaterial, MaterialVariant variant)
    {
        const ufbx_material_pbr_maps& pbr = sourceMaterial.pbr;
        ea::vector<const ufbx_texture*> mappings;
        const auto addMapping = [&](const ufbx_material_map& map)
        {
            if (map.texture_enabled && GetFileTexture(map.texture))
                mappings.push_back(map.texture);
        };
        addMapping(pbr.base_color);
        addMapping(pbr.opacity);
        if (variant != UnlitMaterial)
        {
            addMapping(GetRoughnessMap(sourceMaterial));
            addMapping(pbr.metalness);
            addMapping(pbr.ambient_occlusion);
            addMapping(pbr.emission_color);
        }
        if (variant == LitNormalMapMaterial)
            addMapping(pbr.normal_map);
        if (mappings.empty())
            return;

        const ufbx_texture* reference = mappings.front();
        for (const ufbx_texture* mapping : mappings)
        {
            if (!AreTextureMappingsCompatible(reference, mapping))
            {
                URHO3D_LOGWARNING(
                    "Material '{}' uses texture-specific UV mapping that cannot be represented by one "
                    "standard material transform",
                    ToString(sourceMaterial.name).c_str());
                break;
            }
        }
        if (reference->has_uv_transform)
        {
            const ufbx_matrix& transform = reference->uv_to_texture;
            material.SetShaderParameter(ShaderConsts::Material_UOffset,
                Vector4{static_cast<float>(transform.m00), static_cast<float>(transform.m01), 0.0f,
                    static_cast<float>(transform.m03)});
            material.SetShaderParameter(ShaderConsts::Material_VOffset,
                Vector4{static_cast<float>(transform.m10), static_cast<float>(transform.m11), 0.0f,
                    static_cast<float>(transform.m13)});
        }
    }

    SharedPtr<Texture2D> GetBaseColorTexture(const ufbx_material& sourceMaterial)
    {
        const ufbx_material_pbr_maps& pbr = sourceMaterial.pbr;
        const ufbx_texture* baseMapping = pbr.base_color.texture_enabled ? pbr.base_color.texture : nullptr;
        const ufbx_texture* opacityMapping = pbr.opacity.texture_enabled ? pbr.opacity.texture : nullptr;
        if (!opacityMapping || opacityMapping == baseMapping)
            return GetTexture(baseMapping);
        if (baseMapping && !AreTextureMappingsCompatible(baseMapping, opacityMapping))
        {
            URHO3D_LOGWARNING(
                "Material '{}' uses incompatible base-color and opacity texture mappings; the opacity "
                "texture is ignored",
                ToString(sourceMaterial.name).c_str());
            return GetTexture(baseMapping);
        }

        const ufbx_texture* baseTexture = GetFileTexture(baseMapping);
        const ufbx_texture* opacityTexture = GetFileTexture(opacityMapping);
        const auto base = DecodeTexture(baseTexture);
        const auto opacity = DecodeTexture(opacityTexture);
        if (!opacity)
            return GetTexture(baseTexture);

        const Image& opacityImage = *opacity->image_;
        const IntVector2 size = VectorMax(
            base ? base->image_->GetSize().ToIntVector2() : IntVector2::ZERO, opacityImage.GetSize().ToIntVector2());
        SharedPtr<Image> combined;
        if (base)
            combined = GetResizedRGBAImage(*base->image_, size);
        else
        {
            combined = MakeShared<Image>(context_);
            combined->SetSize(size.x_, size.y_, 4);
            combined->Clear(Color::WHITE);
        }
        const SharedPtr<Image> resizedOpacity = GetResizedRGBAImage(opacityImage, size);
        unsigned char* data = combined->GetData();
        const unsigned char* opacityData = resizedOpacity->GetData();
        const unsigned numPixels = static_cast<unsigned>(size.x_ * size.y_);
        for (unsigned i = 0; i < numPixels; ++i)
        {
            const unsigned opacityValue = ea::min(opacityData[i * 4], opacityData[i * 4 + 3]);
            data[i * 4 + 3] = static_cast<unsigned char>(data[i * 4 + 3] * opacityValue / 255);
        }
        const std::string name = base ? base->name_ + " Base Color" : opacity->name_ + " Opacity";
        return AddTexture(combined, name, baseMapping ? baseMapping : opacityMapping);
    }

    SharedPtr<Texture2D> GetPropertiesTexture(const ufbx_material& sourceMaterial, bool& hasRoughnessTexture)
    {
        hasRoughnessTexture = false;
        const ufbx_material_pbr_maps& pbr = sourceMaterial.pbr;
        const ufbx_material_map& roughnessMap = GetRoughnessMap(sourceMaterial);
        const bool invertRoughness = sourceMaterial.features.roughness_as_glossiness.enabled;
        const ufbx_texture* roughnessMapping = roughnessMap.texture_enabled ? roughnessMap.texture : nullptr;
        const ufbx_texture* metalnessMapping = pbr.metalness.texture_enabled ? pbr.metalness.texture : nullptr;
        const ufbx_texture* occlusionMapping =
            pbr.ambient_occlusion.texture_enabled ? pbr.ambient_occlusion.texture : nullptr;
        const ufbx_texture* referenceMapping = roughnessMapping ? roughnessMapping
            : metalnessMapping                                  ? metalnessMapping
                                                                : occlusionMapping;
        const auto discardIncompatible = [&](const char* channel, const ufbx_texture*& mapping)
        {
            if (mapping && !AreTextureMappingsCompatible(referenceMapping, mapping))
            {
                URHO3D_LOGWARNING("Material '{}' uses an incompatible {} texture mapping; that channel is ignored",
                    ToString(sourceMaterial.name).c_str(), channel);
                mapping = nullptr;
            }
        };
        discardIncompatible("metalness", metalnessMapping);
        discardIncompatible("occlusion", occlusionMapping);

        const ufbx_texture* roughnessTexture = GetFileTexture(roughnessMapping);
        const ufbx_texture* metalnessTexture = GetFileTexture(metalnessMapping);
        const ufbx_texture* occlusionTexture = GetFileTexture(occlusionMapping);
        if (!roughnessTexture && !metalnessTexture && !occlusionTexture)
            return nullptr;

        const auto roughness = DecodeTexture(roughnessTexture);
        const auto metalness = DecodeTexture(metalnessTexture);
        const auto occlusion = DecodeTexture(occlusionTexture);
        hasRoughnessTexture = roughness != nullptr;
        if (!roughness && !metalness && !occlusion)
            return nullptr;

        SharedPtr<Image> roughnessImage = roughness ? roughness->image_ : nullptr;
        if (roughness && invertRoughness)
        {
            // Convert the glossiness to the roughness. Use a copy, because the decoded image is shared.
            const float glossinessFactor =
                roughnessMap.has_value ? static_cast<float>(roughnessMap.value_real) : 1.0f;
            roughnessImage = roughness->image_->ConvertToRGBA();
            unsigned char* data = roughnessImage->GetData();
            const unsigned numPixels = static_cast<unsigned>(roughnessImage->GetWidth() * roughnessImage->GetHeight());
            for (unsigned i = 0; i < numPixels; ++i)
            {
                data[i * 4] =
                    static_cast<unsigned char>(Clamp(RoundToInt(255.0f - data[i * 4] * glossinessFactor), 0, 255));
            }
        }
        const SharedPtr<Image> packed = PackPropertiesImage(context_, {roughnessImage.Get(), 0},
            {metalness ? metalness->image_.Get() : nullptr, 0}, {occlusion ? occlusion->image_.Get() : nullptr, 0});
        if (!packed)
            return nullptr;
        return AddTexture(packed, "Properties", referenceMapping);
    }

    static const ufbx_texture* GetFileTexture(const ufbx_texture* texture)
    {
        if (!texture)
            return nullptr;
        if (texture->type == UFBX_TEXTURE_FILE)
            return texture;
        return texture->file_textures.count > 0 ? texture->file_textures.data[0] : nullptr;
    }

    ea::string FindTextureFile(const ufbx_texture& texture)
    {
        auto fileSystem = context_->GetSubsystem<FileSystem>();
        StringVector hints;
        const auto addHint = [&](const ufbx_string& value)
        {
            if (value.length > 0)
                hints.emplace_back(GetInternalPath(ea::string{value.data, static_cast<unsigned>(value.length)}));
        };
        addHint(texture.filename);
        addHint(texture.relative_filename);
        addHint(texture.absolute_filename);
        for (const ea::string& hint : hints)
        {
            if (fileSystem->FileExists(hint))
                return hint;
        }

        const ea::string& sourceDirectory = sources_[0].sourceDirectory_;
        if (sourceDirectory.empty() || hints.empty())
            return {};
        const ea::string fileName = GetFileNameAndExtension(hints.front());
        if (const auto iter = resolvedTextureFiles_.find(&texture); iter != resolvedTextureFiles_.end())
            return iter->second;

        ea::string searchRoot = sourceDirectory;
        for (unsigned i = 0; i < 2; ++i)
        {
            const ea::string parent = GetParentPath(searchRoot);
            if (parent.empty() || parent == searchRoot || GetParentPath(parent).empty())
                break;
            searchRoot = parent;
        }

        StringVector roots;
        for (ea::string root = sourceDirectory; !root.empty(); root = GetParentPath(root))
        {
            roots.push_back(root);
            if (root == searchRoot)
                break;
        }
        for (const ea::string& root : roots)
        {
            if (fileSystem->FileExists(root + fileName))
                return RememberTextureFile(texture, root + fileName);
            for (const ea::string& hint : hints)
            {
                const StringVector parts = hint.split('/');
                const unsigned maxParts = ea::min<unsigned>(4, parts.size());
                for (unsigned numParts = 2; numParts <= maxParts; ++numParts)
                {
                    ea::string suffix;
                    bool valid = true;
                    for (unsigned i = parts.size() - numParts; i < parts.size(); ++i)
                    {
                        if (parts[i].empty() || parts[i] == "." || parts[i] == ".." || parts[i].contains(':'))
                        {
                            valid = false;
                            break;
                        }
                        suffix += (suffix.empty() ? "" : "/") + parts[i];
                    }
                    const ea::string candidate = root + suffix;
                    if (valid && fileSystem->FileExists(candidate))
                        return RememberTextureFile(texture, candidate);
                }
            }
        }

        // The first pass searches only the model folder and well-known texture folders.
        BuildTextureSearchDirectories(searchRoot, false);
        ea::string found = FindTextureInDirectories(textureSearchDirectories_, fileName);
        if (found.empty())
        {
            // The fallback pass scans the whole search root with a bound on depth and folder count.
            BuildTextureSearchDirectories(searchRoot, true);
            found = FindTextureInDirectories(textureFallbackDirectories_, fileName);
        }
        if (!found.empty())
            return RememberTextureFile(texture, found);
        resolvedTextureFiles_[&texture] = {};
        return {};
    }

    ea::string FindTextureInDirectories(const ea::vector<ea::string>& directories, const ea::string& fileName)
    {
        auto fileSystem = context_->GetSubsystem<FileSystem>();
        for (const ea::string& directory : directories)
        {
            const ea::string candidate = directory + fileName;
            if (fileSystem->FileExists(candidate))
                return candidate;
        }
        const ea::string lowerFileName = fileName.to_lower();
        for (const ea::string& directory : directories)
        {
            const ea::string lowerDirectory = directory.to_lower();
            if (!lowerDirectory.contains("texture") && !lowerDirectory.contains("image")
                && !lowerDirectory.contains("material"))
                continue;
            StringVector files;
            fileSystem->ScanDir(files, directory, "*", SCAN_FILES);
            for (const ea::string& candidate : files)
            {
                if (candidate.to_lower() == lowerFileName)
                    return directory + candidate;
            }
        }
        return {};
    }

    void RecordMissingTexture(const ufbx_texture& texture)
    {
        const ufbx_string* source = nullptr;
        for (const ufbx_string* candidate : {&texture.filename, &texture.relative_filename, &texture.absolute_filename})
        {
            if (candidate->length > 0)
            {
                source = candidate;
                break;
            }
        }
        if (!source)
            return;
        const ea::string name = GetFileNameAndExtension(
            GetInternalPath(ea::string{source->data, static_cast<unsigned>(source->length)}));
        if (name.empty())
            return;
        if (!missingTextures_.contains(name))
            missingTextures_.push_back(name);
    }

    ea::string RememberTextureFile(const ufbx_texture& texture, const ea::string& fileName)
    {
        resolvedTextureFiles_[&texture] = fileName;
        URHO3D_LOGINFO("Resolved FBX texture '{}' as '{}'", ToString(texture.filename).c_str(), fileName);
        return fileName;
    }

    /// Meshes with non-finite vertex data. The importer logs one warning for each mesh.
    ea::unordered_set<const ufbx_mesh*> nonFiniteMeshes_;

    void BuildTextureSearchDirectories(const ea::string& root, bool fallback)
    {
        bool& initialized = fallback ? textureFallbackDirectoriesInitialized_ : textureSearchDirectoriesInitialized_;
        ea::vector<ea::string>& searchDirectories = fallback ? textureFallbackDirectories_ : textureSearchDirectories_;
        if (initialized || root.empty())
            return;
        initialized = true;
        struct PendingDirectory
        {
            ea::string path_;
            unsigned depth_{};
            /// A parent of the source directory. Only well-known texture folders below it are searched.
            bool restricted_{};
        };
        constexpr unsigned MaxSearchDepth = 4;
        constexpr unsigned MaxSearchDirectories = 512;
        std::vector<PendingDirectory> pending;
        std::unordered_set<std::filesystem::path> visited;
        const auto addPending = [&](const ea::string& path, unsigned depth, bool restricted)
        {
            std::error_code error;
            std::filesystem::path canonical = std::filesystem::canonical(std::filesystem::u8path(path.c_str()), error);
            if (error)
                canonical = std::filesystem::u8path(ResolvePath(path).c_str());
            if (visited.emplace(std::move(canonical)).second)
                pending.push_back({AddTrailingSlash(path), depth, restricted});
        };
        const ea::string& sourceDirectory = sources_[0].sourceDirectory_;
        if (fallback)
        {
            // Old behavior: a bounded scan of all folders below the search root.
            addPending(root, 0, false);
        }
        else
        {
            // A parent folder can hold a huge number of unrelated entries. Never scan it fully.
            addPending(sourceDirectory.empty() ? root : sourceDirectory, 0, false);
            for (ea::string parent = GetParentPath(sourceDirectory); !sourceDirectory.empty() && !parent.empty();
                 parent = GetParentPath(parent))
            {
                addPending(parent, 0, true);
                if (parent == root)
                    break;
            }
        }
        auto fileSystem = context_->GetSubsystem<FileSystem>();
        for (size_t index = 0; index < pending.size() && index < MaxSearchDirectories; ++index)
        {
            const PendingDirectory current = pending[index];
            searchDirectories.push_back(current.path_);
            if (current.depth_ >= MaxSearchDepth)
                continue;
            StringVector directories;
            if (current.restricted_)
            {
                for (const char* name : {"textures", "Textures", "texture", "Texture", "tex", "Tex", "images", "Images",
                         "maps", "Maps", "materials", "Materials", "sourceimages", "SourceImages"})
                {
                    if (fileSystem->DirExists(current.path_ + name))
                        directories.push_back(name);
                }
            }
            else
                fileSystem->ScanDir(directories, current.path_, "*", SCAN_DIRS);
            // A folder can hold a huge number of entries (for example /tmp). Compute the score of each entry
            // once, then sort only the entries that can fit into the directory budget.
            struct ScoredDirectory
            {
                int score_{};
                ea::string name_;
            };
            ea::vector<ScoredDirectory> scored;
            scored.reserve(directories.size());
            for (ea::string& directory : directories)
            {
                if (directory == "." || directory == "..")
                    continue;
                const ea::string lower = directory.to_lower();
                const int score = lower.contains("texture") || lower.contains("image") ? 0
                    : lower.contains("material")                                       ? 1
                                                                                       : 2;
                scored.push_back({score, ea::move(directory)});
            }
            const auto isBefore = [](const ScoredDirectory& lhs, const ScoredDirectory& rhs)
            { return lhs.score_ != rhs.score_ ? lhs.score_ < rhs.score_ : lhs.name_ < rhs.name_; };
            // Sort in chunks. Visited folders do not use the budget, so more than one chunk can be necessary.
            // The result is the same as the result of a full sort.
            for (size_t begin = 0; begin < scored.size() && pending.size() < MaxSearchDirectories;)
            {
                const size_t count = ea::min<size_t>(scored.size() - begin, MaxSearchDirectories - pending.size());
                ea::partial_sort(
                    scored.begin() + begin, scored.begin() + begin + count, scored.end(), isBefore);
                for (size_t i = begin; i < begin + count && pending.size() < MaxSearchDirectories; ++i)
                    addPending(current.path_ + scored[i].name_, current.depth_ + 1, false);
                begin += count;
            }
        }
    }

    std::shared_ptr<DecodedImage> DecodeTexture(const ufbx_texture* sourceTexture)
    {
        sourceTexture = GetFileTexture(sourceTexture);
        if (!sourceTexture)
            return {};
        if (const auto iter = decodedTextures_.find(sourceTexture); iter != decodedTextures_.end())
            return iter->second;

        std::vector<unsigned char> encoded;
        if (sourceTexture->content.data && sourceTexture->content.size > 0)
        {
            const auto* begin = static_cast<const unsigned char*>(sourceTexture->content.data);
            encoded.assign(begin, begin + sourceTexture->content.size);
        }
        else
        {
            const ea::string textureFileName = FindTextureFile(*sourceTexture);
            if (!textureFileName.empty())
            {
                File file(context_, textureFileName, FILE_READ);
                if (file.IsOpen())
                {
                    const ByteVector bytes = file.ReadBinary();
                    encoded.assign(bytes.begin(), bytes.end());
                }
            }
        }
        if (encoded.empty())
        {
            URHO3D_LOGWARNING("FBX texture '{}' could not be read", ToString(sourceTexture->filename).c_str());
            RecordMissingTexture(*sourceTexture);
            decodedTextures_[sourceTexture] = {};
            return {};
        }

        MemoryBuffer buffer(encoded.data(), static_cast<unsigned>(encoded.size()));
        auto image = MakeShared<Image>(context_);
        if (!image->BeginLoad(buffer))
        {
            URHO3D_LOGWARNING("FBX texture '{}' has unsupported image data", ToString(sourceTexture->filename).c_str());
            RecordMissingTexture(*sourceTexture);
            decodedTextures_[sourceTexture] = {};
            return {};
        }
        const bool twoChannel = image->GetCompressedFormat() == TextureFormat::TEX_FORMAT_BC5_UNORM;
        const SharedPtr<Image> rgba = image->GetDecompressedImage();
        if (!rgba)
        {
            URHO3D_LOGWARNING("FBX texture '{}' could not be decompressed", ToString(sourceTexture->filename).c_str());
            RecordMissingTexture(*sourceTexture);
            decodedTextures_[sourceTexture] = {};
            return {};
        }

        auto result = std::make_shared<DecodedImage>();
        result->image_ = rgba;
        result->twoChannel_ = twoChannel;
        result->name_ = sourceTexture->name.length ? ToString(sourceTexture->name) : ToString(sourceTexture->filename);
        decodedTextures_[sourceTexture] = result;
        return result;
    }

    /// Compute the Z component of the normal map from the X and Y components.
    static void ReconstructNormalZ(Image& image)
    {
        unsigned char* data = image.GetData();
        const unsigned numPixels = static_cast<unsigned>(image.GetWidth() * image.GetHeight());
        for (unsigned i = 0; i < numPixels; ++i)
        {
            const float x = data[i * 4] / 127.5f - 1.0f;
            const float y = data[i * 4 + 1] / 127.5f - 1.0f;
            const float z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
            data[i * 4 + 2] = static_cast<unsigned char>(std::lround((z * 0.5f + 0.5f) * 255.0f));
        }
    }

    SharedPtr<Texture2D> AddTexture(const SharedPtr<Image>& image, const std::string& name, const ufbx_texture* mapping)
    {
        const ea::string imageName = CreateResourceName(name.c_str(), "Textures/", "Texture", ".png");
        image->SetName(imageName);
        image->SetAbsoluteFileName(GetAbsoluteFileName(imageName));

        TextureSamplerParams samplerParams;
        samplerParams.wrapU_ = mapping && mapping->wrap_u == UFBX_WRAP_CLAMP ? ADDRESS_CLAMP : ADDRESS_WRAP;
        samplerParams.wrapV_ = mapping && mapping->wrap_v == UFBX_WRAP_CLAMP ? ADDRESS_CLAMP : ADDRESS_WRAP;
        const TexturePlaceholder placeholder =
            CreateTexturePlaceholder(context_, imageName, GetAbsoluteFileName(imageName), samplerParams);

        ImportedTexture& imported = textures_.emplace_back();
        imported.image_ = image;
        imported.texture_ = placeholder.texture_;
        imported.sampler_ = placeholder.sampler_;
        return imported.texture_;
    }

    void CreatePrefab()
    {
        scene_ = CreateImportScene(context_, settings_);

        assetRoot_ = scene_->CreateChild(settings_.assetName_);
        assetRoot_->SetRotation(settings_.rotation_);
        ImportRuntimeNode(*assetRoot_, *sources_[0].scene_->root_node);
        AttachModels();
        AttachAnimationControllers();

        assetRoot_ = FinalizeImportScene(*scene_, assetRoot_, settings_);
        prefab_ = CreatePrefabFromScene(*scene_, CreateResourceName("Prefab", "", "Prefab", ".prefab"));
        AddToResourceCache(prefab_);
    }

    void ImportRuntimeNode(Node& parent, const ufbx_node& sourceNode)
    {
        Node* node = parent.CreateChild(nodeNames_.at(&sourceNode));
        node->SetTransform(ConvertPosition(sourceNode.local_transform.translation),
            ConvertRotation(sourceNode.local_transform.rotation), ToVector3(sourceNode.local_transform.scale));
        node->SetEnabled(sourceNode.visible);
        runtimeNodes_[&sourceNode] = node;
        for (const ufbx_node* child : sourceNode.children)
            ImportRuntimeNode(*node, *child);
    }

    bool NeedsSkeletonDriver(const ufbx_node* root) const
    {
        return ea::count_if(models_.begin(), models_.end(), [root](const ImportedModel& model) {
            return !model.skipComponent_ && model.skin_.root_ == root;
        }) > 1;
    }

    ea::string GetRuntimeNodePath(const ufbx_node& node) const
    {
        StringVector parts;
        for (const ufbx_node* current = &node; current; current = current->parent)
        {
            parts.push_back(nodeNames_.at(current));
            if (ea::find(skeletonRoots_.begin(), skeletonRoots_.end(), current) != skeletonRoots_.end())
                parts.push_back(nodeNames_.at(current) + " Skeleton");
        }
        ea::reverse(parts.begin(), parts.end());
        return ea::string::joined(parts, "/");
    }

    ea::string GetSkeletonHostPath(const ufbx_node& root) const
    {
        StringVector parts{nodeNames_.at(&root) + " Skeleton"};
        for (const ufbx_node* current = root.parent; current; current = current->parent)
        {
            parts.push_back(nodeNames_.at(current));
            if (ea::find(skeletonRoots_.begin(), skeletonRoots_.end(), current) != skeletonRoots_.end())
                parts.push_back(nodeNames_.at(current) + " Skeleton");
        }
        ea::reverse(parts.begin(), parts.end());
        return ea::string::joined(parts, "/");
    }

    void InitializeComponentBindings()
    {
        std::unordered_map<const ufbx_node*, unsigned> nextSkeletonIndex;
        for (const ufbx_node* root : skeletonRoots_)
            nextSkeletonIndex[root] = NeedsSkeletonDriver(root) ? 1 : 0;
        std::unordered_map<const ufbx_node*, unsigned> nextMorphIndex;

        for (ImportedModel& imported : models_)
        {
            if (imported.skipComponent_)
                continue;
            if (imported.skin_.IsSkinned())
            {
                imported.componentIndex_ = nextSkeletonIndex[imported.skin_.root_]++;
                imported.componentPath_ = GetSkeletonHostPath(*imported.skin_.root_);
            }
            else if (!imported.morphTargets_.empty())
            {
                imported.componentIndex_ = nextMorphIndex[imported.sourceNode_]++;
                imported.componentPath_ = GetRuntimeNodePath(*imported.sourceNode_);
            }
        }
    }

    void AttachModels()
    {
        for (const ImportedModel& imported : models_)
        {
            if (!imported.skipComponent_ && imported.skin_.IsSkinned())
                GetSkeletonHost(*imported.skin_.root_);
        }

        for (const ufbx_node* root : skeletonRoots_)
        {
            const auto hostIter = skeletonHosts_.find(root);
            const auto modelIter = skeletonDriverModels_.find(root);
            if (hostIter == skeletonHosts_.end() || modelIter == skeletonDriverModels_.end())
                continue;

            Node* host = hostIter->second;
            auto driver = host->CreateComponent<AnimatedModel>();
            driver->SetModel(modelIter->second, false);
            driver->SetCastShadows(false);
            driver->SetUpdateInvisible(true);
            driver->SetAnimationLodBias(0.0f);
        }

        for (ImportedModel& imported : models_)
        {
            if (imported.skipComponent_)
                continue;

            Node* node = runtimeNodes_.at(imported.sourceNode_);
            if (imported.skin_.IsSkinned())
            {
                Node* host = GetSkeletonHost(*imported.skin_.root_);
                auto animatedModel = host->CreateComponent<AnimatedModel>();
                InitializeComponent(*animatedModel, imported);
            }
            else if (!imported.morphTargets_.empty())
            {
                auto animatedModel = node->CreateComponent<AnimatedModel>();
                InitializeComponent(*animatedModel, imported);
            }
            else
            {
                auto staticModel = node->CreateComponent<StaticModel>();
                InitializeComponent(*staticModel, imported);
            }
        }
    }

    Node* GetSkeletonHost(const ufbx_node& root)
    {
        if (const auto iter = skeletonHosts_.find(&root); iter != skeletonHosts_.end())
            return iter->second;

        Node* rootNode = runtimeNodes_.at(&root);
        Node* host = rootNode->GetParent()->CreateChild(nodeNames_.at(&root) + " Skeleton");
        rootNode->SetParent(host);
        skeletonHosts_[&root] = host;
        return host;
    }

    void AttachAnimationControllers()
    {
        const auto attach = [&](Node& node, const ufbx_node* group)
        {
            const auto groupIter = animationsByGroup_.find(group);
            if (groupIter == animationsByGroup_.end() || groupIter->second.empty())
                return;
            const auto animationIter = groupIter->second.find(0);
            CreateAnimationController(
                node, animationIter != groupIter->second.end() ? animationIter->second.Get() : nullptr);
        };
        attach(*assetRoot_, nullptr);
        for (const ufbx_node* root : skeletonRoots_)
        {
            if (const auto iter = skeletonHosts_.find(root); iter != skeletonHosts_.end())
                attach(*iter->second, root);
        }
    }

    void InitializeComponent(StaticModel& component, const ImportedModel& imported)
    {
        component.SetEnabled(imported.sourceNode_->visible);
        InitializeModelComponent(component, imported.model_, imported.materials_, imported.morphWeights_, false);
    }

    const ufbx_node* ResolvePrimaryNode(const FBXSource& source, const ufbx_node& sourceNode) const
    {
        if (&source == &sources_[0])
            return &sourceNode;
        const ea::string configuredName = GetConfiguredNodeName(sourceNode);
        const auto iter = primaryNodesByConfiguredName_.find(configuredName.c_str());
        return iter != primaryNodesByConfiguredName_.end() ? iter->second : nullptr;
    }

    void ImportAnimations()
    {
        unsigned sourceAnimationIndex = 0;
        for (FBXSource& source : sources_)
        {
            const unsigned numAnimations = source.scene_->anim_stacks.count;
            for (const ufbx_anim_stack* stack : source.scene_->anim_stacks)
            {
                ea::string nameHint = ToString(stack->name).c_str();
                if (!source.animationNameOverride_.empty() && numAnimations == 1)
                    nameHint = source.animationNameOverride_;
                const unsigned animationIndex = sourceAnimationIndex++;
                if (!IsNameSkipped(nameHint, settings_.skipTags_))
                    ImportAnimation(source, *stack, animationIndex, nameHint);
            }
        }
    }

    void ImportAnimation(
        FBXSource& source, const ufbx_anim_stack& stack, unsigned sourceAnimationIndex, const ea::string& nameHint)
    {
        ufbx_bake_opts options{};
        options.trim_start_time = true;
        options.resample_rate =
            source.scene_->settings.frames_per_second > 0.0 ? source.scene_->settings.frames_per_second : 30.0;
        options.key_reduction_enabled = true;
        options.key_reduction_rotation = true;
        ufbx_error error{};
        BakedAnimationPtr baked{ufbx_bake_anim(source.scene_.get(), stack.anim, &options, &error)};
        if (!baked)
            throw RuntimeException("Failed to bake FBX animation '{}': {}", nameHint, FormatUFBXError(error));

        std::unordered_map<const ufbx_node*, SharedPtr<Animation>> groupAnimations;
        const auto getAnimation = [&](const ufbx_node* group) -> Animation&
        {
            SharedPtr<Animation>& animation = groupAnimations[group];
            if (!animation)
                animation = MakeShared<Animation>(context_);
            return *animation;
        };
        ea::unordered_set<ea::string> ignoredNodes;
        for (const ufbx_baked_node& bakedNode : baked->nodes)
        {
            if (bakedNode.typed_id >= source.scene_->nodes.count)
                continue;
            const ufbx_node& sourceNode = *source.scene_->nodes.data[bakedNode.typed_id];
            const ufbx_node* primaryNode = ResolvePrimaryNode(source, sourceNode);
            if (!primaryNode)
            {
                ignoredNodes.emplace(GetConfiguredNodeName(sourceNode));
                continue;
            }
            bool importedAsBone = false;
            for (const auto& [root, boneNodes] : skeletonBoneNodes_)
            {
                if (boneNodes.find(primaryNode) == boneNodes.end())
                    continue;
                ImportBoneAnimation(getAnimation(root), *primaryNode, bakedNode);
                importedAsBone = true;
            }
            if (!importedAsBone)
                ImportSceneNodeAnimation(getAnimation(nullptr), *primaryNode, bakedNode);
        }
        ImportMorphAnimation(getAnimation, source, *baked, ignoredNodes);
        if (!ignoredNodes.empty())
        {
            const StringVector names{ignoredNodes.begin(), ignoredNodes.end()};
            URHO3D_LOGWARNING("Ignored nodes in animation '{}': {}", nameHint, ea::string::joined(names, ", "));
        }

        ea::vector<const ufbx_node*> nonEmptyGroups;
        for (const auto& [group, animation] : groupAnimations)
        {
            if (animation->GetNumTracks() > 0 || animation->GetNumVariantTracks() > 0)
                nonEmptyGroups.push_back(group);
        }
        ea::sort(nonEmptyGroups.begin(), nonEmptyGroups.end(), [&](const ufbx_node* lhs, const ufbx_node* rhs)
            { return GetSkeletonIndex(lhs).value_or(M_MAX_UNSIGNED) < GetSkeletonIndex(rhs).value_or(M_MAX_UNSIGNED); });
        for (const ufbx_node* group : nonEmptyGroups)
        {
            SharedPtr<Animation> animation = groupAnimations.at(group);
            if (group)
            {
                AddParentTrackMetadata(*animation, group);
                for (const ImportedModel& model : models_)
                {
                    if (model.skin_.root_ == group && model.modelView_)
                    {
                        animation->AddMetadata(AnimationMetadata::Model, model.modelView_->GetName());
                        break;
                    }
                }
            }
            animation->AddMetadata(AnimationMetadata::SourceAnimationIndex, sourceAnimationIndex);
            animation->SetLength(ea::max(GetAnimationLength(*animation), static_cast<float>(baked->playback_duration)));

            const ea::string groupNameHint = GetAnimationGroupNameHint(
                nameHint, GetSkeletonIndex(group), static_cast<unsigned>(nonEmptyGroups.size()));
            const ea::string resourceName = CreateResourceName(groupNameHint, "Animations/", "Animation", ".ani");
            animation->SetName(resourceName);
            animation->SetAnimationName(GetFileName(resourceName));
            animations_.push_back(animation);
            animationsByGroup_[group][sourceAnimationIndex] = animation;
        }
    }

    /// Return the index of the skeleton root. Return an empty value for null.
    ea::optional<unsigned> GetSkeletonIndex(const ufbx_node* root) const
    {
        if (!root)
            return ea::nullopt;
        return static_cast<unsigned>(ea::find(skeletonRoots_.begin(), skeletonRoots_.end(), root) - skeletonRoots_.begin());
    }

    void AddParentTrackMetadata(Animation& animation, const ufbx_node* root) const
    {
        const auto groupIter = skeletonBoneNodes_.find(root);
        if (groupIter == skeletonBoneNodes_.end())
            return;

        const std::unordered_set<const ufbx_node*>& boneNodes = groupIter->second;
        ea::vector<AnimatedBone> bones;
        std::unordered_map<const ufbx_node*, unsigned> nodeToBone;
        for (const ufbx_node* boneNode : boneNodes)
        {
            nodeToBone[boneNode] = static_cast<unsigned>(bones.size());
            bones.push_back({nodeNames_.at(boneNode)});
        }
        for (const ufbx_node* boneNode : boneNodes)
        {
            // The nearest parent bone can be above the nodes that are not bones.
            const ufbx_node* parent = boneNode->parent;
            while (parent && boneNodes.find(parent) == boneNodes.end())
                parent = parent->parent;
            if (parent)
                bones[nodeToBone.at(boneNode)].parentIndex_ = nodeToBone.at(parent);
        }

        const StringVariantMap parentTracks = GetParentTrackNames(animation, bones);
        if (!parentTracks.empty())
            animation.AddMetadata(AnimationMetadata::ParentTracks, parentTracks);
    }

    void ImportBoneAnimation(
        Animation& animation, const ufbx_node& primaryNode, const ufbx_baked_node& bakedNode)
    {
        ea::vector<ea::pair<float, Vector3>> positions;
        ea::vector<ea::pair<float, Quaternion>> rotations;
        ea::vector<ea::pair<float, Vector3>> scales;
        for (const ufbx_baked_vec3& key : bakedNode.translation_keys)
            positions.emplace_back(static_cast<float>(key.time), ConvertPosition(key.value));
        for (const ufbx_baked_quat& key : bakedNode.rotation_keys)
            rotations.emplace_back(static_cast<float>(key.time), ConvertRotation(key.value));
        for (const ufbx_baked_vec3& key : bakedNode.scale_keys)
            scales.emplace_back(static_cast<float>(key.time), ToVector3(key.value));
        AnimationChannelFlags channels;
        if (!positions.empty())
            channels |= CHANNEL_POSITION;
        if (!rotations.empty())
            channels |= CHANNEL_ROTATION;
        if (!scales.empty())
            channels |= CHANNEL_SCALE;
        if (channels)
        {
            AnimationTrack* track = animation.CreateTrack(nodeNames_.at(&primaryNode));
            track->CreateMerged(channels, positions, rotations, scales, settings_.keyFrameTimeError_);
        }
    }

    void ImportSceneNodeAnimation(
        Animation& animation, const ufbx_node& primaryNode, const ufbx_baked_node& bakedNode)
    {
        const ea::string nodePath = GetRuntimeNodePath(primaryNode);
        ImportVariantTrack(animation, GetNodeTrackPath(nodePath, CHANNEL_POSITION), bakedNode.translation_keys,
            [&](const ufbx_vec3& value) -> Variant { return ConvertPosition(value); });
        ImportVariantTrack(animation, GetNodeTrackPath(nodePath, CHANNEL_ROTATION), bakedNode.rotation_keys,
            [&](const ufbx_quat& value) -> Variant { return ConvertRotation(value); });
        ImportVariantTrack(animation, GetNodeTrackPath(nodePath, CHANNEL_SCALE), bakedNode.scale_keys,
            [&](const ufbx_vec3& value) -> Variant { return ToVector3(value); });
    }

    template <class T, class Convert>
    void ImportVariantTrack(
        Animation& animation, const ea::string& path, const T& keys, const Convert& convert)
    {
        if (keys.count == 0)
            return;
        VariantAnimationTrack* track = animation.CreateVariantTrack(path);
        track->interpolation_ = KeyFrameInterpolation::Linear;
        for (const auto& key : keys)
            track->AddKeyFrame({static_cast<float>(key.time), convert(key.value)});
        track->Commit();
    }

    static const ufbx_baked_prop* FindBakedBlendWeight(const ufbx_baked_anim& baked, const ufbx_blend_channel& channel)
    {
        for (const ufbx_baked_element& element : baked.elements)
        {
            if (element.element_id != channel.element_id)
                continue;
            for (const ufbx_baked_prop& prop : element.props)
            {
                if (ToString(prop.name) == UFBX_DeformPercent)
                    return &prop;
            }
        }
        return nullptr;
    }

    template <class GetAnimation>
    void ImportMorphAnimation(const GetAnimation& getAnimation, FBXSource& source, const ufbx_baked_anim& baked,
        ea::unordered_set<ea::string>& ignoredNodes)
    {
        for (const ufbx_node* sourceNode : source.scene_->nodes)
        {
            if (!sourceNode->mesh)
                continue;

            std::vector<MorphTarget> targets;
            for (const ufbx_blend_deformer* deformer : sourceNode->mesh->blend_deformers)
            {
                for (const ufbx_blend_channel* channel : deformer->channels)
                {
                    for (unsigned keyframeIndex = 0; keyframeIndex < channel->keyframes.count; ++keyframeIndex)
                    {
                        if (const ufbx_blend_shape* shape = channel->keyframes.data[keyframeIndex].shape)
                            targets.push_back({channel, keyframeIndex, shape});
                    }
                }
            }
            if (targets.empty())
                continue;

            const ufbx_node* primaryNode = ResolvePrimaryNode(source, *sourceNode);
            if (!primaryNode)
            {
                ignoredNodes.emplace(GetConfiguredNodeName(*sourceNode));
                continue;
            }
            const auto primaryModelIter = modelIndexByNode_.find(primaryNode);
            if (primaryModelIter == modelIndexByNode_.end())
                continue;
            ImportedModel& primaryModel = models_[primaryModelIter->second];
            if (targets.empty() || targets.size() != primaryModel.morphTargets_.size())
                continue;

            std::unordered_map<const ufbx_blend_channel*, const ufbx_baked_prop*> channelProps;
            ea::vector<double> times;
            for (const MorphTarget& target : targets)
            {
                if (channelProps.find(target.channel_) != channelProps.end())
                    continue;
                const ufbx_baked_prop* prop = FindBakedBlendWeight(baked, *target.channel_);
                channelProps[target.channel_] = prop;
                if (prop)
                {
                    for (const ufbx_baked_vec3& key : prop->keys)
                        times.push_back(key.time);
                }
            }
            if (times.empty())
                continue;
            ea::sort(times.begin(), times.end());
            times.erase(ea::unique(times.begin(), times.end()), times.end());

            const ufbx_node* group = primaryModel.skin_.IsSkinned() ? primaryModel.skin_.root_ : nullptr;
            Animation& animation = getAnimation(group);
            ea::vector<VariantAnimationTrack*> tracks;
            for (unsigned morphIndex = 0; morphIndex < targets.size(); ++morphIndex)
            {
                const ea::string path = GetMorphTrackPath(
                    group ? EMPTY_STRING : primaryModel.componentPath_, primaryModel.componentIndex_, morphIndex);
                VariantAnimationTrack* track = animation.CreateVariantTrack(path);
                track->interpolation_ = KeyFrameInterpolation::Linear;
                tracks.push_back(track);
            }
            for (const double timeValue : times)
            {
                const auto weights = EvaluateMorphWeights(targets, [&](const ufbx_blend_channel& channel)
                {
                    const ufbx_baked_prop* prop = channelProps.at(&channel);
                    return prop ? static_cast<float>(ufbx_evaluate_baked_vec3(prop->keys, timeValue).x * 0.01)
                                : static_cast<float>(channel.weight);
                });
                const float time = static_cast<float>(timeValue);
                for (unsigned morphIndex = 0; morphIndex < tracks.size(); ++morphIndex)
                    tracks[morphIndex]->AddKeyFrame({time, static_cast<float>(weights[morphIndex])});
            }
            for (VariantAnimationTrack* track : tracks)
                track->Commit();
        }
    }

    template <class GetWeight>
    static std::vector<double> EvaluateMorphWeights(
        const std::vector<MorphTarget>& morphTargets, const GetWeight& getWeight)
    {
        std::vector<double> result(morphTargets.size());
        size_t targetBegin = 0;
        while (targetBegin < morphTargets.size())
        {
            const ufbx_blend_channel& channel = *morphTargets[targetBegin].channel_;
            size_t targetEnd = targetBegin;
            while (targetEnd < morphTargets.size() && morphTargets[targetEnd].channel_ == &channel)
                ++targetEnd;
            const std::vector<float> channelWeights = EvaluateBlendChannel(channel, getWeight(channel));
            for (size_t i = targetBegin; i < targetEnd; ++i)
            {
                const unsigned keyframeIndex = morphTargets[i].keyframeIndex_;
                if (keyframeIndex < channelWeights.size())
                    result[i] = channelWeights[keyframeIndex];
            }
            targetBegin = targetEnd;
        }
        return result;
    }

    static std::vector<float> EvaluateBlendChannel(const ufbx_blend_channel& channel, float weight)
    {
        struct Endpoint
        {
            float target_{};
            int index_{-1};
        };
        std::vector<float> result(channel.keyframes.count);
        if (channel.keyframes.count == 0)
            return result;

        int lastNegative = -1;
        for (unsigned i = 0; i < channel.keyframes.count; ++i)
        {
            if (channel.keyframes.data[i].target_weight < 0.0)
                lastNegative = static_cast<int>(i);
        }
        Endpoint previous;
        Endpoint next;
        if (weight > 0.0f)
        {
            if (lastNegative >= 0)
                previous = {static_cast<float>(channel.keyframes.data[lastNegative].target_weight), lastNegative};
            for (unsigned i = static_cast<unsigned>(lastNegative + 1); i < channel.keyframes.count; ++i)
            {
                previous = next;
                next = {static_cast<float>(channel.keyframes.data[i].target_weight), static_cast<int>(i)};
                if (next.target_ > weight)
                    break;
            }
        }
        else
        {
            if (static_cast<unsigned>(lastNegative + 1) < channel.keyframes.count)
                previous = {
                    static_cast<float>(channel.keyframes.data[lastNegative + 1].target_weight), lastNegative + 1};
            for (int i = lastNegative; i >= 0; --i)
            {
                previous = next;
                next = {static_cast<float>(channel.keyframes.data[i].target_weight), i};
                if (next.target_ < weight)
                    break;
            }
        }
        const float delta = next.target_ - previous.target_;
        if (delta != 0.0f)
        {
            const float factor = (weight - previous.target_) / delta;
            if (previous.index_ >= 0)
                result[previous.index_] = 1.0f - factor;
            if (next.index_ >= 0)
                result[next.index_] = factor;
        }
        return result;
    }

    Context* const context_{};
    Material* const defaultMaterial_{};
    const FBXImporterSettings settings_;
    std::vector<FBXSource>& sources_;
    FBXImporterCallback* const callback_{};
    const ImportTransform transform_;

    ImportedResourceRegistry resources_;

    std::unordered_map<const ufbx_node*, ea::string> nodeNames_;
    std::unordered_map<std::string, const ufbx_node*> primaryNodesByConfiguredName_;
    std::unordered_set<const ufbx_node*> primaryBoneNodes_;
    std::unordered_map<const ufbx_node*, Node*> runtimeNodes_;
    std::unordered_map<const ufbx_node*, unsigned> modelIndexByNode_;
    std::unordered_map<const ufbx_node*, std::vector<const ufbx_node*>> skinBoneNodesByRoot_;
    std::unordered_map<const ufbx_mesh*, bool> hasUnweightedVertices_;
    std::unordered_map<const ufbx_node*, std::unordered_set<const ufbx_node*>> skeletonBoneNodes_;
    ea::vector<const ufbx_node*> skeletonRoots_;
    std::unordered_map<const ufbx_node*, Node*> skeletonHosts_;
    std::unordered_map<const ufbx_node*, SharedPtr<Model>> skeletonDriverModels_;

    ea::vector<ImportedModel> models_;
    ea::vector<SharedPtr<Model>> modelsToSave_;
    std::unordered_map<const ufbx_material*, ea::array<SharedPtr<Material>, NumMaterialVariants>> materials_;
    ea::vector<ImportedMaterial> importedMaterials_;
    std::unordered_map<const ufbx_texture*, SharedPtr<Texture2D>> sourceTextures_;
    ea::vector<ImportedTexture> textures_;
    std::unordered_map<const ufbx_texture*, std::shared_ptr<DecodedImage>> decodedTextures_;
    std::unordered_map<const ufbx_texture*, ea::string> resolvedTextureFiles_;
    StringVector textureSearchDirectories_;
    StringVector missingTextures_;
    bool textureSearchDirectoriesInitialized_{};
    StringVector textureFallbackDirectories_;
    bool textureFallbackDirectoriesInitialized_{};

    SharedPtr<Scene> scene_;
    Node* assetRoot_{};
    ea::vector<SharedPtr<Animation>> animations_;
    std::unordered_map<const ufbx_node*, std::unordered_map<unsigned, SharedPtr<Animation>>> animationsByGroup_;
    SharedPtr<PrefabResource> prefab_;
    bool prepared_{};
    bool finalized_{};
};

void LogWarnings(const ufbx_scene& scene)
{
    for (const ufbx_warning& warning : scene.metadata.warnings)
        URHO3D_LOGWARNING("ufbx: {} ({} occurrence(s))", ToString(warning.description).c_str(), warning.count);
}

} // namespace

class FBXImporter::Impl
{
public:
    std::vector<FBXSource> sources_;
    std::unique_ptr<FBXProcessor> processor_;
    /// Default material state. Created on the main thread because worker threads cannot reset a material.
    SharedPtr<Material> defaultMaterial_;
};

FBXImporter::FBXImporter(Context* context, const FBXImporterSettings& settings)
    : Object(context)
    , settings_(settings)
    , impl_(ea::make_unique<Impl>())
{
    impl_->defaultMaterial_ = MakeShared<Material>(context);
}

FBXImporter::~FBXImporter() = default;

bool FBXImporter::LoadFile(const ea::string& fileName)
{
    if (!impl_->sources_.empty() || impl_->processor_)
    {
        URHO3D_LOGERROR("Primary source model is already loaded");
        return false;
    }
    ufbx_load_opts options = GetLoadOptions(GetSourceFileSize(fileName));
    ufbx_error error{};
    ScenePtr scene{ufbx_load_file(fileName.c_str(), &options, &error)};
    if (!scene)
    {
        URHO3D_LOGERROR("Failed to load FBX file '{}': {}", fileName, FormatUFBXError(error));
        return false;
    }
    LogWarnings(*scene);
    auto fileSystem = context_->GetSubsystem<FileSystem>();
    const ea::string directory = GetPath(GetAbsolutePath(fileName, fileSystem->GetCurrentDir()));
    impl_->sources_.push_back({ea::move(scene), directory, {}});
    return true;
}

bool FBXImporter::LoadFileBinary(ConstByteSpan data)
{
    if (!impl_->sources_.empty() || impl_->processor_)
    {
        URHO3D_LOGERROR("Primary source model is already loaded");
        return false;
    }
    ufbx_load_opts options = GetLoadOptions(data.size());
    ufbx_error error{};
    ScenePtr scene{ufbx_load_memory(data.data(), data.size(), &options, &error)};
    if (!scene)
    {
        URHO3D_LOGERROR("Failed to load FBX file from memory: {}", FormatUFBXError(error));
        return false;
    }
    LogWarnings(*scene);
    impl_->sources_.push_back({ea::move(scene), {}, {}});
    return true;
}

bool FBXImporter::MergeFile(const ea::string& fileName, const ea::string& assetName)
{
    if (impl_->sources_.empty())
    {
        URHO3D_LOGERROR("Primary source model is not loaded");
        return false;
    }
    if (impl_->processor_)
    {
        URHO3D_LOGERROR("Source FBX model is already processed");
        return false;
    }
    ufbx_load_opts options = GetLoadOptions(GetSourceFileSize(fileName));
    ufbx_error error{};
    ScenePtr scene{ufbx_load_file(fileName.c_str(), &options, &error)};
    if (!scene)
    {
        URHO3D_LOGERROR("Failed to load secondary FBX file '{}': {}", fileName, FormatUFBXError(error));
        return false;
    }
    LogWarnings(*scene);
    impl_->sources_.push_back({ea::move(scene), {}, settings_.keepNamesOnMerge_ ? EMPTY_STRING : assetName});
    return true;
}

bool FBXImporter::IsAnimationOnly(unsigned sourceIndex) const
{
    if (sourceIndex >= impl_->sources_.size())
        return false;

    const ufbx_scene& scene = *impl_->sources_[sourceIndex].scene_;
    if (scene.anim_stacks.count == 0)
        return false;
    return ea::none_of(scene.nodes.begin(), scene.nodes.end(), [](const ufbx_node* node)
    {
        return node->mesh || ufbx_as_line_curve(node->attrib) || ufbx_as_nurbs_curve(node->attrib)
            || ufbx_as_nurbs_surface(node->attrib);
    });
}

unsigned FBXImporter::GetNumAnimations(unsigned sourceIndex) const
{
    return sourceIndex < impl_->sources_.size()
        ? static_cast<unsigned>(impl_->sources_[sourceIndex].scene_->anim_stacks.count)
        : 0;
}

bool FBXImporter::BeginProcess(
    const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback)
{
    try
    {
        if (impl_->sources_.empty())
            throw RuntimeException("Source FBX model is not loaded");
        if (impl_->processor_)
            throw RuntimeException("Source FBX model is already processed");
        impl_->processor_ = std::make_unique<FBXProcessor>(context_, settings_, impl_->sources_, outputPath,
            resourceNamePrefix, callback ? callback : &defaultCallback_, impl_->defaultMaterial_);
        impl_->processor_->Prepare();
        return true;
    }
    catch (const RuntimeException& e)
    {
        impl_->processor_.reset();
        URHO3D_LOGERROR("Failed to prepare FBX: {}", e.what());
        return false;
    }
}

bool FBXImporter::EndProcess()
{
    try
    {
        if (!impl_->processor_)
            throw RuntimeException("Imported assets weren't prepared");
        impl_->processor_->Finalize();
        return true;
    }
    catch (const RuntimeException& e)
    {
        impl_->processor_.reset();
        URHO3D_LOGERROR("Failed to finalize FBX: {}", e.what());
        return false;
    }
}

bool FBXImporter::Process(
    const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback)
{
    return BeginProcess(outputPath, resourceNamePrefix, callback) && EndProcess();
}

bool FBXImporter::SaveResources()
{
    try
    {
        if (!impl_->processor_)
            throw RuntimeException("Imported assets weren't cooked");
        impl_->processor_->SaveResources();
        return true;
    }
    catch (const RuntimeException& e)
    {
        URHO3D_LOGERROR("Failed to save FBX resources: {}", e.what());
        return false;
    }
}

StringVector FBXImporter::GetMissingTextures() const
{
    return impl_->processor_ ? impl_->processor_->GetMissingTextures() : StringVector{};
}

const FBXImporter::ResourceToFileNameMap& FBXImporter::GetSavedResources() const
{
    if (impl_->processor_)
        return impl_->processor_->GetResourceNames();
    URHO3D_LOGERROR("Imported assets weren't cooked");
    static const ResourceToFileNameMap emptyMap;
    return emptyMap;
}

Transform FBXImporter::ConvertTransform(const Transform& sourceTransform) const
{
    return ImportTransform{!settings_.mirrorX_, settings_.scale_}.ApplyToTransform(sourceTransform);
}

} // namespace Urho3D
