// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/FBXImporter.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Exception.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Light.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/ModelView.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Skybox.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Graphics/TextureCube.h"
#include "Urho3D/Graphics/Zone.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/RenderPipeline/RenderPipeline.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/Image.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/PrefabResource.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Utility/AnimationMetadata.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <regex>
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

constexpr unsigned MaxNameAssignTries = 64 * 1024;

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

Vector3 MirrorX(const Vector3& value)
{
    return {-value.x_, value.y_, value.z_};
}

Quaternion MirrorX(const Quaternion& value)
{
    return {value.w_, value.x_, -value.y_, -value.z_};
}

Matrix3x4 MirrorX(Matrix3x4 value)
{
    value.m01_ = -value.m01_;
    value.m10_ = -value.m10_;
    value.m02_ = -value.m02_;
    value.m20_ = -value.m20_;
    value.m03_ = -value.m03_;
    return value;
}

ea::string FormatUFBXError(const ufbx_error& error)
{
    char buffer[2048]{};
    ufbx_format_error(buffer, sizeof(buffer), &error);
    return buffer;
}

ufbx_load_opts GetLoadOptions()
{
    ufbx_load_opts options{};
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

bool IsNameSkipped(const ea::string& name, const StringVector& skipTags)
{
    return ea::any_of(
        skipTags.begin(), skipTags.end(), [&](const ea::string& tag) { return name.find(tag) != ea::string::npos; });
}

bool IsWordBorder(unsigned char first, unsigned char second)
{
    return std::isblank(first) || std::ispunct(first) || std::isalpha(first) != std::isalpha(second)
        || (std::islower(first) && std::isupper(second));
}

unsigned FindCommonPrefixWord(
    const ea::string& lhs, const ea::string& rhs, bool preserveNonEmptyLeft = true, bool preserveNonEmptyRight = true)
{
    if (lhs.empty() || rhs.empty())
        return 0;

    const unsigned minLength =
        ea::min(lhs.size() - (preserveNonEmptyLeft ? 1 : 0), rhs.size() - (preserveNonEmptyRight ? 1 : 0));
    auto iter = ea::mismatch(lhs.begin(), ea::next(lhs.begin(), minLength), rhs.begin()).first;
    while (iter != lhs.begin())
    {
        const auto previous = ea::prev(iter);
        if (IsWordBorder(*previous, *iter))
            break;
        --iter;
    }
    return static_cast<unsigned>(iter - lhs.begin());
}

unsigned FindCommonPrefixWord(const StringVector& strings)
{
    if (strings.size() <= 1)
        return 0;

    ea::string prefix = strings[0].substr(0, FindCommonPrefixWord(strings[0], strings[1]));
    for (unsigned i = 2; i < strings.size(); ++i)
        prefix = prefix.substr(0, FindCommonPrefixWord(prefix, strings[i], false));
    return prefix.size();
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

struct ImagePixels
{
    int width_{};
    int height_{};
    bool twoChannel_{};
    std::vector<unsigned char> rgba_;
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
    ea::string techniqueName_;
    ea::string fallbackTechniqueName_;
    MaterialQuality minQuality_{};
};

struct ImportedModel
{
    const ufbx_node* sourceNode_{};
    ea::string meshName_;
    ea::string baseMeshName_;
    ea::optional<float> lodDistance_;
    ea::optional<unsigned> lodOrder_;
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
    enum MaterialVariant
    {
        LitNormalMapMaterial,
        LitMaterial,
        UnlitMaterial,

        NumMaterialVariants
    };

public:
    FBXProcessor(Context* context, const FBXImporterSettings& settings, std::vector<FBXSource>& sources,
        const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback)
        : context_(context)
        , settings_(settings)
        , sources_(sources)
        , outputPath_(outputPath)
        , resourceNamePrefix_(resourceNamePrefix)
        , callback_(callback)
        , mirrorX_(!settings.mirrorX_)
    {
        if (sources_.empty() || !sources_[0].scene_)
            throw RuntimeException("Source FBX model is not loaded");
    }

    ~FBXProcessor() { ReleaseManualResources(); }

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

    const FBXImporter::ResourceToFileNameMap& GetResourceNames() const { return resourceNames_; }

private:
    void ReleaseManualResources()
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        for (const auto& [type, name] : manualResources_)
            cache->ReleaseResource(type, name, true);
        manualResources_.clear();
    }

    ea::string CreateResourceName(
        const ea::string& nameHint, const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string body = !nameHint.empty() ? GetSanitizedName(nameHint) : defaultName;
        for (unsigned i = 0; i < MaxNameAssignTries; ++i)
        {
            const ea::string localName =
                i == 0 ? Format("{}{}{}", prefix, body, suffix) : Format("{}{}_{}{}", prefix, body, i, suffix);
            if (!localResourceNames_.emplace(localName).second)
                continue;

            const ea::string resourceName = resourceNamePrefix_ + localName;
            resourceNames_[resourceName] = outputPath_ + localName;
            return resourceName;
        }
        throw RuntimeException("Cannot assign resource name");
    }

    const ea::string& GetAbsoluteFileName(const ea::string& resourceName) const
    {
        const auto iter = resourceNames_.find(resourceName);
        return iter != resourceNames_.end() ? iter->second : EMPTY_STRING;
    }

    void AddToResourceCache(Resource* resource)
    {
        resourcesToCache_.push_back(SharedPtr<Resource>{resource});
        resource->SetAbsoluteFileName(GetAbsoluteFileName(resource->GetName()));
    }

    void CommitResources()
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        for (const SharedPtr<Resource>& resource : resourcesToCache_)
        {
            cache->AddManualResource(resource);
            manualResources_.emplace_back(resource->GetType(), resource->GetName());
        }
        resourcesToCache_.clear();
    }

    void FinalizeMaterials()
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        for (const ImportedMaterial& imported : importedMaterials_)
        {
            imported.material_->SetNumTechniques(imported.fallbackTechniqueName_.empty() ? 1 : 2);
            Technique* technique = cache->GetResource<Technique>(imported.techniqueName_);
            if (!technique)
                throw RuntimeException("Cannot find standard technique '{}'", imported.techniqueName_);
            imported.material_->SetTechnique(0, technique, imported.minQuality_);
            if (!imported.fallbackTechniqueName_.empty())
            {
                Technique* fallbackTechnique = cache->GetResource<Technique>(imported.fallbackTechniqueName_);
                if (!fallbackTechnique)
                    throw RuntimeException("Cannot find standard technique '{}'", imported.fallbackTechniqueName_);
                imported.material_->SetTechnique(1, fallbackTechnique, QUALITY_LOW);
            }
        }
    }

    void SaveResource(Resource* resource)
    {
        if (!resource)
            return;
        const ea::string& fileName = GetAbsoluteFileName(resource->GetName());
        if (fileName.empty())
            throw RuntimeException(
                "Cannot save imported resource '{}': file name is not assigned", resource->GetName());
        resource->SetAbsoluteFileName(fileName);
        if (!resource->SaveFile(fileName))
            throw RuntimeException("Cannot save imported resource '{}'", resource->GetName());
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
        std::unordered_map<std::string, unsigned> nameCounts;
        const ufbx_scene& scene = *sources_[0].scene_;
        for (const ufbx_node* node : scene.nodes)
        {
            const ea::string baseName = GetConfiguredNodeName(*node);
            unsigned& count = nameCounts[baseName.c_str()];
            const ea::string uniqueName = count++ == 0 ? baseName : Format("{}_{}", baseName, count);
            nodeNames_[node] = uniqueName;
            primaryNodesByConfiguredName_.emplace(baseName.c_str(), node);
        }
    }

    Vector3 ConvertPosition(const ufbx_vec3& value) const
    {
        Vector3 result = ToVector3(value) * settings_.scale_;
        return mirrorX_ ? MirrorX(result) : result;
    }

    Quaternion ConvertRotation(const ufbx_quat& value) const
    {
        const Quaternion result = ToQuaternion(value);
        return mirrorX_ ? MirrorX(result) : result;
    }

    Matrix3x4 ConvertOffsetMatrix(const ufbx_matrix& value) const
    {
        Matrix3x4 result = ToMatrix3x4(value);
        if (mirrorX_)
            result = MirrorX(result);
        if (settings_.scale_ != 1.0f)
            result = Matrix3x4::FromScale(settings_.scale_) * result * Matrix3x4::FromScale(1.0f / settings_.scale_);
        return result;
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

    std::vector<const ufbx_node*> CollectSkinBoneNodes(const ufbx_node& root, const ufbx_mesh* sourceMesh)
    {
        std::unordered_set<const ufbx_node*> included{&root};
        const auto includePath = [&](const ufbx_node* node)
        {
            for (const ufbx_node* current = node; current && current != &root; current = current->parent)
                included.emplace(current);
        };
        const auto includeMesh = [&](const ufbx_mesh& mesh)
        {
            for (const ufbx_skin_deformer* skin : mesh.skin_deformers)
            {
                for (const ufbx_skin_cluster* cluster : skin->clusters)
                {
                    if (cluster->bone_node && HasPositiveWeights(*cluster))
                        includePath(cluster->bone_node);
                }
            }
        };

        if (sourceMesh)
            includeMesh(*sourceMesh);
        else
        {
            for (const ufbx_node* node : sources_[0].scene_->nodes)
            {
                if (node->mesh && GetSkinRoot(*node->mesh) == &root)
                    includeMesh(*node->mesh);
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

        const ufbx_node* root = GetSkinRoot(*meshNode.mesh);
        if (!root)
            return result;
        result.root_ = root;
        result.boneNodes_ = CollectSkinBoneNodes(*root, meshNode.mesh);

        std::unordered_map<const ufbx_node*, unsigned> nodeToBone;
        for (unsigned i = 0; i < result.boneNodes_.size(); ++i)
            nodeToBone[result.boneNodes_[i]] = i;

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

        static const std::regex lodPattern{R"(^(.*)_LOD(\d+(\.\d+)?)$)"};
        std::cmatch match;
        if (std::regex_match(model.meshName_.c_str(), match, lodPattern))
        {
            model.baseMeshName_ = ea::string{match[1].first, match[1].second};
            model.lodDistance_ = ToFloat(ea::string{match[2].first, match[2].second});
            return;
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
        if (lhs.root_ != rhs.root_ || lhs.bones_.size() != rhs.bones_.size())
            return false;
        for (unsigned i = 0; i < lhs.bones_.size(); ++i)
        {
            const BoneView& lhsBone = lhs.bones_[i];
            const BoneView& rhsBone = rhs.bones_[i];
            if (lhsBone.name_ != rhsBone.name_ || lhsBone.parentIndex_ != rhsBone.parentIndex_
                || !lhsBone.offsetMatrix_.Equals(rhsBone.offsetMatrix_, settings_.offsetMatrixError_))
                return false;
        }
        return true;
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
            const float modelScale = ea::max(finalView->CalculateBoundingBox().Size().DotProduct(DOT_SCALE), M_EPSILON);
            const auto getDistance = [&](const ImportedModel& lod)
            {
                return lod.absoluteLODDistance_ ? *lod.lodDistance_ * Abs(settings_.scale_) / modelScale
                                                : *lod.lodDistance_;
            };
            auto& finalGeometries = finalView->GetGeometries();
            for (GeometryView& geometry : finalGeometries)
            {
                if (!geometry.lods_.empty())
                {
                    geometry.lods_.resize(1);
                    geometry.lods_.front().lodDistance_ = getDistance(*lods.front());
                }
            }
            for (unsigned lodIndex = 1; lodIndex < lods.size(); ++lodIndex)
            {
                const auto& sourceGeometries = lods[lodIndex]->modelView_->GetGeometries();
                if (sourceGeometries.size() > finalGeometries.size())
                    finalGeometries.resize(sourceGeometries.size());
                for (unsigned geometryIndex = 0; geometryIndex < sourceGeometries.size(); ++geometryIndex)
                {
                    if (sourceGeometries[geometryIndex].lods_.empty())
                        continue;
                    finalGeometries[geometryIndex].lods_.push_back(sourceGeometries[geometryIndex].lods_.front());
                    finalGeometries[geometryIndex].lods_.back().lodDistance_ = getDistance(*lods[lodIndex]);
                }
            }
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
            callback_->OnModelLoaded(*model.modelView_);
            const ea::vector<unsigned> materialMapping = SplitOversizedGeometries(*model.modelView_);
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
            const ea::vector<ea::vector<unsigned>> geometryBoneMappings = RemapGeometryBones(*model.modelView_);
            model.model_ = model.modelView_->ExportModel();
            if (!geometryBoneMappings.empty())
                model.model_->SetGeometryBoneMappings(geometryBoneMappings);
            AddToResourceCache(model.model_);
            modelsToSave_.push_back(model.model_);
            cookedViews[model.modelView_.Get()] = model.model_;
        }
        CookSkeletonDrivers();
    }

    static std::unordered_set<unsigned> CollectGeometryBones(const GeometryView& geometry, unsigned numBones)
    {
        std::unordered_set<unsigned> result;
        for (const GeometryLODView& lod : geometry.lods_)
        {
            if (lod.vertexFormat_.blendIndices_ == ModelVertexFormat::Undefined)
                continue;
            for (const ModelVertex& vertex : lod.vertices_)
            {
                for (const auto [boneIndex, weight] : vertex.GetBlendIndicesAndWeights())
                {
                    if (weight <= 0.0f)
                        continue;
                    if (boneIndex >= numBones)
                        throw RuntimeException("FBX geometry references invalid bone #{}", boneIndex);
                    result.emplace(boneIndex);
                }
            }
        }
        return result;
    }

    static unsigned GetPrimitiveSize(PrimitiveType primitiveType)
    {
        switch (primitiveType)
        {
        case TRIANGLE_LIST: return 3;
        case LINE_LIST: return 2;
        case POINT_LIST: return 1;
        default: return 0;
        }
    }

    static ea::vector<GeometryView> SplitGeometry(const GeometryView& source, unsigned numBones)
    {
        struct Batch
        {
            GeometryView geometry_;
            std::unordered_set<unsigned> bones_;
            std::vector<std::unordered_map<unsigned, unsigned>> vertexMappings_;
        };

        const unsigned maxBones = Graphics::GetMaxBones();
        std::vector<Batch> batches;
        const auto addBatch = [&]() -> unsigned
        {
            Batch& batch = batches.emplace_back();
            batch.geometry_.material_ = source.material_;
            batch.geometry_.exported_ = source.exported_;
            batch.geometry_.lods_.resize(source.lods_.size());
            batch.vertexMappings_.resize(source.lods_.size());
            batch.bones_.reserve(maxBones);
            for (unsigned i = 0; i < source.lods_.size(); ++i)
            {
                batch.geometry_.lods_[i].primitiveType_ = source.lods_[i].primitiveType_;
                batch.geometry_.lods_[i].lodDistance_ = source.lods_[i].lodDistance_;
                batch.geometry_.lods_[i].vertexFormat_ = source.lods_[i].vertexFormat_;
            }
            return batches.size() - 1;
        };

        for (unsigned lodIndex = 0; lodIndex < source.lods_.size(); ++lodIndex)
        {
            const GeometryLODView& sourceLod = source.lods_[lodIndex];
            if (sourceLod.indices_.empty())
                continue;
            const unsigned primitiveSize = GetPrimitiveSize(sourceLod.primitiveType_);
            if (primitiveSize == 0 || sourceLod.indices_.size() % primitiveSize != 0)
                throw RuntimeException(
                    "Cannot split oversized FBX geometry with primitive type {}", sourceLod.primitiveType_);

            for (unsigned primitiveBegin = 0; primitiveBegin < sourceLod.indices_.size();
                primitiveBegin += primitiveSize)
            {
                ea::array<unsigned, ModelVertex::MaxBones * 3> primitiveBones;
                unsigned numPrimitiveBones = 0;
                for (unsigned i = 0; i < primitiveSize; ++i)
                {
                    const unsigned vertexIndex = sourceLod.indices_[primitiveBegin + i];
                    if (vertexIndex >= sourceLod.vertices_.size())
                        throw RuntimeException("FBX geometry references invalid vertex #{}", vertexIndex);
                    if (sourceLod.vertexFormat_.blendIndices_ == ModelVertexFormat::Undefined)
                        continue;
                    for (const auto [boneIndex, weight] : sourceLod.vertices_[vertexIndex].GetBlendIndicesAndWeights())
                    {
                        if (weight <= 0.0f)
                            continue;
                        if (boneIndex >= numBones)
                            throw RuntimeException("FBX geometry references invalid bone #{}", boneIndex);
                        if (ea::find(primitiveBones.begin(), primitiveBones.begin() + numPrimitiveBones, boneIndex)
                            == primitiveBones.begin() + numPrimitiveBones)
                            primitiveBones[numPrimitiveBones++] = boneIndex;
                    }
                }

                unsigned bestBatch = M_MAX_UNSIGNED;
                unsigned bestOverlap = 0;
                for (unsigned batchIndex = 0; batchIndex < batches.size(); ++batchIndex)
                {
                    const Batch& batch = batches[batchIndex];
                    unsigned overlap = 0;
                    for (unsigned i = 0; i < numPrimitiveBones; ++i)
                        overlap += batch.bones_.find(primitiveBones[i]) != batch.bones_.end();
                    if (batch.bones_.size() + numPrimitiveBones - overlap <= maxBones
                        && (bestBatch == M_MAX_UNSIGNED || overlap > bestOverlap))
                    {
                        bestBatch = batchIndex;
                        bestOverlap = overlap;
                    }
                }
                if (bestBatch == M_MAX_UNSIGNED)
                    bestBatch = addBatch();

                Batch& batch = batches[bestBatch];
                for (unsigned i = 0; i < numPrimitiveBones; ++i)
                    batch.bones_.emplace(primitiveBones[i]);
                GeometryLODView& targetLod = batch.geometry_.lods_[lodIndex];
                auto& vertexMapping = batch.vertexMappings_[lodIndex];
                for (unsigned i = 0; i < primitiveSize; ++i)
                {
                    const unsigned sourceIndex = sourceLod.indices_[primitiveBegin + i];
                    const auto [iter, inserted] = vertexMapping.emplace(sourceIndex, targetLod.vertices_.size());
                    if (inserted)
                        targetLod.vertices_.push_back(sourceLod.vertices_[sourceIndex]);
                    targetLod.indices_.push_back(iter->second);
                }
            }
        }

        ea::vector<GeometryView> result;
        result.reserve(batches.size());
        for (Batch& batch : batches)
        {
            for (unsigned lodIndex = 0; lodIndex < source.lods_.size(); ++lodIndex)
            {
                const GeometryLODView& sourceLod = source.lods_[lodIndex];
                GeometryLODView& targetLod = batch.geometry_.lods_[lodIndex];
                const auto& vertexMapping = batch.vertexMappings_[lodIndex];
                for (const auto& [morphIndex, sourceMorphs] : sourceLod.morphs_)
                {
                    ModelVertexMorphVector targetMorphs;
                    for (const ModelVertexMorph& sourceMorph : sourceMorphs)
                    {
                        if (const auto iter = vertexMapping.find(sourceMorph.index_); iter != vertexMapping.end())
                        {
                            ModelVertexMorph targetMorph = sourceMorph;
                            targetMorph.index_ = iter->second;
                            targetMorphs.push_back(targetMorph);
                        }
                    }
                    if (!targetMorphs.empty())
                    {
                        NormalizeModelVertexMorphVector(targetMorphs);
                        targetLod.morphs_[morphIndex] = ea::move(targetMorphs);
                    }
                }
            }
            result.push_back(ea::move(batch.geometry_));
        }
        return result;
    }

    static ea::vector<unsigned> SplitOversizedGeometries(ModelView& modelView)
    {
        if (modelView.GetBones().size() <= Graphics::GetMaxBones())
            return {};

        ea::vector<GeometryView> sourceGeometries = ea::move(modelView.GetGeometries());
        ea::vector<GeometryView> result;
        ea::vector<unsigned> materialMapping;
        result.reserve(sourceGeometries.size());
        unsigned sourceMaterialIndex = 0;
        bool wasSplit = false;
        for (GeometryView& source : sourceGeometries)
        {
            const unsigned currentMaterialIndex = sourceMaterialIndex;
            if (source.exported_)
                ++sourceMaterialIndex;

            const unsigned numGeometryBones = CollectGeometryBones(source, modelView.GetBones().size()).size();
            if (!source.exported_ || numGeometryBones <= Graphics::GetMaxBones())
            {
                if (source.exported_)
                    materialMapping.push_back(currentMaterialIndex);
                result.push_back(ea::move(source));
                continue;
            }

            ea::vector<GeometryView> batches = SplitGeometry(source, modelView.GetBones().size());
            if (batches.empty())
                throw RuntimeException("Cannot split empty oversized FBX geometry");
            URHO3D_LOGINFO("Split FBX model '{}' geometry {} ({} bones) into {} draw batches", modelView.GetName(),
                currentMaterialIndex, numGeometryBones, batches.size());
            for (GeometryView& batch : batches)
            {
                materialMapping.push_back(currentMaterialIndex);
                result.push_back(ea::move(batch));
            }
            wasSplit = true;
        }
        modelView.SetGeometries(ea::move(result));
        return wasSplit ? materialMapping : ea::vector<unsigned>{};
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

    static ea::vector<ea::vector<unsigned>> RemapGeometryBones(ModelView& modelView)
    {
        if (modelView.GetBones().size() <= Graphics::GetMaxBones())
            return {};

        // Blend indices are bytes, so large skeletons need a compact palette for each geometry.
        ea::vector<ea::vector<unsigned>> mappings;
        ea::vector<ea::unordered_map<unsigned, unsigned>> localIndices;
        for (GeometryView& geometry : modelView.GetGeometries())
        {
            if (!geometry.exported_)
                continue;

            ea::vector<unsigned>& mapping = mappings.emplace_back();
            auto& indices = localIndices.emplace_back();
            for (const GeometryLODView& lod : geometry.lods_)
            {
                if (lod.vertexFormat_.blendIndices_ == ModelVertexFormat::Undefined)
                    continue;
                for (const ModelVertex& vertex : lod.vertices_)
                {
                    for (const auto [boneIndex, weight] : vertex.GetBlendIndicesAndWeights())
                    {
                        if (weight <= 0.0f)
                            continue;
                        if (boneIndex >= modelView.GetBones().size())
                            throw RuntimeException("FBX geometry references invalid bone #{}", boneIndex);
                        if (!indices.contains(boneIndex))
                        {
                            indices[boneIndex] = mapping.size();
                            mapping.push_back(boneIndex);
                        }
                    }
                }
            }
        }

        const auto oversized = ea::find_if(mappings.begin(), mappings.end(),
            [](const ea::vector<unsigned>& mapping) { return mapping.size() > Graphics::GetMaxBones(); });
        if (oversized != mappings.end())
            throw RuntimeException(
                "FBX geometry uses {} bones, but only {} are supported", oversized->size(), Graphics::GetMaxBones());

        unsigned exportedGeometryIndex = 0;
        for (GeometryView& geometry : modelView.GetGeometries())
        {
            if (!geometry.exported_)
                continue;
            const auto& indices = localIndices[exportedGeometryIndex++];
            for (GeometryLODView& lod : geometry.lods_)
            {
                if (lod.vertexFormat_.blendIndices_ == ModelVertexFormat::Undefined)
                    continue;
                for (ModelVertex& vertex : lod.vertices_)
                {
                    for (unsigned i = 0; i < ModelVertex::MaxBones; ++i)
                    {
                        if (vertex.blendWeights_[i] > 0.0f)
                            vertex.blendIndices_[i] = static_cast<float>(indices.at(vertex.blendIndices_[i]));
                    }
                }
            }
        }
        return mappings;
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
            ufbx_tessellate_surface_opts options{};
            options.span_subdivision_u = 4;
            options.span_subdivision_v = 4;
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

        if (mirrorX_)
            modelView->MirrorGeometriesX();
        if (settings_.scale_ != 1.0f)
            modelView->ScaleGeometries(settings_.scale_);
        modelView->CalculateMissingNormals(true);
        modelView->CalculateMissingTangents();
        modelView->RepairBoneWeights();
        modelView->RecalculateBoneBoundingBoxes();
        modelView->Normalize();
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
            for (size_t i = 0; i < numIndices; ++i)
            {
                const size_t pointIndex = usePointIndices ? line.point_indices.data[indexBegin + i] : indexBegin + i;
                if (pointIndex >= line.control_points.count)
                    throw RuntimeException(
                        "FBX line curve '{}' has an invalid point index", ToString(line.name).c_str());
                ModelVertex& vertex = lod.vertices_.emplace_back();
                vertex.SetPosition(ToVector3(line.control_points.data[pointIndex]));
                vertex.color_[0] = Vector4{ToVector3(line.color), 1.0f};
                lod.indices_.push_back(static_cast<unsigned>(i));
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
                vertex.SetPosition(ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_position, index)));

                Vector3 normal;
                if (hasNormals)
                {
                    normal = ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_normal, index)).Normalized();
                    vertex.SetNormal(normal);
                }
                if (hasTangents)
                {
                    const Vector3 tangent = ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_tangent, index)).Normalized();
                    const Vector3 bitangent =
                        ToVector3(ufbx_get_vertex_vec3(&mesh.vertex_bitangent, index)).Normalized();
                    vertex.tangent_ = {
                        tangent, normal.CrossProduct(tangent).DotProduct(bitangent) < 0.0f ? -1.0f : 1.0f};
                }
                for (unsigned uvIndex = 0; uvIndex < numUVs; ++uvIndex)
                    vertex.uv_[uvIndex] = {
                        ToVector2(ufbx_get_vertex_vec2(&mesh.uv_sets.data[uvIndex].vertex_uv, index)), Vector2::ZERO};
                for (unsigned colorIndex = 0; colorIndex < numColors; ++colorIndex)
                    vertex.color_[colorIndex] =
                        ToVector4(ufbx_get_vertex_vec4(&mesh.color_sets.data[colorIndex].vertex_color, index));
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
                        morph.positionDelta_ = ToVector3(shape.position_offsets.data[offsetIndex]) * offsetWeight;
                        if (offsetIndex < shape.normal_offsets.count)
                            morph.normalDelta_ = ToVector3(shape.normal_offsets.data[offsetIndex]) * offsetWeight;
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
                const unsigned boneIndex = deformer.clusterToBone_[weight.cluster_index];
                const auto iter = std::find_if(influences.begin(), influences.end(),
                    [=](const auto& influence) { return influence.first == boneIndex; });
                if (iter != influences.end())
                    iter->second += static_cast<float>(weight.weight);
                else
                    influences.emplace_back(boneIndex, static_cast<float>(weight.weight));
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
            vertex.blendWeights_.x_ = 1.0f;
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

        auto material = MakeShared<Material>(context_);
        ea::string techniqueName;
        ea::string fallbackTechniqueName;
        ea::string suffix;
        if (variant == UnlitMaterial)
        {
            techniqueName = transparent ? "Techniques/UnlitTransparent.xml" : "Techniques/UnlitOpaque.xml";
            suffix = "_Unlit";
        }
        else if (variant == LitNormalMapMaterial)
        {
            techniqueName = transparent ? (settings_.fadeTransparency_ ? "Techniques/LitTransparentFadeNormalMap.xml"
                                                                       : "Techniques/LitTransparentNormalMap.xml")
                                        : "Techniques/LitOpaqueNormalMap.xml";
            fallbackTechniqueName = transparent
                ? (settings_.fadeTransparency_ ? "Techniques/LitTransparentFade.xml" : "Techniques/LitTransparent.xml")
                : "Techniques/LitOpaque.xml";
            suffix = "_LitNormalMap";
        }
        else
        {
            techniqueName = transparent
                ? (settings_.fadeTransparency_ ? "Techniques/LitTransparentFade.xml" : "Techniques/LitTransparent.xml")
                : "Techniques/LitOpaque.xml";
            suffix = "_Lit";
        }
        importedMaterials_.push_back({material, techniqueName, fallbackTechniqueName,
            variant == LitNormalMapMaterial ? QUALITY_MEDIUM : QUALITY_LOW});
        if (variant != UnlitMaterial)
        {
            material->SetVertexShaderDefines("PBR ");
            material->SetPixelShaderDefines("PBR ");
        }
        if (sourceMaterial->features.double_sided.enabled)
        {
            material->SetCullMode(CULL_NONE);
            material->SetShadowCullMode(CULL_NONE);
        }

        material->SetShaderParameter(ShaderConsts::Material_MatDiffColor, baseColor.LinearToGamma().ToVector4());
        if (auto texture = GetBaseColorTexture(*sourceMaterial))
            material->SetTexture(ShaderResources::Albedo, texture);
        if (variant != UnlitMaterial)
        {
            material->SetShaderParameter(ShaderConsts::Material_Metallic,
                pbr.metalness.has_value ? static_cast<float>(pbr.metalness.value_real) : 0.0f);
            const ufbx_material_map& roughnessMap = GetRoughnessMap(*sourceMaterial);
            const SharedPtr<Texture2D> emissionTexture = GetTexture(pbr.emission_color);
            bool hasRoughnessTexture = false;
            const SharedPtr<Texture2D> propertiesTexture = GetPropertiesTexture(*sourceMaterial, hasRoughnessTexture);
            const bool glossinessTextureConverted =
                sourceMaterial->features.roughness_as_glossiness.enabled && hasRoughnessTexture;
            const float roughness = roughnessMap.has_value
                ? static_cast<float>(glossinessTextureConverted                      ? 1.0
                          : sourceMaterial->features.roughness_as_glossiness.enabled ? 1.0 - roughnessMap.value_real
                                                                                     : roughnessMap.value_real)
                : 1.0f;
            material->SetShaderParameter(ShaderConsts::Material_Roughness, roughness);
            const float emissionFactor =
                pbr.emission_factor.has_value ? static_cast<float>(pbr.emission_factor.value_real) : 1.0f;
            const Color emissive{static_cast<float>(pbr.emission_color.value_vec3.x) * emissionFactor,
                static_cast<float>(pbr.emission_color.value_vec3.y) * emissionFactor,
                static_cast<float>(pbr.emission_color.value_vec3.z) * emissionFactor};
            material->SetShaderParameter(ShaderConsts::Material_MatEmissiveColor, emissive.LinearToGamma().ToVector3());
            material->SetShaderParameter("OcclusionStrength",
                pbr.ambient_occlusion.has_value ? static_cast<float>(pbr.ambient_occlusion.value_real) : 1.0f);

            if (emissionTexture)
                material->SetTexture(ShaderResources::Emission, emissionTexture);
            if (propertiesTexture)
                material->SetTexture(ShaderResources::Properties, propertiesTexture);
        }
        if (variant == LitNormalMapMaterial)
        {
            material->SetShaderParameter(ShaderConsts::Material_NormalScale,
                pbr.normal_map.has_value ? static_cast<float>(pbr.normal_map.value_real) : 1.0f);
            material->SetTexture(ShaderResources::Normal, GetTexture(pbr.normal_map, true));
        }
        ApplyTextureTransform(*material, *sourceMaterial, variant);

        const ea::string materialName =
            CreateResourceName(ToString(sourceMaterial->name).c_str(), "Materials/", "Material", suffix + ".xml");
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

        const std::shared_ptr<ImagePixels> pixels = DecodeTexture(fileTexture);
        if (!pixels)
            return nullptr;
        const SharedPtr<Texture2D> texture = AddTexture(*pixels, textureMapping, normalMap);
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

        ImagePixels combined;
        combined.width_ = std::max(base ? base->width_ : 0, opacity->width_);
        combined.height_ = std::max(base ? base->height_ : 0, opacity->height_);
        combined.name_ = base ? base->name_ + " Base Color" : opacity->name_ + " Opacity";
        combined.rgba_.resize(static_cast<size_t>(combined.width_) * combined.height_ * 4);
        for (int y = 0; y < combined.height_; ++y)
        {
            for (int x = 0; x < combined.width_; ++x)
            {
                const size_t index = (static_cast<size_t>(y) * combined.width_ + x) * 4;
                for (unsigned channel = 0; channel < 3; ++channel)
                    combined.rgba_[index + channel] =
                        SampleTexture(base.get(), x, y, combined.width_, combined.height_, channel, 255);
                const unsigned baseAlpha = SampleTexture(base.get(), x, y, combined.width_, combined.height_, 3, 255);
                const unsigned opacityValue =
                    std::min<unsigned>(SampleTexture(opacity.get(), x, y, combined.width_, combined.height_, 0, 255),
                        SampleTexture(opacity.get(), x, y, combined.width_, combined.height_, 3, 255));
                combined.rgba_[index + 3] = static_cast<unsigned char>(baseAlpha * opacityValue / 255);
            }
        }
        return AddTexture(combined, baseMapping ? baseMapping : opacityMapping);
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

        ImagePixels packed;
        packed.width_ = std::max(
            {roughness ? roughness->width_ : 0, metalness ? metalness->width_ : 0, occlusion ? occlusion->width_ : 0});
        packed.height_ = std::max({roughness ? roughness->height_ : 0, metalness ? metalness->height_ : 0,
            occlusion ? occlusion->height_ : 0});
        packed.name_ = "Properties";
        packed.rgba_.resize(static_cast<size_t>(packed.width_) * packed.height_ * 4);
        const float roughnessFactor = roughnessMap.has_value ? static_cast<float>(roughnessMap.value_real) : 1.0f;
        for (int y = 0; y < packed.height_; ++y)
        {
            for (int x = 0; x < packed.width_; ++x)
            {
                const size_t index = (static_cast<size_t>(y) * packed.width_ + x) * 4;
                const unsigned char roughnessValue =
                    SampleTexture(roughness.get(), x, y, packed.width_, packed.height_, 0, 255);
                packed.rgba_[index + 0] = roughness && invertRoughness
                    ? static_cast<unsigned char>(Clamp(RoundToInt(255.0f - roughnessValue * roughnessFactor), 0, 255))
                    : roughnessValue;
                packed.rgba_[index + 1] = SampleTexture(metalness.get(), x, y, packed.width_, packed.height_, 0, 255);
                packed.rgba_[index + 2] = 255;
                packed.rgba_[index + 3] = SampleTexture(occlusion.get(), x, y, packed.width_, packed.height_, 0, 255);
            }
        }
        return AddTexture(packed, referenceMapping);
    }

    static unsigned char SampleTexture(const ImagePixels* image, int x, int y, int targetWidth, int targetHeight,
        unsigned channel, unsigned char defaultValue)
    {
        if (!image)
            return defaultValue;
        const int sourceX = std::min(image->width_ - 1, x * image->width_ / targetWidth);
        const int sourceY = std::min(image->height_ - 1, y * image->height_ / targetHeight);
        return image->rgba_[(static_cast<size_t>(sourceY) * image->width_ + sourceX) * 4 + channel];
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

        BuildTextureSearchDirectories(searchRoot);
        for (const ea::string& directory : textureSearchDirectories_)
        {
            const ea::string candidate = directory + fileName;
            if (fileSystem->FileExists(candidate))
                return RememberTextureFile(texture, candidate);
        }
        const ea::string lowerFileName = fileName.to_lower();
        for (const ea::string& directory : textureSearchDirectories_)
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
                    return RememberTextureFile(texture, directory + candidate);
            }
        }
        resolvedTextureFiles_[&texture] = {};
        return {};
    }

    ea::string RememberTextureFile(const ufbx_texture& texture, const ea::string& fileName)
    {
        resolvedTextureFiles_[&texture] = fileName;
        URHO3D_LOGINFO("Resolved FBX texture '{}' as '{}'", ToString(texture.filename).c_str(), fileName);
        return fileName;
    }

    void BuildTextureSearchDirectories(const ea::string& root)
    {
        if (textureSearchDirectoriesInitialized_ || root.empty())
            return;
        textureSearchDirectoriesInitialized_ = true;
        struct PendingDirectory
        {
            ea::string path_;
            unsigned depth_{};
        };
        constexpr unsigned MaxSearchDepth = 4;
        constexpr unsigned MaxSearchDirectories = 512;
        std::vector<PendingDirectory> pending;
        std::unordered_set<std::filesystem::path> visited;
        const auto addPending = [&](const ea::string& path, unsigned depth)
        {
            std::error_code error;
            std::filesystem::path canonical = std::filesystem::canonical(std::filesystem::u8path(path.c_str()), error);
            if (error)
                canonical = std::filesystem::u8path(ResolvePath(path).c_str());
            if (visited.emplace(std::move(canonical)).second)
                pending.push_back({AddTrailingSlash(path), depth});
        };
        addPending(root, 0);
        auto fileSystem = context_->GetSubsystem<FileSystem>();
        for (size_t index = 0; index < pending.size() && index < MaxSearchDirectories; ++index)
        {
            const PendingDirectory current = pending[index];
            textureSearchDirectories_.push_back(current.path_);
            if (current.depth_ >= MaxSearchDepth)
                continue;
            StringVector directories;
            fileSystem->ScanDir(directories, current.path_, "*", SCAN_DIRS);
            ea::sort(directories.begin(), directories.end(), [](const ea::string& lhs, const ea::string& rhs)
            {
                const auto score = [](const ea::string& path)
                {
                    const ea::string lower = path.to_lower();
                    return lower.contains("texture") || lower.contains("image") ? 0
                        : lower.contains("material")                            ? 1
                                                                                : 2;
                };
                const int lhsScore = score(lhs);
                const int rhsScore = score(rhs);
                return lhsScore != rhsScore ? lhsScore < rhsScore : lhs < rhs;
            });
            for (const ea::string& directory : directories)
            {
                if (pending.size() >= MaxSearchDirectories)
                    break;
                if (directory != "." && directory != "..")
                    addPending(current.path_ + directory, current.depth_ + 1);
            }
        }
    }

    std::shared_ptr<ImagePixels> DecodeTexture(const ufbx_texture* sourceTexture)
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
            decodedTextures_[sourceTexture] = {};
            return {};
        }

        MemoryBuffer buffer(encoded.data(), static_cast<unsigned>(encoded.size()));
        auto image = MakeShared<Image>(context_);
        if (!image->BeginLoad(buffer))
        {
            URHO3D_LOGWARNING("FBX texture '{}' has unsupported image data", ToString(sourceTexture->filename).c_str());
            decodedTextures_[sourceTexture] = {};
            return {};
        }
        const bool twoChannel = image->GetCompressedFormat() == TextureFormat::TEX_FORMAT_BC5_UNORM;
        const SharedPtr<Image> rgba = image->GetDecompressedImage();
        if (!rgba)
        {
            URHO3D_LOGWARNING("FBX texture '{}' could not be decompressed", ToString(sourceTexture->filename).c_str());
            decodedTextures_[sourceTexture] = {};
            return {};
        }

        auto result = std::make_shared<ImagePixels>();
        result->width_ = rgba->GetWidth();
        result->height_ = rgba->GetHeight();
        result->twoChannel_ = twoChannel;
        result->name_ = sourceTexture->name.length ? ToString(sourceTexture->name) : ToString(sourceTexture->filename);
        const size_t dataSize = static_cast<size_t>(result->width_) * result->height_ * 4;
        result->rgba_.assign(rgba->GetData(), rgba->GetData() + dataSize);
        decodedTextures_[sourceTexture] = result;
        return result;
    }

    SharedPtr<Texture2D> AddTexture(
        const ImagePixels& pixels, const ufbx_texture* sourceTexture, bool normalMap = false)
    {
        ImportedTexture& imported = textures_.emplace_back();
        imported.image_ = MakeShared<Image>(context_);
        imported.image_->SetSize(pixels.width_, pixels.height_, 4);
        imported.image_->SetData(pixels.rgba_.data());
        if (normalMap && pixels.twoChannel_)
        {
            unsigned char* data = imported.image_->GetData();
            for (size_t i = 0; i < pixels.rgba_.size(); i += 4)
            {
                const float x = data[i] / 127.5f - 1.0f;
                const float y = data[i + 1] / 127.5f - 1.0f;
                const float z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
                data[i + 2] = static_cast<unsigned char>(std::lround((z * 0.5f + 0.5f) * 255.0f));
            }
        }
        const ea::string imageName = CreateResourceName(pixels.name_.c_str(), "Textures/", "Texture", ".png");
        imported.image_->SetName(imageName);
        imported.image_->SetAbsoluteFileName(GetAbsoluteFileName(imageName));

        imported.texture_ = MakeShared<Texture2D>(context_);
        imported.texture_->SetName(imageName);
        const TextureAddressMode wrapU =
            sourceTexture && sourceTexture->wrap_u == UFBX_WRAP_CLAMP ? ADDRESS_CLAMP : ADDRESS_WRAP;
        const TextureAddressMode wrapV =
            sourceTexture && sourceTexture->wrap_v == UFBX_WRAP_CLAMP ? ADDRESS_CLAMP : ADDRESS_WRAP;
        imported.texture_->SetAddressMode(TextureCoordinate::U, wrapU);
        imported.texture_->SetAddressMode(TextureCoordinate::V, wrapV);
        if (wrapU != ADDRESS_WRAP || wrapV != ADDRESS_WRAP)
        {
            imported.sampler_ = MakeShared<XMLFile>(context_);
            XMLElement root = imported.sampler_->CreateRoot("texture");
            if (wrapU != ADDRESS_WRAP)
            {
                XMLElement child = root.CreateChild("address");
                child.SetAttribute("coord", "u");
                child.SetAttribute("mode", "clamp");
            }
            if (wrapV != ADDRESS_WRAP)
            {
                XMLElement child = root.CreateChild("address");
                child.SetAttribute("coord", "v");
                child.SetAttribute("mode", "clamp");
            }
            imported.sampler_->SetName(ReplaceExtension(imageName, ".xml"));
            imported.sampler_->SetAbsoluteFileName(ReplaceExtension(GetAbsoluteFileName(imageName), ".xml"));
        }
        return imported.texture_;
    }

    void CreatePrefab()
    {
        scene_ = MakeShared<Scene>(context_);
        scene_->CreateComponent<Octree>();
        auto renderPipeline = scene_->CreateComponent<RenderPipeline>();
        if (settings_.preview_.highRenderQuality_)
        {
            auto pipelineSettings = renderPipeline->GetSettings();
            pipelineSettings.renderBufferManager_.colorSpace_ = RenderPipelineColorSpace::LinearLDR;
            pipelineSettings.sceneProcessor_.pcfKernelSize_ = 5;
            renderPipeline->SetSettings(pipelineSettings);
            renderPipeline->SetRenderPassEnabled("Postprocess: FXAA v3", true);
        }

        assetRoot_ = scene_->CreateChild(settings_.assetName_);
        assetRoot_->SetRotation(settings_.rotation_);
        ImportRuntimeNode(*assetRoot_, *sources_[0].scene_->root_node);
        AttachModels();
        AttachAnimationControllers();

        if (!settings_.skipTags_.empty())
        {
            const auto children = assetRoot_->GetChildren(true);
            const ea::vector<WeakPtr<Node>> weakChildren(children.begin(), children.end());
            for (const auto& child : weakChildren)
            {
                if (child && IsNameSkipped(child->GetName(), settings_.skipTags_))
                    child->Remove();
            }
        }
        if (settings_.cleanupRootNodes_)
        {
            Node* newRoot = assetRoot_;
            while (newRoot->GetNumChildren() == 1 && newRoot->GetNumComponents() == 0)
                newRoot = newRoot->GetChild(0u);
            if (newRoot != assetRoot_)
            {
                newRoot->SetParent(scene_);
                newRoot->SetName(assetRoot_->GetName());
                assetRoot_->Remove();
                assetRoot_ = newRoot;
            }
        }

        InitializeDefaultSceneContent();
        prefab_ = MakeShared<PrefabResource>(context_);
        prefab_->GetMutableScenePrefab() = scene_->GeneratePrefab();
        prefab_->NormalizeIds();
        prefab_->SetName(CreateResourceName("Prefab", "", "Prefab", ".prefab"));
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
            auto controller = node.CreateComponent<AnimationController>();
            if (const auto animationIter = groupIter->second.find(0); animationIter != groupIter->second.end())
                controller->Play(animationIter->second->GetName(), 0, true);
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
        component.SetModel(imported.model_);
        component.SetCastShadows(true);
        for (unsigned i = 0; i < imported.materials_.size(); ++i)
            component.SetMaterial(i, imported.materials_[i]);
    }

    void InitializeComponent(AnimatedModel& component, const ImportedModel& imported)
    {
        component.SetEnabled(imported.sourceNode_->visible);
        component.SetModel(imported.model_, false);
        component.SetCastShadows(true);
        for (unsigned i = 0; i < imported.materials_.size(); ++i)
            component.SetMaterial(i, imported.materials_[i]);
        for (unsigned i = 0; i < imported.morphWeights_.size(); ++i)
            component.SetMorphWeight(i, imported.morphWeights_[i]);
        if (component.GetNumGeometries() == 0)
        {
            component.SetUpdateInvisible(true);
            component.SetAnimationLodBias(0.0f);
        }
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
        std::unordered_map<const ufbx_node*, float> groupLengths;
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
                ImportBoneAnimation(getAnimation(root), *primaryNode, bakedNode, groupLengths[root]);
                importedAsBone = true;
            }
            if (!importedAsBone)
                ImportSceneNodeAnimation(getAnimation(nullptr), *primaryNode, bakedNode, groupLengths[nullptr]);
        }
        ImportMorphAnimation(groupLengths, getAnimation, source, *baked, ignoredNodes);
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
        {
            const auto getIndex = [&](const ufbx_node* root)
            {
                return root ? static_cast<unsigned>(
                                  ea::find(skeletonRoots_.begin(), skeletonRoots_.end(), root) - skeletonRoots_.begin())
                            : M_MAX_UNSIGNED;
            };
            return getIndex(lhs) < getIndex(rhs);
        });
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
            animation->SetLength(ea::max(groupLengths[group], static_cast<float>(baked->playback_duration)));

            ea::string groupNameHint = nameHint;
            if (nonEmptyGroups.size() > 1)
            {
                if (group)
                {
                    const unsigned groupIndex = static_cast<unsigned>(
                        ea::find(skeletonRoots_.begin(), skeletonRoots_.end(), group) - skeletonRoots_.begin());
                    groupNameHint += Format("_{}", groupIndex);
                }
                else
                    groupNameHint += "_R";
            }
            const ea::string resourceName = CreateResourceName(groupNameHint, "Animations/", "Animation", ".ani");
            animation->SetName(resourceName);
            animation->SetAnimationName(GetFileName(resourceName));
            animations_.push_back(animation);
            animationsByGroup_[group][sourceAnimationIndex] = animation;
        }
    }

    void AddParentTrackMetadata(Animation& animation, const ufbx_node* root) const
    {
        StringVariantMap parentTracks;
        const auto groupIter = skeletonBoneNodes_.find(root);
        if (groupIter == skeletonBoneNodes_.end())
            return;
        const std::unordered_set<const ufbx_node*>& boneNodes = groupIter->second;
        for (const ufbx_node* boneNode : boneNodes)
        {
            const ea::string& boneName = nodeNames_.at(boneNode);
            if (!animation.GetTrack(boneName))
                continue;
            const ufbx_node* parent = boneNode->parent;
            while (parent && (boneNodes.find(parent) == boneNodes.end() || !animation.GetTrack(nodeNames_.at(parent))))
                parent = parent->parent;
            parentTracks[boneName] = parent ? Variant(nodeNames_.at(parent)) : Variant(EMPTY_STRING);
        }
        if (!parentTracks.empty())
            animation.AddMetadata(AnimationMetadata::ParentTracks, parentTracks);
    }

    void ImportBoneAnimation(
        Animation& animation, const ufbx_node& primaryNode, const ufbx_baked_node& bakedNode, float& length)
    {
        ea::vector<ea::pair<float, Vector3>> positions;
        ea::vector<ea::pair<float, Quaternion>> rotations;
        ea::vector<ea::pair<float, Vector3>> scales;
        for (const ufbx_baked_vec3& key : bakedNode.translation_keys)
        {
            positions.emplace_back(static_cast<float>(key.time), ConvertPosition(key.value));
            length = ea::max(length, static_cast<float>(key.time));
        }
        for (const ufbx_baked_quat& key : bakedNode.rotation_keys)
        {
            rotations.emplace_back(static_cast<float>(key.time), ConvertRotation(key.value));
            length = ea::max(length, static_cast<float>(key.time));
        }
        for (const ufbx_baked_vec3& key : bakedNode.scale_keys)
        {
            scales.emplace_back(static_cast<float>(key.time), ToVector3(key.value));
            length = ea::max(length, static_cast<float>(key.time));
        }
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
        Animation& animation, const ufbx_node& primaryNode, const ufbx_baked_node& bakedNode, float& length)
    {
        const ea::string nodePath = GetRuntimeNodePath(primaryNode);
        ImportVariantTrack(animation, nodePath + "/@/Position", bakedNode.translation_keys,
            [&](const ufbx_vec3& value) -> Variant { return ConvertPosition(value); }, length);
        ImportVariantTrack(animation, nodePath + "/@/Rotation", bakedNode.rotation_keys,
            [&](const ufbx_quat& value) -> Variant { return ConvertRotation(value); }, length);
        ImportVariantTrack(animation, nodePath + "/@/Scale", bakedNode.scale_keys,
            [&](const ufbx_vec3& value) -> Variant { return ToVector3(value); }, length);
    }

    template <class T, class Convert>
    void ImportVariantTrack(
        Animation& animation, const ea::string& path, const T& keys, const Convert& convert, float& length)
    {
        if (keys.count == 0)
            return;
        VariantAnimationTrack* track = animation.CreateVariantTrack(path);
        track->interpolation_ = KeyFrameInterpolation::Linear;
        for (const auto& key : keys)
        {
            const float time = static_cast<float>(key.time);
            track->AddKeyFrame({time, convert(key.value)});
            length = ea::max(length, time);
        }
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
    void ImportMorphAnimation(std::unordered_map<const ufbx_node*, float>& groupLengths,
        const GetAnimation& getAnimation, FBXSource& source, const ufbx_baked_anim& baked,
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
                const ea::string path = (group ? EMPTY_STRING : primaryModel.componentPath_)
                    + Format("/@AnimatedModel#{}/Morphs/{}", primaryModel.componentIndex_, morphIndex);
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
                groupLengths[group] = ea::max(groupLengths[group], time);
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

    void InitializeDefaultSceneContent()
    {
        static const Vector3 defaultPosition{-1.0f, 2.0f, 1.0f};
        auto cache = context_->GetSubsystem<ResourceCache>();
        if (settings_.preview_.addLights_ && !scene_->FindComponent<Light>())
        {
            Node* node = scene_->CreateChild("Default Light");
            node->SetPosition(defaultPosition);
            node->SetDirection({1.0f, -2.0f, -1.0f});
            auto light = node->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
            light->SetCastShadows(true);
        }
        if (settings_.preview_.addSkybox_ && !scene_->FindComponent<Skybox>())
        {
            auto skyboxMaterial = cache->GetResource<Material>(settings_.preview_.skyboxMaterial_);
            auto boxModel = cache->GetResource<Model>("Models/Box.mdl");
            if (skyboxMaterial && boxModel)
            {
                Node* node = scene_->CreateChild("Default Skybox");
                node->SetPosition(defaultPosition);
                auto skybox = node->CreateComponent<Skybox>();
                skybox->SetModel(boxModel);
                skybox->SetMaterial(skyboxMaterial);
            }
        }
        if (settings_.preview_.addReflectionProbe_ && !scene_->FindComponent<Zone>())
        {
            auto texture = cache->GetResource<TextureCube>(settings_.preview_.reflectionProbeCubemap_);
            if (texture)
            {
                Node* node = scene_->CreateChild("Default Zone");
                node->SetPosition(defaultPosition);
                auto zone = node->CreateComponent<Zone>();
                zone->SetBackgroundBrightness(0.5f);
                zone->SetZoneTexture(texture);
            }
        }
    }

    Context* const context_{};
    const FBXImporterSettings settings_;
    std::vector<FBXSource>& sources_;
    const ea::string outputPath_;
    const ea::string resourceNamePrefix_;
    FBXImporterCallback* const callback_{};
    const bool mirrorX_{};

    ea::unordered_set<ea::string> localResourceNames_;
    FBXImporter::ResourceToFileNameMap resourceNames_;
    ea::vector<SharedPtr<Resource>> resourcesToCache_;
    ea::vector<ea::pair<StringHash, ea::string>> manualResources_;

    std::unordered_map<const ufbx_node*, ea::string> nodeNames_;
    std::unordered_map<std::string, const ufbx_node*> primaryNodesByConfiguredName_;
    std::unordered_set<const ufbx_node*> primaryBoneNodes_;
    std::unordered_map<const ufbx_node*, Node*> runtimeNodes_;
    std::unordered_map<const ufbx_node*, unsigned> modelIndexByNode_;
    std::unordered_map<const ufbx_node*, std::vector<const ufbx_node*>> skinBoneNodesByRoot_;
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
    std::unordered_map<const ufbx_texture*, std::shared_ptr<ImagePixels>> decodedTextures_;
    std::unordered_map<const ufbx_texture*, ea::string> resolvedTextureFiles_;
    StringVector textureSearchDirectories_;
    bool textureSearchDirectoriesInitialized_{};

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
};

FBXImporter::FBXImporter(Context* context, const FBXImporterSettings& settings)
    : Object(context)
    , settings_(settings)
    , impl_(ea::make_unique<Impl>())
{
}

FBXImporter::~FBXImporter() = default;

bool FBXImporter::LoadFile(const ea::string& fileName)
{
    if (!impl_->sources_.empty() || impl_->processor_)
    {
        URHO3D_LOGERROR("Primary source model is already loaded");
        return false;
    }
    ufbx_load_opts options = GetLoadOptions();
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
    ufbx_load_opts options = GetLoadOptions();
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
    ufbx_load_opts options = GetLoadOptions();
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
            resourceNamePrefix, callback ? callback : &defaultCallback_);
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
    Vector3 position = sourceTransform.position_ * settings_.scale_;
    Quaternion rotation = sourceTransform.rotation_;
    if (!settings_.mirrorX_)
    {
        position = MirrorX(position);
        rotation = MirrorX(rotation);
    }
    return {position, rotation, sourceTransform.scale_};
}

} // namespace Urho3D
