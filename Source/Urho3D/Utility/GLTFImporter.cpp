//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Exception.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/AnimationTrack.h"
#include "../Graphics/Light.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/ModelView.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Zone.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/BinaryFile.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"
#include "../Utility/GLTFImporter.h"

#include <tiny_gltf.h>

#include <EASTL/numeric.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>
#include <EASTL/unordered_map.h>

#include <exception>

#include "../DebugNew.h"

namespace Urho3D
{

namespace tg = tinygltf;

namespace
{

const unsigned MaxNameAssignTries = 64*1024;

template <class T>
struct StaticCaster
{
    template <class U>
    T operator() (U x) const { return static_cast<T>(x); }
};

template <class T, unsigned N, class U>
ea::array<T, N> ToArray(const U& vec)
{
    ea::array<T, N> result{};
    if (vec.size() >= N)
        ea::transform(vec.begin(), vec.begin() + N, result.begin(), StaticCaster<T>{});
    return result;
}

bool IsNegativeScale(const Vector3& scale) { return scale.x_ * scale.y_ * scale.y_ < 0.0f; }

Vector3 MirrorX(const Vector3& vec) { return { -vec.x_, vec.y_, vec.z_ }; }

Quaternion RotationFromVector(const Vector4& vec) { return { vec.w_, vec.x_, vec.y_, vec.z_ }; }

Quaternion MirrorX(const Quaternion& rotation) { return { rotation.w_, rotation.x_, -rotation.y_, -rotation.z_ }; }

Matrix3x4 MirrorX(Matrix3x4 mat)
{
    mat.m01_ = -mat.m01_;
    mat.m10_ = -mat.m10_;
    mat.m02_ = -mat.m02_;
    mat.m20_ = -mat.m20_;
    mat.m03_ = -mat.m03_;
    return mat;
}

static auto transformMirrorX = [](const auto& value) { return MirrorX(value); };

/// Raw imported input, parameters and generic output layout.
class GLTFImporterBase : public NonCopyable
{
public:
    GLTFImporterBase(Context* context, const GLTFImporterSettings& settings, tg::Model model,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : context_(context)
        , settings_(settings)
        , model_(ea::move(model))
        , outputPath_(outputPath)
        , resourceNamePrefix_(resourceNamePrefix)
    {
    }

    ea::string CreateLocalResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string body = !nameHint.empty() ? SanitizeName(nameHint) : defaultName;
        for (unsigned i = 0; i < MaxNameAssignTries; ++i)
        {
            const ea::string_view nameFormat = i != 0 ? "{0}{1}_{2}{3}" : "{0}{1}{3}";
            const ea::string localResourceName = Format(nameFormat, prefix, body, i, suffix);
            if (localResourceNames_.contains(localResourceName))
                continue;

            localResourceNames_.emplace(localResourceName);
            return localResourceName;
        }

        // Should never happen
        throw RuntimeException("Cannot assign resource name");
    }

    ea::string CreateResourceName(const ea::string& localResourceName)
    {
        const ea::string resourceName = resourceNamePrefix_ + localResourceName;
        const ea::string absoluteFileName = outputPath_ + localResourceName;
        resourceNameToAbsoluteFileName_[resourceName] = absoluteFileName;
        return resourceName;
    }

    ea::string GetResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string localResourceName = CreateLocalResourceName(nameHint, prefix, defaultName, suffix);
        return CreateResourceName(localResourceName);
    }

    const ea::string& GetAbsoluteFileName(const ea::string& resourceName)
    {
        const auto iter = resourceNameToAbsoluteFileName_.find(resourceName);
        return iter != resourceNameToAbsoluteFileName_.end() ? iter->second : EMPTY_STRING;
    }

    void AddToResourceCache(Resource* resource)
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        cache->AddManualResource(resource);
    }

    void SaveResource(Resource* resource)
    {
        const ea::string& fileName = GetAbsoluteFileName(resource->GetName());
        if (fileName.empty())
            throw RuntimeException("Cannot save imported resource");
        resource->SaveFile(fileName);
    }

    void SaveResource(Scene* scene)
    {
        XMLFile xmlFile(scene->GetContext());
        XMLElement rootElement = xmlFile.GetOrCreateRoot("scene");
        scene->SaveXML(rootElement);
        xmlFile.SaveFile(scene->GetFileName());
    }

    const tg::Model& GetModel() const { return model_; }
    Context* GetContext() const { return context_; }
    const GLTFImporterSettings& GetSettings() const { return settings_; }

    void CheckAnimation(int index) const { CheckT(index, model_.animations, "Invalid animation #{} referenced"); }
    void CheckAccessor(int index) const { CheckT(index, model_.accessors, "Invalid accessor #{} referenced"); }
    void CheckBufferView(int index) const { CheckT(index, model_.bufferViews, "Invalid buffer view #{} referenced"); }
    void CheckImage(int index) const { CheckT(index, model_.images, "Invalid image #{} referenced"); }
    void CheckMaterial(int index) const { CheckT(index, model_.materials, "Invalid material #{} referenced"); }
    void CheckMesh(int index) const { CheckT(index, model_.meshes, "Invalid mesh #{} referenced"); }
    void CheckNode(int index) const { CheckT(index, model_.nodes, "Invalid node #{} referenced"); }
    void CheckSampler(int index) const { CheckT(index, model_.samplers, "Invalid sampler #{} referenced"); }
    void CheckSkin(int index) const { CheckT(index, model_.skins, "Invalid skin #{} referenced"); }
    void CheckTexture(int index) const { CheckT(index, model_.textures, "Invalid texture #{} referenced"); }

private:
    static ea::string SanitizeName(const ea::string& name)
    {
        static const ea::string32 forbiddenSymbols = U"<>:\"/\\|?*";

        ea::string32 unicodeString{ ea::string32::CtorConvert{}, name };
        for (char32_t ch = 0; ch < 31; ++ch)
            unicodeString.replace(ch, ' ');
        for (char32_t ch : forbiddenSymbols)
            unicodeString.replace(ch, '_');
        return { ea::string::CtorConvert{}, unicodeString };
    }

    template <class T>
    void CheckT(int index, const T& container, const char* message) const
    {
        if (index < 0 || index >= container.size())
            throw RuntimeException(message, index);
    }

    Context* const context_{};
    const GLTFImporterSettings settings_;
    const tg::Model model_;
    const ea::string outputPath_;
    const ea::string resourceNamePrefix_;

    ea::unordered_set<ea::string> localResourceNames_;
    ea::unordered_map<ea::string, ea::string> resourceNameToAbsoluteFileName_;
};

/// Utility to parse GLTF buffers.
class GLTFBufferReader : public NonCopyable
{
public:
    explicit GLTFBufferReader(GLTFImporterBase& base)
        : base_(base)
        , model_(base_.GetModel())
    {
    }

    template <class T>
    ea::vector<T> ReadBufferView(int bufferViewIndex, int byteOffset, int componentType, int type, int count, bool normalized) const
    {
        base_.CheckBufferView(bufferViewIndex);

        const int numComponents = tg::GetNumComponentsInType(type);
        if (numComponents <= 0)
            throw RuntimeException("Unexpected type {} of buffer view elements", type);

        const tg::BufferView& bufferView = model_.bufferViews[bufferViewIndex];

        ea::vector<T> result(count * numComponents);
        switch (componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            ReadBufferViewImpl<signed char>(result, bufferView, byteOffset, componentType, type, count);
            if constexpr (ea::is_floating_point_v<T>)
                NormalizeFloats(result, normalized, 127);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            ReadBufferViewImpl<unsigned char>(result, bufferView, byteOffset, componentType, type, count);
            if constexpr (ea::is_floating_point_v<T>)
                NormalizeFloats(result, normalized, 255);
            break;

        case TINYGLTF_COMPONENT_TYPE_SHORT:
            ReadBufferViewImpl<short>(result, bufferView, byteOffset, componentType, type, count);
            if constexpr (ea::is_floating_point_v<T>)
                NormalizeFloats(result, normalized, 32767);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            ReadBufferViewImpl<unsigned short>(result, bufferView, byteOffset, componentType, type, count);
            if constexpr (ea::is_floating_point_v<T>)
                NormalizeFloats(result, normalized, 65535);
            break;

        case TINYGLTF_COMPONENT_TYPE_INT:
            ReadBufferViewImpl<int>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            ReadBufferViewImpl<unsigned>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            ReadBufferViewImpl<float>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            ReadBufferViewImpl<double>(result, bufferView, byteOffset, componentType, type, count);
            break;

        default:
            throw RuntimeException("Unsupported component type {} of buffer view elements", componentType);
        }

        return result;
    }

    template <class T>
    ea::vector<T> ReadAccessorChecked(const tg::Accessor& accessor) const
    {
        const auto result = ReadAccessor<T>(accessor);
        if (result.size() != accessor.count)
            throw RuntimeException("Unexpected number of objects in accessor");
        return result;
    }

    template <class T>
    ea::vector<T> ReadAccessor(const tg::Accessor& accessor) const
    {
        const int numComponents = tg::GetNumComponentsInType(accessor.type);
        if (numComponents <= 0)
            throw RuntimeException("Unexpected type {} of buffer view elements", accessor.type);

        // Read dense buffer data
        ea::vector<T> result;
        if (accessor.bufferView >= 0)
        {
            result = ReadBufferView<T>(accessor.bufferView, accessor.byteOffset,
                accessor.componentType, accessor.type, accessor.count, accessor.normalized);
        }
        else
        {
            result.resize(accessor.count * numComponents);
        }

        // Read sparse buffer data
        const int numSparseElements = accessor.sparse.count;
        if (accessor.sparse.isSparse && numSparseElements > 0)
        {
            const auto& accessorIndices = accessor.sparse.indices;
            const auto& accessorValues = accessor.sparse.values;

            const auto indices = ReadBufferView<unsigned>(accessorIndices.bufferView, accessorIndices.byteOffset,
                accessorIndices.componentType, TINYGLTF_TYPE_SCALAR, numSparseElements, false);

            const auto values = ReadBufferView<T>(accessorValues.bufferView, accessorValues.byteOffset,
                accessor.componentType, accessor.type, numSparseElements, accessor.normalized);

            for (unsigned i = 0; i < indices.size(); ++i)
                ea::copy_n(&values[i * numComponents], numComponents, &result[indices[i] * numComponents]);
        }

        return result;
    }

private:
    static int GetByteStride(const tg::BufferView& bufferViewObject, int componentType, int type)
    {
        const int componentSizeInBytes = tg::GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
        const int numComponents = tg::GetNumComponentsInType(static_cast<uint32_t>(type));
        if (componentSizeInBytes <= 0 || numComponents <= 0)
            return -1;

        return bufferViewObject.byteStride == 0
            ? componentSizeInBytes * numComponents
            : static_cast<int>(bufferViewObject.byteStride);
    }

    template <class T, class U>
    void ReadBufferViewImpl(ea::vector<U>& result,
        const tg::BufferView& bufferView, int byteOffset, int componentType, int type, int count) const
    {
        const tg::Buffer& buffer = model_.buffers[bufferView.buffer];

        const auto* bufferViewData = buffer.data.data() + bufferView.byteOffset + byteOffset;
        const int stride = GetByteStride(bufferView, componentType, type);

        const int numComponents = tg::GetNumComponentsInType(type);
        for (unsigned i = 0; i < count; ++i)
        {
            for (unsigned j = 0; j < numComponents; ++j)
            {
                T elementValue{};
                memcpy(&elementValue, bufferViewData + sizeof(T) * j, sizeof(T));
                result[i * numComponents + j] = static_cast<U>(elementValue);
            }
            bufferViewData += stride;
        }
    }

    template <class T>
    static ea::vector<T> RepackFloats(const ea::vector<float>& source)
    {
        static constexpr unsigned numComponents = sizeof(T) / sizeof(float);
        if (source.size() % numComponents != 0)
            throw RuntimeException("Unexpected number of components in array");

        const unsigned numElements = source.size() / numComponents;

        ea::vector<T> result;
        result.resize(numElements);
        for (unsigned i = 0; i < numElements; ++i)
            std::memcpy(&result[i], &source[i * numComponents], sizeof(T));
        return result;
    }

    template <class T>
    static void NormalizeFloats(ea::vector<T>& result, bool normalize, float maxValue)
    {
        if (normalize)
        {
            for (T& value : result)
                value = ea::max(static_cast<T>(-1), static_cast<T>(value / maxValue));
        }
    }

    const GLTFImporterBase& base_;
    const tg::Model& model_;
};

template <>
ea::vector<Vector2> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector2>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Vector3> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector3>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Vector4> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector4>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Matrix4> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Matrix4>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Quaternion> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const
{
    auto values = RepackFloats<Vector4>(ReadAccessor<float>(accessor));
    ea::vector<Quaternion> result(values.size());
    ea::transform(values.begin(), values.end(), result.begin(), RotationFromVector);
    return result;
}

/// GLTF node reference used for hierarchy view.
struct GLTFNode;
using GLTFNodePtr = ea::shared_ptr<GLTFNode>;

struct GLTFNode : public ea::enable_shared_from_this<GLTFNode>
{
    /// Hierarchy metadata
    /// @{
    GLTFNode* root_{};
    GLTFNode* parent_{};
    ea::vector<GLTFNodePtr> children_;
    /// @}

    /// Data directly imported from GLTF
    /// @{
    unsigned index_{};
    ea::string name_;

    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_{ Vector3::ONE };

    ea::optional<unsigned> mesh_;
    ea::optional<unsigned> skin_;
    ea::vector<float> morphWeights_;

    ea::vector<unsigned> containedInSkins_;
    /// @}

    /// Data generated by importer
    /// @{
    ea::optional<unsigned> skeletonIndex_;
    ea::optional<ea::string> uniqueBoneName_;
    ea::vector<unsigned> skinnedMeshNodes_;
    /// @}

    const ea::string& GetEffectiveName() const { return uniqueBoneName_ ? *uniqueBoneName_ : name_; }
};

/// Represents Urho skeleton which may be composed from one or more GLTF skins.
/// Does *not* contain mesh-related specifics like bone indices and bind matrices.
struct GLTFSkeleton
{
    unsigned index_{};
    ea::vector<unsigned> skins_;
    GLTFNode* rootNode_{};
    ea::unordered_map<ea::string, GLTFNode*> boneNameToNode_;
};

/// Represents GLTF skin as Urho skeleton with bone indices and bind matrices.
struct GLTFSkin
{
    unsigned index_{};
    const GLTFSkeleton* skeleton_{};

    /// Bone nodes, contain exactly one root node.
    ea::vector<const GLTFNode*> boneNodes_;
    ea::vector<Matrix3x4> inverseBindMatrices_;
    /// Note: Does *not* contain bounding volumes.
    ea::vector<BoneView> cookedBones_;
};

/// Represents unique Urho model with optional skin.
struct GLTFMeshSkinPair
{
    unsigned mesh_{};
    ea::optional<unsigned> skin_;
};
using GLTFMeshSkinPairPtr = ea::shared_ptr<GLTFMeshSkinPair>;

/// Represents Urho AnimationTrack with possibly separate keys.
struct GLTFBoneTrack
{
    AnimationChannelFlags channelMask_;

    ea::vector<float> positionKeys_;
    ea::vector<Vector3> positionValues_;

    ea::vector<float> rotationKeys_;
    ea::vector<Quaternion> rotationValues_;

    ea::vector<float> scaleKeys_;
    ea::vector<Vector3> scaleValues_;
};

/// Represents attribute track.
struct GLTFAttributeTrack
{
    KeyFrameInterpolation interpolation_{ KeyFrameInterpolation::Linear };
    ea::vector<float> keys_;
    ea::vector<Variant> values_;
    ea::vector<Variant> inTangents_;
    ea::vector<Variant> outTangents_;
};

/// Represents subset of animation tracks of single GLTF animation that corresponds to single Urho animation.
struct GLTFAnimationTrackGroup
{
    ea::unordered_map<ea::string, GLTFBoneTrack> boneTracksByBoneName_;
    ea::unordered_map<ea::string, GLTFAttributeTrack> attributeTracksByPath_;
};

/// Represents preprocessed GLTF animation which may correspond to one or more Urho animations.
struct GLTFAnimation
{
    unsigned index_{};
    ea::string name_;
    /// Animations grouped by the nearest parent skeleton.
    ea::unordered_map<ea::optional<unsigned>, GLTFAnimationTrackGroup> animationGroups_;
};

/// Utility to process scene and node hierarchy of source GLTF asset.
/// Mirrors the scene to convert from right-handed to left-handed coordinates.
///
/// Implements simple heuristics: if no models are actually mirrored after initial mirror,
/// then GLTF exporter implemented lazy export from left-handed to right-handed coordinate system
/// by adding top-level mirroring. In this case, keep scene as is.
/// TODO: Cleanup redundant mirrors?
/// Otherwise scene is truly left-handed and deep mirroring is needed.
///
/// Converts skins to the format consumable by Urho scene.
class GLTFHierarchyAnalyzer : public NonCopyable
{
public:
    explicit GLTFHierarchyAnalyzer(GLTFImporterBase& base, const GLTFBufferReader& bufferReader)
        : base_(base)
        , bufferReader_(bufferReader)
        , model_(base_.GetModel())
    {
        ProcessMeshMorphs();
        InitializeParents();
        InitializeTrees();
        ConvertToLeftHandedCoordinates();
        PreProcessSkins();
        InitializeSkeletons();
        InitializeSkins();
        AssignSkinnedModelsToNodes();
        EnumerateUniqueMeshSkinPairs();
        AssignNamesToSkeletonRoots();

        ImportAnimations();
    }

    bool IsDeepMirrored() const { return isDeepMirrored_; }

    const GLTFNode& GetNode(int nodeIndex) const
    {
        base_.CheckNode(nodeIndex);
        return *nodeByIndex_[nodeIndex];
    }

    const auto& GetRootNodes() const { return rootNodes_; }

    const ea::vector<GLTFMeshSkinPairPtr>& GetUniqueMeshSkinPairs() const { return uniqueMeshSkinPairs_; }

    unsigned GetNumMorphsInMesh(int meshIndex) const
    {
        base_.CheckMesh(meshIndex);
        return numMorphsInMesh_[meshIndex];
    }

    unsigned GetUniqueMeshSkin(int meshIndex, int skinIndex) const
    {
        const auto key = ea::make_pair(meshIndex, skinIndex);
        const auto iter = meshSkinPairs_.find(key);
        if (iter == meshSkinPairs_.end())
            throw RuntimeException("Cannot find mesh #{} with skin #{}", meshIndex, skinIndex);
        return iter->second;
    }

    const ea::vector<BoneView>& GetSkinBones(ea::optional<unsigned> skinIndex) const
    {
        static const ea::vector<BoneView> emptyBones;
        if (!skinIndex)
            return emptyBones;

        base_.CheckSkin(*skinIndex);
        return skins_[*skinIndex].cookedBones_;
    }

    const GLTFSkeleton& GetSkeleton(unsigned skeletonIndex) const
    {
        if (skeletonIndex >= skeletons_.size())
            throw RuntimeException("Invalid skeleton #{} is referenced", skeletonIndex);
        return skeletons_[skeletonIndex];
    }

    const GLTFAnimation& GetAnimation(unsigned animationIndex) const
    {
        base_.CheckAnimation(animationIndex);
        return animations_[animationIndex];
    }

private:
    void ProcessMeshMorphs()
    {
        const unsigned numMeshes = model_.meshes.size();
        numMorphsInMesh_.resize(numMeshes);
        for (unsigned meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
        {
            const tg::Mesh& mesh = model_.meshes[meshIndex];
            if (mesh.primitives.empty())
                throw RuntimeException("Mesh #{} has no primitives", meshIndex);
            numMorphsInMesh_[meshIndex] = mesh.primitives[0].targets.size();
        }
    }

    void InitializeParents()
    {
        const unsigned numNodes = model_.nodes.size();
        nodeToParent_.resize(numNodes);
        for (unsigned nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            const tg::Node& node = model_.nodes[nodeIndex];
            for (const int childIndex : node.children)
            {
                base_.CheckNode(childIndex);

                if (nodeToParent_[childIndex].has_value())
                {
                    throw RuntimeException("Node #{} has multiple parents: #{} and #{}",
                        childIndex, nodeIndex, *nodeToParent_[childIndex]);
                }

                nodeToParent_[childIndex] = nodeIndex;
            }
        }
    }

    void InitializeTrees()
    {
        const unsigned numNodes = model_.nodes.size();
        nodeByIndex_.resize(numNodes);
        for (unsigned nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            if (!nodeToParent_[nodeIndex])
                rootNodes_.push_back(ImportTree(nodeIndex, nullptr, nullptr));
        }

        for (const GLTFNodePtr& node : rootNodes_)
            ReadNodeProperties(*node);
    }

    GLTFNodePtr ImportTree(unsigned nodeIndex, GLTFNode* parent, GLTFNode* root)
    {
        base_.CheckNode(nodeIndex);
        const tg::Node& sourceNode = model_.nodes[nodeIndex];

        auto node = ea::make_shared<GLTFNode>();
        root = root ? root : node.get();

        node->index_ = nodeIndex;
        node->root_ = root;
        node->parent_ = parent;
        for (const int childIndex : sourceNode.children)
            node->children_.push_back(ImportTree(childIndex, node.get(), root));

        nodeByIndex_[nodeIndex] = node.get();
        return node;
    }

    void ReadNodeProperties(GLTFNode& node) const
    {
        const tg::Node& sourceNode = model_.nodes[node.index_];
        node.name_ = sourceNode.name.c_str();

        if (sourceNode.mesh >= 0)
        {
            base_.CheckMesh(sourceNode.mesh);
            node.mesh_ = sourceNode.mesh;

            const unsigned numMorphs = numMorphsInMesh_[sourceNode.mesh];
            if (numMorphs > 0)
            {
                const auto& morphWeights = !sourceNode.weights.empty()
                    ? sourceNode.weights
                    : model_.meshes[sourceNode.mesh].weights;

                node.morphWeights_.resize(numMorphs);
                if (!morphWeights.empty())
                    ea::copy(morphWeights.begin(), morphWeights.end(), node.morphWeights_.begin());
            }
        }

        if (sourceNode.skin >= 0)
        {
            base_.CheckSkin(sourceNode.skin);
            node.skin_ = sourceNode.skin;
        }

        if (!sourceNode.matrix.empty())
        {
            const Matrix3x4 matrix = ReadMatrix3x4(sourceNode.matrix);
            matrix.Decompose(node.position_, node.rotation_, node.scale_);
        }
        else
        {
            if (!sourceNode.translation.empty())
                node.position_ = ReadVector3(sourceNode.translation);
            if (!sourceNode.rotation.empty())
                node.rotation_ = ReadQuaternion(sourceNode.rotation);
            if (!sourceNode.scale.empty())
                node.scale_ = ReadVector3(sourceNode.scale);
        }

        for (const GLTFNodePtr& child : node.children_)
            ReadNodeProperties(*child);
    }

    void ConvertToLeftHandedCoordinates()
    {
        isDeepMirrored_ = HasMirroredMeshes(rootNodes_, true);
        if (!isDeepMirrored_)
        {
            for (const GLTFNodePtr& node : rootNodes_)
            {
                node->position_ = MirrorX(node->position_);
                node->rotation_ = MirrorX(node->rotation_);
                node->scale_ = MirrorX(node->scale_);
            }
        }
        else
        {
            for (const GLTFNodePtr& node : rootNodes_)
                DeepMirror(*node);
        }
    }

    bool HasMirroredMeshes(const ea::vector<GLTFNodePtr>& nodes, bool isParentMirrored) const
    {
        return ea::any_of(nodes.begin(), nodes.end(),
            [&](const GLTFNodePtr& node) { return HasMirroredMeshes(*node, isParentMirrored); });
    }

    bool HasMirroredMeshes(GLTFNode& node, bool isParentMirrored) const
    {
        const tg::Node& sourceNode = model_.nodes[node.index_];
        const bool hasMesh = sourceNode.mesh >= 0;
        const bool isMirroredLocal = IsNegativeScale(node.scale_);
        const bool isMirroredWorld = (isParentMirrored != isMirroredLocal);
        if (isMirroredWorld && hasMesh)
            return true;

        return HasMirroredMeshes(node.children_, isMirroredWorld);
    }

    void DeepMirror(GLTFNode& node) const
    {
        node.position_ = MirrorX(node.position_);
        node.rotation_ = MirrorX(node.rotation_);
        for (const GLTFNodePtr& node : node.children_)
            DeepMirror(*node);
    }

    void PreProcessSkins()
    {
        const unsigned numSkins = model_.skins.size();
        skinToRootNode_.resize(numSkins);
        for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
        {
            const tg::Skin& sourceSkin = model_.skins[skinIndex];
            GLTFNode& rootNode = *nodeByIndex_[GetSkinRoot(sourceSkin).index_];

            MarkInSkin(rootNode, skinIndex);
            for (const int jointNodeIndex : sourceSkin.joints)
            {
                base_.CheckNode(jointNodeIndex);
                GLTFNode& jointNode = *nodeByIndex_[jointNodeIndex];

                ForEachInPathExceptParent(jointNode, rootNode,
                    [&](GLTFNode& node) { MarkInSkin(node, skinIndex); });
            }
            skinToRootNode_[skinIndex] = &rootNode;
        }
    }

    const GLTFNode& GetSkinRoot(const tg::Skin& sourceSkin) const
    {
        if (sourceSkin.skeleton >= 0)
        {
            base_.CheckNode(sourceSkin.skeleton);
            const GLTFNode& skeletonNode = *nodeByIndex_[sourceSkin.skeleton];

            if (IsValidSkeletonRootNode(skeletonNode, sourceSkin))
                return skeletonNode;
        }

        const GLTFNode* rootNode = nullptr;
        for (int nodeIndex : sourceSkin.joints)
        {
            base_.CheckNode(nodeIndex);
            const GLTFNode& node = *nodeByIndex_[nodeIndex];

            if (!rootNode)
                rootNode = nodeByIndex_[nodeIndex];
            else
            {
                rootNode = GetCommonParent(*rootNode, node);
                if (!rootNode)
                    throw RuntimeException("Skin doesn't have common root node");
            }
        }

        if (!rootNode)
            throw RuntimeException("Skin doesn't have joints");

        return *rootNode;
    }

    bool IsValidSkeletonRootNode(const GLTFNode& skeletonNode, const tg::Skin& sourceSkin) const
    {
        for (int nodeIndex : sourceSkin.joints)
        {
            base_.CheckNode(nodeIndex);
            GLTFNode& node = *nodeByIndex_[nodeIndex];
            if (!IsChildOf(node, skeletonNode) && &node != &skeletonNode)
            {
                URHO3D_LOGWARNING("Skeleton node #{} is not a parent of joint node #{}", sourceSkin.skeleton, nodeIndex);
                return false;
            }
        }

        return true;
    }

    void InitializeSkeletons()
    {
        const unsigned numSkins = model_.skins.size();
        ea::vector<unsigned> skinToGroup(numSkins);
        ea::iota(skinToGroup.begin(), skinToGroup.end(), 0);

        ForEach(rootNodes_, [&](const GLTFNode& child)
        {
            if (child.containedInSkins_.size() <= 1)
                return;

            const unsigned newGroup = skinToGroup[child.containedInSkins_[0]];
            for (unsigned i = 1; i < child.containedInSkins_.size(); ++i)
            {
                const unsigned oldGroup = skinToGroup[child.containedInSkins_[i]];
                if (oldGroup != newGroup)
                    ea::replace(skinToGroup.begin(), skinToGroup.end(), oldGroup, newGroup);
            }
        });

        auto uniqueGroups = skinToGroup;
        ea::sort(uniqueGroups.begin(), uniqueGroups.end());
        uniqueGroups.erase(ea::unique(uniqueGroups.begin(), uniqueGroups.end()), uniqueGroups.end());

        const unsigned numSkeletons = uniqueGroups.size();
        skeletons_.resize(numSkeletons);
        skinToSkeleton_.resize(numSkins);
        for (unsigned skeletonIndex = 0; skeletonIndex < numSkeletons; ++skeletonIndex)
        {
            GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
            for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
            {
                if (skinToGroup[skinIndex] == uniqueGroups[skeletonIndex])
                {
                    skeleton.skins_.push_back(skinIndex);
                    skinToSkeleton_[skinIndex] = skeletonIndex;
                }
            }
            if (skeleton.skins_.empty())
                throw RuntimeException("Skeleton must contain at least one skin");
        }

        AssignNodesToSkeletons();

        for (unsigned skeletonIndex = 0; skeletonIndex < skeletons_.size(); ++skeletonIndex)
        {
            GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
            skeleton.index_ = skeletonIndex;
            InitializeSkeletonRootNode(skeleton);
            AssignSkeletonBoneNames(skeleton);
        }
    }

    void AssignNodesToSkeletons()
    {
        // Every skeleton is a subtree.
        // A set of overlapping subtrees is a subtree as well.
        ForEach(rootNodes_, [&](GLTFNode& child)
        {
            if (child.containedInSkins_.empty())
                return;

            const unsigned skeletonIndex = skinToSkeleton_[child.containedInSkins_[0]];
            for (unsigned i = 1; i < child.containedInSkins_.size(); ++i)
            {
                if (skeletonIndex != skinToSkeleton_[child.containedInSkins_[i]])
                    throw RuntimeException("Incorrect skeleton merge");
            }

            child.skeletonIndex_ = skeletonIndex;
        });
    }

    void InitializeSkeletonRootNode(GLTFSkeleton& skeleton) const
    {
        for (unsigned skinIndex : skeleton.skins_)
        {
            if (!skeleton.rootNode_)
                skeleton.rootNode_ = skinToRootNode_[skinIndex];
            else if (const GLTFNode* skeletonRoot = GetCommonParent(*skeleton.rootNode_, *skinToRootNode_[skinIndex]))
                skeleton.rootNode_ = nodeByIndex_[skeletonRoot->index_];

            if (!skeleton.rootNode_ || (skeleton.rootNode_->skeletonIndex_ != skeleton.index_))
                throw RuntimeException("Cannot find root of the skeleton when processing skin #{}", skinIndex);
        }
    }

    void AssignSkeletonBoneNames(GLTFSkeleton& skeleton) const
    {
        ForEachSkeletonNode(*skeleton.rootNode_, skeleton.index_,
            [&](GLTFNode& boneNode)
        {
            const ea::string nameHint = !boneNode.name_.empty() ? boneNode.name_ : "Bone";

            bool success = false;
            for (unsigned i = 0; i < MaxNameAssignTries; ++i)
            {
                const ea::string_view nameFormat = i != 0 ? "{0}_{1}" : "{0}";
                const ea::string name = Format(nameFormat, nameHint, i);
                if (skeleton.boneNameToNode_.contains(name))
                    continue;

                boneNode.uniqueBoneName_ = name;
                skeleton.boneNameToNode_.emplace(name, &boneNode);
                success = true;
                break;
            }

            if (!success)
                throw RuntimeException("Failed to assign name to bone");
        });
    }

    void InitializeSkins()
    {
        const unsigned numSkins = model_.skins.size();
        skins_.resize(numSkins);
        for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
        {
            GLTFSkin& skin = skins_[skinIndex];
            skin.index_ = skinIndex;
            InitializeSkin(skin);
        }
    }

    void InitializeSkin(GLTFSkin& skin) const
    {
        const tg::Skin& sourceSkin = model_.skins[skin.index_];
        const GLTFSkeleton& skeleton = skeletons_[skinToSkeleton_[skin.index_]];

        skin.skeleton_ = &skeleton;

        // Fill joints first
        ea::unordered_set<unsigned> jointNodes;
        for (int jointNodeIndex : sourceSkin.joints)
        {
            const GLTFNode& jointNode = GetNode(jointNodeIndex);
            if (!jointNode.uniqueBoneName_)
                throw RuntimeException("Cannot use node #{} in skin #{}", jointNodeIndex, skin.index_);

            skin.boneNodes_.push_back(&jointNode);
            jointNodes.insert(jointNodeIndex);
        }

        // Fill other nodes
        ForEachSkeletonNode(*skeleton.rootNode_, skeleton.index_,
            [&](GLTFNode& boneNode)
        {
            if (jointNodes.contains(boneNode.index_))
                return;
            if (!boneNode.uniqueBoneName_)
                throw RuntimeException("Cannot use node #{} in skin #{}", boneNode.index_, skin.index_);

            skin.boneNodes_.push_back(&boneNode);
        });

        // Fill bind matrices
        const unsigned numBones = skin.boneNodes_.size();
        skin.inverseBindMatrices_.resize(numBones);
        if (sourceSkin.inverseBindMatrices >= 0)
        {
            base_.CheckAccessor(sourceSkin.inverseBindMatrices);
            const tg::Accessor& accessor = model_.accessors[sourceSkin.inverseBindMatrices];
            const auto sourceBindMatrices = bufferReader_.ReadAccessorChecked<Matrix4>(accessor);

            if (sourceSkin.joints.size() > sourceBindMatrices.size())
                throw RuntimeException("Unexpected size of bind matrices array");

            for (unsigned i = 0; i < sourceSkin.joints.size(); ++i)
                skin.inverseBindMatrices_[i] = Matrix3x4{ sourceBindMatrices[i].Transpose() };
            MirrorIfNecessary(skin.inverseBindMatrices_);
        }

        // Generate skeleton bones
        skin.cookedBones_.resize(numBones);
        for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
        {
            BoneView& bone = skin.cookedBones_[boneIndex];
            const GLTFNode& boneNode = *skin.boneNodes_[boneIndex];

            if (&boneNode != skeleton.rootNode_)
            {
                if (!boneNode.parent_)
                    throw RuntimeException("Bone parent must be present for child node");
                bone.parentIndex_ = skin.boneNodes_.index_of(boneNode.parent_);
                if (bone.parentIndex_ >= numBones)
                    throw RuntimeException("Bone parent must be within the skeleton");
            }

            bone.name_ = *boneNode.uniqueBoneName_;
            bone.SetInitialTransform(boneNode.position_, boneNode.rotation_, boneNode.scale_);
            if (boneIndex < skin.inverseBindMatrices_.size())
                bone.offsetMatrix_ = skin.inverseBindMatrices_[boneIndex];
        }
    }

    void AssignSkinnedModelsToNodes()
    {
        ForEach(rootNodes_, [&](GLTFNode& node)
        {
            if (node.mesh_ && node.skin_)
            {
                const unsigned skeletonIndex = skinToSkeleton_[*node.skin_];
                GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
                GLTFNode& skeletonRoot = *skeleton.rootNode_;

                skeletonRoot.skinnedMeshNodes_.emplace_back(node.index_);
                skinnedMeshNodeRemapping_[node.index_] = skeletonRoot.index_;
            }
        });
    }

    void EnumerateUniqueMeshSkinPairs()
    {
        ForEach(rootNodes_, [&](const GLTFNode& node)
        {
            if (node.mesh_ && node.skin_)
            {
                auto key = ea::make_pair<int, int>(*node.mesh_, *node.skin_);
                meshSkinPairs_[key] = GetOrCreateMatchingMeshSkinPair(*node.mesh_, node.skin_);
            }
        });

        ForEach(rootNodes_, [&](const GLTFNode& node)
        {
            if (node.mesh_ && !node.skin_)
            {
                auto key = ea::make_pair<int, int>(*node.mesh_, -1);
                meshSkinPairs_[key] = GetOrCreateMatchingMeshSkinPair(*node.mesh_, ea::nullopt);
            }
        });
    }

    unsigned GetOrCreateMatchingMeshSkinPair(unsigned meshIndex, ea::optional<unsigned> skinIndex)
    {
        for (unsigned pairIndex = 0; pairIndex < uniqueMeshSkinPairs_.size(); ++pairIndex)
        {
            const GLTFMeshSkinPair& existingMeshSkin = *uniqueMeshSkinPairs_[pairIndex];
            if (!existingMeshSkin.skin_ && skinIndex)
                throw RuntimeException("Skinned meshes should be processed before non-skinned");

            // Always skip other meshes
            if (existingMeshSkin.mesh_ != meshIndex)
                continue;

            // Match non-skinned model to the first mesh
            if (!skinIndex || skinIndex == existingMeshSkin.skin_)
                return pairIndex;

            const GLTFSkin& existingSkin = skins_[*existingMeshSkin.skin_];
            const GLTFSkin& newSkin = skins_[*skinIndex];

            const bool areBonesMatching = ea::identical(
                existingSkin.cookedBones_.begin(), existingSkin.cookedBones_.end(),
                newSkin.cookedBones_.begin(), newSkin.cookedBones_.end(),
                [&](const BoneView& lhs, const BoneView& rhs)
            {
                if (lhs.name_ != rhs.name_)
                    return false;
                if (lhs.parentIndex_ != rhs.parentIndex_)
                    return false;
                if (!lhs.offsetMatrix_.Equals(rhs.offsetMatrix_, base_.GetSettings().offsetMatrixError_))
                    return false;
                // Don't compare initial transforms and bounding shapes
                return true;
            });

            if (areBonesMatching)
                return pairIndex;
        }

        const unsigned pairIndex = uniqueMeshSkinPairs_.size();
        auto pair = ea::make_shared<GLTFMeshSkinPair>(GLTFMeshSkinPair{ meshIndex, skinIndex });
        uniqueMeshSkinPairs_.push_back(pair);
        return pairIndex;
    }

    void AssignNamesToSkeletonRoots()
    {
        ForEach(rootNodes_, [&](GLTFNode& node)
        {
            if (node.skinnedMeshNodes_.empty())
                return;

            ea::string name;
            for (int meshNodeIndex : node.skinnedMeshNodes_)
            {
                base_.CheckNode(meshNodeIndex);
                const GLTFNode& meshNode = *nodeByIndex_[meshNodeIndex];
                if (!meshNode.name_.empty())
                {
                    if (!name.empty())
                        name += '_';
                    name += meshNode.name_;
                }
            }

            if (!name.empty())
                node.name_ = name;
            else if (node.name_.empty())
                node.name_ = "SkinnedMesh";
        });
    }

    void ImportAnimations()
    {
        const unsigned numAnimations = model_.animations.size();
        animations_.resize(numAnimations);
        for (unsigned animationIndex = 0; animationIndex < numAnimations; ++animationIndex)
        {
            GLTFAnimation& animation = animations_[animationIndex];
            animation.index_ = animationIndex;
            ImportAnimation(animation);
        }
    }

    void ImportAnimation(GLTFAnimation& animation)
    {
        const tg::Animation& sourceAnimation = model_.animations[animation.index_];
        animation.name_ = sourceAnimation.name.c_str();

        for (const tg::AnimationChannel& sourceChannel : sourceAnimation.channels)
        {
            const GLTFNode& targetNode = GetEffectiveTargetNode(sourceChannel);
            const auto parentSkeletonIndex = GetNearestParentSkeleton(targetNode);
            GLTFAnimationTrackGroup& animationGroup = animation.animationGroups_[parentSkeletonIndex];

            if (sourceChannel.sampler >= sourceAnimation.samplers.size())
                throw RuntimeException("Unknown animation sampler #{} is referenced", sourceChannel.sampler);
            const tg::AnimationSampler& sourceSampler = sourceAnimation.samplers[sourceChannel.sampler];
            const KeyFrameInterpolation interpolation = GetInterpolationMode(sourceSampler);

            base_.CheckAccessor(sourceSampler.input);
            base_.CheckAccessor(sourceSampler.output);
            const auto channelKeys = bufferReader_.ReadAccessorChecked<float>(model_.accessors[sourceSampler.input]);
            const tg::Accessor& channelValuesAccessor = model_.accessors[sourceSampler.output];

            // Handle bones and nodes separately, mainly because bones have unique names and nodes don't
            if (sourceChannel.target_path == "weights")
            {
                const unsigned numMorphs = GetNumMorphsForNode(sourceChannel.target_node);
                if (numMorphs == 0)
                    throw RuntimeException("Animation #{} weights channel targets node #{} without morphs", animation.index_, targetNode.index_);

                const ea::string nodePath = GetNodePathRelativeToSkeleton(targetNode, parentSkeletonIndex);
                if (!targetNode.skinnedMeshNodes_.empty() && !targetNode.skinnedMeshNodes_.contains(sourceChannel.target_node))
                    throw RuntimeException("Cannot connect morph weights animation to skinned mesh at node #{}", sourceChannel.target_node);
                const unsigned componentIndex = targetNode.skinnedMeshNodes_.empty() ? 0 : targetNode.skinnedMeshNodes_.index_of(sourceChannel.target_node);

                const auto weightsValues = bufferReader_.ReadAccessorChecked<float>(channelValuesAccessor);
                for (unsigned morphIndex = 0; morphIndex < numMorphs; ++morphIndex)
                {
                    const ea::string trackPath = nodePath + Format("/@AnimatedModel#{}/Morphs/{}", componentIndex, morphIndex);

                    GLTFAttributeTrack& track = animationGroup.attributeTracksByPath_[trackPath];
                    track.interpolation_ = interpolation;
                    track.keys_ = channelKeys;

                    if (interpolation == KeyFrameInterpolation::TangentSpline)
                    {
                        const auto morphWeightInTangents = ReadVericalSlice(weightsValues, morphIndex * 3, numMorphs * 3);
                        const auto morphWeightValues = ReadVericalSlice(weightsValues, morphIndex * 3 + 1, numMorphs * 3);
                        const auto morphWeightOutTangents = ReadVericalSlice(weightsValues, morphIndex * 3 + 2, numMorphs * 3);
                        ea::copy(morphWeightValues.begin(), morphWeightValues.end(), ea::back_inserter(track.values_));
                        ea::copy(morphWeightValues.begin(), morphWeightValues.end(), ea::back_inserter(track.inTangents_));
                        ea::copy(morphWeightValues.begin(), morphWeightValues.end(), ea::back_inserter(track.outTangents_));
                    }
                    else
                    {
                        const auto morphWeightValues = ReadVericalSlice(weightsValues, morphIndex, numMorphs);
                        ea::copy(morphWeightValues.begin(), morphWeightValues.end(), ea::back_inserter(track.values_));
                    }
                }
            }
            else if (targetNode.skeletonIndex_)
            {
                if (!targetNode.uniqueBoneName_)
                    throw RuntimeException("Cannot connect animation track to node");

                GLTFBoneTrack& track = animationGroup.boneTracksByBoneName_[*targetNode.uniqueBoneName_];

                const AnimationChannel newChannel = ReadAnimationChannel(sourceChannel.target_path);
                if (track.channelMask_.Test(newChannel))
                    throw RuntimeException("Duplicate animation for '{}' in animation #{}", sourceChannel.target_path.c_str(), animation.index_);
                track.channelMask_ |= newChannel;

                if (newChannel == CHANNEL_POSITION)
                {
                    track.positionKeys_ = channelKeys;
                    track.positionValues_ = bufferReader_.ReadAccessorChecked<Vector3>(channelValuesAccessor);
                    MirrorIfNecessary(track.positionValues_);

                    if (interpolation == KeyFrameInterpolation::TangentSpline)
                        track.positionValues_ = ReadVericalSlice(track.positionValues_, 1, 3);

                    if (track.positionValues_.size() != channelKeys.size())
                        throw RuntimeException("Animation #{} channel input and output are mismatched", animation.index_);
                }
                else if (newChannel == CHANNEL_ROTATION)
                {
                    track.rotationKeys_ = channelKeys;
                    track.rotationValues_ = bufferReader_.ReadAccessorChecked<Quaternion>(channelValuesAccessor);
                    MirrorIfNecessary(track.rotationValues_);

                    if (interpolation == KeyFrameInterpolation::TangentSpline)
                        track.rotationValues_ = ReadVericalSlice(track.rotationValues_, 1, 3);

                    if (track.rotationValues_.size() != channelKeys.size())
                        throw RuntimeException("Animation #{} channel input and output are mismatched", animation.index_);
                }
                else if (newChannel == CHANNEL_SCALE)
                {
                    track.scaleKeys_ = channelKeys;
                    track.scaleValues_ = bufferReader_.ReadAccessorChecked<Vector3>(channelValuesAccessor);

                    if (interpolation == KeyFrameInterpolation::TangentSpline)
                        track.scaleValues_ = ReadVericalSlice(track.scaleValues_, 1, 3);

                    if (track.scaleValues_.size() != channelKeys.size())
                        throw RuntimeException("Animation #{} channel input and output are mismatched", animation.index_);
                }
            }
            else
            {
                const AnimationChannel newChannel = ReadAnimationChannel(sourceChannel.target_path);
                const ea::string nodePath = GetNodePathRelativeToSkeleton(targetNode, parentSkeletonIndex);
                const ea::string trackPath = nodePath + "/" + ReadAttributeTrackName(newChannel);

                if (animationGroup.attributeTracksByPath_.contains(trackPath))
                    throw RuntimeException("Duplicate animation track '{}'", trackPath);

                GLTFAttributeTrack& track = animationGroup.attributeTracksByPath_[trackPath];
                track.interpolation_ = interpolation;
                track.keys_ = channelKeys;

                if (newChannel == CHANNEL_POSITION)
                {
                    auto positionValues = bufferReader_.ReadAccessorChecked<Vector3>(channelValuesAccessor);
                    MirrorIfNecessary(positionValues);
                    ea::copy(positionValues.begin(), positionValues.end(), ea::back_inserter(track.values_));
                }
                else if (newChannel == CHANNEL_ROTATION)
                {
                    auto rotationValues = bufferReader_.ReadAccessorChecked<Quaternion>(channelValuesAccessor);
                    MirrorIfNecessary(rotationValues);
                    ea::copy(rotationValues.begin(), rotationValues.end(), ea::back_inserter(track.values_));
                }
                else if (newChannel == CHANNEL_SCALE)
                {
                    auto scaleValues = bufferReader_.ReadAccessorChecked<Vector3>(channelValuesAccessor);
                    ea::copy(scaleValues.begin(), scaleValues.end(), ea::back_inserter(track.values_));
                }

                if (interpolation == KeyFrameInterpolation::TangentSpline)
                {
                    track.inTangents_ = ReadVericalSlice(track.values_, 0, 3);
                    track.outTangents_ = ReadVericalSlice(track.values_, 2, 3);
                    track.values_ = ReadVericalSlice(track.values_, 1, 3);
                }

                if (track.values_.size() != channelKeys.size())
                    throw RuntimeException("Animation #{} channel input and output are mismatched", animation.index_);
            }
        }
    }

    unsigned GetNumMorphsForNode(unsigned nodeIndex) const
    {
        const GLTFNode& node = *nodeByIndex_[nodeIndex];
        if (!node.mesh_)
            throw RuntimeException("Animation weights channel targets node #{} without mesh", node.index_);

        base_.CheckMesh(*node.mesh_);
        return numMorphsInMesh_[*node.mesh_];
    }

    const GLTFNode& GetEffectiveTargetNode(const tg::AnimationChannel& channel) const
    {
        base_.CheckNode(channel.target_node);
        if (channel.target_path == "weights" && skinnedMeshNodeRemapping_.contains(channel.target_node))
            return *nodeByIndex_[skinnedMeshNodeRemapping_.find(channel.target_node)->second];
        return *nodeByIndex_[channel.target_node];
    }

    bool IsUniquelyNamedSibling(const GLTFNode& node) const
    {
        if (node.name_.empty())
            return false;
        if (!node.parent_)
            return true;

        const ea::string name = node.GetEffectiveName();
        for (const GLTFNodePtr& child : node.parent_->children_)
        {
            if (child.get() == &node)
                continue;
            if (child->GetEffectiveName() == name)
                return false;
        }
        return true;
    }

    ea::string GetNodePathRelativeToSkeleton(const GLTFNode& node, ea::optional<unsigned> skeletonIndex)
    {
        const auto path = GetPathIncludingSelf(node);
        const GLTFNode* skeletonRoot = skeletonIndex ? skeletons_[*skeletonIndex].rootNode_ : nullptr;
        if (skeletonRoot && !path.contains(skeletonRoot))
            throw RuntimeException("Skeleton doesn't contain required node");

        const unsigned startIndex = skeletonRoot ? path.index_of(skeletonRoot) + 1 : 0;
        const unsigned endIndex = path.size();

        ea::string pathString;
        for (unsigned i = startIndex; i < endIndex; ++i)
        {
            const GLTFNode& pathNode = *path[i];
            if (!pathString.empty())
                pathString += '/';
            pathString += IsUniquelyNamedSibling(pathNode)
                ? pathNode.GetEffectiveName()
                : Format("#{}", GetChildIndex(pathNode));
        }
        return pathString;
    }

    template <class T>
    void MirrorIfNecessary(ea::vector<T>& vec) const
    {
        if (isDeepMirrored_)
            ea::transform(vec.begin(), vec.end(), vec.begin(), transformMirrorX);
    }

    unsigned GetChildIndex(const GLTFNode& node) const
    {
        if (!node.parent_)
        {
            const auto iter = ea::find_if(rootNodes_.begin(), rootNodes_.end(),
                [&](const GLTFNodePtr& rootNode) { return rootNode.get() == &node; });
            if (iter == rootNodes_.end())
                throw RuntimeException("Cannot get index of root node");
            return static_cast<unsigned>(iter - rootNodes_.begin());
        }

        const auto& children = node.parent_->children_;
        const auto iter = ea::find_if(children.begin(), children.end(),
            [&](const GLTFNodePtr& child) { return child.get() == &node; });
        if (iter == children.end())
            throw RuntimeException("Cannot find child in parent node");

        return static_cast<unsigned>(iter - children.begin());
    }

private:
    static bool IsChildOf(const GLTFNode& child, const GLTFNode& parent)
    {
        return child.parent_ == &parent || (child.parent_ && IsChildOf(*child.parent_, parent));
    }

    static ea::vector<const GLTFNode*> GetPathIncludingSelf(const GLTFNode& node)
    {
        ea::vector<const GLTFNode*> path;
        path.push_back(&node);

        const GLTFNode* currentParent = node.parent_;
        while (currentParent)
        {
            path.push_back(currentParent);
            currentParent = currentParent->parent_;
        }

        ea::reverse(path.begin(), path.end());
        return path;
    }

    static const GLTFNode* GetCommonParent(const GLTFNode& lhs, const GLTFNode& rhs)
    {
        if (lhs.root_ != rhs.root_)
            return nullptr;

        const auto lhsPath = GetPathIncludingSelf(lhs);
        const auto rhsPath = GetPathIncludingSelf(rhs);

        const unsigned numCommonParents = ea::min(lhsPath.size(), rhsPath.size());
        for (int i = numCommonParents - 1; i >= 0; --i)
        {
            if (lhsPath[i] == rhsPath[i])
                return lhsPath[i];
        }

        assert(0);
        return nullptr;
    }

    static void MarkInSkin(GLTFNode& node, unsigned skin)
    {
        if (!node.containedInSkins_.contains(skin))
            node.containedInSkins_.push_back(skin);
    }

    template <class T>
    static void ForEachInPathExceptParent(GLTFNode& child, GLTFNode& parent, const T& callback)
    {
        if (&child == &parent)
            return;
        if (!IsChildOf(child, parent))
            throw RuntimeException("Invalid ForEachInPath call");

        GLTFNode* node = &child;
        while (node != &parent)
        {
            callback(*node);
            node = node->parent_;
        }
    }

    template <class T>
    static void ForEachChild(GLTFNode& parent, const T& callback)
    {
        for (const GLTFNodePtr& child : parent.children_)
        {
            callback(*child);
            ForEachChild(*child, callback);
        }
    }

    template <class T>
    static void ForEach(ea::span<const GLTFNodePtr> nodes, const T& callback)
    {
        for (const GLTFNodePtr& node : nodes)
        {
            callback(*node);
            ForEachChild(*node, callback);
        }
    }

    template <class T>
    static void ForEachSkeletonNode(GLTFNode& skeletonRoot, unsigned skeletonIndex, const T& callback)
    {
        if (skeletonRoot.skeletonIndex_ != skeletonIndex)
            throw RuntimeException("Invalid call to ForEachSkeletonNode");

        callback(skeletonRoot);
        for (const GLTFNodePtr& child : skeletonRoot.children_)
        {
            if (child->skeletonIndex_ != skeletonIndex)
                continue;

            ForEachSkeletonNode(*child, skeletonIndex, callback);
        }
    }

    static Matrix3x4 ReadMatrix3x4(const std::vector<double>& src)
    {
        if (src.size() != 16)
            throw RuntimeException("Unexpected size of matrix object");

        Matrix4 temp;
        ea::transform(src.begin(), src.end(), &temp.m00_, StaticCaster<float>{});

        return Matrix3x4{ temp.Transpose() };
    }

    static Vector3 ReadVector3(const std::vector<double>& src)
    {
        if (src.size() != 3)
            throw RuntimeException("Unexpected size of matrix object");

        Vector3 temp;
        ea::transform(src.begin(), src.end(), &temp.x_, StaticCaster<float>{});
        return temp;
    }

    static Quaternion ReadQuaternion(const std::vector<double>& src)
    {
        if (src.size() != 4)
            throw RuntimeException("Unexpected size of matrix object");

        Vector4 temp;
        ea::transform(src.begin(), src.end(), &temp.x_, StaticCaster<float>{});
        return RotationFromVector(temp);
    }

    static AnimationChannel ReadAnimationChannel(const std::string& targetPath)
    {
        if (targetPath == "translation")
            return CHANNEL_POSITION;
        else if (targetPath == "rotation")
            return CHANNEL_ROTATION;
        else if (targetPath == "scale")
            return CHANNEL_SCALE;
        throw RuntimeException("Unknown animation channel '{}'", targetPath.c_str());
    }

    static ea::string ReadAttributeTrackName(AnimationChannel channel)
    {
        if (channel == CHANNEL_POSITION)
            return "@/Position";
        else if (channel == CHANNEL_ROTATION)
            return "@/Rotation";
        else if (channel == CHANNEL_SCALE)
            return "@/Scale";
        throw RuntimeException("Invalid animation channel '{}'", channel);
    }

    static ea::optional<unsigned> GetNearestParentSkeleton(const GLTFNode& node)
    {
        const GLTFNode* currentNode = &node;
        while (currentNode)
        {
            if (currentNode->skeletonIndex_)
                return currentNode->skeletonIndex_;
            currentNode = currentNode->parent_;
        }
        return ea::nullopt;
    }

    template <class T>
    static ea::vector<T> ReadVericalSlice(const ea::vector<T>& source, unsigned index, unsigned count)
    {
        if (source.size() % count != 0 || index >= count)
            throw RuntimeException("Invalid array slice specified");

        const unsigned numElements = source.size() / count;
        ea::vector<T> result(numElements);
        for (unsigned i = 0; i < numElements; ++i)
            result[i] = source[i * count + index];
        return result;
    }

    static KeyFrameInterpolation GetInterpolationMode(const tg::AnimationSampler& sourceSampler)
    {
        if (sourceSampler.interpolation == "STEP")
            return KeyFrameInterpolation::None;
        else if (sourceSampler.interpolation == "LINEAR")
            return KeyFrameInterpolation::Linear;
        else if (sourceSampler.interpolation == "CUBICSPLINE")
            return KeyFrameInterpolation::TangentSpline;
        throw RuntimeException("Unsupported interpolation mode '{}'", sourceSampler.interpolation.c_str());
    }

private:
    const GLTFImporterBase& base_;
    const GLTFBufferReader& bufferReader_;
    const tg::Model& model_;

    ea::vector<unsigned> numMorphsInMesh_;

    ea::vector<ea::optional<unsigned>> nodeToParent_;

    ea::vector<GLTFNodePtr> rootNodes_;
    ea::vector<GLTFNode*> nodeByIndex_;
    bool isDeepMirrored_{};

    ea::vector<GLTFNode*> skinToRootNode_;
    ea::vector<unsigned> skinToSkeleton_;

    ea::unordered_map<unsigned, unsigned> skinnedMeshNodeRemapping_;

    ea::vector<GLTFSkeleton> skeletons_;
    ea::vector<GLTFSkin> skins_;

    ea::unordered_map<ea::pair<int, int>, unsigned> meshSkinPairs_;
    ea::vector<GLTFMeshSkinPairPtr> uniqueMeshSkinPairs_;

    ea::vector<GLTFAnimation> animations_;
};

/// Utility to import textures on-demand.
/// Textures cannot be imported after cooking.
class GLTFTextureImporter : public NonCopyable
{
public:
    explicit GLTFTextureImporter(GLTFImporterBase& base)
        : base_(base)
        , model_(base_.GetModel())
    {
        const unsigned numTextures = model_.textures.size();
        texturesAsIs_.resize(numTextures);
        for (unsigned i = 0; i < numTextures; ++i)
            texturesAsIs_[i] = ImportTexture(i, model_.textures[i]);
    }

    void CookTextures()
    {
        if (texturesCooked_)
            throw RuntimeException("Textures are already cooking");

        texturesCooked_ = true;
        for (auto& [indices, texture] : texturesMRO_)
        {
            const auto [metallicRoughnessTextureIndex, occlusionTextureIndex] = indices;

            texture.repackedImage_ = ImportRMOTexture(metallicRoughnessTextureIndex, occlusionTextureIndex,
                texture.fakeTexture_->GetName());
        }
    }

    void SaveResources()
    {
        for (const ImportedTexture& texture : texturesAsIs_)
        {
            if (!texture.isReferenced_)
                continue;
            base_.SaveResource(texture.image_);
            if (auto xmlFile = texture.cookedSamplerParams_)
                xmlFile->SaveFile(xmlFile->GetAbsoluteFileName());
        }

        for (const auto& elem : texturesMRO_)
        {
            const ImportedRMOTexture& texture = elem.second;
            base_.SaveResource(texture.repackedImage_);
            if (auto xmlFile = texture.cookedSamplerParams_)
                xmlFile->SaveFile(xmlFile->GetAbsoluteFileName());
        }
    }

    SharedPtr<Texture2D> ReferenceTextureAsIs(int textureIndex)
    {
        if (texturesCooked_)
            throw RuntimeException("Cannot reference textures after cooking");

        if (textureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid texture #{} is referenced", textureIndex);

        ImportedTexture& texture = texturesAsIs_[textureIndex];
        texture.isReferenced_ = true;
        return texture.fakeTexture_;
    }

    SharedPtr<Texture2D> ReferenceRoughnessMetallicOcclusionTexture(
        int metallicRoughnessTextureIndex, int occlusionTextureIndex)
    {
        if (texturesCooked_)
            throw RuntimeException("Cannot reference textures after cooking");

        if (metallicRoughnessTextureIndex < 0 && occlusionTextureIndex < 0)
            throw RuntimeException("At least one texture should be referenced");
        if (metallicRoughnessTextureIndex >= 0 && metallicRoughnessTextureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid metallic-roughness texture #{} is referenced", metallicRoughnessTextureIndex);
        if (occlusionTextureIndex >= 0 && occlusionTextureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid occlusion texture #{} is referenced", occlusionTextureIndex);

        const auto key = ea::make_pair(metallicRoughnessTextureIndex, occlusionTextureIndex);
        const auto partialKeyA = ea::make_pair(metallicRoughnessTextureIndex, -1);
        const auto partialKeyB = ea::make_pair(-1, occlusionTextureIndex);

        // Try to find exact match
        auto iter = texturesMRO_.find(key);
        if (iter != texturesMRO_.end())
            return iter->second.fakeTexture_;

        // Try to re-purpose partial match A
        iter = texturesMRO_.find(partialKeyA);
        if (iter != texturesMRO_.end())
        {
            assert(occlusionTextureIndex != -1);
            const ImportedRMOTexture result = iter->second;
            texturesMRO_.erase(iter);
            texturesMRO_.emplace(key, result);
            return result.fakeTexture_;
        }

        // Try to re-purpose partial match B
        iter = texturesMRO_.find(partialKeyB);
        if (iter != texturesMRO_.end())
        {
            assert(metallicRoughnessTextureIndex != -1);
            const ImportedRMOTexture result = iter->second;
            texturesMRO_.erase(iter);
            texturesMRO_.emplace(key, result);
            return result.fakeTexture_;
        }

        // Create new texture
        const ImportedTexture& referenceTexture = metallicRoughnessTextureIndex >= 0
            ? texturesAsIs_[metallicRoughnessTextureIndex]
            : texturesAsIs_[occlusionTextureIndex];

        const ea::string imageName = base_.GetResourceName(
            referenceTexture.nameHint_, "Textures/", "Texture", ".png");

        ImportedRMOTexture& result = texturesMRO_[key];
        result.fakeTexture_ = MakeShared<Texture2D>(base_.GetContext());
        result.fakeTexture_->SetName(imageName);
        result.cookedSamplerParams_ = CookSamplerParams(result.fakeTexture_, referenceTexture.samplerParams_);
        return result.fakeTexture_;
    }

    static bool LoadImageData(tg::Image* image, const int imageIndex, std::string*, std::string*,
        int reqWidth, int reqHeight, const unsigned char* bytes, int size, void*)
    {
        image->name = GetFileName(image->uri.c_str()).c_str();
        image->as_is = true;
        image->image.resize(size);
        ea::copy_n(bytes, size, image->image.begin());
        return true;
    }

private:
    struct SamplerParams
    {
        TextureFilterMode filterMode_{ FILTER_DEFAULT };
        bool mipmaps_{ true };
        TextureAddressMode wrapU_{ ADDRESS_WRAP };
        TextureAddressMode wrapV_{ ADDRESS_WRAP };
    };

    struct ImportedTexture
    {
        bool isReferenced_{};

        ea::string nameHint_;
        SharedPtr<BinaryFile> image_;
        SharedPtr<Texture2D> fakeTexture_;
        SamplerParams samplerParams_;
        SharedPtr<XMLFile> cookedSamplerParams_;
    };

    struct ImportedRMOTexture
    {
        SharedPtr<Texture2D> fakeTexture_;
        SharedPtr<XMLFile> cookedSamplerParams_;

        SharedPtr<Image> repackedImage_;
    };

    static TextureFilterMode GetFilterMode(const tg::Sampler& sampler)
    {
        if (sampler.minFilter == -1 || sampler.magFilter == -1)
            return FILTER_DEFAULT;
        else if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)
                return FILTER_NEAREST;
            else
                return FILTER_NEAREST_ANISOTROPIC;
        }
        else
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST)
                return FILTER_BILINEAR;
            else
                return FILTER_DEFAULT;
        }
    }

    static bool HasMipmaps(const tg::Sampler& sampler)
    {
        return sampler.minFilter == -1 || sampler.magFilter == -1
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    }

    static TextureAddressMode GetAddressMode(int sourceMode)
    {
        switch (sourceMode)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return ADDRESS_CLAMP;

        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return ADDRESS_MIRROR;

        case TINYGLTF_TEXTURE_WRAP_REPEAT:
        default:
            return ADDRESS_WRAP;
        }
    }

    SharedPtr<BinaryFile> ImportImageAsIs(unsigned imageIndex, const tg::Image& sourceImage) const
    {
        auto image = MakeShared<BinaryFile>(base_.GetContext());
        const ea::string imageUri = sourceImage.uri.c_str();

        if (sourceImage.mimeType == "image/jpeg" || imageUri.ends_with(".jpg") || imageUri.ends_with(".jpeg"))
        {
            const ea::string imageName = base_.GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".jpg");
            image->SetName(imageName);
        }
        else if (sourceImage.mimeType == "image/png" || imageUri.ends_with(".png"))
        {
            const ea::string imageName = base_.GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".png");
            image->SetName(imageName);
        }
        else
        {
            throw RuntimeException("Image #{} '{}' has unknown type '{}'",
                imageIndex, sourceImage.name.c_str(), sourceImage.mimeType.c_str());
        }

        ByteVector imageBytes;
        imageBytes.resize(sourceImage.image.size());
        ea::copy(sourceImage.image.begin(), sourceImage.image.end(), imageBytes.begin());
        image->SetData(imageBytes);
        return image;
    }

    SharedPtr<Image> DecodeImage(BinaryFile* imageAsIs) const
    {
        Deserializer& deserializer = imageAsIs->AsDeserializer();
        deserializer.Seek(0);

        auto decodedImage = MakeShared<Image>(base_.GetContext());
        decodedImage->SetName(imageAsIs->GetName());
        decodedImage->Load(deserializer);
        return decodedImage;
    }

    ImportedTexture ImportTexture(unsigned textureIndex, const tg::Texture& sourceTexture) const
    {
        base_.CheckImage(sourceTexture.source);

        const tg::Image& sourceImage = model_.images[sourceTexture.source];

        ImportedTexture result;
        result.nameHint_ = sourceImage.name.c_str();
        result.image_ = ImportImageAsIs(sourceTexture.source, sourceImage);
        result.fakeTexture_ = MakeShared<Texture2D>(base_.GetContext());
        result.fakeTexture_->SetName(result.image_->GetName());
        if (sourceTexture.sampler >= 0)
        {
            base_.CheckSampler(sourceTexture.sampler);

            const tg::Sampler& sourceSampler = model_.samplers[sourceTexture.sampler];
            result.samplerParams_.filterMode_ = GetFilterMode(sourceSampler);
            result.samplerParams_.mipmaps_ = HasMipmaps(sourceSampler);
            result.samplerParams_.wrapU_ = GetAddressMode(sourceSampler.wrapS);
            result.samplerParams_.wrapV_ = GetAddressMode(sourceSampler.wrapT);
        }
        result.cookedSamplerParams_ = CookSamplerParams(result.image_, result.samplerParams_);
        return result;
    }

    SharedPtr<XMLFile> CookSamplerParams(Resource* image, const SamplerParams& samplerParams) const
    {
        static const ea::string addressModeNames[] =
        {
            "wrap",
            "mirror",
            "clamp",
            "border"
        };

        static const ea::string filterModeNames[] =
        {
            "nearest",
            "bilinear",
            "trilinear",
            "anisotropic",
            "nearestanisotropic",
            "default"
        };

        auto xmlFile = MakeShared<XMLFile>(base_.GetContext());

        XMLElement rootElement = xmlFile->CreateRoot("texture");

        if (samplerParams.wrapU_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "u");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapU_]);
        };

        if (samplerParams.wrapV_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "v");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapV_]);
        };

        if (samplerParams.filterMode_ != FILTER_DEFAULT)
        {
            XMLElement childElement = rootElement.CreateChild("filter");
            childElement.SetAttribute("mode", filterModeNames[samplerParams.filterMode_]);
        }

        if (!samplerParams.mipmaps_)
        {
            XMLElement childElement = rootElement.CreateChild("mipmap");
            childElement.SetBool("enable", false);
        }

        // Don't create XML if all parameters are default
        if (!rootElement.GetChild())
            return nullptr;

        const ea::string& imageName = image->GetName();
        xmlFile->SetName(ReplaceExtension(imageName, ".xml"));
        xmlFile->SetAbsoluteFileName(ReplaceExtension(base_.GetAbsoluteFileName(imageName), ".xml"));
        return xmlFile;
    }

    SharedPtr<Image> ImportRMOTexture(
        int metallicRoughnessTextureIndex, int occlusionTextureIndex, const ea::string& name)
    {
        // Unpack input images
        SharedPtr<Image> metallicRoughnessImage = metallicRoughnessTextureIndex >= 0
            ? DecodeImage(texturesAsIs_[metallicRoughnessTextureIndex].image_)
            : nullptr;

        SharedPtr<Image> occlusionImage = occlusionTextureIndex >= 0
            ? DecodeImage(texturesAsIs_[occlusionTextureIndex].image_)
            : nullptr;

        if (!metallicRoughnessImage && !occlusionImage)
        {
            throw RuntimeException("Neither metallic-roughness texture #{} nor occlusion texture #{} can be loaded",
                metallicRoughnessTextureIndex, occlusionTextureIndex);
        }

        const IntVector3 metallicRoughnessImageSize = metallicRoughnessImage ? metallicRoughnessImage->GetSize() : IntVector3::ZERO;
        const IntVector3 occlusionImageSize = occlusionImage ? occlusionImage->GetSize() : IntVector3::ZERO;
        const IntVector2 repackedImageSize = VectorMax(metallicRoughnessImageSize.ToVector2(), occlusionImageSize.ToVector2());

        if (repackedImageSize.x_ <= 0 || repackedImageSize.y_ <= 0)
            throw RuntimeException("Repacked metallic-roughness-occlusion texture has invalid size");

        if (metallicRoughnessImage && metallicRoughnessImageSize.ToVector2() != repackedImageSize)
            metallicRoughnessImage->Resize(repackedImageSize.x_, repackedImageSize.y_);

        if (occlusionImage && occlusionImageSize.ToVector2() != repackedImageSize)
            occlusionImage->Resize(repackedImageSize.x_, repackedImageSize.y_);

        auto finalImage = MakeShared<Image>(base_.GetContext());
        finalImage->SetName(name);
        finalImage->SetSize(repackedImageSize.x_, repackedImageSize.y_, 1, occlusionImage ? 4 : 3);

        for (const IntVector2 texel : IntRect{ IntVector2::ZERO, repackedImageSize })
        {
            // 0xOO__MMRR
            unsigned color{};
            if (metallicRoughnessImage)
            {
                // 0x__MMRR__
                const unsigned value = metallicRoughnessImage->GetPixelInt(texel.x_, texel.y_);
                color |= (value >> 8) & 0xffff;
            }
            else
            {
                color |= 0x0000ffff;
            }
            if (occlusionImage)
            {
                // 0x______OO
                const unsigned value = occlusionImage->GetPixelInt(texel.x_, texel.y_);
                color |= (value & 0xff) << 24;
            }
            else
            {
                color |= 0xff000000;
            }
            finalImage->SetPixelInt(texel.x_, texel.y_, color);
        }

        return finalImage;
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;

    ea::vector<ImportedTexture> texturesAsIs_;
    ea::unordered_map<ea::pair<int, int>, ImportedRMOTexture> texturesMRO_;

    bool texturesCooked_{};
};

/// Utility to import materials. Only referenced materials are saved.
class GLTFMaterialImporter : public NonCopyable
{
public:
    /// Specifies material variant of specific material.
    /// If specific variant is not supported, highest available variant is selected.
    enum MaterialVariant
    {
        LitNormalMapMaterial,
        LitMaterial,
        UnlitMaterial,

        NumMaterialVariants
    };

    explicit GLTFMaterialImporter(GLTFImporterBase& base, GLTFTextureImporter& textureImporter)
        : base_(base)
        , model_(base_.GetModel())
        , textureImporter_(textureImporter)
    {
        InitializeStandardTechniques();
        InitializeMaterials();
        textureImporter_.CookTextures();
    }

    SharedPtr<Material> GetMaterial(int materialIndex, MaterialVariant variant)
    {
        base_.CheckMaterial(materialIndex);
        const SharedPtr<Material> material = materials_[materialIndex].variants_[variant];
        referencedMaterials_.insert(material);
        return material;
    }

    void SaveResources()
    {
        for (const auto& material : referencedMaterials_)
            base_.SaveResource(material);
    }

private:
    void InitializeStandardTechniques()
    {
        litOpaqueNormalMapTechnique_ = LoadTechnique("Techniques/LitOpaqueNormalMap.xml");
        litOpaqueTechnique_ = LoadTechnique("Techniques/LitOpaque.xml");
        unlitOpaqueTechnique_ = LoadTechnique("Techniques/UnlitOpaque.xml");

        litTransparentFadeNormalMapTechnique_ = LoadTechnique("Techniques/LitTransparentFadeNormalMap.xml");
        litTransparentFadeTechnique_ = LoadTechnique("Techniques/LitTransparentFade.xml");
        unlitTransparentTechnique_ = LoadTechnique("Techniques/UnlitTransparent.xml");
    }

    Technique* LoadTechnique(const ea::string& name)
    {
        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();
        auto technique = cache->GetResource<Technique>(name);
        if (!technique)
            throw RuntimeException("Cannot find standard technique '{}'", name);
        return technique;
    }

    void InitializeMaterials()
    {
        materials_.resize(model_.materials.size());
        for (unsigned i = 0; i < model_.materials.size(); ++i)
        {
            const tg::Material& sourceMaterial = model_.materials[i];
            ImportedMaterial& result = materials_[i];

            const bool isLit = !IsUnlitMaterial(sourceMaterial);
            if (isLit && sourceMaterial.normalTexture.index >= 0)
                result.variants_[LitNormalMapMaterial] = ImportMaterial(sourceMaterial, LitNormalMapMaterial);
            if (isLit)
                result.variants_[LitMaterial] = ImportMaterial(sourceMaterial, LitMaterial);
            result.variants_[UnlitMaterial] = ImportMaterial(sourceMaterial, UnlitMaterial);

            if (!result.variants_[LitMaterial])
                result.variants_[LitMaterial] = result.variants_[UnlitMaterial];

            if (!result.variants_[LitNormalMapMaterial])
                result.variants_[LitNormalMapMaterial] = result.variants_[LitMaterial];
        }
    }

    SharedPtr<Material> ImportMaterial(const tg::Material& sourceMaterial, MaterialVariant variant)
    {
        auto material = MakeShared<Material>(base_.GetContext());

        InitializeTechnique(*material, sourceMaterial, variant);
        InitializeBaseColor(*material, sourceMaterial);

        switch (variant)
        {
        case UnlitMaterial:
            InitializeMaterialName(*material, sourceMaterial, "_Unlit");
            break;

        case LitMaterial:
            InitializeMaterialName(*material, sourceMaterial, "_Lit");
            InitializeRoughnessMetallicOcclusion(*material, sourceMaterial);
            InitializeEmissiveMap(*material, sourceMaterial);
            break;

        case LitNormalMapMaterial:
            InitializeMaterialName(*material, sourceMaterial, "_LitNormalMap");
            InitializeRoughnessMetallicOcclusion(*material, sourceMaterial);
            InitializeNormalMap(*material, sourceMaterial);
            InitializeEmissiveMap(*material, sourceMaterial);
            break;

        default:
            break;
        }

        base_.AddToResourceCache(material);
        return material;
    }

    void InitializeTechnique(Material& material, const tg::Material& sourceMaterial, MaterialVariant variant) const
    {
        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();

        const bool isLit = !IsUnlitMaterial(sourceMaterial);
        const bool isOpaque = sourceMaterial.alphaMode == "OPAQUE";
        const bool isAlphaMask = sourceMaterial.alphaMode == "MASK";
        const bool isTransparent = sourceMaterial.alphaMode == "BLEND";

        if (!isOpaque && !isAlphaMask && !isTransparent)
            throw RuntimeException("Unknown alpha mode '{}'", sourceMaterial.alphaMode.c_str());

        Technique* litNormalMapTechnique = isOpaque || isAlphaMask ? litOpaqueNormalMapTechnique_ : litTransparentFadeNormalMapTechnique_;
        Technique* litTechnique = isOpaque || isAlphaMask ? litOpaqueTechnique_ : litTransparentFadeTechnique_;
        Technique* unlitTechnique = isOpaque || isAlphaMask ? unlitOpaqueTechnique_ : unlitTransparentTechnique_;

        ea::string shaderDefines;
        if (isAlphaMask)
        {
            shaderDefines += "ALPHAMASK ";
            // TODO: Add support in standard shader
            material.SetShaderParameter("AlphaCutoff", static_cast<float>(sourceMaterial.alphaCutoff));
        }

        if (isAlphaMask && sourceMaterial.alphaCutoff != 0.5)
            URHO3D_LOGWARNING("Material '{}' has non-standard alpha cutoff", sourceMaterial.name.c_str());

        if (variant == LitNormalMapMaterial && sourceMaterial.normalTexture.index >= 0 && isLit)
        {
            shaderDefines += "PBR ";
            material.SetTechnique(0, litNormalMapTechnique, QUALITY_MEDIUM);
            material.SetTechnique(1, litTechnique, QUALITY_LOW);
        }
        else if (variant == LitMaterial && isLit)
        {
            shaderDefines += "PBR ";
            material.SetTechnique(0, litTechnique, QUALITY_LOW);
        }
        else
        {
            material.SetTechnique(0, unlitTechnique, QUALITY_LOW);
        }

        material.SetVertexShaderDefines(shaderDefines);
        material.SetPixelShaderDefines(shaderDefines);

        if (sourceMaterial.doubleSided)
        {
            material.SetCullMode(CULL_NONE);
            material.SetShadowCullMode(CULL_NONE);
        }
    }

    void InitializeBaseColor(Material& material, const tg::Material& sourceMaterial) const
    {
        const tg::PbrMetallicRoughness& pbr = sourceMaterial.pbrMetallicRoughness;

        const Vector4 baseColor{ ToArray<float, 4>(pbr.baseColorFactor).data() };
        material.SetShaderParameter(ShaderConsts::Material_MatDiffColor, Color(baseColor).LinearToGamma().ToVector4());

        if (pbr.baseColorTexture.index >= 0)
        {
            base_.CheckTexture(pbr.baseColorTexture.index);
            if (pbr.baseColorTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for diffuse texture #{}",
                    sourceMaterial.name.c_str(), pbr.baseColorTexture.index);
            }

            const SharedPtr<Texture2D> diffuseTexture = textureImporter_.ReferenceTextureAsIs(pbr.baseColorTexture.index);
            material.SetTexture(TU_DIFFUSE, diffuseTexture);
        }
    }

    void InitializeRoughnessMetallicOcclusion(Material& material, const tg::Material& sourceMaterial) const
    {
        const tg::PbrMetallicRoughness& pbr = sourceMaterial.pbrMetallicRoughness;

        material.SetShaderParameter(ShaderConsts::Material_Metallic, static_cast<float>(pbr.metallicFactor));
        material.SetShaderParameter(ShaderConsts::Material_Roughness, static_cast<float>(pbr.roughnessFactor));

        // Occlusion and metallic-roughness textures are backed together,
        // ignore occlusion if is uses different UV.
        int occlusionTextureIndex = sourceMaterial.occlusionTexture.index;
        int metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
        if (occlusionTextureIndex >= 0 && metallicRoughnessTextureIndex >= 0
            && sourceMaterial.occlusionTexture.texCoord != pbr.metallicRoughnessTexture.texCoord)
        {
            URHO3D_LOGWARNING("Material '{}' uses different UV for metallic-roughness texture #{} "
                "and for occlusion texture #{}. Occlusion texture is ignored.",
                sourceMaterial.name.c_str(), metallicRoughnessTextureIndex, occlusionTextureIndex);
            occlusionTextureIndex = -1;
        }

        if (metallicRoughnessTextureIndex >= 0 || occlusionTextureIndex >= 0)
        {
            if (metallicRoughnessTextureIndex >= 0 && pbr.metallicRoughnessTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for metallic-roughness texture #{}",
                    sourceMaterial.name.c_str(), metallicRoughnessTextureIndex);
            }

            if (occlusionTextureIndex >= 0)
            {
                if (sourceMaterial.occlusionTexture.texCoord != 0)
                {
                    URHO3D_LOGWARNING("Material '{}' has non-standard UV for occlusion texture #{}",
                        sourceMaterial.name.c_str(), occlusionTextureIndex);
                }
                if (sourceMaterial.occlusionTexture.strength != 1.0)
                {
                    URHO3D_LOGWARNING("Material '{}' has non-default occlusion strength for occlusion texture #{}",
                        sourceMaterial.name.c_str(), occlusionTextureIndex);
                }

                // TODO: Add support in standard shader
                material.SetShaderParameter("OcclusionStrength", static_cast<float>(sourceMaterial.occlusionTexture.strength));
            }

            const SharedPtr<Texture2D> metallicRoughnessTexture = textureImporter_.ReferenceRoughnessMetallicOcclusionTexture(
                metallicRoughnessTextureIndex, occlusionTextureIndex);
            material.SetTexture(TU_SPECULAR, metallicRoughnessTexture);
        }
    }

    void InitializeNormalMap(Material& material, const tg::Material& sourceMaterial) const
    {
        const int normalTextureIndex = sourceMaterial.normalTexture.index;
        if (normalTextureIndex >= 0)
        {
            base_.CheckTexture(normalTextureIndex);
            if (sourceMaterial.normalTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for normal texture #{}",
                    sourceMaterial.name.c_str(), normalTextureIndex);
            }

            material.SetShaderParameter(ShaderConsts::Material_NormalScale, static_cast<float>(sourceMaterial.normalTexture.scale));

            const SharedPtr<Texture2D> normalTexture = textureImporter_.ReferenceTextureAsIs(
                normalTextureIndex);
            material.SetTexture(TU_NORMAL, normalTexture);
        }
    }

    void InitializeEmissiveMap(Material& material, const tg::Material& sourceMaterial)
    {
        const Vector3 emissiveColor{ ToArray<float, 3>(sourceMaterial.emissiveFactor).data() };
        material.SetShaderParameter(ShaderConsts::Material_MatEmissiveColor, Color(emissiveColor).LinearToGamma().ToVector3());

        const int emissiveTextureIndex = sourceMaterial.emissiveTexture.index;
        if (emissiveTextureIndex >= 0)
        {
            base_.CheckTexture(emissiveTextureIndex);
            if (sourceMaterial.emissiveTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for emissive texture #{}",
                    sourceMaterial.name.c_str(), emissiveTextureIndex);
            }

            const SharedPtr<Texture2D> emissiveTexture = textureImporter_.ReferenceTextureAsIs(emissiveTextureIndex);
            material.SetTexture(TU_EMISSIVE, emissiveTexture);
        }
    }

    void InitializeMaterialName(Material& material, const tg::Material& sourceMaterial, const ea::string& suffix) const
    {
        const ea::string materialName = base_.GetResourceName(
            sourceMaterial.name.c_str(), "Materials/", "Material", suffix + ".xml");
        material.SetName(materialName);
    }

    static bool IsUnlitMaterial(const tg::Material& sourceMaterial)
    {
        return sourceMaterial.extensions.find("KHR_materials_unlit") != sourceMaterial.extensions.end();
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;
    GLTFTextureImporter& textureImporter_;

    Technique* litOpaqueNormalMapTechnique_{};
    Technique* litOpaqueTechnique_{};
    Technique* unlitOpaqueTechnique_{};

    Technique* litTransparentFadeNormalMapTechnique_{};
    Technique* litTransparentFadeTechnique_{};
    Technique* unlitTransparentTechnique_{};

    struct ImportedMaterial
    {
        SharedPtr<Material> variants_[NumMaterialVariants];
    };

    ea::vector<ImportedMaterial> materials_;
    ea::unordered_set<SharedPtr<Material>> referencedMaterials_;
};

/// Utility to import models.
class GLTFModelImporter : public NonCopyable
{
public:
    explicit GLTFModelImporter(GLTFImporterBase& base,
        const GLTFBufferReader& bufferReader, const GLTFHierarchyAnalyzer& hierarchyAnalyzer,
        GLTFMaterialImporter& materialImporter)
        : base_(base)
        , model_(base_.GetModel())
        , bufferReader_(bufferReader)
        , hierarchyAnalyzer_(hierarchyAnalyzer)
        , materialImporter_(materialImporter)
    {
        InitializeModels();
    }

    void SaveResources()
    {
        for (const ImportedModel& model : models_)
            base_.SaveResource(model.model_);
    }

    SharedPtr<Model> GetModel(int meshIndex, int skinIndex) const
    {
        return GetImportedModel(meshIndex, skinIndex).model_;
    }

    const StringVector& GetModelMaterials(int meshIndex, int skinIndex) const
    {
        return GetImportedModel(meshIndex, skinIndex).materials_;
    }

private:
    struct ImportedModel
    {
        GLTFNodePtr skeleton_;
        SharedPtr<ModelView> modelView_;
        SharedPtr<Model> model_;
        StringVector materials_;
    };
    using ImportedModelPtr = ea::shared_ptr<ImportedModel>;

    void InitializeModels()
    {
        for (const GLTFMeshSkinPairPtr& pair : hierarchyAnalyzer_.GetUniqueMeshSkinPairs())
        {
            const tg::Mesh& sourceMesh = model_.meshes[pair->mesh_];

            ImportedModel model;
            model.modelView_ = ImportModelView(sourceMesh, hierarchyAnalyzer_.GetSkinBones(pair->skin_));
            model.model_ = model.modelView_->ExportModel();
            model.materials_ = model.modelView_->ExportMaterialList();
            base_.AddToResourceCache(model.model_);
            models_.push_back(model);
        }
    }

    const ImportedModel& GetImportedModel(int meshIndex, int skinIndex) const
    {
        const unsigned modelIndex = hierarchyAnalyzer_.GetUniqueMeshSkin(meshIndex, skinIndex);
        return models_[modelIndex];
    }

    SharedPtr<ModelView> ImportModelView(const tg::Mesh& sourceMesh, const ea::vector<BoneView>& bones)
    {
        const ea::string modelName = base_.GetResourceName(sourceMesh.name.c_str(), "Models/", "Model", ".mdl");

        auto modelView = MakeShared<ModelView>(base_.GetContext());
        modelView->SetName(modelName);
        modelView->SetBones(bones);

        const unsigned numMorphWeights = sourceMesh.weights.size();
        for (unsigned morphIndex = 0; morphIndex < numMorphWeights; ++morphIndex)
            modelView->SetMorph(morphIndex, { "", static_cast<float>(sourceMesh.weights[morphIndex]) });

        auto& geometries = modelView->GetGeometries();

        const unsigned numGeometries = sourceMesh.primitives.size();
        geometries.resize(numGeometries);
        for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
        {
            GeometryView& geometryView = geometries[geometryIndex];
            geometryView.lods_.resize(1);
            GeometryLODView& geometryLODView = geometryView.lods_[0];

            const tg::Primitive& primitive = sourceMesh.primitives[geometryIndex];
            geometryLODView.primitiveType_ = GetPrimitiveType(primitive.mode);

            if (primitive.attributes.empty())
                throw RuntimeException("No attributes in primitive #{} in mesh '{}'.", geometryIndex, sourceMesh.name.c_str());

            const unsigned numVertices = model_.accessors[primitive.attributes.begin()->second].count;
            geometryLODView.vertices_.resize(numVertices);
            for (const auto& attribute : primitive.attributes)
            {
                const tg::Accessor& accessor = model_.accessors[attribute.second];
                ReadVertexData(geometryLODView.vertexFormat_, geometryLODView.vertices_, attribute.first.c_str(), accessor);
            }

            if (primitive.indices >= 0)
            {
                base_.CheckAccessor(primitive.indices);
                geometryLODView.indices_ = bufferReader_.ReadAccessorChecked<unsigned>(model_.accessors[primitive.indices]);
            }
            else
            {
                geometryLODView.indices_.resize(geometryLODView.vertices_.size());
                ea::iota(geometryLODView.indices_.begin(), geometryLODView.indices_.end(), 0);
            }

            // Manually connect line loop to convert it to line strip
            if (primitive.mode == TINYGLTF_MODE_LINE_LOOP)
                geometryLODView.indices_.push_back(0);

            if (primitive.material >= 0)
            {
                if (auto material = materialImporter_.GetMaterial(primitive.material, GetMaterialVariant(geometryLODView)))
                    geometryView.material_ = material->GetName();
            }

            if (numMorphWeights > 0 && primitive.targets.size() != numMorphWeights)
            {
                throw RuntimeException("Primitive #{} in mesh '{}' has incorrect number of morph weights.",
                    geometryIndex, sourceMesh.name.c_str());
            }

            for (unsigned morphIndex = 0; morphIndex < primitive.targets.size(); ++morphIndex)
            {
                const auto& morphAttributes = primitive.targets[morphIndex];
                geometryLODView.morphs_[morphIndex] = ReadVertexMorphs(morphAttributes, numVertices);
            }
        }

        if (hierarchyAnalyzer_.IsDeepMirrored())
            modelView->MirrorGeometriesX();

        modelView->CalculateMissingNormals(true);
        modelView->CalculateMissingTangents();
        modelView->RecalculateBoneBoundingBoxes();
        modelView->RepairBoneWeights();
        modelView->Normalize();
        return modelView;
    }

    static GLTFMaterialImporter::MaterialVariant GetMaterialVariant(const GeometryLODView& lodView)
    {
        if (lodView.IsTriangleGeometry() || lodView.vertexFormat_.tangent_ != ModelVertexFormat::Undefined)
            return GLTFMaterialImporter::LitNormalMapMaterial;
        else if (lodView.vertexFormat_.normal_ != ModelVertexFormat::Undefined)
            return GLTFMaterialImporter::LitMaterial;
        else
            return GLTFMaterialImporter::UnlitMaterial;
    }

    static PrimitiveType GetPrimitiveType(int primitiveMode)
    {
        switch (primitiveMode)
        {
        case TINYGLTF_MODE_POINTS:
            return POINT_LIST;
        case TINYGLTF_MODE_LINE:
            return LINE_LIST;
        case TINYGLTF_MODE_LINE_LOOP:
        case TINYGLTF_MODE_LINE_STRIP:
            return LINE_STRIP;
        case TINYGLTF_MODE_TRIANGLES:
            return TRIANGLE_LIST;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
            return TRIANGLE_STRIP;
        case TINYGLTF_MODE_TRIANGLE_FAN:
            return TRIANGLE_FAN;
        default:
            throw RuntimeException("Unknown primitive type #{}", primitiveMode);
        }
    }

    void ReadVertexData(ModelVertexFormat& vertexFormat, ea::vector<ModelVertex>& vertices,
        const ea::string& semantics, const tg::Accessor& accessor)
    {
        const auto& parsedSemantics = semantics.split('_');
        const ea::string& semanticsName = parsedSemantics[0];
        const unsigned semanticsIndex = parsedSemantics.size() > 1 ? FromString<unsigned>(parsedSemantics[1]) : 0;

        if (semanticsName == "POSITION" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
                throw RuntimeException("Unexpected type of vertex position");

            vertexFormat.position_ = TYPE_VECTOR3;

            const auto positions = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetPosition(positions[i]);
        }
        else if (semanticsName == "NORMAL" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
                throw RuntimeException("Unexpected type of vertex normal");

            vertexFormat.normal_ = TYPE_VECTOR3;

            const auto normals = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetNormal(normals[i].Normalized());
        }
        else if (semanticsName == "TANGENT" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of vertex tangent");

            vertexFormat.tangent_ = TYPE_VECTOR4;

            const auto tangents = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].tangent_ = tangents[i];
        }
        else if (semanticsName == "TEXCOORD" && semanticsIndex < ModelVertex::MaxUVs)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC2)
                throw RuntimeException("Unexpected type of vertex uv");

            vertexFormat.uv_[semanticsIndex] = TYPE_VECTOR2;

            const auto uvs = bufferReader_.ReadAccessorChecked<Vector2>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].uv_[semanticsIndex] = { uvs[i], Vector2::ZERO };
        }
        else if (semanticsName == "COLOR" && semanticsIndex < ModelVertex::MaxColors)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of vertex color");

            if (accessor.type == TINYGLTF_TYPE_VEC3)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR3;

                const auto colors = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = { colors[i], 1.0f };
            }
            else if (accessor.type == TINYGLTF_TYPE_VEC4)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR4;

                const auto colors = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = colors[i];
            }
        }
        else if (semanticsName == "JOINTS" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of skin joints");

            vertexFormat.blendIndices_ = TYPE_UBYTE4;

            const auto indices = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].blendIndices_ = indices[i];
        }
        else if (semanticsName == "WEIGHTS" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of skin weights");

            vertexFormat.blendWeights_ = TYPE_UBYTE4_NORM;

            const auto weights = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].blendWeights_ = weights[i];
        }
    }

    ModelVertexMorphVector ReadVertexMorphs(const std::map<std::string, int>& accessors, unsigned numVertices)
    {
        ea::vector<Vector3> positionDeltas(numVertices);
        ea::vector<Vector3> normalDeltas(numVertices);
        ea::vector<Vector3> tangentDeltas(numVertices);

        if (const auto positionIter = accessors.find("POSITION"); positionIter != accessors.end())
        {
            base_.CheckAccessor(positionIter->second);
            positionDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[positionIter->second]);
        }

        if (const auto normalIter = accessors.find("NORMAL"); normalIter != accessors.end())
        {
            base_.CheckAccessor(normalIter->second);
            normalDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[normalIter->second]);
        }

        if (const auto tangentIter = accessors.find("TANGENT"); tangentIter != accessors.end())
        {
            base_.CheckAccessor(tangentIter->second);
            tangentDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[tangentIter->second]);
        }

        if (numVertices != positionDeltas.size() || numVertices != normalDeltas.size() || numVertices != tangentDeltas.size())
            throw RuntimeException("Morph target has inconsistent sizes of accessors");

        ModelVertexMorphVector vertexMorphs(numVertices);
        for (unsigned i = 0; i < numVertices; ++i)
        {
            vertexMorphs[i].index_ = i;
            vertexMorphs[i].positionDelta_ = positionDeltas[i];
            vertexMorphs[i].normalDelta_ = normalDeltas[i];
            vertexMorphs[i].tangentDelta_ = tangentDeltas[i];
        }
        return vertexMorphs;
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;
    const GLTFBufferReader& bufferReader_;
    const GLTFHierarchyAnalyzer& hierarchyAnalyzer_;
    GLTFMaterialImporter& materialImporter_;

    ea::vector<ImportedModel> models_;
};

/// Utility to import animations.
class GLTFAnimationImporter : public NonCopyable
{
public:
    GLTFAnimationImporter(GLTFImporterBase& base, const GLTFHierarchyAnalyzer& hierarchyAnalyzer)
        : base_(base)
        , hierarchyAnalyzer_(hierarchyAnalyzer)
    {
        ImportAnimations();
    }

    void SaveResources()
    {
        for (const auto& [key, animation] : animations_)
            base_.SaveResource(animation);
    }

    Animation* FindAnimation(unsigned animationIndex, ea::optional<unsigned> groupIndex) const
    {
        const auto iter = animations_.find(ea::make_pair(animationIndex, groupIndex));
        return iter != animations_.end() ? iter->second : nullptr;
    }

    bool HasAnimations() const { return !animations_.empty(); }
    bool HasSceneAnimations() const { return hasSceneAnimations_; }

private:
    using AnimationKey = ea::pair<unsigned, ea::optional<unsigned>>;

    void ImportAnimations()
    {
        const unsigned numAnimations = base_.GetModel().animations.size();
        for (unsigned animationIndex = 0; animationIndex < numAnimations; ++animationIndex)
        {
            const GLTFAnimation& sourceAnimation = hierarchyAnalyzer_.GetAnimation(animationIndex);
            for (const auto& [groupIndex, group] : sourceAnimation.animationGroups_)
            {
                const ea::string animationNameHint = GetAnimationGroupName(sourceAnimation, groupIndex);
                const ea::string animationName = base_.GetResourceName(animationNameHint, "Animations/", "Animation", ".ani");

                auto animation = ImportAnimation(animationName, group);
                base_.AddToResourceCache(animation);
                animations_[{ animationIndex, groupIndex }] = animation;
                if (!groupIndex)
                    hasSceneAnimations_ = true;
            }
        }
    }

    SharedPtr<Animation> ImportAnimation(const ea::string animationName, const GLTFAnimationTrackGroup& sourceGroup) const
    {
        auto animation = MakeShared<Animation>(base_.GetContext());
        animation->SetName(animationName);

        for (const auto& [boneName, boneTrack] : sourceGroup.boneTracksByBoneName_)
        {
            const bool hasPositions = boneTrack.channelMask_.Test(CHANNEL_POSITION);
            const bool hasRotations = boneTrack.channelMask_.Test(CHANNEL_ROTATION);
            const bool hasScales = boneTrack.channelMask_.Test(CHANNEL_SCALE);

            AnimationTrack* track = animation->CreateTrack(boneName);
            track->channelMask_ = boneTrack.channelMask_;

            const float epsilon = base_.GetSettings().keyFrameTimeError_;
            const auto keyTimes = MergeTimes({ &boneTrack.positionKeys_, &boneTrack.rotationKeys_, &boneTrack.scaleKeys_ }, epsilon);
            const auto keyPositions = RemapAnimationVector(keyTimes, boneTrack.positionKeys_, boneTrack.positionValues_);
            const auto keyRotations = RemapAnimationVector(keyTimes, boneTrack.rotationKeys_, boneTrack.rotationValues_);
            const auto keyScales = RemapAnimationVector(keyTimes, boneTrack.scaleKeys_, boneTrack.scaleValues_);

            if (!keyPositions && hasPositions)
                throw RuntimeException("Position array is empty for animation '{}'", animationName);
            if (!keyRotations && hasRotations)
                throw RuntimeException("Rotation array is empty for animation '{}'", animationName);
            if (!keyScales && hasScales)
                throw RuntimeException("Scale array is empty for animation '{}'", animationName);

            for (unsigned i = 0; i < keyTimes.size(); ++i)
            {
                AnimationKeyFrame keyFrame;
                keyFrame.time_ = keyTimes[i];
                if (hasPositions)
                    keyFrame.position_ = (*keyPositions)[i];
                if (hasRotations)
                    keyFrame.rotation_ = (*keyRotations)[i];
                if (hasScales)
                    keyFrame.scale_ = (*keyScales)[i];
                track->AddKeyFrame(keyFrame);
            }
        }

        for (const auto& [attributePath, attributeTrack] : sourceGroup.attributeTracksByPath_)
        {
            VariantAnimationTrack* track = animation->CreateVariantTrack(attributePath);
            track->interpolation_ = attributeTrack.interpolation_;
            for (unsigned i = 0; i < attributeTrack.keys_.size(); ++i)
            {
                track->keyFrames_.push_back({ attributeTrack.keys_[i], attributeTrack.values_[i] });
                if (track->interpolation_ == KeyFrameInterpolation::TangentSpline)
                {
                    track->inTangents_.push_back(attributeTrack.inTangents_[i]);
                    track->outTangents_.push_back(attributeTrack.outTangents_[i]);
                }
            }
        }

        animation->SetLength(CalculateLength(*animation));
        return animation;
    }

    static ea::string GetAnimationGroupName(const GLTFAnimation& animation, ea::optional<unsigned> groupIndex)
    {
        const ea::string namePrefix = !animation.name_.empty() ? animation.name_ : Format("Animation_{}", animation.index_);
        if (animation.animationGroups_.size() <= 1)
            return namePrefix;

        const ea::string groupSuffix = groupIndex ? Format("_{}", *groupIndex) : "_R";
        return namePrefix + groupSuffix;
    }

    static ea::vector<float> MergeTimes(std::initializer_list<const ea::vector<float>*> vectors, float epsilon)
    {
        ea::vector<float> result;
        for (const auto* input : vectors)
            result.append(*input);
        ea::sort(result.begin(), result.end());

        unsigned lastValidIndex = 0;
        for (unsigned i = 1; i < result.size(); ++i)
        {
            if (result[i] - result[lastValidIndex] < epsilon)
                result[i] = -M_LARGE_VALUE;
            else
                lastValidIndex = i;
        }

        ea::erase_if(result, [](float time) { return time < 0.0f; });
        return result;
    }

    template <class T>
    static ea::optional<ea::vector<T>> RemapAnimationVector(const ea::vector<float>& destKeys,
        const ea::vector<float>& sourceKeys, const ea::vector<T>& sourceValues)
    {
        if (sourceKeys.empty())
            return ea::nullopt;

        if (sourceKeys.size() != sourceValues.size())
            throw RuntimeException("Mismathcing keys and values in animation track");

        ea::vector<T> result(destKeys.size());
        for (unsigned i = 0; i < destKeys.size(); ++i)
        {
            const float destKey = destKeys[i];
            const auto iter = ea::lower_bound(sourceKeys.begin(), sourceKeys.end(), destKey);
            const unsigned secondIndex = ea::min<unsigned>(sourceKeys.size() - 1, iter - sourceKeys.begin());
            const unsigned firstIndex = iter == sourceKeys.end() ? secondIndex : ea::max(1u, secondIndex) - 1;

            if (firstIndex == secondIndex)
                result[i] = sourceValues[firstIndex];
            else
            {
                const float factor = InverseLerp(sourceKeys[firstIndex], sourceKeys[secondIndex], destKey);
                result[i] = LerpValue(sourceValues[firstIndex], sourceValues[secondIndex], factor);
            }
        }
        return result;
    }

    static float CalculateLength(const Animation& animation)
    {
        float length = 0.0f;
        for (const auto& [nameHash, track] : animation.GetTracks())
        {
            if (!track.keyFrames_.empty())
                length = ea::max(length, track.keyFrames_.back().time_);
        }
        for (const auto& [nameHash, track] : animation.GetVariantTracks())
        {
            if (!track.keyFrames_.empty())
                length = ea::max(length, track.keyFrames_.back().time_);
        }
        return length;
    }

    static Vector3 LerpValue(const Vector3& lhs, const Vector3& rhs, float factor) { return Lerp(lhs, rhs, factor); }
    static Quaternion LerpValue(const Quaternion& lhs, const Quaternion& rhs, float factor) { return lhs.Slerp(rhs, factor); }

    GLTFImporterBase& base_;
    const GLTFHierarchyAnalyzer& hierarchyAnalyzer_;

    ea::unordered_map<AnimationKey, SharedPtr<Animation>> animations_;
    bool hasSceneAnimations_{};
};

/// Utility to import scenes.
class GLTFSceneImporter : public NonCopyable
{
public:
    GLTFSceneImporter(GLTFImporterBase& base, const GLTFHierarchyAnalyzer& hierarchyAnalyzer,
        const GLTFModelImporter& modelImporter, const GLTFAnimationImporter& animationImporter)
        : base_(base)
        , model_(base_.GetModel())
        , hierarchyAnalyzer_(hierarchyAnalyzer)
        , modelImporter_(modelImporter)
        , animationImporter_(animationImporter)
    {
        ImportScenes();
    }

    void SaveResources()
    {
        for (const ImportedScene& scene : scenes_)
            base_.SaveResource(scene.scene_);
    }

private:
    struct ImportedScene
    {
        unsigned index_{};
        SharedPtr<Scene> scene_;
        ea::unordered_map<Node*, unsigned> nodeToIndex_;
        ea::unordered_map<unsigned, Node*> indexToNode_;
    };

    void ImportScenes()
    {
        const unsigned numScenes = model_.scenes.size();
        scenes_.resize(numScenes);
        for (unsigned sceneIndex = 0; sceneIndex < numScenes; ++sceneIndex)
        {
            const tg::Scene& sourceScene = model_.scenes[sceneIndex];
            ImportedScene& scene = scenes_[sceneIndex];
            scene.index_ = sceneIndex;
            scene.scene_ = MakeShared<Scene>(base_.GetContext());
            ImportScene(scene);
        }
    }

    void ImportScene(ImportedScene& importedScene)
    {
        const tg::Scene& sourceScene = model_.scenes[importedScene.index_];
        auto scene = importedScene.scene_;

        const ea::string sceneName = base_.GetResourceName(sourceScene.name.c_str(), "", "Scene", ".xml");
        scene->SetFileName(base_.GetAbsoluteFileName(sceneName));
        scene->CreateComponent<Octree>();

        auto renderPipeline = scene->CreateComponent<RenderPipeline>();
        if (base_.GetSettings().highRenderQuality_)
        {
            auto settings = renderPipeline->GetSettings();
            settings.renderBufferManager_.colorSpace_ = RenderPipelineColorSpace::LinearLDR;
            settings.sceneProcessor_.pcfKernelSize_ = 5;
            settings.antialiasing_ = PostProcessAntialiasing::FXAA3;
            renderPipeline->SetSettings(settings);
        }

        Node* rootNode = scene->CreateChild("Imported Scene");

        if (animationImporter_.HasSceneAnimations())
            InitializeAnimationController(*rootNode, ea::nullopt);

        for (const GLTFNodePtr& sourceRootNode : hierarchyAnalyzer_.GetRootNodes())
        {
            // Preserve order and insert placeholders if Scene has root animations to keep animation track paths valid
            if (ea::find(sourceScene.nodes.begin(), sourceScene.nodes.end(), sourceRootNode->index_) != sourceScene.nodes.end())
                ImportNode(importedScene, *rootNode, *sourceRootNode);
            else if (animationImporter_.HasSceneAnimations())
                scene->CreateChild("Disabled Node Placeholder");
        }

        InitializeDefaultSceneContent(importedScene);
    }

    void ImportNode(ImportedScene& importedScene, Node& parent, const GLTFNode& sourceNode)
    {
        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();

        // Skip skinned mesh nodes w/o children because Urho instantiates such nodes at skeleton root.
        if (sourceNode.mesh_ && sourceNode.skin_ && sourceNode.children_.empty() && sourceNode.skinnedMeshNodes_.empty())
            return;

        Node* node = GetOrCreateNode(importedScene, parent, sourceNode);
        if (!sourceNode.skinnedMeshNodes_.empty())
        {
            const GLTFSkeleton& skeleton = hierarchyAnalyzer_.GetSkeleton(*sourceNode.skeletonIndex_);

            for (const unsigned nodeIndex : sourceNode.skinnedMeshNodes_)
            {
                const GLTFNode& skinnedMeshNode = hierarchyAnalyzer_.GetNode(nodeIndex);
                // Always create animated model in order to preserve moprh animation order
                auto animatedModel = node->CreateComponent<AnimatedModel>();
                InitializeComponentModelAndMaterials(*animatedModel, *skinnedMeshNode.mesh_, *skinnedMeshNode.skin_);
                InitializeDefaultMorphWeights(*animatedModel, skinnedMeshNode);
            }

            if (animationImporter_.HasAnimations())
                InitializeAnimationController(*node, skeleton.index_);

            if (node->GetNumChildren() != 1)
                throw RuntimeException("Cannot connect node #{} to its children", sourceNode.index_);

            // Connect bone nodes to GLTF nodes
            Node* skeletonRootNode = node->GetChild(0u);
            skeletonRootNode->SetTransform(sourceNode.position_, sourceNode.rotation_, sourceNode.scale_);

            for (const auto& [boneName, boneSourceNode] : skeleton.boneNameToNode_)
            {
                Node* boneNode = skeletonRootNode->GetName() == boneName ? skeletonRootNode : skeletonRootNode->GetChild(boneName, true);
                if (!boneNode)
                    throw RuntimeException("Cannot connect node #{} to skeleton bone", boneSourceNode->index_, boneName);

                RegisterNode(importedScene, *boneNode, *boneSourceNode);
            }

            for (const GLTFNodePtr& childNode : sourceNode.children_)
            {
                ImportNode(importedScene, *node->GetChild(0u), *childNode);
            }
        }
        else
        {
            // Skip skinned mesh nodes w/o children because Urho instantiates such nodes at skeleton root.
            if (sourceNode.mesh_ && sourceNode.skin_ && sourceNode.children_.empty())
                return;

            node->SetTransform(sourceNode.position_, sourceNode.rotation_, sourceNode.scale_);

            if (sourceNode.mesh_ && !sourceNode.skin_)
            {
                if (hierarchyAnalyzer_.GetNumMorphsInMesh(*sourceNode.mesh_) > 0)
                {
                    auto animatedModel = node->CreateComponent<AnimatedModel>();
                    InitializeComponentModelAndMaterials(*animatedModel, *sourceNode.mesh_, -1);
                    InitializeDefaultMorphWeights(*animatedModel, sourceNode);
                }
                else
                {
                    auto staticModel = node->CreateComponent<StaticModel>();
                    InitializeComponentModelAndMaterials(*staticModel, *sourceNode.mesh_, -1);
                }
            }

            for (const GLTFNodePtr& childNode : sourceNode.children_)
            {
                ImportNode(importedScene, *node, *childNode);
            }
        }
    }

    void InitializeComponentModelAndMaterials(StaticModel& staticModel, int meshIndex, int skinIndex) const
    {
        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();

        Model* model = modelImporter_.GetModel(meshIndex, skinIndex);
        if (!model)
            return;

        staticModel.SetModel(model);
        staticModel.SetCastShadows(true);

        const StringVector& materialList = modelImporter_.GetModelMaterials(meshIndex, skinIndex);
        for (unsigned i = 0; i < materialList.size(); ++i)
        {
            auto material = cache->GetResource<Material>(materialList[i]);
            staticModel.SetMaterial(i, material);
        }
    }

    void InitializeAnimationController(Node& node, ea::optional<unsigned> groupIndex)
    {
        auto animationController = node.CreateComponent<AnimationController>();
        if (Animation* animation = animationImporter_.FindAnimation(defaultAnimationIndex_, groupIndex))
            animationController->Play(animation->GetName(), 0, true);
    }

    void InitializeDefaultSceneContent(ImportedScene& importedScene)
    {
        static const Vector3 defaultPosition{ -1.0f, 2.0f, 1.0f };

        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();
        auto scene = importedScene.scene_;
        const GLTFImporterSettings& settings = base_.GetSettings();

        if (settings.addLights_ && !scene->GetComponent<Light>(true))
        {
            // Model forward is Z+, make default lighting from top right when looking at forward side of model.
            Node* node = scene->CreateChild("Default Light");
            node->SetPosition(defaultPosition);
            node->SetDirection({ 1.0f, -2.0f, -1.0f });
            auto light = node->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
            light->SetCastShadows(true);
        }

        if (settings.addSkybox_ && !scene->GetComponent<Skybox>(true))
        {
            static const ea::string skyboxModelName = "Models/Box.mdl";

            auto skyboxMaterial = cache->GetResource<Material>("Materials/Skybox.xml");
            auto boxModel = cache->GetResource<Model>(skyboxModelName);

            if (!skyboxMaterial)
                URHO3D_LOGWARNING("Cannot add default skybox with material '{}'", settings.skyboxMaterial_);
            else if (!boxModel)
                URHO3D_LOGWARNING("Cannot add default skybox with model '{}'", skyboxModelName);
            else
            {
                Node* skyboxNode = scene->CreateChild("Default Skybox");
                skyboxNode->SetPosition(defaultPosition);
                auto skybox = skyboxNode->CreateComponent<Skybox>();
                skybox->SetModel(boxModel);
                skybox->SetMaterial(skyboxMaterial);
            }
        }

        if (settings.addReflectionProbe_ && !scene->GetComponent<Zone>(true))
        {
            auto skyboxTexture = cache->GetResource<TextureCube>(settings.reflectionProbeCubemap_);
            if (!skyboxTexture)
                URHO3D_LOGWARNING("Cannot add default reflection probe with material '{}'", settings.reflectionProbeCubemap_);
            else
            {
                Node* zoneNode = scene->CreateChild("Default Zone");
                zoneNode->SetPosition(defaultPosition);
                auto zone = zoneNode->CreateComponent<Zone>();
                zone->SetBackgroundBrightness(0.5f);
                zone->SetZoneTexture(skyboxTexture);
            }
        }
    }

    void InitializeDefaultMorphWeights(AnimatedModel& animatedModel, const GLTFNode& sourceNode)
    {
        const unsigned numMorphs = animatedModel.GetNumMorphs();
        if (numMorphs != sourceNode.morphWeights_.size())
            throw RuntimeException("Cannot setup mesh morphs");

        for (unsigned morphIndex = 0; morphIndex < numMorphs; ++morphIndex)
            animatedModel.SetMorphWeight(morphIndex, sourceNode.morphWeights_[morphIndex]);
    }

    static void RegisterNode(ImportedScene& importedScene, Node& node, const GLTFNode& sourceNode)
    {
        importedScene.indexToNode_[sourceNode.index_] = &node;
        importedScene.nodeToIndex_[&node] = sourceNode.index_;
    }

    static Node* GetOrCreateNode(ImportedScene& importedScene, Node& parentNode, const GLTFNode& sourceNode)
    {
        // If node is not in the skeleton, or it is skeleton root node, create as is.
        // Otherwise, node should be already created by AnimatedModel, connect to it.
        if (!sourceNode.skeletonIndex_ || !sourceNode.skinnedMeshNodes_.empty())
        {
            Node* node = parentNode.CreateChild(sourceNode.GetEffectiveName());
            RegisterNode(importedScene, *node, sourceNode);
            return node;
        }
        else
        {
            Node* node = importedScene.indexToNode_[sourceNode.index_];
            if (!node)
                throw RuntimeException("Cannot find bone node #{}", sourceNode.index_);
            return node;
        }
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;
    const GLTFHierarchyAnalyzer& hierarchyAnalyzer_;
    const GLTFModelImporter& modelImporter_;
    const GLTFAnimationImporter& animationImporter_;

    unsigned defaultAnimationIndex_{};

    ea::vector<ImportedScene> scenes_;
};

void ValidateExtensions(const tg::Model& model)
{
    static const ea::unordered_set<ea::string> supportedExtensions = {
        "KHR_materials_unlit"
    };

    for (const std::string& extension : model.extensionsUsed)
    {
        if (!supportedExtensions.contains(extension.c_str()))
            URHO3D_LOGWARNING("Unsupported extension used: '{}'", extension.c_str());
    }
}

tg::Model LoadGLTF(const ea::string& fileName)
{
    tg::TinyGLTF loader;
    loader.SetImageLoader(&GLTFTextureImporter::LoadImageData, nullptr);

    std::string errorMessage;
    tg::Model model;
    if (fileName.ends_with(".gltf"))
    {
        if (!loader.LoadASCIIFromFile(&model, &errorMessage, nullptr, fileName.c_str()))
            throw RuntimeException("Failed to import GLTF file '{}' due to error: {}", fileName, errorMessage.c_str());
    }
    else if (fileName.ends_with(".glb"))
    {
        if (!loader.LoadBinaryFromFile(&model, &errorMessage, nullptr, fileName.c_str()))
            throw RuntimeException("Failed to import GLTF file '{}' due to error: {}", fileName, errorMessage.c_str());
    }
    else
        throw RuntimeException("Unknown extension of file '{}'", fileName);

    ValidateExtensions(model);
    return model;
}

}

class GLTFImporter::Impl
{
public:
    explicit Impl(Context* context, const GLTFImporterSettings& settings, const ea::string& fileName,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : importerContext_(context, settings, LoadGLTF(fileName), outputPath, resourceNamePrefix)
        , bufferReader_(importerContext_)
        , hierarchyAnalyzer_(importerContext_, bufferReader_)
        , textureImporter_(importerContext_)
        , materialImporter_(importerContext_, textureImporter_)
        , modelImporter_(importerContext_, bufferReader_, hierarchyAnalyzer_, materialImporter_)
        , animationImporter_(importerContext_, hierarchyAnalyzer_)
        , sceneImporter_(importerContext_, hierarchyAnalyzer_, modelImporter_, animationImporter_)
    {
    }

    void SaveResources()
    {
        textureImporter_.SaveResources();
        materialImporter_.SaveResources();
        modelImporter_.SaveResources();
        animationImporter_.SaveResources();
        sceneImporter_.SaveResources();
    }

private:
    GLTFImporterBase importerContext_;
    const GLTFBufferReader bufferReader_;
    const GLTFHierarchyAnalyzer hierarchyAnalyzer_;
    GLTFTextureImporter textureImporter_;
    GLTFMaterialImporter materialImporter_;
    GLTFModelImporter modelImporter_;
    GLTFAnimationImporter animationImporter_;
    GLTFSceneImporter sceneImporter_;
};

void SerializeValue(Archive& archive, const char* name, GLTFImporterSettings& value)
{
    auto block = archive.OpenUnorderedBlock(name);
    SerializeValue(archive, "addLights", value.addLights_);
    SerializeValue(archive, "addSkybox", value.addSkybox_);
    SerializeValue(archive, "skyboxMaterial", value.skyboxMaterial_);
    SerializeValue(archive, "addReflectionProbe", value.addReflectionProbe_);
    SerializeValue(archive, "reflectionProbeCubemap", value.reflectionProbeCubemap_);
    SerializeValue(archive, "highRenderQuality", value.highRenderQuality_);
    SerializeValue(archive, "offsetMatrixError", value.offsetMatrixError_);
    SerializeValue(archive, "keyFrameTimeError", value.keyFrameTimeError_);
}

GLTFImporter::GLTFImporter(Context* context, const GLTFImporterSettings& settings)
    : Object(context)
    , settings_(settings)
{

}

GLTFImporter::~GLTFImporter()
{

}

bool GLTFImporter::LoadFile(const ea::string& fileName,
    const ea::string& outputPath, const ea::string& resourceNamePrefix)
{
    try
    {
        impl_ = ea::make_unique<Impl>(context_, settings_, fileName, outputPath, resourceNamePrefix);
        return true;
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

bool GLTFImporter::SaveResources()
{
    try
    {
        if (!impl_)
            throw RuntimeException("Imported asserts weren't cooked");

        impl_->SaveResources();
        return true;
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

}
