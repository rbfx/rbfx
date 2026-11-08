// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "AssetViewer.h"

namespace Urho3D
{

using namespace AssetViewerDetail;

namespace
{

// Runs the function when the scope ends. A completion function uses it to start the queued file last.
template <class T> struct ScopeExit
{
    explicit ScopeExit(T function)
        : function_(ea::move(function))
    {
    }
    ScopeExit(const ScopeExit&) = delete;
    ~ScopeExit() { function_(); }
    T function_;
};

/// Split a node name like "Mesh_LOD1" into the group name "Mesh" and the level 1. Return false for other names.
bool ParseLodNodeName(const ea::string& name, ea::string& group, unsigned& level)
{
    unsigned digitsBegin = name.length();
    while (digitsBegin > 0 && name[digitsBegin - 1] >= '0' && name[digitsBegin - 1] <= '9')
        --digitsBegin;
    const unsigned digitCount = name.length() - digitsBegin;
    if (digitCount == 0 || digitCount > 2 || digitsBegin < 3)
        return false;
    if (name.substr(digitsBegin - 3, 3).to_upper() != "LOD")
        return false;

    level = ToUInt(name.substr(digitsBegin));
    unsigned groupEnd = digitsBegin - 3;
    if (groupEnd > 0 && (name[groupEnd - 1] == '_' || name[groupEnd - 1] == ' ' || name[groupEnd - 1] == '-'))
        --groupEnd;
    group = name.substr(0, groupEnd);
    return true;
}

/// Return true if the node has an enabled drawable component (a model or a particle emitter) on the node itself.
bool HasOwnDrawable(const Node* node)
{
    for (const Component* component : node->GetComponents())
    {
        const auto* drawable = dynamic_cast<const Drawable*>(component);
        if (drawable && drawable->IsEnabled())
            return true;
    }
    return false;
}

/// Return the number of geometry LOD levels of the model. This is the highest count of its geometries.
unsigned GetNumModelLodLevels(const Model* model)
{
    unsigned result = 0;
    if (model)
    {
        for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
            result = Max(result, model->GetNumGeometryLodLevels(i));
    }
    return result;
}

/// Return a LOD distance value that makes the model select the geometry LOD level.
float GetModelLodTargetDistance(const Model* model, unsigned level)
{
    for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
    {
        const unsigned numLevels = model->GetNumGeometryLodLevels(i);
        if (level >= numLevels)
            continue;
        const float lower = level == 0 ? 0.0f : model->GetGeometry(i, level)->GetLodDistance();
        if (level + 1 >= numLevels)
            return lower * 1.5f + 1.0f;
        const float upper = model->GetGeometry(i, level + 1)->GetLodDistance();
        // The engine selects the level when the distance is above the lower value and not above the upper value.
        return upper > lower ? (lower + upper) * 0.5f : lower + 1.0f;
    }
    return 1.0f;
}

} // namespace

SharedPtr<Animation> AssetViewer::LoadAnimationResource(const ea::string& fileName)
{
    auto animation = MakeShared<Animation>(context_);
    animation->SetName(GetFileNameAndExtension(fileName));
    animation->SetAbsoluteFileName(fileName);
    return animation->LoadFile(fileName) ? animation : nullptr;
}

bool AssetViewer::HasAnimationTarget() const
{
    if (!contentRoot_ || !animationDebugNodes_.empty())
        return false;
    ea::vector<AnimatedModel*> models;
    contentRoot_->FindComponents<AnimatedModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    return ea::any_of(models.begin(), models.end(), [](AnimatedModel* model)
    {
        return model->GetModel() && model->GetModel()->GetSkeleton().GetNumBones() > 0;
    });
}

void AssetViewer::BuildAnimationDebugPreview(Animation* animation)
{
    contentRoot_ = scene_->CreateChild("Animation Debug Preview");
    ea::unordered_map<ea::string, Node*> trackNodes;
    const unsigned columns = Max(1, CeilToInt(Sqrt(static_cast<float>(animation->GetNumTracks()))));
    unsigned index = 0;
    for (const auto& [_, track] : animation->GetTracks())
    {
        Node* node = contentRoot_->CreateChild(track.name_);
        Transform transform;
        unsigned frame = 0;
        track.Sample(0.0f, animation->GetLength(), false, frame, transform);
        if (!track.channelMask_.Test(CHANNEL_POSITION))
            transform.position_ =
                Vector3(static_cast<float>(index % columns), 0.0f, static_cast<float>(index / columns)) * 0.35f;
        node->SetTransform(transform);
        animationDebugNodes_.emplace_back(node);
        trackNodes[track.name_] = node;
        ++index;
    }

    const Variant& parentMetadata = animation->GetMetadata(AnimationMetadata::ParentTracks);
    if (parentMetadata.GetType() == VAR_STRINGVARIANTMAP)
    {
        for (const auto& [name, parentValue] : parentMetadata.GetStringVariantMap())
        {
            const auto nodeIter = trackNodes.find(name);
            const auto parentIter = trackNodes.find(parentValue.GetString());
            if (nodeIter != trackNodes.end() && parentIter != trackNodes.end()
                && nodeIter->second != parentIter->second)
                nodeIter->second->SetParent(parentIter->second);
        }
    }

    contentRoot_->CreateComponent<AnimationController>();
}

bool AssetViewer::TryLoadAnimationOnCurrentAsset(const ea::string& fileName, ea::string* mismatchNote)
{
    if (!contentRoot_ || animationDebugNodes_.size())
        return false;

    SharedPtr<Animation> animation = LoadAnimationResource(fileName);
    if (!animation)
        return false;

    ea::vector<AnimatedModel*> models;
    contentRoot_->FindComponents<AnimatedModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    ea::vector<AnimatedModel*> compatibleModels;
    for (AnimatedModel* model : models)
    {
        if (IsAnimationCompatible(animation, model))
            compatibleModels.push_back(model);
    }
    if (compatibleModels.empty())
    {
        if (mismatchNote && HasAnimationTarget())
            *mismatchNote = Format("Animation {} does not fit the previous model ({})",
                GetAnimationLabel(animation), DescribeTrackMatch({animation}));
        return false;
    }

    for (AnimatedModel* model : compatibleModels)
    {
        if (!model->GetNode()->GetComponent<AnimationController>())
            model->GetNode()->CreateComponent<AnimationController>();
    }

    assetResources_.push_back(animation);
    animationSources_[animation] = GetFileNameAndExtension(fileName);
    error_.clear();
    lastFailedFile_.clear();
    CollectSceneResources();
    CollectAnimations();
    const auto iter = ea::find(animations_.begin(), animations_.end(), animation.Get());
    if (iter != animations_.end())
        SetSelectedAnimation(static_cast<int>(iter - animations_.begin()), true);
    statusMessage_ = Format("Added animation {} from file {} ({})", GetAnimationLabel(animation),
        GetFileNameAndExtension(fileName), DescribeTrackMatch({animation}));
    GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_) + " + "
        + GetFileNameAndExtension(fileName));
    URHO3D_LOGINFO("AssetViewer applied animation '{}' to the loaded model", fileName);
    return true;
}

bool AssetViewer::LoadAsset(const ea::string& requestedFile)
{
    if (loading_)
    {
        queuedFile_ = requestedFile;
        URHO3D_LOGINFO("AssetViewer queued '{}'", requestedFile);
        return false;
    }

    FileSystem* fs = GetSubsystem<FileSystem>();
    const ea::string absoluteFile = GetAbsolutePath(requestedFile, fs->GetCurrentDir());
    requestedFile_ = absoluteFile;
    if (fs->DirExists(absoluteFile))
        return Fail(Format("'{}' is a directory. Drop a single file instead.", absoluteFile));
    if (!fs->FileExists(absoluteFile))
        return Fail(Format("File does not exist: {}", absoluteFile));
    const ea::string extension = GetExtension(absoluteFile);
    if (!IsSupportedAssetExtension(extension))
        return Fail(Format("Unsupported file type {} for '{}'.\nSupported extensions: {}",
            extension.empty() ? "(no extension)" : "'" + extension + "'", GetFileNameAndExtension(absoluteFile),
            GetSupportedExtensionsText()));
    // A skinned model is loaded. The file can add animations to it or replace the model.
    ea::string mismatchNote;
    if (extension == ".ani" && TryLoadAnimationOnCurrentAsset(absoluteFile, &mismatchNote))
        return true;
    if (extension == ".fbx" && HasAnimationTarget())
        return BeginLoadPotentialFBXAnimation(absoluteFile);
    if ((extension == ".gltf" || extension == ".glb") && HasAnimationTarget())
    {
        bool isAnimationFile = false;
        const bool applied = TryLoadGLTFAnimationOnCurrentAsset(absoluteFile, isAnimationFile);
        if (isAnimationFile)
            return applied;
    }

    ClearAsset();
    sourceFile_ = absoluteFile;
    bool success = false;
    if (extension == ".gltf" || extension == ".glb")
        success = LoadGLTFAsset(sourceFile_);
    else if (extension == ".fbx")
        return BeginLoadFBXAsset();
    else if (extension == ".blend")
        success = ConvertAndLoadExternal(extension);
    else if (extension == ".mdl")
        success = LoadNativeModel();
    else if (extension == ".prefab")
        success = LoadNativePrefab();
    else if (extension == ".scene")
        success = LoadNativeScene();
    else if (extension == ".material")
        success = LoadMaterialAsset();
    else if (extension == ".ani")
        success = LoadAnimationAsset();
    else if (extension == ".pex")
        success = LoadParticleEffect2DAsset();
    else if (extension == ".xml")
        success = LoadXMLAsset();
    else if (IsImageExtension(extension))
        success = LoadTextureAsset();
    else
        return Fail(Format("Unsupported file type '{}'", extension));

    if (success)
    {
        FinalizeAssetLoad(extension);
        if (!mismatchNote.empty())
            statusMessage_ += "; " + mismatchNote;
    }
    return success;
}

void AssetViewer::FinalizeAssetLoad(const ea::string& extension)
{
    EnsureVisibleMaterials();
    CollectSceneResources();
    SetNeutralMaterials(neutralMaterials_);
    CollectAnimations();
    CollectLods();
    SetSelectedLod(0);
    CollectParticleEmitters();
    UpdatePreviewProjection();
    if (extension == ".ani" && !animations_.empty())
        SetSelectedAnimation(selectedAnimation_, true);
    if (legacyParticleEmitters_.empty() && graphParticleEmitters_.empty() && particleEmitters2D_.empty())
    {
        FrameContent();
        framesUntilFrame_ = 2;
    }
    GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_));
    environment_.Update(GetContentBoundingBox(true));
    const bool isModel = extension == ".fbx" || extension == ".gltf" || extension == ".glb" || extension == ".blend"
        || extension == ".mdl" || extension == ".prefab" || extension == ".scene";
    statusMessage_ = Format("Loaded {}{}", isModel ? "model " : "", GetFileNameAndExtension(sourceFile_));
    URHO3D_LOGINFO("AssetViewer loaded '{}'", sourceFile_);
}

bool AssetViewer::ConvertAndLoadExternal(const ea::string& extension)
{
    FileSystem* fs = GetSubsystem<FileSystem>();
    const ea::string tempPath =
        AddTrailingSlash(fs->GetTemporaryDir()) + "rbfx-asset-viewer-" + GenerateUUID() + "/";
    tempDir_ = ea::make_unique<TemporaryDir>(context_, tempPath);
    convertedFile_ = tempPath + "converted.glb";

    const ea::string script = Format(
        "import "
        "bpy;bpy.ops.export_scene.gltf(filepath='{}',export_format='GLB',export_apply=True,export_def_bones="
        "True);",
        convertedFile_);
    const StringVector arguments = {"-b", sourceFile_, "--python-expr", script};

    if (fs->SystemRun(blender_, arguments, converterOutput_) != 0 || !fs->FileExists(convertedFile_))
        return Fail(Format("Failed to convert {} with '{}'.\n{}", extension, blender_, converterOutput_));
    return LoadGLTFAsset(convertedFile_);
}

GLTFImporterSettings AssetViewer::GetImportSettings(const ea::string& fileName) const
{
    GLTFImporterSettings settings;
    settings.assetName_ = GetFileName(fileName.empty() ? sourceFile_ : fileName);
    settings.gpuResources_ = true;
    settings.preview_.addLights_ = false;
    settings.preview_.addSkybox_ = false;
    settings.preview_.addReflectionProbe_ = false;
    settings.highlightMissingTextures_ = true;
    return settings;
}

bool AssetViewer::BeginLoadFBXAsset()
{
    convertedFile_.clear();
    const ea::string prefix = PrepareImportOutput();
    const ea::string outputPath = tempDir_->GetPath();
    const auto importer = MakeShared<FBXImporter>(context_, GetImportSettings());
    const ea::string fileName = sourceFile_;
    const WeakPtr<AssetViewer> weakSelf{this};
    loading_ = true;
    loadingFile_ = fileName;
    GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - Loading " + GetFileNameAndExtension(fileName));
    GetSubsystem<WorkQueue>()->PostTask(
        [importer, fileName, outputPath, prefix, weakSelf](unsigned, WorkQueue* queue)
    {
        const bool success = importer->LoadFile(fileName) && importer->BeginProcess(outputPath, prefix, nullptr);
        queue->PostDelayedTaskForMainThread([importer, fileName, weakSelf, success]()
        {
            if (const auto self = weakSelf.Lock())
                self->CompleteFBXAsset(importer, fileName, success);
        });
    });
    return true;
}

void AssetViewer::CompleteFBXAsset(const SharedPtr<FBXImporter>& importer, const ea::string& fileName, bool sourcePrepared)
{
    const ScopeExit startQueued{[this] { StartQueuedFile(); }};
    if (!loading_ || fileName != sourceFile_)
        return;
    loading_ = false;
    loadingFile_.clear();
    const bool success = sourcePrepared && importer->EndProcess()
        ? InstantiateImportedAsset(importer)
        : Fail("rbfx FBXImporter failed to prepare the asset");
    if (success)
    {
        primaryFBXFile_ = fileName;
        FinalizeAssetLoad(".fbx");
    }
}

bool AssetViewer::BeginLoadPotentialFBXAnimation(const ea::string& fileName)
{
    const ea::string tempPath = AddTrailingSlash(GetSubsystem<FileSystem>()->GetTemporaryDir())
        + "rbfx-asset-viewer-" + GenerateUUID() + "/";
    pendingTempDir_ = ea::make_unique<TemporaryDir>(context_, tempPath);
    const ea::string outputPath = pendingTempDir_->GetPath();
    const ea::string prefix = "AssetViewer/" + GenerateUUID() + "/";
    const ea::string primaryFileName = primaryFBXFile_;
    const ea::string animationName = GetFileName(fileName);
    const auto standaloneImporter = MakeShared<FBXImporter>(context_, GetImportSettings(fileName));
    const auto mergedImporter = MakeShared<FBXImporter>(context_, GetImportSettings(primaryFileName));
    const WeakPtr<AssetViewer> weakSelf{this};

    error_.clear();
    lastFailedFile_.clear();
    loading_ = true;
    loadingFile_ = fileName;
    GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - Loading " + GetFileNameAndExtension(fileName));
    GetSubsystem<WorkQueue>()->PostTask(
        [standaloneImporter, mergedImporter, fileName, primaryFileName, animationName, outputPath, prefix,
            weakSelf](unsigned, WorkQueue* queue)
    {
        const bool sourceLoaded = standaloneImporter->LoadFile(fileName);
        const bool animationOnly = sourceLoaded && standaloneImporter->IsAnimationOnly();
        unsigned primaryAnimationCount = 0;
        SharedPtr<FBXImporter> importer = standaloneImporter;
        bool success = false;
        if (animationOnly && primaryFileName.empty())
            success = true;
        else if (animationOnly)
        {
            importer = mergedImporter;
            success = importer->LoadFile(primaryFileName);
            if (success)
            {
                primaryAnimationCount = importer->GetNumAnimations();
                success = importer->MergeFile(fileName, animationName);
            }
        }
        else
            success = sourceLoaded;
        success = success && importer->BeginProcess(outputPath, prefix, nullptr);

        queue->PostDelayedTaskForMainThread(
            [importer, fileName, weakSelf, animationOnly, primaryAnimationCount, success]()
        {
            if (const auto self = weakSelf.Lock())
                self->CompletePotentialFBXAnimation(
                    importer, fileName, animationOnly, primaryAnimationCount, success);
        });
    });
    return true;
}

void AssetViewer::CompletePotentialFBXAnimation(const SharedPtr<FBXImporter>& importer, const ea::string& fileName,
    bool animationOnly, unsigned primaryAnimationCount, bool sourcePrepared)
{
    const ScopeExit startQueued{[this] { StartQueuedFile(); }};
    if (!loading_ || fileName != loadingFile_)
        return;
    loading_ = false;
    loadingFile_.clear();

    if (!sourcePrepared || !importer->EndProcess())
    {
        pendingTempDir_.reset();
        Fail("rbfx FBXImporter failed to prepare the asset");
        GetSubsystem<Graphics>()->SetWindowTitle(
            "rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_));
        return;
    }

    if (animationOnly)
    {
        ApplyImportedFBXAnimations(importer, fileName, primaryAnimationCount);
        pendingTempDir_.reset();
        return;
    }

    auto tempDir = ea::move(pendingTempDir_);
    ClearAsset();
    tempDir_ = ea::move(tempDir);
    sourceFile_ = fileName;
    if (InstantiateImportedAsset(importer))
    {
        primaryFBXFile_ = fileName;
        FinalizeAssetLoad(".fbx");
    }
}

bool AssetViewer::ApplyImportedFBXAnimations(
    const SharedPtr<FBXImporter>& importer, const ea::string& fileName, unsigned primaryAnimationCount)
{
    ea::vector<SharedPtr<Animation>> animations;
    for (const SharedPtr<Resource>& resource : GetImportedResources(importer))
    {
        if (auto animation = DynamicCast<Animation>(resource))
            animations.push_back(animation);
    }
    return ApplyExternalAnimations(ea::move(animations), fileName, primaryAnimationCount);
}

unsigned AssetViewer::CountMatchingTracks(Animation* animation, AnimatedModel* animatedModel) const
{
    if (!animation || !animatedModel || !animatedModel->GetModel())
        return 0;
    const Skeleton& skeleton = animatedModel->GetModel()->GetSkeleton();
    unsigned matched = 0;
    for (const auto& [nameHash, _] : animation->GetTracks())
    {
        if (skeleton.GetBoneIndex(nameHash) != M_MAX_UNSIGNED)
            ++matched;
    }
    return matched;
}

ea::string AssetViewer::DescribeTrackMatch(const ea::vector<SharedPtr<Animation>>& animations) const
{
    ea::vector<AnimatedModel*> models;
    if (contentRoot_)
        contentRoot_->FindComponents<AnimatedModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    unsigned bestMatched = 0;
    unsigned bestTotal = 0;
    for (Animation* animation : animations)
    {
        for (AnimatedModel* model : models)
        {
            const unsigned matched = CountMatchingTracks(animation, model);
            if (matched > bestMatched || (matched == bestMatched && animation->GetNumTracks() > bestTotal))
            {
                bestMatched = matched;
                bestTotal = animation->GetNumTracks();
            }
        }
        if (models.empty())
            bestTotal = Max(bestTotal, animation->GetNumTracks());
    }
    return Format("{} of {} tracks match", bestMatched, bestTotal);
}

ea::string AssetViewer::GetAnimationSource(Animation* animation) const
{
    const auto iter = animationSources_.find(animation);
    if (iter != animationSources_.end())
        return iter->second;
    return GetExtension(sourceFile_) == ".ani" ? GetFileNameAndExtension(sourceFile_) : "embedded";
}

bool AssetViewer::ApplyExternalAnimations(
    ea::vector<SharedPtr<Animation>> animations, const ea::string& fileName, unsigned primaryAnimationCount)
{
    ea::vector<AnimatedModel*> models;
    contentRoot_->FindComponents<AnimatedModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);

    int nextAnimationIndex = 0;
    for (const SharedPtr<Resource>& resource : assetResources_)
    {
        if (const auto* animation = dynamic_cast<Animation*>(resource.Get()))
        {
            const Variant& value = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
            if (value.GetType() == VAR_INT)
                nextAnimationIndex = Max(nextAnimationIndex, value.GetInt() + 1);
        }
    }

    ea::unordered_map<int, int> remappedIndices;
    ea::vector<SharedPtr<Animation>> candidates;
    ea::vector<SharedPtr<Animation>> compatibleAnimations;
    for (const SharedPtr<Animation>& animation : animations)
    {
        const Variant& value = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
        const bool hasIndex = value.GetType() == VAR_INT;
        if (hasIndex && value.GetInt() < static_cast<int>(primaryAnimationCount))
            continue;
        candidates.push_back(animation);
        if (!ea::any_of(models.begin(), models.end(),
                [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); }))
            continue;

        if (hasIndex)
        {
            const auto [iter, inserted] = remappedIndices.emplace(value.GetInt(), nextAnimationIndex);
            if (inserted)
                ++nextAnimationIndex;
            animation->AddMetadata(AnimationMetadata::SourceAnimationIndex, iter->second);
        }
        compatibleAnimations.push_back(animation);
    }
    if (compatibleAnimations.empty())
    {
        GetSubsystem<Graphics>()->SetWindowTitle(
            "rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_));
        return Fail(Format("No animation in '{}' matches the loaded model ({}). The model stays loaded.",
            GetFileNameAndExtension(fileName), DescribeTrackMatch(candidates)));
    }

    const ea::string source = GetFileNameAndExtension(fileName);
    for (const SharedPtr<Animation>& animation : compatibleAnimations)
    {
        assetResources_.push_back(animation);
        animationSources_[animation] = source;
    }
    for (AnimatedModel* model : models)
    {
        const bool compatible = ea::any_of(compatibleAnimations.begin(), compatibleAnimations.end(),
            [this, model](Animation* animation) { return IsAnimationCompatible(animation, model); });
        if (compatible && !model->GetNode()->GetComponent<AnimationController>())
            model->GetNode()->CreateComponent<AnimationController>();
    }

    error_.clear();
    lastFailedFile_.clear();
    CollectSceneResources();
    CollectAnimations();
    const auto selected = ea::find(animations_.begin(), animations_.end(), compatibleAnimations.front().Get());
    if (selected != animations_.end())
        SetSelectedAnimation(static_cast<int>(selected - animations_.begin()), true);
    ea::string names = GetAnimationLabel(compatibleAnimations.front());
    if (compatibleAnimations.size() > 1)
        names += Format(" (+{} more)", compatibleAnimations.size() - 1);
    statusMessage_ = Format("Added animation {} from file {} ({})", names, source,
        DescribeTrackMatch(compatibleAnimations));
    GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_) + " + "
        + source);
    URHO3D_LOGINFO("AssetViewer applied {} animation resource(s) from '{}' to the loaded model",
        compatibleAnimations.size(), fileName);
    return true;
}

bool AssetViewer::TryLoadGLTFAnimationOnCurrentAsset(const ea::string& fileName, bool& isAnimationFile)
{
    isAnimationFile = false;
    // Parse without image decoding. An animation file has animations and no meshes.
    tinygltf::Model parsed;
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*,
                              int, void*) { return true; },
        nullptr);
    std::string errors;
    const bool ok = GetExtension(fileName) == ".gltf"
        ? loader.LoadASCIIFromFile(&parsed, &errors, nullptr, fileName.c_str())
        : loader.LoadBinaryFromFile(&parsed, &errors, nullptr, fileName.c_str());
    if (!ok || !parsed.meshes.empty() || parsed.animations.empty())
        return false;
    isAnimationFile = true;

    const ea::string tempPath = AddTrailingSlash(GetSubsystem<FileSystem>()->GetTemporaryDir())
        + "rbfx-asset-viewer-" + GenerateUUID() + "/";
    pendingTempDir_ = ea::make_unique<TemporaryDir>(context_, tempPath);
    const auto importer = MakeShared<GLTFImporter>(context_, GetImportSettings(fileName));
    const bool processed = importer->LoadFile(fileName)
        && importer->Process(pendingTempDir_->GetPath(), "AssetViewer/" + GenerateUUID() + "/", nullptr);
    ea::vector<SharedPtr<Animation>> animations;
    if (processed)
    {
        for (const SharedPtr<Resource>& resource : GetImportedResources(importer))
        {
            if (auto animation = DynamicCast<Animation>(resource))
                animations.push_back(animation);
        }
    }
    const bool applied = processed && ApplyExternalAnimations(ea::move(animations), fileName, 0);
    if (!processed)
        Fail(Format("rbfx GLTFImporter failed to read the animations of '{}'", GetFileNameAndExtension(fileName)));
    // The animations stay in the resource cache. The temporary directory holds no file they need.
    pendingTempDir_.reset();
    return applied;
}

bool AssetViewer::LoadGLTFAsset(const ea::string& gltfFile)
{
    convertedFile_ = gltfFile == sourceFile_ ? EMPTY_STRING : gltfFile;
    sourceJson_ = LoadGLTFJsonChunk(context_, gltfFile);
    if (!sourceJson_.empty())
    {
        JSONFile json(context_);
        if (json.FromString(sourceJson_))
            sourceJson_ = json.ToString("  ");
    }

    gltfModel_ = ea::make_unique<tinygltf::Model>();
    tinygltf::TinyGLTF loader;
    loader.SetStoreOriginalJSONForExtrasAndExtensions(true);
    std::string warnings;
    std::string errors;
    const bool parsed = GetExtension(gltfFile) == ".gltf"
        ? loader.LoadASCIIFromFile(gltfModel_.get(), &errors, &warnings, gltfFile.c_str())
        : loader.LoadBinaryFromFile(gltfModel_.get(), &errors, &warnings, gltfFile.c_str());
    if (!warnings.empty())
        converterOutput_ += Format("\nTinyGLTF warnings:\n{}", warnings.c_str());
    if (!parsed)
        return Fail(Format("Failed to parse glTF source.\n{}", errors.c_str()));

    const auto importer = MakeShared<GLTFImporter>(context_, GetImportSettings());
    if (!importer->LoadFile(gltfFile))
        return Fail("rbfx GLTFImporter failed to load the asset");

    return ProcessImportedAsset(importer);
}

template <class T>
bool AssetViewer::ProcessImportedAsset(const SharedPtr<T>& importer)
{
    const ea::string prefix = PrepareImportOutput();
    if (!importer->Process(tempDir_->GetPath(), prefix, nullptr))
        return Fail("rbfx model importer failed while converting the asset");
    return InstantiateImportedAsset(importer);
}

ea::string AssetViewer::PrepareImportOutput()
{
    if (!tempDir_)
    {
        const ea::string tempPath = AddTrailingSlash(GetSubsystem<FileSystem>()->GetTemporaryDir())
            + "rbfx-asset-viewer-" + GenerateUUID() + "/";
        tempDir_ = ea::make_unique<TemporaryDir>(context_, tempPath);
    }
    return "AssetViewer/" + GenerateUUID() + "/";
}

template <class T>
ea::vector<SharedPtr<Resource>> AssetViewer::GetImportedResources(const SharedPtr<T>& importer) const
{
    ea::unordered_set<ea::string> importedNames;
    for (const auto& [resourceName, _] : importer->GetSavedResources())
        importedNames.insert(resourceName);

    ea::vector<SharedPtr<Resource>> result;
    for (const auto& [_, group] : GetSubsystem<ResourceCache>()->GetAllResources())
    {
        for (const auto& [__, resource] : group.resources_)
        {
            if (resource && importedNames.contains(resource->GetName()))
                result.push_back(resource);
        }
    }
    return result;
}

template <class T>
bool AssetViewer::InstantiateImportedAsset(const SharedPtr<T>& importer)
{
    importer_ = importer;
    if constexpr (ea::is_same_v<T, FBXImporter>)
        missingTextures_ = importer->GetMissingTextures();

    PrefabResource* prefab = nullptr;
    for (const SharedPtr<Resource>& resource : GetImportedResources(importer))
    {
        assetResources_.push_back(resource);
        if (!prefab)
            prefab = dynamic_cast<PrefabResource*>(resource.Get());
    }
    if (!prefab)
        return Fail("Imported model did not produce a preview prefab");

    contentRoot_ = scene_->InstantiatePrefab(prefab);
    if (!contentRoot_)
        return Fail("Failed to instantiate imported model prefab");
    ea::vector<AnimationController*> controllers;
    contentRoot_->FindComponents<AnimationController>(
        controllers, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (AnimationController* controller : controllers)
        animationControllers_.emplace_back(controller);
    SetAnimationsPlaying(false);
    return true;
}

void AssetViewer::MountSourceDirectory()
{
    MountPoint* mount = GetSubsystem<VirtualFileSystem>()->MountDir(GetPath(sourceFile_));
    mountedDirectory_ = ea::make_unique<MountPointGuard>(mount);
}

bool AssetViewer::LoadTextureAsset()
{
    MountSourceDirectory();

    auto image = MakeShared<Image>(context_);
    image->SetName(GetFileNameAndExtension(sourceFile_));
    image->SetAbsoluteFileName(sourceFile_);
    if (!image->LoadFile(sourceFile_))
        return Fail("Failed to decode image/texture file");
    assetResources_.push_back(image);

    SharedPtr<Texture> texture;
    if (image->IsCubemap())
        texture = MakeShared<TextureCube>(context_);
    else if (image->IsArray())
        texture = MakeShared<Texture2DArray>(context_);
    else if (image->GetDepth() > 1)
        texture = MakeShared<Texture3D>(context_);
    else
        texture = MakeShared<Texture2D>(context_);
    texture->SetName(GetFileNameAndExtension(sourceFile_));
    texture->SetAbsoluteFileName(sourceFile_);
    if (texture->LoadFile(sourceFile_))
        assetResources_.push_back(texture);

    if (auto texture2D = DynamicCast<Texture2D>(texture))
        previewTexture_ = texture2D;
    else
    {
        SharedPtr<Image> displayImage = image;
        if (image->IsCompressed())
            displayImage = image->GetDecompressedImage();
        if (displayImage)
        {
            previewTexture_ = MakeShared<Texture2D>(context_);
            if (!previewTexture_->SetData(displayImage))
                previewTexture_.Reset();
        }
    }
    return true;
}

bool AssetViewer::LoadTextureDescriptor(const ea::string& rootName)
{
    MountSourceDirectory();
    SharedPtr<Texture> texture;
    if (rootName == "cubemap")
        texture = MakeShared<TextureCube>(context_);
    else if (rootName == "texture3d")
        texture = MakeShared<Texture3D>(context_);
    else
        texture = MakeShared<Texture2DArray>(context_);
    texture->SetName(GetFileNameAndExtension(sourceFile_));
    texture->SetAbsoluteFileName(sourceFile_);
    if (!texture->LoadFile(sourceFile_))
        return Fail(Format("Failed to load {} texture descriptor", rootName));
    assetResources_.push_back(texture);

    if (auto* cube = dynamic_cast<TextureCube*>(texture.Get()))
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        contentRoot_ = scene_->CreateChild("Cubemap Preview");
        Skybox* skybox = contentRoot_->CreateComponent<Skybox>();
        skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        auto material = MakeShared<Material>(context_);
        material->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffSkybox.xml"));
        material->SetTexture(ShaderResources::Albedo, cube);
        skybox->SetMaterial(material);
        assetResources_.push_back(material);
        if (Zone* zone = scene_->FindComponent<Zone>())
            zone->SetZoneTexture(cube);
    }
    return true;
}

bool AssetViewer::LoadMaterialAsset()
{
    MountSourceDirectory();
    auto material = MakeShared<Material>(context_);
    material->SetName(GetFileNameAndExtension(sourceFile_));
    material->SetAbsoluteFileName(sourceFile_);
    if (!material->LoadFile(sourceFile_))
        return Fail("Failed to load rbfx material");
    assetResources_.push_back(material);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Model* previewModel = cache->GetResource<Model>("Models/MaterialPreview.mdl");
    if (!previewModel)
        previewModel = cache->GetResource<Model>("Models/Sphere.mdl");
    if (previewModel)
    {
        contentRoot_ = scene_->CreateChild("Material Preview");
        StaticModel* drawable = contentRoot_->CreateComponent<StaticModel>();
        drawable->SetModel(previewModel);
        drawable->SetMaterial(material);
    }
    return true;
}

bool AssetViewer::LoadAnimationAsset()
{
    MountSourceDirectory();
    SharedPtr<Animation> animation = LoadAnimationResource(sourceFile_);
    if (!animation)
        return Fail("Failed to load rbfx animation");
    assetResources_.push_back(animation);
    BuildAnimationDebugPreview(animation);
    return true;
}

bool AssetViewer::LoadParticleEffectAsset()
{
    MountSourceDirectory();
    auto effect = MakeShared<ParticleEffect>(context_);
    effect->SetName(GetFileNameAndExtension(sourceFile_));
    effect->SetAbsoluteFileName(sourceFile_);
    if (!effect->LoadFile(sourceFile_))
        return Fail("Failed to load legacy 3D particle effect");
    assetResources_.push_back(effect);
    contentRoot_ = scene_->CreateChild(GetFileName(sourceFile_));
    ParticleEmitter* emitter = contentRoot_->CreateComponent<ParticleEmitter>();
    emitter->SetEffect(effect);
    emitter->SetEmitting(true);
    return true;
}

bool AssetViewer::LoadParticleGraphEffectAsset()
{
    MountSourceDirectory();
    auto effect = MakeShared<ParticleGraphEffect>(context_);
    effect->SetName(GetFileNameAndExtension(sourceFile_));
    effect->SetAbsoluteFileName(sourceFile_);
    if (!effect->LoadFile(sourceFile_))
        return Fail("Failed to load particle graph effect");
    assetResources_.push_back(effect);
    contentRoot_ = scene_->CreateChild(GetFileName(sourceFile_));
    ParticleGraphEmitter* emitter = contentRoot_->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    emitter->SetEmitting(true);
    return true;
}

bool AssetViewer::LoadParticleEffect2DAsset()
{
    MountSourceDirectory();
    XMLFile document(context_);
    if (document.LoadFile(sourceFile_))
        sourceText_ = document.ToString("  ");

    auto effect = MakeShared<ParticleEffect2D>(context_);
    effect->SetName(GetFileNameAndExtension(sourceFile_));
    effect->SetAbsoluteFileName(sourceFile_);
    if (!effect->LoadFile(sourceFile_))
        return Fail("Failed to load Urho2D particle effect");
    assetResources_.push_back(effect);
    contentRoot_ = scene_->CreateChild(GetFileName(sourceFile_));
    ParticleEmitter2D* emitter = contentRoot_->CreateComponent<ParticleEmitter2D>();
    emitter->SetEffect(effect);
    emitter->SetEmitting(true);
    return true;
}

bool AssetViewer::LoadXMLAsset()
{
    XMLFile document(context_);
    if (!document.LoadFile(sourceFile_))
        return Fail("Failed to parse XML resource");
    sourceText_ = document.ToString("  ");
    const ea::string rootName = document.GetRoot().GetName();
    if (rootName == "scene")
        return LoadNativeScene();
    if (rootName == "material")
        return LoadMaterialAsset();
    if (rootName == "cubemap" || rootName == "texture3d" || rootName == "texturearray")
        return LoadTextureDescriptor(rootName);
    if (rootName == "particleeffect" || rootName == "particleemitter")
        return LoadParticleEffectAsset();
    if (rootName == "particleGraphEffect")
        return LoadParticleGraphEffectAsset();
    return Fail(Format("Unsupported XML resource root '{}'", rootName));
}

bool AssetViewer::LoadNativeModel()
{
    MountSourceDirectory();
    auto model = MakeShared<Model>(context_);
    model->SetName(GetFileNameAndExtension(sourceFile_));
    model->SetAbsoluteFileName(sourceFile_);
    if (!model->LoadFile(sourceFile_))
        return Fail("Failed to load native rbfx model");

    assetResources_.push_back(model);
    contentRoot_ = scene_->CreateChild(GetFileName(sourceFile_));
    StaticModel* drawable = model->GetSkeleton().GetNumBones() > 0
        ? static_cast<StaticModel*>(contentRoot_->CreateComponent<AnimatedModel>())
        : contentRoot_->CreateComponent<StaticModel>();
    drawable->SetModel(model);
    drawable->ApplyMaterialList(ReplaceExtension(sourceFile_, ".txt"));
    if (model->GetSkeleton().GetNumBones() > 0)
        contentRoot_->CreateComponent<AnimationController>();
    return true;
}

bool AssetViewer::LoadNativePrefab()
{
    MountSourceDirectory();
    auto prefab = MakeShared<PrefabResource>(context_);
    prefab->SetName(GetFileNameAndExtension(sourceFile_));
    prefab->SetAbsoluteFileName(sourceFile_);
    if (!prefab->LoadFile(sourceFile_))
        return Fail("Failed to load native rbfx prefab");
    assetResources_.push_back(prefab);
    contentRoot_ = scene_->InstantiatePrefab(prefab);
    return contentRoot_ != nullptr || Fail("Failed to instantiate native rbfx prefab");
}

bool AssetViewer::LoadNativeScene()
{
    MountSourceDirectory();
    auto resource = MakeShared<SceneResource>(context_);
    resource->SetName(GetFileNameAndExtension(sourceFile_));
    resource->SetAbsoluteFileName(sourceFile_);
    if (!resource->LoadFile(sourceFile_))
        return Fail("Failed to load native rbfx scene");

    assetResources_.push_back(resource);
    scene_ = resource->GetScene();
    contentRoot_ = scene_;
    if (!scene_->GetComponent<Octree>())
        scene_->CreateComponent<Octree>();
    if (!scene_->GetComponent<DebugRenderer>())
        scene_->CreateComponent<DebugRenderer>();
    cameraNode_ = scene_->CreateChild("Viewer Camera");
    cameraNode_->SetTemporary(true);
    camera_ = cameraNode_->CreateComponent<Camera>();
    freeFlyController_ = cameraNode_->CreateComponent<FreeFlyController>();
    freeFlyController_->SetEnabled(false);
    camera_->SetNearClip(0.01f);
    camera_->SetFarClip(100000.0f);
    viewport_->SetScene(scene_);
    viewport_->SetCamera(camera_);
    environment_.Attach(scene_, camera_);
    return true;
}

void AssetViewer::EnsureVisibleMaterials()
{
    if (!contentRoot_)
        return;
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Material* defaultMaterial = cache->GetResource<Material>("Materials/DefaultGrey.xml");
    Technique* unlitTechnique = cache->GetResource<Technique>("Techniques/NoTextureUnlit.xml");
    if (!defaultMaterial || !unlitTechnique)
        return;
    SharedPtr<Material> fallback = defaultMaterial->Clone("AssetViewer/Fallback");
    fallback->SetTechnique(0, unlitTechnique);
    fallback->SetVertexShaderDefines("IGNOREVERTEXCOLOR ");
    fallback->SetPixelShaderDefines("IGNOREVERTEXCOLOR ");
    fallback->SetCullMode(CULL_NONE);

    ea::vector<StaticModel*> models;
    contentRoot_->FindComponents<StaticModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (StaticModel* model : models)
    {
        for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
        {
            if (!model->GetMaterial(i))
            {
                model->SetMaterial(i, fallback);
                ++fallbackMaterialSlots_;
            }
        }
    }
}

void AssetViewer::SetNeutralMaterials(bool neutral)
{
    // Restore first. Then a repeated call never saves a neutral material as the original.
    for (const SavedMaterial& saved : savedMaterials_)
    {
        if (saved.model_)
            saved.model_->SetMaterial(saved.slot_, saved.material_);
    }
    savedMaterials_.clear();
    neutralMaterials_ = neutral;
    if (!neutral || !contentRoot_)
        return;
    // The material preview shows the authored material. Keep it.
    if (contentRoot_->GetName() == "Material Preview")
        return;

    if (!neutralMaterial_)
    {
        Material* defaultMaterial = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/DefaultGrey.xml");
        if (!defaultMaterial)
            return;
        neutralMaterial_ = defaultMaterial->Clone("AssetViewer/Neutral");
        neutralMaterial_->SetVertexShaderDefines("PBR IGNOREVERTEXCOLOR ");
        neutralMaterial_->SetPixelShaderDefines("PBR IGNOREVERTEXCOLOR ");
    }

    ea::vector<StaticModel*> models;
    contentRoot_->FindComponents<StaticModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (StaticModel* model : models)
    {
        if (dynamic_cast<Skybox*>(model) || environment_.IsEnvironmentNode(model->GetNode()))
            continue;
        for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
        {
            savedMaterials_.push_back({WeakPtr<StaticModel>(model), i, SharedPtr<Material>(model->GetMaterial(i))});
            model->SetMaterial(i, neutralMaterial_);
        }
    }
}

void AssetViewer::AddResource(Resource* resource, ea::unordered_set<Resource*>& visited)
{
    // The neutral material is a viewer helper. It is not an asset resource.
    if (!resource || resource == neutralMaterial_.Get() || !visited.insert(resource).second)
        return;
    assetResources_.push_back(SharedPtr<Resource>(resource));
    if (auto* material = dynamic_cast<Material*>(resource))
    {
        for (const auto& [_, texture] : material->GetTextures())
            AddResource(texture.value_, visited);
    }
    else if (auto* effect = dynamic_cast<ParticleEffect*>(resource))
        AddResource(effect->GetMaterial(), visited);
    else if (auto* effect = dynamic_cast<ParticleEffect2D*>(resource))
        AddResource(effect->GetSprite(), visited);
    else if (auto* sprite = dynamic_cast<Sprite2D*>(resource))
        AddResource(sprite->GetTexture(), visited);
    else if (auto* effect = dynamic_cast<ParticleGraphEffect*>(resource))
    {
        for (unsigned layerIndex = 0; layerIndex < effect->GetNumLayers(); ++layerIndex)
        {
            ParticleGraphLayer* layer = effect->GetLayer(layerIndex);
            CollectSerializableResources(layer, visited);
            ParticleGraph* graphs[]{&layer->GetEmitGraph(), &layer->GetInitGraph(), &layer->GetUpdateGraph()};
            for (ParticleGraph* graph : graphs)
            {
                for (unsigned nodeIndex = 0; nodeIndex < graph->GetNumNodes(); ++nodeIndex)
                    CollectSerializableResources(graph->GetNode(nodeIndex), visited);
            }
        }
    }
}

void AssetViewer::CollectSerializableResources(Serializable* serializable, ea::unordered_set<Resource*>& visited)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    const ea::vector<AttributeInfo>* attributes = serializable->GetAttributes();
    if (!attributes)
        return;
    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const Variant value = serializable->GetAttribute(i);
        if (value.GetType() == VAR_RESOURCEREF)
        {
            const ResourceRef& ref = value.GetResourceRef();
            AddResource(cache->GetResource(ref.type_, ref.name_, false), visited);
        }
        else if (value.GetType() == VAR_RESOURCEREFLIST)
        {
            const ResourceRefList& list = value.GetResourceRefList();
            for (const ea::string& name : list.names_)
                AddResource(cache->GetResource(list.type_, name, false), visited);
        }
    }
}

void AssetViewer::CollectSceneResources()
{
    ea::unordered_set<Resource*> visited;
    ea::vector<SharedPtr<Resource>> existing = assetResources_;
    assetResources_.clear();
    for (Resource* resource : existing)
        AddResource(resource, visited);

    // While the neutral toggle is on, the authored materials are only in the saved list.
    for (const SavedMaterial& saved : savedMaterials_)
        AddResource(saved.material_, visited);

    if (!contentRoot_)
        return;
    ea::vector<Node*> nodes{contentRoot_};
    const ea::vector<Node*> children = contentRoot_->GetChildren(true);
    nodes.insert(nodes.end(), children.begin(), children.end());
    for (Node* node : nodes)
    {
        if (environment_.IsEnvironmentNode(node))
            continue;
        CollectSerializableResources(node, visited);
        for (const SharedPtr<Component>& component : node->GetComponents())
            CollectSerializableResources(component, visited);
    }
}

void AssetViewer::CollectAnimations()
{
    animationControllers_.clear();
    Animation* initiallySelectedAnimation = nullptr;
    if (contentRoot_)
    {
        ea::vector<AnimationController*> controllers;
        contentRoot_->FindComponents<AnimationController>(
            controllers, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        for (AnimationController* controller : controllers)
        {
            animationControllers_.emplace_back(controller);
            if (!initiallySelectedAnimation)
            {
                for (const AnimationParameters& parameters : controller->GetAnimationParameters())
                {
                    if (parameters.GetAnimation())
                    {
                        initiallySelectedAnimation = parameters.GetAnimation();
                        break;
                    }
                }
            }
        }
    }

    animations_.clear();
    ea::unordered_set<int> sourceAnimationIndices;
    for (const SharedPtr<Resource>& resource : assetResources_)
    {
        if (Animation* animation = dynamic_cast<Animation*>(resource.Get()))
        {
            const Variant& sourceAnimationIndex = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
            if (sourceAnimationIndex.GetType() == VAR_INT
                && !sourceAnimationIndices.insert(sourceAnimationIndex.GetInt()).second)
                continue;
            animations_.emplace_back(animation);
        }
    }
    ea::sort(animations_.begin(), animations_.end(),
        [this](Animation* lhs, Animation* rhs) { return GetAnimationLabel(lhs) < GetAnimationLabel(rhs); });

    selectedAnimation_ = animations_.empty() ? -1 : 0;
    if (initiallySelectedAnimation)
    {
        const auto iter = ea::find(animations_.begin(), animations_.end(), initiallySelectedAnimation);
        if (iter != animations_.end())
            selectedAnimation_ = static_cast<int>(iter - animations_.begin());
    }
    animationTime_ = 0.0f;
    SetAnimationsPlaying(false);
}

void AssetViewer::CollectParticleEmitters()
{
    legacyParticleEmitters_.clear();
    graphParticleEmitters_.clear();
    particleEmitters2D_.clear();
    if (!contentRoot_)
        return;

    ea::vector<ParticleEmitter*> legacyEmitters;
    contentRoot_->FindComponents<ParticleEmitter>(
        legacyEmitters, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (ParticleEmitter* emitter : legacyEmitters)
        legacyParticleEmitters_.emplace_back(emitter);

    ea::vector<ParticleGraphEmitter*> graphEmitters;
    contentRoot_->FindComponents<ParticleGraphEmitter>(
        graphEmitters, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (ParticleGraphEmitter* emitter : graphEmitters)
        graphParticleEmitters_.emplace_back(emitter);

    ea::vector<ParticleEmitter2D*> emitters2D;
    contentRoot_->FindComponents<ParticleEmitter2D>(
        emitters2D, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (ParticleEmitter2D* emitter : emitters2D)
        particleEmitters2D_.emplace_back(emitter);

    if (!legacyParticleEmitters_.empty() || !graphParticleEmitters_.empty() || !particleEmitters2D_.empty())
        BeginParticleAutoFrame();
}

void AssetViewer::CollectLods()
{
    lodNodes_.clear();
    lodModels_.clear();
    lodLevelCount_ = 0;
    selectedLod_ = 0;
    if (!contentRoot_)
        return;

    ea::vector<Node*> nodes{contentRoot_};
    const ea::vector<Node*> children = contentRoot_->GetChildren(true);
    nodes.insert(nodes.end(), children.begin(), children.end());
    ea::unordered_map<ea::string, unsigned> groupSizes;
    for (Node* node : nodes)
    {
        ea::string group;
        unsigned level{};
        if (node == contentRoot_ || !node->GetParent() || environment_.IsEnvironmentNode(node)
            || !ParseLodNodeName(node->GetName(), group, level))
            continue;
        // A node without a drawable of its own is not a level. The geometry LOD levels of a merged model use this case.
        if (!HasOwnDrawable(node))
            continue;
        // The parent separates the groups of different parents that have the same name.
        group = Format("{}/{}", node->GetParent()->GetID(), group);
        ++groupSizes[group];
        lodNodes_.push_back(LodNode{WeakPtr<Node>(node), group, level, node->IsEnabled()});
    }
    // A single node with a drawable and a LOD name is not a choice between levels.
    lodNodes_.erase(ea::remove_if(lodNodes_.begin(), lodNodes_.end(),
                        [&](const LodNode& entry) { return groupSizes[entry.group_] < 2; }),
        lodNodes_.end());
    for (const LodNode& entry : lodNodes_)
        lodLevelCount_ = Max(lodLevelCount_, entry.level_ + 1);

    ea::vector<StaticModel*> models;
    contentRoot_->FindComponents<StaticModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (StaticModel* model : models)
    {
        if (environment_.IsEnvironmentNode(model->GetNode()))
            continue;
        const unsigned numLevels = GetNumModelLodLevels(model->GetModel());
        if (numLevels < 2)
            continue;
        lodModels_.push_back(LodModel{WeakPtr<StaticModel>(model), model->GetLodBias()});
        lodLevelCount_ = Max(lodLevelCount_, numLevels);
    }
}

void AssetViewer::SetSelectedLod(int level)
{
    selectedLod_ = lodModels_.empty() ? Max(level, 0) : level;

    // Show the best node of each group: the highest level that is not above the selected level.
    struct GroupLevels
    {
        int best_{-1};
        unsigned lowest_{M_MAX_UNSIGNED};
    };
    ea::unordered_map<ea::string, GroupLevels> groups;
    const int wanted = Max(selectedLod_, 0);
    for (const LodNode& entry : lodNodes_)
    {
        GroupLevels& levels = groups[entry.group_];
        levels.lowest_ = Min(levels.lowest_, entry.level_);
        if (static_cast<int>(entry.level_) <= wanted)
            levels.best_ = Max(levels.best_, static_cast<int>(entry.level_));
    }
    for (const LodNode& entry : lodNodes_)
    {
        Node* node = entry.node_;
        if (!node)
            continue;
        const GroupLevels& levels = groups[entry.group_];
        const unsigned shown = levels.best_ >= 0 ? static_cast<unsigned>(levels.best_) : levels.lowest_;
        node->SetEnabled(entry.enabled_ && entry.level_ == shown);
    }

    if (selectedLod_ < 0)
    {
        for (const LodModel& entry : lodModels_)
        {
            if (StaticModel* model = entry.model_)
                model->SetLodBias(entry.lodBias_);
        }
    }
    else
        UpdateLodOverride();
}

void AssetViewer::UpdateLodOverride()
{
    if (selectedLod_ < 0 || lodModels_.empty() || !camera_)
        return;

    for (const LodModel& entry : lodModels_)
    {
        StaticModel* staticModel = entry.model_;
        if (!staticModel || !staticModel->GetModel())
            continue;
        const unsigned numLevels = GetNumModelLodLevels(staticModel->GetModel());
        if (numLevels < 2)
            continue;
        const BoundingBox& worldBox = staticModel->GetWorldBoundingBox();
        if (!worldBox.Defined())
            continue;

        // The engine computes the scale in the same way as here. See StaticModel and AnimatedModel UpdateBatches.
        BoundingBox scaleBox = worldBox;
        if (const auto* animatedModel = dynamic_cast<const AnimatedModel*>(staticModel))
            scaleBox = animatedModel->GetBoundingBox().Transformed(animatedModel->GetNode()->GetWorldTransform());
        const float scale = scaleBox.Size().DotProduct(DOT_SCALE);
        const float distance = camera_->GetDistance(worldBox.Center());
        const float currentLodDistance = camera_->GetLodDistance(distance, scale, 1.0f);
        const unsigned level = Min(static_cast<unsigned>(selectedLod_), numLevels - 1);
        const float targetLodDistance = Max(GetModelLodTargetDistance(staticModel->GetModel(), level), M_EPSILON);
        // The LOD distance of the model is the current distance at bias 1 divided by the bias.
        staticModel->SetLodBias(Max(currentLodDistance / targetLodDistance, M_EPSILON));
    }
}

} // namespace Urho3D
