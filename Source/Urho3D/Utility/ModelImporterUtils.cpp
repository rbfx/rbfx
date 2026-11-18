// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/ModelImporterUtils.h"

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
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderPipeline/RenderPipeline.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/Image.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/PrefabResource.h"
#include "Urho3D/Scene/Scene.h"

#include <cctype>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace ModelImporterUtils
{

namespace
{

bool IsWordBorder(unsigned char first, unsigned char second)
{
    // Blanks and punctuation are always word borders.
    if (std::isblank(first) || std::ispunct(first))
        return true;

    // A change of the symbol class is a word border.
    if (std::isalpha(first) != std::isalpha(second))
        return true;

    // A change from lower case to upper case is a word border.
    return std::islower(first) && std::isupper(second);
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

/// Return true if the vertex format has bone indices.
bool HasBlendIndices(const GeometryLODView& lod)
{
    return lod.vertexFormat_.blendIndices_ != ModelVertexFormat::Undefined;
}

std::unordered_set<unsigned> CollectGeometryBones(const GeometryView& geometry, unsigned numBones)
{
    std::unordered_set<unsigned> result;
    for (const GeometryLODView& lod : geometry.lods_)
    {
        if (!HasBlendIndices(lod))
            continue;
        for (const ModelVertex& vertex : lod.vertices_)
        {
            for (const auto [boneIndex, weight] : vertex.GetBlendIndicesAndWeights())
            {
                if (weight <= 0.0f)
                    continue;
                if (boneIndex >= numBones)
                    throw RuntimeException("Geometry references invalid bone #{}", boneIndex);
                result.emplace(boneIndex);
            }
        }
    }
    return result;
}

unsigned GetPrimitiveSize(PrimitiveType primitiveType)
{
    switch (primitiveType)
    {
    case TRIANGLE_LIST: return 3;
    case LINE_LIST: return 2;
    case POINT_LIST: return 1;
    default: return 0;
    }
}

ea::vector<GeometryView> SplitGeometry(const GeometryView& source, unsigned numBones)
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
            throw RuntimeException("Cannot split oversized geometry with primitive type {}", sourceLod.primitiveType_);

        for (unsigned primitiveBegin = 0; primitiveBegin < sourceLod.indices_.size(); primitiveBegin += primitiveSize)
        {
            ea::array<unsigned, ModelVertex::MaxBones * 3> primitiveBones;
            unsigned numPrimitiveBones = 0;
            for (unsigned i = 0; i < primitiveSize; ++i)
            {
                const unsigned vertexIndex = sourceLod.indices_[primitiveBegin + i];
                if (vertexIndex >= sourceLod.vertices_.size())
                    throw RuntimeException("Geometry references invalid vertex #{}", vertexIndex);
                if (!HasBlendIndices(sourceLod))
                    continue;
                for (const auto [boneIndex, weight] : sourceLod.vertices_[vertexIndex].GetBlendIndicesAndWeights())
                {
                    if (weight <= 0.0f)
                        continue;
                    if (boneIndex >= numBones)
                        throw RuntimeException("Geometry references invalid bone #{}", boneIndex);
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

} // namespace

bool IsNameSkipped(const ea::string& name, const StringVector& skipTags)
{
    return ea::any_of(
        skipTags.begin(), skipTags.end(), [&](const ea::string& tag) { return name.find(tag) != ea::string::npos; });
}

ea::string GetUniqueName(const ea::string& nameHint, const ea::function<bool(const ea::string& name)>& isNameUsed)
{
    for (unsigned i = 0; i < MaxNameAssignTries; ++i)
    {
        ea::string name = i == 0 ? nameHint : Format("{}_{}", nameHint, i);
        if (!isNameUsed(name))
            return name;
    }
    throw RuntimeException("Cannot assign a unique name for '{}'", nameHint);
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

Vector3 ImportTransform::ApplyToPosition(const Vector3& value) const
{
    const Vector3 result = value * scale_;
    return mirrorX_ ? MirrorX(result) : result;
}

Quaternion ImportTransform::ApplyToRotation(const Quaternion& value) const
{
    return mirrorX_ ? MirrorX(value) : value;
}

Matrix3x4 ImportTransform::ApplyToOffsetMatrix(const Matrix3x4& value) const
{
    const Matrix3x4 result = mirrorX_ ? MirrorX(value) : value;
    // Always multiply, also for the scale 1. The multiplication changes -0 to +0, and the old glTF output has +0.
    return Matrix3x4::FromScale(scale_) * result * Matrix3x4::FromScale(1.0f / scale_);
}

Transform ImportTransform::ApplyToTransform(const Transform& value) const
{
    return {ApplyToPosition(value.position_), ApplyToRotation(value.rotation_), value.scale_};
}

void FinalizeModelView(ModelView& modelView, const ImportTransform& transform)
{
    if (transform.mirrorX_)
        modelView.MirrorGeometriesX();
    if (transform.scale_ != 1.0f)
        modelView.ScaleGeometries(transform.scale_);
    modelView.CalculateMissingNormals(true);
    modelView.CalculateMissingTangents();
    modelView.RepairBoneWeights();
    modelView.RecalculateBoneBoundingBoxes();
    modelView.Normalize();
}

LodName ParseLodName(const ea::string& name)
{
    static const std::regex lodPattern{R"(^(.*)_LOD(\d+(\.\d+)?)$)"};

    LodName result;
    std::cmatch match;
    if (!std::regex_match(name.c_str(), match, lodPattern))
    {
        result.baseName_ = name;
        return result;
    }

    const ea::string number{match[2].first, match[2].second};
    result.baseName_ = ea::string{match[1].first, match[1].second};
    result.distance_ = ToFloat(number);
    if (!match[3].matched && number.length() <= 4)
        result.index_ = ToUInt(number);
    return result;
}

void CombineModelViewLODs(const ea::vector<ModelView*>& lods, const ea::vector<float>& distances)
{
    URHO3D_ASSERT(!lods.empty() && lods.size() == distances.size());

    auto& finalGeometries = lods[0]->GetGeometries();
    for (GeometryView& geometry : finalGeometries)
    {
        if (!geometry.lods_.empty())
        {
            geometry.lods_.resize(1);
            geometry.lods_.front().lodDistance_ = distances[0];
        }
    }

    // TODO: Handle materials more gracefully
    for (unsigned lodIndex = 1; lodIndex < lods.size(); ++lodIndex)
    {
        const auto& sourceGeometries = lods[lodIndex]->GetGeometries();
        if (sourceGeometries.size() > finalGeometries.size())
            finalGeometries.resize(sourceGeometries.size());
        for (unsigned geometryIndex = 0; geometryIndex < sourceGeometries.size(); ++geometryIndex)
        {
            if (sourceGeometries[geometryIndex].lods_.empty())
                continue;
            finalGeometries[geometryIndex].lods_.push_back(sourceGeometries[geometryIndex].lods_.front());
            finalGeometries[geometryIndex].lods_.back().lodDistance_ = distances[lodIndex];
        }
    }
}

bool AreBonesEquivalent(const ea::vector<BoneView>& lhs, const ea::vector<BoneView>& rhs, float offsetMatrixError)
{
    return ea::identical(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
        [&](const BoneView& lhsBone, const BoneView& rhsBone)
    {
        return lhsBone.name_ == rhsBone.name_ && lhsBone.parentIndex_ == rhsBone.parentIndex_
            && lhsBone.offsetMatrix_.Equals(rhsBone.offsetMatrix_, offsetMatrixError);
    });
}

ea::vector<unsigned> SplitOversizedGeometries(ModelView& modelView)
{
    const unsigned numBones = modelView.GetBones().size();
    if (numBones <= Graphics::GetMaxBones())
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

        const unsigned numGeometryBones = CollectGeometryBones(source, numBones).size();
        if (!source.exported_ || numGeometryBones <= Graphics::GetMaxBones())
        {
            if (source.exported_)
                materialMapping.push_back(currentMaterialIndex);
            result.push_back(ea::move(source));
            continue;
        }

        ea::vector<GeometryView> batches = SplitGeometry(source, numBones);
        if (batches.empty())
            throw RuntimeException("Cannot split empty oversized geometry");
        URHO3D_LOGINFO("Split model '{}' geometry {} ({} bones) into {} draw batches", modelView.GetName(),
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

ea::vector<ea::vector<unsigned>> RemapGeometryBones(ModelView& modelView)
{
    const unsigned numBones = modelView.GetBones().size();
    if (numBones <= Graphics::GetMaxBones())
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
            if (!HasBlendIndices(lod))
                continue;
            for (const ModelVertex& vertex : lod.vertices_)
            {
                for (const auto [boneIndex, weight] : vertex.GetBlendIndicesAndWeights())
                {
                    if (weight <= 0.0f)
                        continue;
                    if (boneIndex >= numBones)
                        throw RuntimeException("Geometry references invalid bone #{}", boneIndex);
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
            "Geometry uses {} bones, but only {} are supported", oversized->size(), Graphics::GetMaxBones());

    unsigned exportedGeometryIndex = 0;
    for (GeometryView& geometry : modelView.GetGeometries())
    {
        if (!geometry.exported_)
            continue;
        const auto& indices = localIndices[exportedGeometryIndex++];
        for (GeometryLODView& lod : geometry.lods_)
        {
            if (!HasBlendIndices(lod))
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

ExportedModel ExportModelView(ModelView& modelView, GLTFImporterCallback& callback)
{
    callback.OnModelLoaded(modelView);

    ExportedModel result;
    result.materialMapping_ = SplitOversizedGeometries(modelView);
    const ea::vector<ea::vector<unsigned>> geometryBoneMappings = RemapGeometryBones(modelView);
    result.model_ = modelView.ExportModel();
    if (!geometryBoneMappings.empty())
        result.model_->SetGeometryBoneMappings(geometryBoneMappings);
    return result;
}

ea::string GetAnimationGroupNameHint(
    const ea::string& animationName, ea::optional<unsigned> skeletonIndex, unsigned numGroups)
{
    if (numGroups <= 1)
        return animationName;
    return animationName + (skeletonIndex ? Format("_{}", *skeletonIndex) : ea::string{"_R"});
}

StringVariantMap GetParentTrackNames(const Animation& animation, const ea::vector<AnimatedBone>& bones)
{
    const auto& tracks = animation.GetTracks();
    const auto hasTrack = [&](const AnimatedBone& bone)
    { return !bone.name_.empty() && tracks.contains(StringHash{bone.name_}); };

    StringVariantMap result;
    for (const AnimatedBone& bone : bones)
    {
        if (!hasTrack(bone))
            continue;

        ea::optional<unsigned> parentIndex = bone.parentIndex_;
        while (parentIndex && !hasTrack(bones[*parentIndex]))
            parentIndex = bones[*parentIndex].parentIndex_;
        result[bone.name_] = parentIndex ? bones[*parentIndex].name_ : EMPTY_STRING;
    }
    return result;
}

namespace
{

template <class T> ea::optional<float> GetTrackLength(const T& track)
{
    if (track.keyFrames_.empty())
        return ea::nullopt;
    return track.keyFrames_.back().time_;
}

} // namespace

float GetAnimationLength(const Animation& animation)
{
    float length = 0.0f;
    for (const auto& [nameHash, track] : animation.GetTracks())
    {
        if (const auto trackLength = GetTrackLength(track))
            length = ea::max(length, *trackLength);
    }
    for (const auto& [nameHash, track] : animation.GetVariantTracks())
    {
        if (const auto trackLength = GetTrackLength(track))
            length = ea::max(length, *trackLength);
    }
    return length;
}

ea::string GetNodeTrackPath(const ea::string& nodePath, AnimationChannel channel)
{
    switch (channel)
    {
    case CHANNEL_POSITION: return nodePath + "/@/Position";
    case CHANNEL_ROTATION: return nodePath + "/@/Rotation";
    case CHANNEL_SCALE: return nodePath + "/@/Scale";
    default: throw RuntimeException("Invalid animation channel '{}'", channel);
    }
}

ea::string GetMorphTrackPath(const ea::string& nodePath, unsigned componentIndex, unsigned morphIndex)
{
    return nodePath + Format("/@AnimatedModel#{}/Morphs/{}", componentIndex, morphIndex);
}

SharedPtr<XMLFile> CreateTextureSamplerXML(
    Context* context, const TextureSamplerParams& params, const ea::string& imageName, const ea::string& imageFileName)
{
    static const ea::string addressModeNames[] = {
        "wrap",
        "mirror",
        "clamp",
        "", // The engine does not support the border mode.
    };

    static const ea::string filterModeNames[] = {
        "nearest",
        "bilinear",
        "trilinear",
        "anisotropic",
        "nearestanisotropic",
        "default",
    };

    auto xmlFile = MakeShared<XMLFile>(context);
    XMLElement rootElement = xmlFile->CreateRoot("texture");

    if (params.wrapU_ != ADDRESS_WRAP)
    {
        XMLElement childElement = rootElement.CreateChild("address");
        childElement.SetAttribute("coord", "u");
        childElement.SetAttribute("mode", addressModeNames[params.wrapU_]);
    }

    if (params.wrapV_ != ADDRESS_WRAP)
    {
        XMLElement childElement = rootElement.CreateChild("address");
        childElement.SetAttribute("coord", "v");
        childElement.SetAttribute("mode", addressModeNames[params.wrapV_]);
    }

    if (params.filterMode_ != FILTER_DEFAULT)
    {
        XMLElement childElement = rootElement.CreateChild("filter");
        childElement.SetAttribute("mode", filterModeNames[params.filterMode_]);
    }

    if (!params.mipmaps_)
    {
        XMLElement childElement = rootElement.CreateChild("mipmap");
        childElement.SetBool("enable", false);
    }

    // Do not create the file if all parameters have default values.
    if (!rootElement.GetChild())
        return nullptr;

    xmlFile->SetName(ReplaceExtension(imageName, ".xml"));
    xmlFile->SetAbsoluteFileName(ReplaceExtension(imageFileName, ".xml"));
    return xmlFile;
}

TexturePlaceholder CreateTexturePlaceholder(
    Context* context, const ea::string& imageName, const ea::string& imageFileName, const TextureSamplerParams& params)
{
    TexturePlaceholder result;
    result.texture_ = MakeShared<Texture2D>(context);
    result.texture_->SetName(imageName);
    result.texture_->SetFilterMode(params.filterMode_);
    result.texture_->SetAddressMode(TextureCoordinate::U, params.wrapU_);
    result.texture_->SetAddressMode(TextureCoordinate::V, params.wrapV_);
    result.sampler_ = CreateTextureSamplerXML(context, params, imageName, imageFileName);
    return result;
}

ea::string GetStandardTechniqueName(bool lit, bool normalMap, bool transparent, bool fadeTransparency)
{
    if (!lit)
        return transparent ? "Techniques/UnlitTransparent.xml" : "Techniques/UnlitOpaque.xml";
    if (!transparent)
        return normalMap ? "Techniques/LitOpaqueNormalMap.xml" : "Techniques/LitOpaque.xml";
    if (fadeTransparency)
        return normalMap ? "Techniques/LitTransparentFadeNormalMap.xml" : "Techniques/LitTransparentFade.xml";
    return normalMap ? "Techniques/LitTransparentNormalMap.xml" : "Techniques/LitTransparent.xml";
}

Technique* LoadStandardTechnique(ResourceCache* cache, const ea::string& name)
{
    Technique* technique = cache->GetResource<Technique>(name);
    if (!technique)
        throw RuntimeException("Cannot find standard technique '{}'", name);
    return technique;
}

ea::string GetMaterialVariantSuffix(MaterialVariant variant)
{
    switch (variant)
    {
    case LitNormalMapMaterial: return "_LitNormalMap";
    case LitMaterial: return "_Lit";
    case UnlitMaterial: return "_Unlit";
    default: return EMPTY_STRING;
    }
}

StandardMaterialTechniques GetStandardMaterialTechniques(
    MaterialVariant variant, bool transparent, bool fadeTransparency)
{
    const bool lit = variant != UnlitMaterial;
    const bool normalMap = variant == LitNormalMapMaterial;

    StandardMaterialTechniques result;
    result.name_ = GetStandardTechniqueName(lit, normalMap, transparent, fadeTransparency);
    if (normalMap)
    {
        result.fallbackName_ = GetStandardTechniqueName(lit, false, transparent, fadeTransparency);
        result.minQuality_ = QUALITY_MEDIUM;
    }
    return result;
}

void SetStandardMaterialTechniques(Material& material, const StandardMaterialTechniques& techniques, ResourceCache* cache)
{
    const bool hasFallback = !techniques.fallbackName_.empty();
    material.SetNumTechniques(hasFallback ? 2 : 1);
    material.SetTechnique(0, LoadStandardTechnique(cache, techniques.name_), techniques.minQuality_);
    if (hasFallback)
        material.SetTechnique(1, LoadStandardTechnique(cache, techniques.fallbackName_), QUALITY_LOW);
}

void SetStandardMaterialFlags(
    Material& material, MaterialVariant variant, bool doubleSided, const ea::string& extraDefines)
{
    const ea::string defines = variant != UnlitMaterial ? extraDefines + "PBR " : extraDefines;
    if (!defines.empty())
    {
        material.SetVertexShaderDefines(defines);
        material.SetPixelShaderDefines(defines);
    }

    if (doubleSided)
    {
        material.SetCullMode(CULL_NONE);
        material.SetShadowCullMode(CULL_NONE);
    }
}

void SetStandardMaterialParameters(
    Material& material, MaterialVariant variant, const StandardMaterialParameters& parameters)
{
    material.SetShaderParameter(ShaderConsts::Material_MatDiffColor, parameters.baseColor_.LinearToGamma().ToVector4());
    if (parameters.albedoTexture_)
        material.SetTexture(ShaderResources::Albedo, parameters.albedoTexture_);

    if (variant != UnlitMaterial)
    {
        material.SetShaderParameter(ShaderConsts::Material_Metallic, parameters.metallic_);
        material.SetShaderParameter(ShaderConsts::Material_Roughness, parameters.roughness_);
        material.SetShaderParameter(
            ShaderConsts::Material_MatEmissiveColor, parameters.emissiveColor_.LinearToGamma().ToVector3());
        if (parameters.occlusionStrength_)
            material.SetShaderParameter("OcclusionStrength", *parameters.occlusionStrength_);
        if (parameters.propertiesTexture_)
            material.SetTexture(ShaderResources::Properties, parameters.propertiesTexture_);
        if (parameters.emissionTexture_)
            material.SetTexture(ShaderResources::Emission, parameters.emissionTexture_);
    }

    if (variant == LitNormalMapMaterial)
    {
        material.SetShaderParameter(ShaderConsts::Material_NormalScale, parameters.normalScale_);
        if (parameters.normalTexture_)
            material.SetTexture(ShaderResources::Normal, parameters.normalTexture_);
    }
}

SharedPtr<Image> GetResizedRGBAImage(const Image& image, const IntVector2& size)
{
    SharedPtr<Image> result = image.IsCompressed() ? image.GetDecompressedImage() : image.ConvertToRGBA();
    if (!result)
        throw RuntimeException("Cannot convert image '{}' to RGBA format", image.GetName());
    if (result->GetSize().ToIntVector2() != size && !result->Resize(size.x_, size.y_))
        throw RuntimeException("Cannot resize image '{}'", image.GetName());
    return result;
}

SharedPtr<Image> PackImageChannels(Context* context, const ea::array<ImageChannelSource, 4>& channels)
{
    IntVector2 size;
    for (const ImageChannelSource& channel : channels)
    {
        if (channel.image_)
            size = VectorMax(size, channel.image_->GetSize().ToIntVector2());
    }
    if (size.x_ <= 0 || size.y_ <= 0)
        return nullptr;

    // Convert each image only once, even if it gives more than one channel.
    ea::unordered_map<const Image*, SharedPtr<Image>> resizedImages;
    for (const ImageChannelSource& channel : channels)
    {
        if (channel.image_ && !resizedImages.contains(channel.image_))
            resizedImages[channel.image_] = GetResizedRGBAImage(*channel.image_, size);
    }

    auto result = MakeShared<Image>(context);
    result->SetSize(size.x_, size.y_, 4);
    unsigned char* data = result->GetData();
    const unsigned numPixels = static_cast<unsigned>(size.x_ * size.y_);
    for (unsigned channelIndex = 0; channelIndex < 4; ++channelIndex)
    {
        const ImageChannelSource& channel = channels[channelIndex];
        const unsigned char* source = channel.image_ ? resizedImages[channel.image_]->GetData() : nullptr;
        for (unsigned i = 0; i < numPixels; ++i)
            data[i * 4 + channelIndex] = source ? source[i * 4 + channel.channel_] : channel.defaultValue_;
    }
    return result;
}

SharedPtr<Image> PackPropertiesImage(
    Context* context, const ImageChannelSource& roughness, const ImageChannelSource& metallic, const ImageChannelSource& occlusion)
{
    return PackImageChannels(context, {roughness, metallic, ImageChannelSource{}, occlusion});
}

void TweakEmptyAnimatedModel(AnimatedModel& animatedModel)
{
    if (animatedModel.GetNumGeometries() == 0)
    {
        animatedModel.SetUpdateInvisible(true);
        animatedModel.SetAnimationLodBias(0.0f);
    }
}

void InitializeModelComponent(StaticModel& component, Model* model, const ea::vector<SharedPtr<Material>>& materials,
    const ea::vector<float>& morphWeights, bool createBones)
{
    auto animatedModel = component.Cast<AnimatedModel>();
    if (animatedModel)
        animatedModel->SetModel(model, createBones);
    else
        component.SetModel(model);
    component.SetCastShadows(true);
    for (unsigned i = 0; i < materials.size(); ++i)
        component.SetMaterial(i, materials[i]);

    if (animatedModel)
    {
        for (unsigned i = 0; i < morphWeights.size(); ++i)
            animatedModel->SetMorphWeight(i, morphWeights[i]);
        TweakEmptyAnimatedModel(*animatedModel);
    }
}

void CreateAnimationController(Node& node, Animation* defaultAnimation)
{
    auto animationController = node.CreateComponent<AnimationController>();
    if (defaultAnimation)
        animationController->Play(defaultAnimation->GetName(), 0, true);
}

SharedPtr<Scene> CreateImportScene(Context* context, const GLTFImporterSettings& settings)
{
    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<Octree>();

    auto renderPipeline = scene->CreateComponent<RenderPipeline>();
    if (settings.preview_.highRenderQuality_)
    {
        auto pipelineSettings = renderPipeline->GetSettings();
        pipelineSettings.renderBufferManager_.colorSpace_ = RenderPipelineColorSpace::LinearLDR;
        pipelineSettings.sceneProcessor_.pcfKernelSize_ = 5;
        renderPipeline->SetSettings(pipelineSettings);
        renderPipeline->SetRenderPassEnabled("Postprocess: FXAA v3", true);
    }
    return scene;
}

namespace
{

void AddDefaultSceneContent(Scene& scene, const GLTFImporterSettings::PreviewSettings& settings)
{
    static const Vector3 defaultPosition{-1.0f, 2.0f, 1.0f};
    static const ea::string skyboxModelName = "Models/Box.mdl";

    auto cache = scene.GetSubsystem<ResourceCache>();

    if (settings.addLights_ && !scene.FindComponent<Light>())
    {
        // The model forward is Z+. The default light comes from the top right of the model front side.
        Node* node = scene.CreateChild("Default Light");
        node->SetPosition(defaultPosition);
        node->SetDirection({1.0f, -2.0f, -1.0f});
        auto light = node->CreateComponent<Light>();
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetCastShadows(true);
    }

    if (settings.addSkybox_ && !scene.FindComponent<Skybox>())
    {
        auto skyboxMaterial = cache->GetResource<Material>(settings.skyboxMaterial_);
        auto boxModel = cache->GetResource<Model>(skyboxModelName);

        if (!skyboxMaterial)
            URHO3D_LOGWARNING("Cannot add default skybox with material '{}'", settings.skyboxMaterial_);
        else if (!boxModel)
            URHO3D_LOGWARNING("Cannot add default skybox with model '{}'", skyboxModelName);
        else
        {
            Node* node = scene.CreateChild("Default Skybox");
            node->SetPosition(defaultPosition);
            auto skybox = node->CreateComponent<Skybox>();
            skybox->SetModel(boxModel);
            skybox->SetMaterial(skyboxMaterial);
        }
    }

    if (settings.addReflectionProbe_ && !scene.FindComponent<Zone>())
    {
        auto texture = cache->GetResource<TextureCube>(settings.reflectionProbeCubemap_);
        if (!texture)
            URHO3D_LOGWARNING("Cannot add default reflection probe with texture '{}'", settings.reflectionProbeCubemap_);
        else
        {
            Node* node = scene.CreateChild("Default Zone");
            node->SetPosition(defaultPosition);
            auto zone = node->CreateComponent<Zone>();
            zone->SetBackgroundBrightness(0.5f);
            zone->SetZoneTexture(texture);
        }
    }
}

} // namespace

Node* FinalizeImportScene(Scene& scene, Node* assetRoot, const GLTFImporterSettings& settings)
{
    if (!settings.skipTags_.empty())
    {
        const auto children = assetRoot->GetChildren(true);
        const ea::vector<WeakPtr<Node>> weakChildren(children.begin(), children.end());
        for (const auto& child : weakChildren)
        {
            if (child && IsNameSkipped(child->GetName(), settings.skipTags_))
                child->Remove();
        }
    }

    if (settings.cleanupRootNodes_)
    {
        Node* newRoot = assetRoot;
        while (newRoot->GetNumChildren() == 1 && newRoot->GetNumComponents() == 0)
            newRoot = newRoot->GetChild(0u);

        if (newRoot != assetRoot)
        {
            newRoot->SetParent(&scene);
            newRoot->SetName(assetRoot->GetName());
            assetRoot->Remove();
            assetRoot = newRoot;
        }
    }

    AddDefaultSceneContent(scene, settings.preview_);
    return assetRoot;
}

SharedPtr<PrefabResource> CreatePrefabFromScene(Scene& scene, const ea::string& name)
{
    auto prefab = MakeShared<PrefabResource>(scene.GetContext());
    prefab->GetMutableScenePrefab() = scene.GeneratePrefab();
    prefab->NormalizeIds();
    prefab->SetName(name);
    return prefab;
}

ImportedResourceRegistry::ImportedResourceRegistry(
    Context* context, const ea::string& outputPath, const ea::string& resourceNamePrefix)
    : context_(context)
    , outputPath_(outputPath)
    , resourceNamePrefix_(resourceNamePrefix)
{
}

ImportedResourceRegistry::~ImportedResourceRegistry()
{
    ReleaseResources();
}

ea::string ImportedResourceRegistry::CreateResourceName(
    const ea::string& nameHint, const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
{
    const ea::string body = !nameHint.empty() ? GetSanitizedName(nameHint) : defaultName;
    const ea::string uniqueBody = GetUniqueName(
        body, [&](const ea::string& candidate) { return localResourceNames_.contains(prefix + candidate + suffix); });
    const ea::string localName = prefix + uniqueBody + suffix;
    localResourceNames_.emplace(localName);

    const ea::string resourceName = resourceNamePrefix_ + localName;
    resourceNames_[resourceName] = outputPath_ + localName;
    return resourceName;
}

const ea::string& ImportedResourceRegistry::GetAbsoluteFileName(const ea::string& resourceName) const
{
    const auto iter = resourceNames_.find(resourceName);
    return iter != resourceNames_.end() ? iter->second : EMPTY_STRING;
}

void ImportedResourceRegistry::AddResource(Resource* resource)
{
    queuedResources_.emplace_back(resource);
    resource->SetAbsoluteFileName(GetAbsoluteFileName(resource->GetName()));
}

void ImportedResourceRegistry::CommitResources()
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    for (const SharedPtr<Resource>& resource : queuedResources_)
    {
        cache->AddManualResource(resource);
        cachedResources_.emplace_back(resource->GetType(), resource->GetName());
    }
    queuedResources_.clear();
}

void ImportedResourceRegistry::ReleaseResources()
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    for (const auto& [type, name] : cachedResources_)
        cache->ReleaseResource(type, name, true);
    cachedResources_.clear();
}

void ImportedResourceRegistry::SaveResource(Resource* resource) const
{
    if (!resource)
        return;

    const ea::string& fileName = GetAbsoluteFileName(resource->GetName());
    if (fileName.empty())
        throw RuntimeException("Cannot save imported resource '{}': file name is not assigned", resource->GetName());

    resource->SetAbsoluteFileName(fileName);
    if (!resource->SaveFile(fileName))
        throw RuntimeException("Cannot save imported resource '{}'", resource->GetName());
}

} // namespace ModelImporterUtils

} // namespace Urho3D
