// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#pragma once

#include "AssetViewerHelpers.h"
#include "ViewerEnvironment.h"

namespace Urho3D
{

class AssetViewer : public Application
{
    URHO3D_OBJECT(AssetViewer, Application);

public:
    explicit AssetViewer(Context* context)
        : Application(context)
    {
    }


    void Setup() override;
    void Start() override;
    void Stop() override;

private:
    /// Load the persistent settings. Use the defaults when the file is missing or invalid.
    void LoadSettings();

    /// Save the persistent settings to the preferences directory.
    void SaveSettings();

    /// Restore the default view settings.
    void ResetViewSettings();

    void CreateScene();

    void ClearAsset();

    void HandleDropFile(StringHash, VariantMap& eventData);

    ea::string DescribeEmptyDrop(const ea::string& rawFile) const;

    void AddDroppedFile(const ea::string& rawFile);

    void StartQueuedFile();

    void DismissError();

    void HandleKeyDown(StringHash, VariantMap& eventData);

    void HandleUpdate(StringHash, VariantMap& eventData);

    void HandlePostRenderUpdate(StringHash, VariantMap&);

    void UpdateCameraInput();

    void UpdateCameraTransform();

    BoundingBox GetContentBoundingBox(bool ignoreLargeOutliers = false) const;

    Animation* GetSelectedAnimation() const;

    ea::vector<Animation*> GetSelectedAnimationGroup() const;

    ea::string GetAnimationLabel(Animation* animation) const;

    bool IsAnimationCompatible(Animation* animation, AnimatedModel* animatedModel) const;

    bool CanControllerPlayAnimation(AnimationController* controller, Animation* animation) const;

    void UpdateAnimationTimeFromController();

    void ApplySelectedAnimation(bool playing);

    void SetSelectedAnimation(int index, bool playing = true);

    void SetAnimationsPlaying(bool playing);

    void SetAnimationTime(float time);

    void SetAnimationSpeed(float speed);

    void FrameContent();

    void FrameBounds(const BoundingBox& box);

    void UpdatePreviewProjection();

    void BeginParticleAutoFrame();

    void UpdateParticleAutoFrame(float timeStep);

    SharedPtr<Animation> LoadAnimationResource(const ea::string& fileName);

    bool HasAnimationTarget() const;

    /// Return the number of tracks of the animation that match a bone of the model.
    unsigned CountMatchingTracks(Animation* animation, AnimatedModel* animatedModel) const;

    /// Describe how well the animations match the models, for example "3 of 40 tracks match".
    ea::string DescribeTrackMatch(const ea::vector<SharedPtr<Animation>>& animations) const;

    /// Return the source of the animation: "embedded" or the name of the external file.
    ea::string GetAnimationSource(Animation* animation) const;

    /// Add animations from an external file to the loaded model. Return false if no animation fits.
    bool ApplyExternalAnimations(ea::vector<SharedPtr<Animation>> animations, const ea::string& fileName,
        unsigned primaryAnimationCount);

    /// Try to use a glTF or GLB file as an animation source for the loaded model.
    /// Set isAnimationFile when the file has animations but no meshes.
    bool TryLoadGLTFAnimationOnCurrentAsset(const ea::string& fileName, bool& isAnimationFile);

    /// Swap all materials of the content to a plain grey material or back. No reload happens.
    void SetNeutralMaterials(bool neutral);

    /// Find the LOD levels of the content. A level is a node named like "Mesh_LOD1" or a geometry LOD level of a model.
    void CollectLods();

    /// Show one LOD level of the content. A negative level selects the levels with the camera distance (Auto).
    void SetSelectedLod(int level);

    /// Keep the geometry LOD level of the models at the selected level while the camera moves.
    void UpdateLodOverride();

    void BuildAnimationDebugPreview(Animation* animation);

    bool TryLoadAnimationOnCurrentAsset(const ea::string& fileName, ea::string* mismatchNote = nullptr);

    bool LoadAsset(const ea::string& requestedFile);

    void FinalizeAssetLoad(const ea::string& extension);

    bool ConvertAndLoadExternal(const ea::string& extension);

    GLTFImporterSettings GetImportSettings(const ea::string& fileName = EMPTY_STRING) const;

    bool BeginLoadFBXAsset();

    void CompleteFBXAsset(const SharedPtr<FBXImporter>& importer, const ea::string& fileName, bool sourcePrepared);

    bool BeginLoadPotentialFBXAnimation(const ea::string& fileName);

    void CompletePotentialFBXAnimation(const SharedPtr<FBXImporter>& importer, const ea::string& fileName,
        bool animationOnly, unsigned primaryAnimationCount, bool sourcePrepared);

    bool ApplyImportedFBXAnimations(
        const SharedPtr<FBXImporter>& importer, const ea::string& fileName, unsigned primaryAnimationCount);

    bool LoadGLTFAsset(const ea::string& gltfFile);

    template <class T> bool ProcessImportedAsset(const SharedPtr<T>& importer);

    ea::string PrepareImportOutput();

    template <class T> ea::vector<SharedPtr<Resource>> GetImportedResources(const SharedPtr<T>& importer) const;

    template <class T> bool InstantiateImportedAsset(const SharedPtr<T>& importer);

    void MountSourceDirectory();

    bool LoadTextureAsset();

    bool LoadTextureDescriptor(const ea::string& rootName);

    bool LoadMaterialAsset();

    bool LoadAnimationAsset();

    bool LoadParticleEffectAsset();

    bool LoadParticleGraphEffectAsset();

    bool LoadParticleEffect2DAsset();

    bool LoadXMLAsset();

    bool LoadNativeModel();

    bool LoadNativePrefab();

    bool LoadNativeScene();

    void EnsureVisibleMaterials();

    bool Fail(const ea::string& message);

    void AddResource(Resource* resource, ea::unordered_set<Resource*>& visited);

    void CollectSerializableResources(Serializable* serializable, ea::unordered_set<Resource*>& visited);

    void CollectSceneResources();

    void CollectAnimations();

    void CollectParticleEmitters();

    void SetParticlesEmitting(bool emitting);

    void RestartParticles();

    void RenderParticleControls();

    void RenderAnimationControls();

    void RenderMainMenu();

    void RenderToolbar(float top, float width);

    void RenderStatusBar(float top, float width);

    void RenderSplitter(float left, float top, float height, float windowWidth);

    void RenderViewport(const ImVec2& position, const ImVec2& size);

    void RenderViewportOverlay(const ImVec2& position, const ImVec2& size);

    void RenderInspector();

    void RenderUI();

    void DrawOverview();

    void DrawOverviewSummary(unsigned long long fileSize);

    void DrawGLTF();

    void DrawGLTFScenes(const tinygltf::Model& model);

    void DrawGLTFNodes(const tinygltf::Model& model);

    void DrawGLTFMeshes(const tinygltf::Model& model);

    void DrawTextureInfo(const char* name, const tinygltf::TextureInfo& info);

    void DrawGLTFMaterials(const tinygltf::Model& model);

    void DrawGLTFParameters(const char* label, const tinygltf::ParameterMap& parameters);

    void DrawGLTFAccessorsAndBuffers(const tinygltf::Model& model);

    void DrawGLTFTextures(const tinygltf::Model& model);

    void DrawGLTFSkinsAndAnimations(const tinygltf::Model& model);

    void DrawGLTFCamerasAndLights(const tinygltf::Model& model);

    void DrawResources();

    void DrawResource(Resource* resource);

    void DrawParticleEffect(ParticleEffect* effect);

    void DrawParticleEffect2D(ParticleEffect2D* effect);

    void DrawParticleGraphSpan(const char* label, const ParticleGraphSpan& span);

    void DrawParticleGraph(const char* label, ParticleGraph& graph);

    void DrawParticleGraphEffect(ParticleGraphEffect* effect);

    void DrawModel(Model* model);

    void DrawSkeleton(const Skeleton& skeleton);

    void DrawMaterial(Material* material);

    void DrawAnimation(Animation* animation);

    void DrawTexture(Texture* texture);

    void DrawNode(Node* node);

    void DrawSerializable(Serializable* serializable);

    SharedPtr<Scene> scene_;
    WeakPtr<Node> contentRoot_;
    SharedPtr<Node> cameraNode_;
    WeakPtr<FreeFlyController> freeFlyController_;
    ViewerEnvironment environment_;
    SharedPtr<Camera> camera_;
    SharedPtr<Viewport> viewport_;
    SharedPtr<Texture2D> previewTexture_;

    ea::vector<SharedPtr<Resource>> assetResources_;
    ea::vector<WeakPtr<AnimationController>> animationControllers_;
    ea::vector<WeakPtr<Animation>> animations_;
    ea::vector<WeakPtr<Node>> animationDebugNodes_;
    ea::vector<WeakPtr<ParticleEmitter>> legacyParticleEmitters_;
    ea::vector<WeakPtr<ParticleGraphEmitter>> graphParticleEmitters_;
    ea::vector<WeakPtr<ParticleEmitter2D>> particleEmitters2D_;
    SharedPtr<Object> importer_;
    ea::unique_ptr<tinygltf::Model> gltfModel_;
    ea::unique_ptr<TemporaryDir> tempDir_;
    ea::unique_ptr<TemporaryDir> pendingTempDir_;
    ea::unique_ptr<MountPointGuard> mountedDirectory_;

    std::vector<std::string> startupAssets_;
    /// Files that still wait for the load of the previous file. The first file is a model, the rest are animations.
    ea::vector<ea::string> pendingStartupFiles_;
    ea::string statusMessage_;
    StringVector missingTextures_;
    struct SavedMaterial
    {
        WeakPtr<StaticModel> model_;
        unsigned slot_{};
        SharedPtr<Material> material_;
    };
    ea::vector<SavedMaterial> savedMaterials_;
    /// A node that holds one LOD level of a mesh, for example the node "Mesh_LOD1". The nodes of a mesh share a group.
    struct LodNode
    {
        WeakPtr<Node> node_;
        ea::string group_;
        unsigned level_{};
        bool enabled_{};
    };
    /// A model with more than one geometry LOD level.
    struct LodModel
    {
        WeakPtr<StaticModel> model_;
        float lodBias_{1.0f};
    };
    ea::vector<LodNode> lodNodes_;
    ea::vector<LodModel> lodModels_;
    unsigned lodLevelCount_{};
    /// The shown LOD level. A negative value selects the level with the camera distance.
    int selectedLod_{};
    SharedPtr<Material> neutralMaterial_;
    ea::unordered_map<const Animation*, ea::string> animationSources_;
    ea::string sourceFile_;
    ea::string primaryFBXFile_;
    ea::string loadingFile_;
    ea::string convertedFile_;
    ea::string sourceJson_;
    ea::string sourceText_;
    ea::string converterOutput_;
    ea::string error_;
    ea::string requestedFile_;
    ea::string lastFailedFile_;
    ea::string queuedFile_;
    unsigned dropBurstFrame_{M_MAX_UNSIGNED};
    bool dropBurstHasValid_{};
    ea::string blender_{"blender"};
    ea::string settingsPath_;

    Vector3 cameraTarget_;
    float yaw_{180.0f};
    float pitch_{12.0f};
    float distance_{5.0f};
    float sidePanelWidth_{420.0f};
    /// Whether the user drags the splitter. The value comes from the previous frame.
    bool splitterDragging_{};
    float animationTime_{};
    float animationSpeed_{1.0f};
    float particleAutoFrameTimeRemaining_{};
    unsigned framesUntilFrame_{};
    unsigned fallbackMaterialSlots_{};
    int selectedAnimation_{-1};
    bool animationLooped_{true};
    bool neutralMaterials_{};
    bool animationsPlaying_{};
    bool particlesEmitting_{true};
    bool particleAutoFramePending_{};
    bool is2DPreview_{};
    bool freeLookEnabled_{};
    bool sceneHovered_{};
    /// The camera owns the right or middle mouse button after a press over the viewport.
    bool rightButtonOwned_{};
    bool middleButtonOwned_{};
    bool loading_{};
    BoundingBox particleAutoFrameBounds_;
};

} // namespace Urho3D
