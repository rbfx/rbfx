// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

/// \file
/// Format-independent helpers for the model importers.
/// GLTFImporter and FBXImporter use these helpers. They are not a public API.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/NonCopyable.h"
#include "Urho3D/Core/Variant.h"
#include "Urho3D/Graphics/Skeleton.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Math/Color.h"
#include "Urho3D/Math/Matrix3x4.h"
#include "Urho3D/Math/Quaternion.h"
#include "Urho3D/Math/StringHash.h"
#include "Urho3D/Math/Vector3.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/Utility/GLTFImporter.h"

#include <EASTL/array.h>
#include <EASTL/functional.h>
#include <EASTL/optional.h>

namespace Urho3D
{

class AnimatedModel;
class Animation;
class Context;
class Image;
class Material;
class Model;
class ModelView;
class Node;
class PrefabResource;
class Resource;
class ResourceCache;
class Scene;
class StaticModel;
class Technique;
class Texture2D;
class XMLFile;
struct BoneView;

namespace ModelImporterUtils
{

/// Maximum number of attempts to find a unique name.
inline constexpr unsigned MaxNameAssignTries = 64 * 1024;

/// Return the first name that is not used. Try the hint first, then "<hint>_1", "<hint>_2" and so on.
/// Throw an exception if all tries fail.
ea::string GetUniqueName(const ea::string& nameHint, const ea::function<bool(const ea::string& name)>& isNameUsed);

/// Return true if the name contains one of the tags.
bool IsNameSkipped(const ea::string& name, const StringVector& skipTags);
/// Return the length of the longest prefix that ends on a word border and is common for all strings.
/// The prefix never takes the full first string.
unsigned FindCommonPrefixWord(const StringVector& strings);

/// Mirror the value on the X axis.
/// @{
Vector3 MirrorX(const Vector3& value);
Quaternion MirrorX(const Quaternion& value);
Matrix3x4 MirrorX(Matrix3x4 value);
/// @}

/// Conversion from the source coordinate system to the engine coordinate system.
struct ImportTransform
{
    bool mirrorX_{};
    float scale_{1.0f};

    Vector3 ApplyToPosition(const Vector3& value) const;
    Quaternion ApplyToRotation(const Quaternion& value) const;
    /// Convert the offset matrix of a bone. The matrix maps the model space to the bone space.
    Matrix3x4 ApplyToOffsetMatrix(const Matrix3x4& value) const;
    /// Convert the position and the rotation. The scale does not change.
    Transform ApplyToTransform(const Transform& value) const;
};

/// Convert the geometries to the engine coordinate system and fill the missing vertex data.
/// Repair the bone weights, recalculate the bone bounding boxes and normalize the model view.
void FinalizeModelView(ModelView& modelView, const ImportTransform& transform);

/// Parsed form of the name like "Mesh_LOD2" or "Mesh_LOD25.5".
struct LodName
{
    /// Name without the LOD suffix. The full name if the name has no LOD suffix.
    ea::string baseName_;
    /// Number after "_LOD". Empty if the name has no LOD suffix.
    ea::optional<float> distance_;
    /// Number after "_LOD" if it is a short integer. The number can be a level index.
    ea::optional<unsigned> index_;
};

/// Parse the LOD suffix of the name.
LodName ParseLodName(const ea::string& name);

/// Merge the first LOD of each model view into the geometries of the first model view.
/// Each LOD gets the distance with the same index. The other model views do not change.
void CombineModelViewLODs(const ea::vector<ModelView*>& lods, const ea::vector<float>& distances);

/// Return true if the bones have the same names, parents and offset matrices.
/// The function ignores the initial transforms and the bounding shapes.
bool AreBonesEquivalent(const ea::vector<BoneView>& lhs, const ea::vector<BoneView>& rhs, float offsetMatrixError);

/// Split each geometry that uses more bones than the GPU supports into draw batches.
/// Return the source material index for each exported geometry after the split.
/// Return an empty vector if no geometry changes.
ea::vector<unsigned> SplitOversizedGeometries(ModelView& modelView);

/// Give each geometry of a large skeleton a compact bone palette.
/// Return the bone mapping of each exported geometry. Return an empty vector if the skeleton is small.
ea::vector<ea::vector<unsigned>> RemapGeometryBones(ModelView& modelView);

/// Model and material mapping after the export of a model view.
struct ExportedModel
{
    SharedPtr<Model> model_;
    /// Source material index for each geometry of the model. Empty if no geometry changed.
    ea::vector<unsigned> materialMapping_;
};

/// Notify the callback, fit the geometries to the bone limit of the GPU and export the model.
ExportedModel ExportModelView(ModelView& modelView, GLTFImporterCallback& callback);

/// Return the resource name hint for one group of an animation.
/// The skeleton index or "R" for the scene nodes goes after the name if the animation has more than one group.
ea::string GetAnimationGroupNameHint(
    const ea::string& animationName, ea::optional<unsigned> skeletonIndex, unsigned numGroups);

/// Bone of a skeleton that can have an animation track.
struct AnimatedBone
{
    /// Name of the track. An empty name never has a track.
    ea::string name_;
    /// Index of the parent bone in the same list. Empty for a root bone.
    ea::optional<unsigned> parentIndex_;
};

/// Return the parent track name for each bone that has a track in the animation.
/// The parent track is the track of the nearest parent bone that has a track. It is empty if there is none.
StringVariantMap GetParentTrackNames(const Animation& animation, const ea::vector<AnimatedBone>& bones);

/// Return the time of the last key frame in the animation.
float GetAnimationLength(const Animation& animation);

/// Return the path of the animation track for the node position, rotation or scale.
/// The node path is empty for the node of the animation controller.
ea::string GetNodeTrackPath(const ea::string& nodePath, AnimationChannel channel);
/// Return the path of the animation track for a morph weight of an animated model.
ea::string GetMorphTrackPath(const ea::string& nodePath, unsigned componentIndex, unsigned morphIndex);

/// Material variant for a type of geometry.
/// If the source material does not support the variant, the importer uses the next variant.
enum MaterialVariant
{
    LitNormalMapMaterial,
    LitMaterial,
    UnlitMaterial,

    NumMaterialVariants
};

/// Return the resource name suffix of the material variant.
ea::string GetMaterialVariantSuffix(MaterialVariant variant);

/// Techniques of a standard material.
struct StandardMaterialTechniques
{
    ea::string name_;
    /// Technique for the low quality. Empty if the material has one technique.
    ea::string fallbackName_;
    MaterialQuality minQuality_{QUALITY_LOW};
};

/// Return the techniques of the material variant.
StandardMaterialTechniques GetStandardMaterialTechniques(
    MaterialVariant variant, bool transparent, bool fadeTransparency);
/// Load the techniques and set them to the material. Call this function on the main thread.
void SetStandardMaterialTechniques(Material& material, const StandardMaterialTechniques& techniques, ResourceCache* cache);
/// Set the shader defines and the cull modes of the material variant.
/// The extra defines go before the defines of the variant.
void SetStandardMaterialFlags(
    Material& material, MaterialVariant variant, bool doubleSided, const ea::string& extraDefines = EMPTY_STRING);

/// Shader parameters and textures of a standard material.
struct StandardMaterialParameters
{
    /// Base color in the linear color space.
    Color baseColor_{Color::WHITE};
    SharedPtr<Texture2D> albedoTexture_;

    /// The lit variants use these values.
    /// @{
    float metallic_{};
    float roughness_{1.0f};
    /// Emissive color in the linear color space.
    Color emissiveColor_{Color::BLACK};
    ea::optional<float> occlusionStrength_;
    SharedPtr<Texture2D> propertiesTexture_;
    SharedPtr<Texture2D> emissionTexture_;
    /// @}

    /// The normal map variant uses these values.
    /// @{
    float normalScale_{1.0f};
    SharedPtr<Texture2D> normalTexture_;
    /// @}
};

/// Set the shader parameters and the textures that the material variant uses.
void SetStandardMaterialParameters(
    Material& material, MaterialVariant variant, const StandardMaterialParameters& parameters);

/// Return the RGBA copy of the image with the specified size. Scale the image with a bilinear filter.
SharedPtr<Image> GetResizedRGBAImage(const Image& image, const IntVector2& size);

/// Channel of a source image for PackImageChannels.
struct ImageChannelSource
{
    /// Source image. If it is null, the channel gets the default value.
    const Image* image_{};
    /// Index of the channel in the RGBA form of the source image.
    unsigned channel_{};
    unsigned char defaultValue_{255};
};

/// Pack one channel of each source into a new RGBA image.
/// The image has the size of the largest source. Smaller sources are scaled with a bilinear filter.
/// Return null if no source has an image.
SharedPtr<Image> PackImageChannels(Context* context, const ea::array<ImageChannelSource, 4>& channels);

/// Pack the properties texture of a standard material.
/// The channels are roughness in red, metallic in green and occlusion in alpha. Blue is not used.
SharedPtr<Image> PackPropertiesImage(
    Context* context, const ImageChannelSource& roughness, const ImageChannelSource& metallic, const ImageChannelSource& occlusion);


/// Sampler parameters of an imported texture.
struct TextureSamplerParams
{
    TextureFilterMode filterMode_{FILTER_DEFAULT};
    bool mipmaps_{true};
    TextureAddressMode wrapU_{ADDRESS_WRAP};
    TextureAddressMode wrapV_{ADDRESS_WRAP};
};

/// Create the XML file with the sampler parameters for the image.
/// Return null if all parameters have default values.
SharedPtr<XMLFile> CreateTextureSamplerXML(Context* context, const TextureSamplerParams& params,
    const ea::string& imageName, const ea::string& imageFileName);

/// Texture object and sampler parameters for an imported image.
struct TexturePlaceholder
{
    /// Texture with the name of the image. It has no data until the importer loads it.
    SharedPtr<Texture2D> texture_;
    /// Sampler parameters file of the image. Null if all parameters have default values.
    SharedPtr<XMLFile> sampler_;
};

/// Create the texture object and the sampler parameters file for the image.
TexturePlaceholder CreateTexturePlaceholder(
    Context* context, const ea::string& imageName, const ea::string& imageFileName, const TextureSamplerParams& params);

/// Return the name of the standard technique for a material.
ea::string GetStandardTechniqueName(bool lit, bool normalMap, bool transparent, bool fadeTransparency);
/// Load the standard technique. Throw an exception if the technique is not found.
Technique* LoadStandardTechnique(ResourceCache* cache, const ea::string& name);

/// Keep the animated model with no geometries updated, because it drives a skeleton.
void TweakEmptyAnimatedModel(AnimatedModel& animatedModel);
/// Set the model, the materials and the morph weights of the component. The component casts shadows.
/// An animated model creates the bone nodes if createBones is true. It also gets TweakEmptyAnimatedModel.
void InitializeModelComponent(StaticModel& component, Model* model, const ea::vector<SharedPtr<Material>>& materials,
    const ea::vector<float>& morphWeights = {}, bool createBones = true);
/// Create the animation controller. Play the default animation in a loop if it is not null.
void CreateAnimationController(Node& node, Animation* defaultAnimation);

/// Create the preview scene with an octree and a render pipeline.
SharedPtr<Scene> CreateImportScene(Context* context, const GLTFImporterSettings& settings);
/// Remove the skipped nodes, clean up the root nodes and add the default preview content.
/// Return the new asset root node.
Node* FinalizeImportScene(Scene& scene, Node* assetRoot, const GLTFImporterSettings& settings);
/// Create the prefab resource from the scene.
SharedPtr<PrefabResource> CreatePrefabFromScene(Scene& scene, const ea::string& name);

/// Assigns the names of the imported resources and owns their entries in the resource cache.
class ImportedResourceRegistry : public NonCopyable
{
public:
    ImportedResourceRegistry(Context* context, const ea::string& outputPath, const ea::string& resourceNamePrefix);
    /// Release the resources from the resource cache.
    ~ImportedResourceRegistry();

    /// Create a unique resource name and assign the output file name to it.
    ea::string CreateResourceName(
        const ea::string& nameHint, const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix);
    /// Return the output file name of the resource. Return an empty string if the name is not known.
    const ea::string& GetAbsoluteFileName(const ea::string& resourceName) const;

    /// Set the output file name of the resource and queue it for the resource cache.
    /// This function is safe on a worker thread.
    void AddResource(Resource* resource);
    /// Add the queued resources to the resource cache. Call this function on the main thread.
    void CommitResources();
    /// Remove the resources from the resource cache.
    void ReleaseResources();
    /// Save the resource to its output file. Throw an exception on failure. Do nothing for null.
    void SaveResource(Resource* resource) const;

    const GLTFImporter::ResourceToFileNameMap& GetResourceNames() const { return resourceNames_; }

private:
    Context* const context_{};
    const ea::string outputPath_;
    const ea::string resourceNamePrefix_;

    ea::unordered_set<ea::string> localResourceNames_;
    GLTFImporter::ResourceToFileNameMap resourceNames_;
    ea::vector<SharedPtr<Resource>> queuedResources_;
    ea::vector<ea::pair<StringHash, ea::string>> cachedResources_;
};

} // namespace ModelImporterUtils

} // namespace Urho3D
