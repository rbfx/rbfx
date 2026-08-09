// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Texture2DArray.h>
#include <Urho3D/Graphics/Texture3D.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Particles/ParticleGraph.h>
#include <Urho3D/Particles/ParticleGraphEffect.h>
#include <Urho3D/Particles/ParticleGraphEmitter.h>
#include <Urho3D/Particles/ParticleGraphLayer.h>
#include <Urho3D/Particles/ParticleGraphNode.h>
#include <Urho3D/Particles/ParticleGraphPin.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneResource.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Utility/AnimationMetadata.h>
#include <Urho3D/Utility/FBXImporter.h>
#include <Urho3D/Utility/GLTFImporter.h>

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>

#include <cmath>
#include <tiny_gltf.h>

namespace Urho3D
{

namespace
{

bool IsFiniteBoundingBox(const BoundingBox& box)
{
    const auto isFinite = [](const Vector3& value)
    { return std::isfinite(value.x_) && std::isfinite(value.y_) && std::isfinite(value.z_); };
    return box.Defined() && isFinite(box.min_) && isFinite(box.max_) && box.min_.x_ <= box.max_.x_
        && box.min_.y_ <= box.max_.y_ && box.min_.z_ <= box.max_.z_;
}

bool IsImageExtension(const ea::string& extension)
{
    return extension == ".dds" || extension == ".ktx" || extension == ".pvr" || extension == ".bmp"
        || extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".tga"
        || extension == ".webp" || extension == ".hdr" || extension == ".psd" || extension == ".gif"
        || extension == ".pic" || extension == ".pnm";
}

ea::string FormatBytes(unsigned long long value)
{
    static const char* suffixes[]{"B", "KiB", "MiB", "GiB", "TiB"};
    double amount = static_cast<double>(value);
    unsigned suffix = 0;
    while (amount >= 1024.0 && suffix + 1 < ea::size(suffixes))
    {
        amount /= 1024.0;
        ++suffix;
    }
    return suffix == 0 ? Format("{} {}", value, suffixes[suffix])
                       : Format("{:.2f} {} ({} bytes)", amount, suffixes[suffix], value);
}

ea::string NameOrIndex(const std::string& name, unsigned index)
{
    return !name.empty() ? Format("{} [{}]", name.c_str(), index) : Format("[{}]", index);
}

ea::string JoinInts(const std::vector<int>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

ea::string JoinUnsigned(const ea::vector<unsigned>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

ea::string JoinDoubles(const std::vector<double>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{:.9g}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

ea::string JoinStrings(const StringVector& values)
{
    ea::string result;
    for (const ea::string& value : values)
        result += (result.empty() ? "" : ", ") + value;
    return result.empty() ? "(none)" : result;
}

void Property(const char* label, const ea::string& value)
{
    ui::TextWrapped("%s: %s", label, value.c_str());
}

template <class T> void Property(const char* label, const T& value)
{
    Property(label, Format("{}", value));
}

bool TreeNode(const char* label, unsigned count)
{
    return ui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%u)", label, count);
}

void DrawTinyValue(const char* label, const tinygltf::Value& value)
{
    if (value.IsObject())
    {
        const auto& object = value.Get<tinygltf::Value::Object>();
        if (ui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s {%zu}", label, object.size()))
        {
            for (const auto& [key, child] : object)
                DrawTinyValue(key.c_str(), child);
            ui::TreePop();
        }
    }
    else if (value.IsArray())
    {
        const auto& array = value.Get<tinygltf::Value::Array>();
        if (ui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%zu]", label, array.size()))
        {
            for (unsigned i = 0; i < array.size(); ++i)
            {
                const ea::string itemLabel = Format("[{}]", i);
                DrawTinyValue(itemLabel.c_str(), array[i]);
            }
            ui::TreePop();
        }
    }
    else if (value.IsString())
        Property(label, value.Get<std::string>().c_str());
    else if (value.IsBool())
        Property(label, value.Get<bool>() ? "true" : "false");
    else if (value.IsInt())
        Property(label, value.Get<int>());
    else if (value.IsNumber())
        Property(label, value.GetNumberAsDouble());
    else if (value.IsBinary())
        Property(label, FormatBytes(value.Get<std::vector<unsigned char>>().size()));
    else
        Property(label, "null");
}

void DrawTinyExtensionsAndExtras(const tinygltf::ExtensionMap& extensions, const tinygltf::Value& extras)
{
    if (!extensions.empty() && TreeNode("Extensions", extensions.size()))
    {
        for (const auto& [name, value] : extensions)
            DrawTinyValue(name.c_str(), value);
        ui::TreePop();
    }
    if (extras.Type() != tinygltf::NULL_TYPE)
        DrawTinyValue("Extras", extras);
}

const char* PrimitiveName(int mode)
{
    switch (mode)
    {
    case TINYGLTF_MODE_POINTS: return "Points";
    case TINYGLTF_MODE_LINE: return "Lines";
    case TINYGLTF_MODE_LINE_LOOP: return "Line Loop";
    case TINYGLTF_MODE_LINE_STRIP: return "Line Strip";
    case TINYGLTF_MODE_TRIANGLES: return "Triangles";
    case TINYGLTF_MODE_TRIANGLE_STRIP: return "Triangle Strip";
    case TINYGLTF_MODE_TRIANGLE_FAN: return "Triangle Fan";
    default: return "Unspecified";
    }
}

const char* AccessorTypeName(int type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR: return "SCALAR";
    case TINYGLTF_TYPE_VEC2: return "VEC2";
    case TINYGLTF_TYPE_VEC3: return "VEC3";
    case TINYGLTF_TYPE_VEC4: return "VEC4";
    case TINYGLTF_TYPE_MAT2: return "MAT2";
    case TINYGLTF_TYPE_MAT3: return "MAT3";
    case TINYGLTF_TYPE_MAT4: return "MAT4";
    default: return "Unknown";
    }
}

const char* ComponentTypeName(int type)
{
    switch (type)
    {
    case TINYGLTF_COMPONENT_TYPE_BYTE: return "BYTE";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return "UNSIGNED_BYTE";
    case TINYGLTF_COMPONENT_TYPE_SHORT: return "SHORT";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return "UNSIGNED_SHORT";
    case TINYGLTF_COMPONENT_TYPE_INT: return "INT";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return "UNSIGNED_INT";
    case TINYGLTF_COMPONENT_TYPE_FLOAT: return "FLOAT";
    case TINYGLTF_COMPONENT_TYPE_DOUBLE: return "DOUBLE";
    default: return "Unknown";
    }
}

ea::string LoadGLTFJsonChunk(Context* context, const ea::string& fileName)
{
    File file(context, fileName, FILE_READ);
    if (!file.IsOpen())
        return {};

    if (GetExtension(fileName) == ".gltf")
    {
        ea::string result;
        result.resize(file.GetSize());
        if (!result.empty())
            file.Read(result.data(), result.size());
        return result;
    }

    if (file.GetSize() < 20 || file.ReadUInt() != 0x46546c67)
        return {};
    file.ReadUInt(); // Version
    file.ReadUInt(); // Total length
    const unsigned jsonLength = file.ReadUInt();
    const unsigned chunkType = file.ReadUInt();
    if (chunkType != 0x4e4f534a || jsonLength > file.GetSize() - file.GetPosition())
        return {};

    ea::string result;
    result.resize(jsonLength);
    file.Read(result.data(), jsonLength);
    while (!result.empty()
        && (result.back() == '\0' || result.back() == ' ' || result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

} // namespace

class AssetViewer : public Application
{
    URHO3D_OBJECT(AssetViewer, Application);

public:
    explicit AssetViewer(Context* context)
        : Application(context)
    {
    }

    void Setup() override
    {
        auto* fs = GetSubsystem<FileSystem>();
        engineParameters_[EP_WINDOW_TITLE] = "rbfx Asset Viewer";
        engineParameters_[EP_APPLICATION_NAME] = "AssetViewer";
        engineParameters_[EP_LOG_NAME] = "conf://AssetViewer.log";
        engineParameters_[EP_HEADLESS] = false;
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_BORDERLESS] = false;
        engineParameters_[EP_WINDOW_MAXIMIZE] = false;
        engineParameters_[EP_WINDOW_RESIZABLE] = true;
        engineParameters_[EP_WINDOW_WIDTH] = 1440;
        engineParameters_[EP_WINDOW_HEIGHT] = 900;
        engineParameters_[EP_RESOURCE_PATHS] = "CoreData;Data";
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir() + ";..;../..";

        GetCommandLineParser().add_option("asset", startupAsset_, "Asset to open on startup")->type_name("path");
        GetCommandLineParser().add_option("--blender", blender_, "Path to Blender executable")->type_name("path");
    }

    void Start() override
    {
        uiIniPath_ = engine_->GetAppPreferencesDir() + "AssetViewer.ini";
        ImGuiIO& io = ui::GetIO();
        io.IniFilename = uiIniPath_.c_str();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigWindowsResizeFromEdges = true;
        GetSubsystem<SystemUI>()->ApplyStyleDefault(true, 1.0f);
        GetSubsystem<Input>()->SetMouseMode(MM_ABSOLUTE);
        GetSubsystem<Input>()->SetMouseVisible(true);

        CreateScene();
        SubscribeToEvent(E_DROPFILE, URHO3D_HANDLER(AssetViewer, HandleDropFile));
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(AssetViewer, HandleUpdate));
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(AssetViewer, HandleKeyDown));
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(AssetViewer, HandlePostRenderUpdate));

        if (!startupAsset_.empty())
            LoadAsset(startupAsset_);
    }

private:
    void CreateScene()
    {
        scene_ = MakeShared<Scene>(context_);
        scene_->CreateComponent<Octree>();
        scene_->CreateComponent<DebugRenderer>();

        EnsureViewerEnvironment();

        cameraNode_ = scene_->CreateChild("Viewer Camera");
        cameraNode_->SetTemporary(true);
        camera_ = cameraNode_->CreateComponent<Camera>();
        freeFlyController_ = cameraNode_->CreateComponent<FreeFlyController>();
        freeFlyController_->SetEnabled(freeLookEnabled_);
        camera_->SetNearClip(0.01f);
        camera_->SetFarClip(100000.0f);
        viewport_ = MakeShared<Viewport>(context_, scene_, camera_);
        GetSubsystem<Renderer>()->SetViewport(0, viewport_);
        UpdateCameraTransform();
    }

    void EnsureViewerEnvironment()
    {
        viewerZoneNode_.Reset();
        viewerLightNode_.Reset();
        if (!scene_->FindComponent<Zone>())
        {
            viewerZoneNode_ = scene_->CreateChild("Viewer Zone");
            viewerZoneNode_->SetTemporary(true);
            Zone* zone = viewerZoneNode_->CreateComponent<Zone>();
            zone->SetBoundingBox(BoundingBox(-10000.0f, 10000.0f));
            zone->SetAmbientColor(Color(0.3f, 0.3f, 0.3f));
        }

        if (!scene_->FindComponent<Light>())
        {
            viewerLightNode_ = scene_->CreateChild("Viewer Light");
            viewerLightNode_->SetTemporary(true);
            viewerLightNode_->SetDirection(Vector3(0.35f, -0.65f, -0.65f));
            Light* light = viewerLightNode_->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
            light->SetBrightness(1.15f);
            light->SetCastShadows(true);
        }
    }

    void ClearAsset()
    {
        assetResources_.clear();
        gltfModel_.reset();
        importer_.Reset();
        tempDir_.reset();
        pendingTempDir_.reset();
        sourceJson_.clear();
        sourceText_.clear();
        converterOutput_.clear();
        mountedDirectory_.reset();
        contentRoot_.Reset();
        previewTexture_.Reset();
        animationControllers_.clear();
        animations_.clear();
        animationDebugNodes_.clear();
        legacyParticleEmitters_.clear();
        graphParticleEmitters_.clear();
        particleEmitters2D_.clear();
        selectedAnimation_ = -1;
        animationTime_ = 0.0f;
        animationSpeed_ = 1.0f;
        animationLooped_ = true;
        animationsPlaying_ = false;
        particlesEmitting_ = true;
        particleAutoFramePending_ = false;
        particleAutoFrameTimeRemaining_ = 0.0f;
        particleAutoFrameBounds_.Clear();
        framesUntilFrame_ = 0;
        is2DPreview_ = false;
        sourceFile_.clear();
        primaryFBXFile_.clear();
        loadingFile_.clear();
        convertedFile_.clear();
        error_.clear();
        loading_ = false;
        fallbackMaterialSlots_ = 0;
        CreateScene();
    }

    void HandleDropFile(StringHash, VariantMap& eventData) { LoadAsset(eventData[DropFile::P_FILENAME].GetString()); }

    void HandleKeyDown(StringHash, VariantMap& eventData)
    {
        const int key = eventData[KeyDown::P_KEY].GetInt();
        if (key == KEY_F)
        {
            particleAutoFramePending_ = false;
            FrameContent();
        }
        else if (key == KEY_ESCAPE)
            engine_->Exit();
    }

    void HandleUpdate(StringHash, VariantMap& eventData)
    {
        if (framesUntilFrame_ > 0 && --framesUntilFrame_ == 0)
            FrameContent();
        UpdateParticleAutoFrame(eventData[Update::P_TIMESTEP].GetFloat());
        RenderUI();
        UpdateCameraInput();
    }

    void HandlePostRenderUpdate(StringHash, VariantMap&)
    {
        if (animationDebugNodes_.empty())
            return;

        DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();
        if (!debug)
            return;
        for (const WeakPtr<Node>& weakNode : animationDebugNodes_)
        {
            Node* node = weakNode;
            if (!node)
                continue;
            Node* parent = node->GetParent();
            if (parent && parent != contentRoot_)
                debug->AddLine(parent->GetWorldPosition(), node->GetWorldPosition(), Color(0.95f, 0.8f, 0.2f));
            debug->AddFrame(node->GetWorldTransform(), 0.08f);
        }
    }

    void UpdateCameraInput()
    {
        Input* input = GetSubsystem<Input>();
        if (freeFlyController_ && freeFlyController_->IsEnabled())
        {
            const Vector3 angles = cameraNode_->GetRotation().EulerAngles();
            pitch_ = angles.x_;
            yaw_ = angles.y_;
            cameraTarget_ = cameraNode_->GetPosition() + cameraNode_->GetDirection() * distance_;
            return;
        }

        if (!sceneHovered_)
            return;

        const IntVector2 delta = input->GetMouseMove();
        if (!is2DPreview_ && input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            particleAutoFramePending_ = false;
            yaw_ += delta.x_ * 0.25f;
            pitch_ = Clamp(pitch_ + delta.y_ * 0.25f, -89.0f, 89.0f);
        }
        if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
        {
            particleAutoFramePending_ = false;
            if (is2DPreview_)
            {
                const float scale = camera_->GetOrthoSize() / Max(1, GetSubsystem<Graphics>()->GetHeight());
                cameraTarget_.x_ -= delta.x_ * scale;
                cameraTarget_.y_ += delta.y_ * scale;
            }
            else
            {
                const Quaternion rotation(pitch_, yaw_, 0.0f);
                const float scale = distance_ * 0.0015f;
                cameraTarget_ -= rotation * Vector3::RIGHT * delta.x_ * scale;
                cameraTarget_ += rotation * Vector3::UP * delta.y_ * scale;
            }
        }
        if (const int wheel = input->GetMouseMoveWheel())
        {
            particleAutoFramePending_ = false;
            if (is2DPreview_)
                camera_->SetOrthoSize(Max(0.001f, camera_->GetOrthoSize() * Pow(0.82f, static_cast<float>(wheel))));
            else
                distance_ = Max(0.001f, distance_ * Pow(0.82f, static_cast<float>(wheel)));
        }

        UpdateCameraTransform();
    }

    void UpdateCameraTransform()
    {
        if (is2DPreview_)
        {
            cameraNode_->SetRotation(Quaternion::IDENTITY);
            cameraNode_->SetPosition(cameraTarget_ - Vector3::FORWARD * distance_);
            return;
        }
        const Quaternion rotation(pitch_, yaw_, 0.0f);
        cameraNode_->SetPosition(cameraTarget_ - rotation * Vector3::FORWARD * distance_);
        cameraNode_->LookAt(cameraTarget_, Vector3::UP);
    }

    BoundingBox GetContentBoundingBox(bool ignoreLargeOutliers = false) const
    {
        BoundingBox result;
        BoundingBox degenerateResult;
        ea::vector<BoundingBox> drawableBoxes;
        if (!contentRoot_)
            return result;

        ea::vector<Drawable*> drawables;
        contentRoot_->FindComponents<Drawable>(drawables, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        for (Drawable* drawable : drawables)
        {
            if (!drawable || !drawable->IsEnabledEffective() || drawable->GetType() == Skybox::GetTypeStatic())
                continue;
            const DrawableFlags flags = drawable->GetDrawableFlags();
            if (!flags.Test(DRAWABLE_GEOMETRY) && !flags.Test(DRAWABLE_GEOMETRY2D))
                continue;
            const BoundingBox& drawableBox = drawable->GetWorldBoundingBox();
            if (IsFiniteBoundingBox(drawableBox))
            {
                if (drawableBox.Size().LengthSquared() > M_EPSILON * M_EPSILON)
                {
                    result.Merge(drawableBox);
                    drawableBoxes.push_back(drawableBox);
                }
                else
                    degenerateResult.Merge(drawableBox);
            }
        }
        if (!result.Defined())
            result = degenerateResult;
        // Ignore a tiny leading group of proxy/background meshes only when it is clearly larger than everything else.
        if (ignoreLargeOutliers && drawableBoxes.size() >= 100)
        {
            ea::sort(drawableBoxes.begin(), drawableBoxes.end(), [](const BoundingBox& lhs, const BoundingBox& rhs)
            { return lhs.Size().LengthSquared() > rhs.Size().LengthSquared(); });
            const unsigned maxIgnored = ea::min<unsigned>(16, drawableBoxes.size() / 100);
            for (unsigned i = 0; i < maxIgnored && i + 1 < drawableBoxes.size(); ++i)
            {
                const float currentSize = drawableBoxes[i].Size().Length();
                const float nextSize = drawableBoxes[i + 1].Size().Length();
                if (currentSize <= nextSize * 8.0f)
                    continue;
                result.Clear();
                for (unsigned j = i + 1; j < drawableBoxes.size(); ++j)
                    result.Merge(drawableBoxes[j]);
                // Vast aggregate scenes are useful to inspect at a representative upper-percentile draw scale.
                const float representativeSize = drawableBoxes[drawableBoxes.size() / 50].Size().Length() * 2.0f;
                const float resultSize = result.Size().Length();
                if (representativeSize < resultSize)
                {
                    const Vector3 center = result.Center();
                    const Vector3 halfSize = result.HalfSize() * (representativeSize / resultSize);
                    result = BoundingBox(center - halfSize, center + halfSize);
                }
                break;
            }
        }
        for (const WeakPtr<Node>& node : animationDebugNodes_)
        {
            if (node)
                result.Merge(node->GetWorldPosition());
        }
        if (!animationDebugNodes_.empty() && result.Defined() && result.Size().Length() < 0.1f)
            result.Merge(BoundingBox(result.Center() - Vector3::ONE * 0.5f, result.Center() + Vector3::ONE * 0.5f));
        return result;
    }

    Animation* GetSelectedAnimation() const
    {
        return selectedAnimation_ >= 0 && selectedAnimation_ < animations_.size() ? animations_[selectedAnimation_]
                                                                                  : nullptr;
    }

    ea::vector<Animation*> GetSelectedAnimationGroup() const
    {
        Animation* selectedAnimation = GetSelectedAnimation();
        if (!selectedAnimation)
            return {};

        const Variant& sourceAnimationIndex = selectedAnimation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
        if (sourceAnimationIndex.GetType() != VAR_INT)
            return {selectedAnimation};

        ea::vector<Animation*> result;
        for (const SharedPtr<Resource>& resource : assetResources_)
        {
            Animation* animation = dynamic_cast<Animation*>(resource.Get());
            if (!animation)
                continue;
            const Variant& candidateIndex = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
            if (candidateIndex.GetType() == VAR_INT && candidateIndex.GetInt() == sourceAnimationIndex.GetInt())
                result.push_back(animation);
        }
        return result;
    }

    ea::string GetAnimationLabel(Animation* animation) const
    {
        if (!animation->GetAnimationName().empty())
            return animation->GetAnimationName();
        if (!animation->GetName().empty())
            return GetFileName(animation->GetName());
        return "Unnamed animation";
    }

    bool IsAnimationCompatible(Animation* animation, AnimatedModel* animatedModel) const
    {
        if (!animation || !animatedModel || !animatedModel->GetModel())
            return false;

        Model* model = animatedModel->GetModel();
        const ea::string expectedModel = animation->GetMetadata(AnimationMetadata::Model).GetString();
        if (!expectedModel.empty())
        {
            const ea::string expectedFile = GetFileNameAndExtension(expectedModel).to_lower();
            const ea::string actualFile = GetFileNameAndExtension(model->GetName()).to_lower();
            if (expectedFile == actualFile
                || GetFileName(expectedModel).to_lower() == GetFileName(model->GetName()).to_lower())
                return true;
        }

        const Skeleton& skeleton = model->GetSkeleton();
        unsigned matchedTracks = 0;
        for (const auto& [nameHash, _] : animation->GetTracks())
        {
            if (skeleton.GetBoneIndex(nameHash) != M_MAX_UNSIGNED)
                ++matchedTracks;
        }
        const unsigned numTracks = animation->GetNumTracks();
        if (matchedTracks > 0 && (numTracks <= 2 || matchedTracks * 2 >= numTracks))
            return true;

        if (animatedModel->GetNumMorphs() > 0)
        {
            for (const auto& [_, track] : animation->GetVariantTracks())
            {
                if (track.name_.contains("@AnimatedModel") && track.name_.contains("/Morphs/"))
                    return true;
            }
        }
        return false;
    }

    bool CanControllerPlayAnimation(AnimationController* controller, Animation* animation) const
    {
        if (!controller || !animation)
            return false;

        const bool isImportedSceneAnimation = animation->GetNumTracks() == 0 && animation->GetNumVariantTracks() > 0
            && animation->GetMetadata(AnimationMetadata::SourceAnimationIndex).GetType() == VAR_INT;
        if (isImportedSceneAnimation)
            return controller->GetNode() == contentRoot_;

        ea::vector<AnimatedModel*> models;
        controller->GetNode()->GetComponents<AnimatedModel>(models);
        if (!models.empty())
        {
            return ea::any_of(models.begin(), models.end(),
                [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); });
        }

        if (controller->GetNode() == contentRoot_
            && animation->GetMetadata(AnimationMetadata::SourceAnimationIndex).GetType() == VAR_INT)
            return false;

        controller->GetNode()->FindComponents<AnimatedModel>(
            models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        if (models.empty())
            return true;
        return ea::any_of(models.begin(), models.end(),
            [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); });
    }

    void UpdateAnimationTimeFromController()
    {
        const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
        if (animations.empty())
            return;
        for (const WeakPtr<AnimationController>& controller : animationControllers_)
        {
            if (controller)
            {
                for (Animation* animation : animations)
                {
                    if (const AnimationParameters* parameters = controller->GetLastAnimationParameters(animation))
                    {
                        animationTime_ = parameters->GetTime();
                        return;
                    }
                }
            }
        }
    }

    void ApplySelectedAnimation(bool playing)
    {
        const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
        bool hasCompatibleController = false;
        if (animations.empty())
        {
            animationsPlaying_ = false;
            return;
        }

        for (const WeakPtr<AnimationController>& controller : animationControllers_)
        {
            if (controller)
            {
                controller->SetEnabled(true);
                controller->StopAll();
                Animation* compatibleAnimation = nullptr;
                for (Animation* animation : animations)
                {
                    if (CanControllerPlayAnimation(controller, animation))
                    {
                        compatibleAnimation = animation;
                        break;
                    }
                }
                if (!compatibleAnimation)
                {
                    controller->SetEnabled(false);
                    continue;
                }
                hasCompatibleController = true;
                controller->PlayNewExclusive(AnimationParameters{compatibleAnimation}
                        .Looped(animationLooped_)
                        .Speed(animationSpeed_)
                        .Time(animationTime_));
                controller->UpdatePose();
                controller->SetEnabled(playing);
            }
        }
        animationsPlaying_ = playing && hasCompatibleController;
    }

    void SetSelectedAnimation(int index, bool playing = true)
    {
        if (index < 0 || index >= animations_.size())
            return;
        selectedAnimation_ = index;
        animationTime_ = 0.0f;
        ApplySelectedAnimation(playing);
    }

    void SetAnimationsPlaying(bool playing)
    {
        if (playing && !GetSelectedAnimation())
            return;
        UpdateAnimationTimeFromController();
        const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
        bool hasCompatibleController = false;
        for (const WeakPtr<AnimationController>& controller : animationControllers_)
        {
            if (controller)
            {
                const bool compatible =
                    ea::any_of(animations.begin(), animations.end(), [this, controller](Animation* animation)
                { return CanControllerPlayAnimation(controller, animation); });
                hasCompatibleController |= compatible;
                controller->SetEnabled(playing && compatible);
            }
        }
        animationsPlaying_ = playing && hasCompatibleController;
    }

    void SetAnimationTime(float time)
    {
        Animation* animation = GetSelectedAnimation();
        if (!animation)
            return;
        animationTime_ = Clamp(time, 0.0f, animation->GetLength());
        const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
        for (const WeakPtr<AnimationController>& controller : animationControllers_)
        {
            bool updated = false;
            for (Animation* groupedAnimation : animations)
                updated |= controller && controller->UpdateAnimationTime(groupedAnimation, animationTime_);
            if (controller && updated)
                controller->UpdatePose();
        }
    }

    void SetAnimationSpeed(float speed)
    {
        Animation* animation = GetSelectedAnimation();
        animationSpeed_ = speed;
        if (!animation)
            return;
        const ea::vector<Animation*> animations = GetSelectedAnimationGroup();
        for (const WeakPtr<AnimationController>& controller : animationControllers_)
        {
            if (controller)
            {
                for (Animation* groupedAnimation : animations)
                    controller->UpdateAnimationSpeed(groupedAnimation, animationSpeed_);
            }
        }
    }

    void FrameContent()
    {
        const BoundingBox box = GetContentBoundingBox(true);
        if (!box.Defined())
            return;
        FrameBounds(box);
    }

    void FrameBounds(const BoundingBox& box)
    {
        if (!IsFiniteBoundingBox(box))
            return;
        BoundingBox sceneBounds = box;
        const float padding = Max(1.0f, box.Size().Length() * 0.01f);
        sceneBounds.min_ -= Vector3::ONE * padding;
        sceneBounds.max_ += Vector3::ONE * padding;
        if (Octree* octree = scene_->GetComponent<Octree>();
            octree && octree->GetRootOctant()->GetWorldBoundingBox().IsInside(sceneBounds) != INSIDE)
        {
            BoundingBox octreeBounds = octree->GetRootOctant()->GetWorldBoundingBox();
            octreeBounds.Merge(sceneBounds);
            octree->SetSize(octreeBounds, octree->GetNumLevels());
        }
        if (Zone* zone = viewerZoneNode_ ? viewerZoneNode_->GetComponent<Zone>() : nullptr;
            zone && zone->GetBoundingBox().IsInside(sceneBounds) != INSIDE)
        {
            BoundingBox zoneBounds = zone->GetBoundingBox();
            zoneBounds.Merge(sceneBounds);
            zone->SetBoundingBox(zoneBounds);
        }

        cameraTarget_ = box.Center();
        const float radius = Max(0.05f, box.Size().Length() * 0.5f);
        if (is2DPreview_)
        {
            const Vector3 size = box.Size();
            const float verticalSize = Max(size.y_, size.x_ / Max(camera_->GetAspectRatio(), M_EPSILON));
            camera_->SetOrthoSize(Max(0.01f, verticalSize * 1.25f));
            distance_ = radius * 2.0f;
        }
        else
        {
            distance_ = radius / Tan(camera_->GetFov() * 0.5f) * 1.25f;
            pitch_ = 12.0f;
            yaw_ = 180.0f;
        }
        camera_->SetNearClip(Max(0.001f, radius * 0.001f));
        camera_->SetFarClip(Max(100000.0f, distance_ + radius * 2.0f));
        if (freeFlyController_)
        {
            const float speed = Max(0.1f, radius * 0.5f);
            freeFlyController_->SetSpeed(speed);
            freeFlyController_->SetAcceleratedSpeed(speed * 5.0f);
        }
        UpdateCameraTransform();
    }

    void UpdatePreviewProjection()
    {
        bool has3DGeometry = false;
        bool has2DGeometry = !particleEmitters2D_.empty();
        if (contentRoot_)
        {
            ea::vector<Drawable*> drawables;
            contentRoot_->FindComponents<Drawable>(drawables, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
            for (Drawable* drawable : drawables)
            {
                if (!drawable || !drawable->IsEnabledEffective() || drawable->GetType() == Skybox::GetTypeStatic())
                    continue;
                const DrawableFlags flags = drawable->GetDrawableFlags();
                has3DGeometry |= flags.Test(DRAWABLE_GEOMETRY);
                has2DGeometry |= flags.Test(DRAWABLE_GEOMETRY2D);
            }
        }

        is2DPreview_ = has2DGeometry && !has3DGeometry;
        if (freeFlyController_)
            freeFlyController_->SetEnabled(freeLookEnabled_ && !is2DPreview_ && sceneHovered_);
        camera_->SetOrthographic(is2DPreview_);
        camera_->SetZoom(1.0f);
        UpdateCameraTransform();
    }

    void BeginParticleAutoFrame()
    {
        float observationTime = 0.0f;
        for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
        {
            if (emitter && emitter->GetEffect())
                observationTime = Max(observationTime, emitter->GetEffect()->GetMaxTimeToLive());
        }
        for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
        {
            if (!emitter || !emitter->GetEffect())
                continue;
            ParticleGraphEffect* effect = emitter->GetEffect();
            for (unsigned i = 0; i < effect->GetNumLayers(); ++i)
                observationTime = Max(observationTime, effect->GetLayer(i)->GetDuration());
        }
        for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
        {
            if (emitter && emitter->GetEffect())
            {
                ParticleEffect2D* effect = emitter->GetEffect();
                observationTime =
                    Max(observationTime, effect->GetParticleLifeSpan() + Abs(effect->GetParticleLifespanVariance()));
            }
        }

        particleAutoFrameTimeRemaining_ = Clamp(observationTime * 0.75f, 0.25f, 2.0f);
        particleAutoFrameBounds_.Clear();
        particleAutoFramePending_ = true;
    }

    void UpdateParticleAutoFrame(float timeStep)
    {
        if (!particleAutoFramePending_)
            return;

        const BoundingBox currentBounds = GetContentBoundingBox();
        if (currentBounds.Defined())
            particleAutoFrameBounds_.Merge(currentBounds);

        particleAutoFrameTimeRemaining_ -= timeStep;
        if (particleAutoFrameBounds_.Defined() && particleAutoFrameTimeRemaining_ <= 0.0f)
        {
            FrameBounds(particleAutoFrameBounds_);
            particleAutoFramePending_ = false;
        }
    }

    SharedPtr<Animation> LoadAnimationResource(const ea::string& fileName)
    {
        auto animation = MakeShared<Animation>(context_);
        animation->SetName(GetFileNameAndExtension(fileName));
        animation->SetAbsoluteFileName(fileName);
        return animation->LoadFile(fileName) ? animation : nullptr;
    }

    bool HasFBXAnimationTarget() const
    {
        if (primaryFBXFile_.empty() || !contentRoot_)
            return false;
        ea::vector<AnimatedModel*> models;
        contentRoot_->FindComponents<AnimatedModel>(models, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        return ea::any_of(models.begin(), models.end(), [](AnimatedModel* model)
        {
            return model->GetModel() && model->GetModel()->GetSkeleton().GetNumBones() > 0;
        });
    }

    void BuildAnimationDebugPreview(Animation* animation)
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

    bool TryLoadAnimationOnCurrentAsset(const ea::string& fileName)
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
            return false;

        for (AnimatedModel* model : compatibleModels)
        {
            if (!model->GetNode()->GetComponent<AnimationController>())
                model->GetNode()->CreateComponent<AnimationController>();
        }

        assetResources_.push_back(animation);
        error_.clear();
        CollectSceneResources();
        CollectAnimations();
        const auto iter = ea::find(animations_.begin(), animations_.end(), animation.Get());
        if (iter != animations_.end())
            SetSelectedAnimation(static_cast<int>(iter - animations_.begin()), true);
        GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_) + " + "
            + GetFileNameAndExtension(fileName));
        URHO3D_LOGINFO("AssetViewer applied animation '{}' to the loaded model", fileName);
        return true;
    }

    bool LoadAsset(const ea::string& requestedFile)
    {
        if (loading_)
        {
            URHO3D_LOGWARNING("AssetViewer is already loading '{}'; ignored '{}'", loadingFile_, requestedFile);
            return false;
        }

        const ea::string absoluteFile = GetAbsolutePath(requestedFile, GetSubsystem<FileSystem>()->GetCurrentDir());
        FileSystem* fs = GetSubsystem<FileSystem>();
        if (!fs->FileExists(absoluteFile))
            return Fail(Format("File does not exist: {}", absoluteFile));
        const ea::string extension = GetExtension(absoluteFile);
        if (extension == ".ani" && TryLoadAnimationOnCurrentAsset(absoluteFile))
            return true;
        if (extension == ".fbx" && HasFBXAnimationTarget())
            return BeginLoadPotentialFBXAnimation(absoluteFile);

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
            return Fail(
                Format("Unsupported asset type '{}'. Supported: .mdl, .prefab, .scene/.xml, .material, .ani, .pex, "
                       "particle XML, glTF, FBX, Blend, and image/texture files",
                    extension));

        if (success)
            FinalizeAssetLoad(extension);
        return success;
    }

    void FinalizeAssetLoad(const ea::string& extension)
    {
        EnsureVisibleMaterials();
        CollectSceneResources();
        CollectAnimations();
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
        URHO3D_LOGINFO("AssetViewer loaded '{}'", sourceFile_);
    }

    bool ConvertAndLoadExternal(const ea::string& extension)
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

    GLTFImporterSettings GetImportSettings(const ea::string& fileName = EMPTY_STRING) const
    {
        GLTFImporterSettings settings;
        settings.assetName_ = GetFileName(fileName.empty() ? sourceFile_ : fileName);
        settings.gpuResources_ = true;
        settings.preview_.addLights_ = false;
        settings.preview_.addSkybox_ = false;
        settings.preview_.addReflectionProbe_ = false;
        return settings;
    }

    bool BeginLoadFBXAsset()
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

    void CompleteFBXAsset(const SharedPtr<FBXImporter>& importer, const ea::string& fileName, bool sourcePrepared)
    {
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

    bool BeginLoadPotentialFBXAnimation(const ea::string& fileName)
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
            if (animationOnly)
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

    void CompletePotentialFBXAnimation(const SharedPtr<FBXImporter>& importer, const ea::string& fileName,
        bool animationOnly, unsigned primaryAnimationCount, bool sourcePrepared)
    {
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

    bool ApplyImportedFBXAnimations(
        const SharedPtr<FBXImporter>& importer, const ea::string& fileName, unsigned primaryAnimationCount)
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
        ea::vector<SharedPtr<Animation>> compatibleAnimations;
        for (const SharedPtr<Resource>& resource : GetImportedResources(importer))
        {
            auto animation = DynamicCast<Animation>(resource);
            if (!animation)
                continue;
            const Variant& value = animation->GetMetadata(AnimationMetadata::SourceAnimationIndex);
            if (value.GetType() != VAR_INT || value.GetInt() < static_cast<int>(primaryAnimationCount)
                || !ea::any_of(models.begin(), models.end(),
                    [this, animation](AnimatedModel* model) { return IsAnimationCompatible(animation, model); }))
                continue;

            const int sourceIndex = value.GetInt();
            const auto [iter, inserted] = remappedIndices.emplace(sourceIndex, nextAnimationIndex);
            if (inserted)
                ++nextAnimationIndex;
            animation->AddMetadata(AnimationMetadata::SourceAnimationIndex, iter->second);
            compatibleAnimations.push_back(animation);
        }
        if (compatibleAnimations.empty())
        {
            GetSubsystem<Graphics>()->SetWindowTitle(
                "rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_));
            return Fail(Format("No animation in '{}' is compatible with the loaded model",
                GetFileNameAndExtension(fileName)));
        }

        for (const SharedPtr<Animation>& animation : compatibleAnimations)
            assetResources_.push_back(animation);
        for (AnimatedModel* model : models)
        {
            const bool compatible = ea::any_of(compatibleAnimations.begin(), compatibleAnimations.end(),
                [this, model](Animation* animation) { return IsAnimationCompatible(animation, model); });
            if (compatible && !model->GetNode()->GetComponent<AnimationController>())
                model->GetNode()->CreateComponent<AnimationController>();
        }

        error_.clear();
        CollectSceneResources();
        CollectAnimations();
        const auto selected = ea::find(animations_.begin(), animations_.end(), compatibleAnimations.front().Get());
        if (selected != animations_.end())
            SetSelectedAnimation(static_cast<int>(selected - animations_.begin()), true);
        GetSubsystem<Graphics>()->SetWindowTitle("rbfx Asset Viewer - " + GetFileNameAndExtension(sourceFile_) + " + "
            + GetFileNameAndExtension(fileName));
        URHO3D_LOGINFO("AssetViewer applied {} animation resource(s) from '{}' to the loaded model",
            compatibleAnimations.size(), fileName);
        return true;
    }

    bool LoadGLTFAsset(const ea::string& gltfFile)
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

    template <class T> bool ProcessImportedAsset(const SharedPtr<T>& importer)
    {
        const ea::string prefix = PrepareImportOutput();
        if (!importer->Process(tempDir_->GetPath(), prefix, nullptr))
            return Fail("rbfx model importer failed while converting the asset");
        return InstantiateImportedAsset(importer);
    }

    ea::string PrepareImportOutput()
    {
        if (!tempDir_)
        {
            const ea::string tempPath = AddTrailingSlash(GetSubsystem<FileSystem>()->GetTemporaryDir())
                + "rbfx-asset-viewer-" + GenerateUUID() + "/";
            tempDir_ = ea::make_unique<TemporaryDir>(context_, tempPath);
        }
        return "AssetViewer/" + GenerateUUID() + "/";
    }

    template <class T> ea::vector<SharedPtr<Resource>> GetImportedResources(const SharedPtr<T>& importer) const
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

    template <class T> bool InstantiateImportedAsset(const SharedPtr<T>& importer)
    {
        importer_ = importer;

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

    void MountSourceDirectory()
    {
        MountPoint* mount = GetSubsystem<VirtualFileSystem>()->MountDir(GetPath(sourceFile_));
        mountedDirectory_ = ea::make_unique<MountPointGuard>(mount);
    }

    bool LoadTextureAsset()
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

    bool LoadTextureDescriptor(const ea::string& rootName)
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

    bool LoadMaterialAsset()
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

    bool LoadAnimationAsset()
    {
        MountSourceDirectory();
        SharedPtr<Animation> animation = LoadAnimationResource(sourceFile_);
        if (!animation)
            return Fail("Failed to load rbfx animation");
        assetResources_.push_back(animation);
        BuildAnimationDebugPreview(animation);
        return true;
    }

    bool LoadParticleEffectAsset()
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

    bool LoadParticleGraphEffectAsset()
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

    bool LoadParticleEffect2DAsset()
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

    bool LoadXMLAsset()
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

    bool LoadNativeModel()
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

    bool LoadNativePrefab()
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

    bool LoadNativeScene()
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
        EnsureViewerEnvironment();

        cameraNode_ = scene_->CreateChild("Viewer Camera");
        cameraNode_->SetTemporary(true);
        camera_ = cameraNode_->CreateComponent<Camera>();
        freeFlyController_ = cameraNode_->CreateComponent<FreeFlyController>();
        freeFlyController_->SetEnabled(freeLookEnabled_);
        camera_->SetNearClip(0.01f);
        camera_->SetFarClip(100000.0f);
        viewport_->SetScene(scene_);
        viewport_->SetCamera(camera_);
        return true;
    }

    void EnsureVisibleMaterials()
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

    bool Fail(const ea::string& message)
    {
        error_ = message;
        URHO3D_LOGERROR("AssetViewer: {}", message);
        return false;
    }

    void AddResource(Resource* resource, ea::unordered_set<Resource*>& visited)
    {
        if (!resource || !visited.insert(resource).second)
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

    void CollectSerializableResources(Serializable* serializable, ea::unordered_set<Resource*>& visited)
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

    void CollectSceneResources()
    {
        ea::unordered_set<Resource*> visited;
        ea::vector<SharedPtr<Resource>> existing = assetResources_;
        assetResources_.clear();
        for (Resource* resource : existing)
            AddResource(resource, visited);

        if (!contentRoot_)
            return;
        ea::vector<Node*> nodes{contentRoot_};
        const ea::vector<Node*> children = contentRoot_->GetChildren(true);
        nodes.insert(nodes.end(), children.begin(), children.end());
        for (Node* node : nodes)
        {
            CollectSerializableResources(node, visited);
            for (const SharedPtr<Component>& component : node->GetComponents())
                CollectSerializableResources(component, visited);
        }
    }

    void CollectAnimations()
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

    void CollectParticleEmitters()
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

    void SetParticlesEmitting(bool emitting)
    {
        particlesEmitting_ = emitting;
        for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
        {
            if (emitter)
                emitter->SetEmitting(emitting);
        }
        for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
        {
            if (emitter)
                emitter->SetEmitting(emitting);
        }
        for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
        {
            if (emitter)
                emitter->SetEmitting(emitting);
        }
    }

    void RestartParticles()
    {
        for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
        {
            if (emitter)
                emitter->Reset();
        }
        for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
        {
            if (emitter)
                emitter->Reset();
        }
        for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
        {
            if (emitter)
            {
                emitter->SetEmitting(false);
                emitter->SetEmitting(true);
            }
        }
        particlesEmitting_ = true;
        BeginParticleAutoFrame();
    }

    void RenderParticleControls()
    {
        const unsigned numEmitters =
            legacyParticleEmitters_.size() + graphParticleEmitters_.size() + particleEmitters2D_.size();
        if (numEmitters == 0)
            return;

        if (ui::Checkbox("Emit particles", &particlesEmitting_))
            SetParticlesEmitting(particlesEmitting_);
        ui::SameLine();
        if (ui::Button("Restart particles"))
            RestartParticles();
        ui::TextDisabled("%u emitter%s: %u legacy, %u graph, %u 2D", numEmitters, numEmitters == 1 ? "" : "s",
            legacyParticleEmitters_.size(), graphParticleEmitters_.size(), particleEmitters2D_.size());
    }

    void RenderAnimationControls()
    {
        if (animations_.empty())
            return;

        Animation* selected = GetSelectedAnimation();
        const ea::string selectedLabel = selected ? GetAnimationLabel(selected) : "Select animation";
        ui::SetNextItemWidth(-1.0f);
        if (ui::BeginCombo("Animation", selectedLabel.c_str()))
        {
            for (unsigned i = 0; i < animations_.size(); ++i)
            {
                Animation* animation = animations_[i];
                const ea::string label = GetAnimationLabel(animation);
                ui::PushID(static_cast<int>(i));
                if (ui::Selectable(label.c_str(), selectedAnimation_ == i))
                    SetSelectedAnimation(i);
                if (selectedAnimation_ == i)
                    ui::SetItemDefaultFocus();
                if (ui::IsItemHovered() && animation && animation->GetName() != label)
                    ui::SetTooltip("%s", animation->GetName().c_str());
                ui::PopID();
            }
            ui::EndCombo();
        }

        if (animationControllers_.empty())
        {
            ui::TextDisabled("No AnimationController in the viewed content.");
            return;
        }

        UpdateAnimationTimeFromController();
        bool playing = animationsPlaying_;
        if (ui::Checkbox("Play", &playing))
            SetAnimationsPlaying(playing);
        ui::SameLine();
        if (ui::Checkbox("Loop", &animationLooped_))
        {
            UpdateAnimationTimeFromController();
            ApplySelectedAnimation(animationsPlaying_);
        }
        ui::SameLine();
        ui::TextDisabled("%u clip%s", animations_.size(), animations_.size() == 1 ? "" : "s");

        if (selected)
        {
            ui::SetNextItemWidth(-1.0f);
            if (ui::SliderFloat("Time", &animationTime_, 0.0f, Max(0.001f, selected->GetLength()), "%.3f s"))
                SetAnimationTime(animationTime_);
        }
        ui::SetNextItemWidth(-1.0f);
        if (ui::DragFloat("Speed", &animationSpeed_, 0.05f, -10.0f, 10.0f, "%.2fx"))
            SetAnimationSpeed(animationSpeed_);
    }

    void RenderViewport()
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (!previewTexture_)
            flags |= ImGuiWindowFlags_NoBackground;
        const bool visible = ui::Begin("Viewport", nullptr, flags);
        Graphics* graphics = GetSubsystem<Graphics>();
        if (visible)
        {
            const ImVec2 origin = ui::GetCursorScreenPos();
            const ImVec2 available = ui::GetContentRegionAvail();
            if (available.x > 0.0f && available.y > 0.0f)
            {
                const int left = Clamp(RoundToInt(origin.x), 0, Max(0, graphics->GetWidth() - 1));
                const int top = Clamp(RoundToInt(origin.y), 0, Max(0, graphics->GetHeight() - 1));
                const int right = Clamp(RoundToInt(origin.x + available.x), left + 1, graphics->GetWidth());
                const int bottom = Clamp(RoundToInt(origin.y + available.y), top + 1, graphics->GetHeight());
                viewport_->SetRect(IntRect(left, top, right, bottom));
                camera_->SetAspectRatio(static_cast<float>(right - left) / (bottom - top));

                if (previewTexture_)
                {
                    const ImVec2 original(static_cast<float>(previewTexture_->GetWidth()),
                        static_cast<float>(previewTexture_->GetHeight()));
                    Widgets::Image(previewTexture_, Widgets::FitContent(available, original));
                    sceneHovered_ = false;
                }
                else
                {
                    ui::InvisibleButton("##SceneViewport", available,
                        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
                            | ImGuiButtonFlags_MouseButtonMiddle);
                    sceneHovered_ = ui::IsItemHovered();
                }
            }
            else
                sceneHovered_ = false;
        }
        else
            sceneHovered_ = false;
        ui::End();

        Input* input = GetSubsystem<Input>();
        if (freeFlyController_ && !input->GetMouseButtonDown(MOUSEB_RIGHT))
            freeFlyController_->SetEnabled(freeLookEnabled_ && !is2DPreview_ && sceneHovered_);
    }

    void RenderInspector()
    {
        if (ui::Begin("Asset Inspector"))
        {
            ui::TextDisabled("Drop an asset file to inspect it.");
            if (contentRoot_ && ui::Button("Frame (F)"))
            {
                particleAutoFramePending_ = false;
                FrameContent();
            }
            if (contentRoot_ && !is2DPreview_)
            {
                ui::SameLine();
                if (ui::Checkbox("Free look", &freeLookEnabled_) && freeFlyController_)
                    freeFlyController_->SetEnabled(freeLookEnabled_ && sceneHovered_);
                ui::TextDisabled(freeLookEnabled_ ? "RMB + WASD/QE, Shift boosts" : "RMB orbit, MMB pan, wheel zoom");
            }
            RenderAnimationControls();
            RenderParticleControls();
            ui::Separator();

            if (loading_)
                ui::TextDisabled("Loading %s...", GetFileNameAndExtension(loadingFile_).c_str());
            if (!error_.empty())
            {
                ui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.25f, 1.0f));
                ui::TextWrapped("%s", error_.c_str());
                ui::PopStyleColor();
            }
            if (sourceFile_.empty())
                ui::TextDisabled("No asset loaded.");
            else if (ui::BeginTabBar("InspectorTabs"))
            {
                if (ui::BeginTabItem("Overview"))
                {
                    DrawOverview();
                    ui::EndTabItem();
                }
                if (gltfModel_ && ui::BeginTabItem("glTF Source"))
                {
                    DrawGLTF();
                    ui::EndTabItem();
                }
                if (ui::BeginTabItem("Engine"))
                {
                    DrawResources();
                    ui::EndTabItem();
                }
                if (contentRoot_ && ui::BeginTabItem("Scene"))
                {
                    DrawNode(contentRoot_);
                    ui::EndTabItem();
                }
                if (!sourceJson_.empty() && ui::BeginTabItem("Raw JSON"))
                {
                    ui::InputTextMultiline(
                        "##RawJson", &sourceJson_, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
                    ui::EndTabItem();
                }
                if (!sourceText_.empty() && ui::BeginTabItem("Raw Source"))
                {
                    ui::InputTextMultiline(
                        "##RawSource", &sourceText_, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
                    ui::EndTabItem();
                }
                ui::EndTabBar();
            }
        }
        ui::End();
    }

    void RenderUI()
    {
        static const ImGuiID dockspaceId = ImHashStr("AssetViewerDockSpace");
        const bool initializeLayout = ui::DockBuilderGetNode(dockspaceId) == nullptr;
        ui::DockSpaceOverViewport(dockspaceId, ui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        if (initializeLayout)
        {
            ui::DockBuilderRemoveNode(dockspaceId);
            ui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ui::DockBuilderSetNodeSize(dockspaceId, ui::GetMainViewport()->Size);
            ImGuiID viewportDock{};
            ImGuiID inspectorDock{};
            ui::DockBuilderSplitNode(
                dockspaceId, ImGuiDir_Right, 0.34f, &inspectorDock, &viewportDock);
            ui::DockBuilderDockWindow("Viewport", viewportDock);
            ui::DockBuilderDockWindow("Asset Inspector", inspectorDock);
            ui::DockBuilderFinish(dockspaceId);
        }
        RenderViewport();
        RenderInspector();
    }

    void DrawOverview()
    {
        File file(context_, sourceFile_, FILE_READ);
        Property("File", GetFileNameAndExtension(sourceFile_));
        Property("Absolute path", sourceFile_);
        Property("Extension", GetExtension(sourceFile_));
        Property("File size", FormatBytes(file.GetSize()));
        if (!convertedFile_.empty())
            Property("Converted glTF", convertedFile_);
        Property("Engine resources", assetResources_.size());
        Property("Viewer fallback material slots", fallbackMaterialSlots_);
        Property("Preview projection", is2DPreview_ ? "Orthographic 2D" : "Perspective 3D");
        Property("Camera target", cameraTarget_.ToString());
        Property(
            is2DPreview_ ? "Orthographic size" : "Camera distance", is2DPreview_ ? camera_->GetOrthoSize() : distance_);
        Property("Camera clip", Format("{} .. {}", camera_->GetNearClip(), camera_->GetFarClip()));
        Property("Rendered geometries", GetSubsystem<Renderer>()->GetNumGeometries());

        const BoundingBox box = GetContentBoundingBox();
        if (box.Defined())
        {
            Property("Bounds min", box.min_.ToString());
            Property("Bounds max", box.max_.ToString());
            Property("Bounds size", box.Size().ToString());
        }
        if (particleAutoFrameBounds_.Defined())
        {
            Property("Particle sample min", particleAutoFrameBounds_.min_.ToString());
            Property("Particle sample max", particleAutoFrameBounds_.max_.ToString());
            Property("Particle sample size", particleAutoFrameBounds_.Size().ToString());
        }
        if (gltfModel_)
        {
            Property("glTF version", gltfModel_->asset.version.c_str());
            Property("Generator", gltfModel_->asset.generator.c_str());
            Property("Scenes", gltfModel_->scenes.size());
            Property("Nodes", gltfModel_->nodes.size());
            Property("Meshes", gltfModel_->meshes.size());
            Property("Materials", gltfModel_->materials.size());
            Property("Animations", gltfModel_->animations.size());
            Property("Skins", gltfModel_->skins.size());
        }
        if (!converterOutput_.empty() && ui::TreeNodeEx("Importer / converter log", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            ui::TextWrapped("%s", converterOutput_.c_str());
            ui::TreePop();
        }
    }

    void DrawGLTF()
    {
        const tinygltf::Model& model = *gltfModel_;
        if (ui::TreeNodeEx("Asset", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            Property("Version", model.asset.version.c_str());
            Property("Minimum version", model.asset.minVersion.c_str());
            Property("Generator", model.asset.generator.c_str());
            Property("Copyright", model.asset.copyright.c_str());
            Property("Default scene", model.defaultScene);
            if (!model.extensionsUsed.empty())
            {
                ea::string value;
                for (const auto& extension : model.extensionsUsed)
                    value += (value.empty() ? "" : ", ") + ea::string(extension.c_str());
                Property("Extensions used", value);
            }
            if (!model.extensionsRequired.empty())
            {
                ea::string value;
                for (const auto& extension : model.extensionsRequired)
                    value += (value.empty() ? "" : ", ") + ea::string(extension.c_str());
                Property("Required", value);
            }
            DrawTinyExtensionsAndExtras(model.asset.extensions, model.asset.extras);
            DrawTinyExtensionsAndExtras(model.extensions, model.extras);
            ui::TreePop();
        }
        DrawGLTFScenes(model);
        DrawGLTFNodes(model);
        DrawGLTFMeshes(model);
        DrawGLTFMaterials(model);
        DrawGLTFAccessorsAndBuffers(model);
        DrawGLTFTextures(model);
        DrawGLTFSkinsAndAnimations(model);
        DrawGLTFCamerasAndLights(model);
    }

    void DrawGLTFScenes(const tinygltf::Model& model)
    {
        if (!TreeNode("Scenes", model.scenes.size()))
            return;
        for (unsigned i = 0; i < model.scenes.size(); ++i)
        {
            const auto& scene = model.scenes[i];
            ui::PushID(i);
            if (ui::TreeNodeEx("scene", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(scene.name, i).c_str()))
            {
                Property("Root nodes", JoinInts(scene.nodes));
                DrawTinyExtensionsAndExtras(scene.extensions, scene.extras);
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawGLTFNodes(const tinygltf::Model& model)
    {
        if (!TreeNode("Nodes", model.nodes.size()))
            return;
        for (unsigned i = 0; i < model.nodes.size(); ++i)
        {
            const auto& node = model.nodes[i];
            ui::PushID(i);
            if (ui::TreeNodeEx("node", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(node.name, i).c_str()))
            {
                Property("Mesh", node.mesh);
                Property("Skin", node.skin);
                Property("Camera", node.camera);
                Property("Children", JoinInts(node.children));
                if (!node.translation.empty())
                    Property("Translation", JoinDoubles(node.translation));
                if (!node.rotation.empty())
                    Property("Rotation", JoinDoubles(node.rotation));
                if (!node.scale.empty())
                    Property("Scale", JoinDoubles(node.scale));
                if (!node.matrix.empty())
                    Property("Matrix", JoinDoubles(node.matrix));
                if (!node.weights.empty())
                    Property("Morph weights", JoinDoubles(node.weights));
                DrawTinyExtensionsAndExtras(node.extensions, node.extras);
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawGLTFMeshes(const tinygltf::Model& model)
    {
        if (!TreeNode("Meshes", model.meshes.size()))
            return;
        for (unsigned meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
        {
            const auto& mesh = model.meshes[meshIndex];
            ui::PushID(meshIndex);
            if (ui::TreeNodeEx(
                    "mesh", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(mesh.name, meshIndex).c_str()))
            {
                Property("Default weights", JoinDoubles(mesh.weights));
                if (TreeNode("Primitives", mesh.primitives.size()))
                {
                    for (unsigned primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
                    {
                        const auto& primitive = mesh.primitives[primitiveIndex];
                        ui::PushID(primitiveIndex);
                        if (ui::TreeNodeEx(
                                "primitive", ImGuiTreeNodeFlags_SpanAvailWidth, "Primitive %u", primitiveIndex))
                        {
                            Property("Mode", Format("{} ({})", PrimitiveName(primitive.mode), primitive.mode));
                            Property("Indices", primitive.indices);
                            Property("Material", primitive.material);
                            if (TreeNode("Attributes", primitive.attributes.size()))
                            {
                                for (const auto& [semantic, accessor] : primitive.attributes)
                                    Property(semantic.c_str(), accessor);
                                ui::TreePop();
                            }
                            if (TreeNode("Morph targets", primitive.targets.size()))
                            {
                                for (unsigned targetIndex = 0; targetIndex < primitive.targets.size(); ++targetIndex)
                                {
                                    ui::PushID(targetIndex);
                                    if (ui::TreeNodeEx(
                                            "target", ImGuiTreeNodeFlags_SpanAvailWidth, "Target %u", targetIndex))
                                    {
                                        for (const auto& [semantic, accessor] : primitive.targets[targetIndex])
                                            Property(semantic.c_str(), accessor);
                                        ui::TreePop();
                                    }
                                    ui::PopID();
                                }
                                ui::TreePop();
                            }
                            DrawTinyExtensionsAndExtras(primitive.extensions, primitive.extras);
                            ui::TreePop();
                        }
                        ui::PopID();
                    }
                    ui::TreePop();
                }
                DrawTinyExtensionsAndExtras(mesh.extensions, mesh.extras);
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawTextureInfo(const char* name, const tinygltf::TextureInfo& info)
    {
        if (ui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            Property("Texture", info.index);
            Property("TexCoord", info.texCoord);
            DrawTinyExtensionsAndExtras(info.extensions, info.extras);
            ui::TreePop();
        }
    }

    void DrawGLTFMaterials(const tinygltf::Model& model)
    {
        if (!TreeNode("Materials", model.materials.size()))
            return;
        for (unsigned i = 0; i < model.materials.size(); ++i)
        {
            const auto& material = model.materials[i];
            ui::PushID(i);
            if (ui::TreeNodeEx(
                    "material", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(material.name, i).c_str()))
            {
                Property("Alpha mode", material.alphaMode.c_str());
                Property("Alpha cutoff", material.alphaCutoff);
                Property("Double sided", material.doubleSided ? "true" : "false");
                Property("Emissive factor", JoinDoubles(material.emissiveFactor));
                if (ui::TreeNodeEx("PBR metallic-roughness", ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    Property("Base color", JoinDoubles(material.pbrMetallicRoughness.baseColorFactor));
                    Property("Metallic", material.pbrMetallicRoughness.metallicFactor);
                    Property("Roughness", material.pbrMetallicRoughness.roughnessFactor);
                    DrawTextureInfo("Base color texture", material.pbrMetallicRoughness.baseColorTexture);
                    DrawTextureInfo(
                        "Metallic/roughness texture", material.pbrMetallicRoughness.metallicRoughnessTexture);
                    DrawTinyExtensionsAndExtras(
                        material.pbrMetallicRoughness.extensions, material.pbrMetallicRoughness.extras);
                    ui::TreePop();
                }
                DrawTextureInfo("Emissive texture", material.emissiveTexture);
                if (ui::TreeNodeEx("Normal texture", ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    Property("Texture", material.normalTexture.index);
                    Property("TexCoord", material.normalTexture.texCoord);
                    Property("Scale", material.normalTexture.scale);
                    DrawTinyExtensionsAndExtras(material.normalTexture.extensions, material.normalTexture.extras);
                    ui::TreePop();
                }
                if (ui::TreeNodeEx("Occlusion texture", ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    Property("Texture", material.occlusionTexture.index);
                    Property("TexCoord", material.occlusionTexture.texCoord);
                    Property("Strength", material.occlusionTexture.strength);
                    DrawTinyExtensionsAndExtras(material.occlusionTexture.extensions, material.occlusionTexture.extras);
                    ui::TreePop();
                }
                DrawGLTFParameters("Values", material.values);
                DrawGLTFParameters("Additional values", material.additionalValues);
                DrawTinyExtensionsAndExtras(material.extensions, material.extras);
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawGLTFParameters(const char* label, const tinygltf::ParameterMap& parameters)
    {
        if (parameters.empty() || !TreeNode(label, parameters.size()))
            return;
        for (const auto& [name, parameter] : parameters)
        {
            if (ui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                Property("Bool", parameter.bool_value ? "true" : "false");
                Property("Has number", parameter.has_number_value ? "true" : "false");
                Property("Number", parameter.number_value);
                Property("String", parameter.string_value.c_str());
                Property("Number array", JoinDoubles(parameter.number_array));
                for (const auto& [key, value] : parameter.json_double_value)
                    Property(key.c_str(), value);
                ui::TreePop();
            }
        }
        ui::TreePop();
    }

    void DrawGLTFAccessorsAndBuffers(const tinygltf::Model& model)
    {
        if (TreeNode("Accessors", model.accessors.size()))
        {
            for (unsigned i = 0; i < model.accessors.size(); ++i)
            {
                const auto& accessor = model.accessors[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "accessor", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(accessor.name, i).c_str()))
                {
                    Property("Buffer view", accessor.bufferView);
                    Property("Byte offset", accessor.byteOffset);
                    Property("Count", accessor.count);
                    Property("Type", Format("{} ({})", AccessorTypeName(accessor.type), accessor.type));
                    Property("Component",
                        Format("{} ({})", ComponentTypeName(accessor.componentType), accessor.componentType));
                    Property("Normalized", accessor.normalized ? "true" : "false");
                    Property("Minimum", JoinDoubles(accessor.minValues));
                    Property("Maximum", JoinDoubles(accessor.maxValues));
                    Property("Sparse", accessor.sparse.isSparse ? "true" : "false");
                    if (accessor.sparse.isSparse)
                    {
                        Property("Sparse count", accessor.sparse.count);
                        Property("Indices view", accessor.sparse.indices.bufferView);
                        Property("Indices offset", accessor.sparse.indices.byteOffset);
                        Property("Indices component", ComponentTypeName(accessor.sparse.indices.componentType));
                        Property("Values view", accessor.sparse.values.bufferView);
                        Property("Values offset", accessor.sparse.values.byteOffset);
                    }
                    DrawTinyExtensionsAndExtras(accessor.extensions, accessor.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Buffer views", model.bufferViews.size()))
        {
            for (unsigned i = 0; i < model.bufferViews.size(); ++i)
            {
                const auto& view = model.bufferViews[i];
                ui::PushID(i);
                if (ui::TreeNodeEx("view", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(view.name, i).c_str()))
                {
                    Property("Buffer", view.buffer);
                    Property("Byte offset", view.byteOffset);
                    Property("Byte length", FormatBytes(view.byteLength));
                    Property("Byte stride", view.byteStride);
                    Property("Target", view.target);
                    Property("Draco decoded", view.dracoDecoded ? "true" : "false");
                    DrawTinyExtensionsAndExtras(view.extensions, view.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Buffers", model.buffers.size()))
        {
            for (unsigned i = 0; i < model.buffers.size(); ++i)
            {
                const auto& buffer = model.buffers[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "buffer", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(buffer.name, i).c_str()))
                {
                    Property("URI", buffer.uri.c_str());
                    Property("Decoded size", FormatBytes(buffer.data.size()));
                    DrawTinyExtensionsAndExtras(buffer.extensions, buffer.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
    }

    void DrawGLTFTextures(const tinygltf::Model& model)
    {
        if (TreeNode("Images", model.images.size()))
        {
            for (unsigned i = 0; i < model.images.size(); ++i)
            {
                const auto& image = model.images[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "image", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(image.name, i).c_str()))
                {
                    Property("URI", image.uri.c_str());
                    Property("MIME type", image.mimeType.c_str());
                    Property("Buffer view", image.bufferView);
                    Property("Dimensions", Format("{} x {}", image.width, image.height));
                    Property("Components", image.component);
                    Property("Bits", image.bits);
                    Property("Pixel type", ComponentTypeName(image.pixel_type));
                    Property("Decoded bytes", FormatBytes(image.image.size()));
                    Property("Stored as-is", image.as_is ? "true" : "false");
                    DrawTinyExtensionsAndExtras(image.extensions, image.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Textures", model.textures.size()))
        {
            for (unsigned i = 0; i < model.textures.size(); ++i)
            {
                const auto& texture = model.textures[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "texture", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(texture.name, i).c_str()))
                {
                    Property("Image", texture.source);
                    Property("Sampler", texture.sampler);
                    DrawTinyExtensionsAndExtras(texture.extensions, texture.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Samplers", model.samplers.size()))
        {
            for (unsigned i = 0; i < model.samplers.size(); ++i)
            {
                const auto& sampler = model.samplers[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "sampler", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(sampler.name, i).c_str()))
                {
                    Property("Min filter", sampler.minFilter);
                    Property("Mag filter", sampler.magFilter);
                    Property("Wrap S", sampler.wrapS);
                    Property("Wrap T", sampler.wrapT);
                    DrawTinyExtensionsAndExtras(sampler.extensions, sampler.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
    }

    void DrawGLTFSkinsAndAnimations(const tinygltf::Model& model)
    {
        if (TreeNode("Skins", model.skins.size()))
        {
            for (unsigned i = 0; i < model.skins.size(); ++i)
            {
                const auto& skin = model.skins[i];
                ui::PushID(i);
                if (ui::TreeNodeEx("skin", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(skin.name, i).c_str()))
                {
                    Property("Skeleton root", skin.skeleton);
                    Property("Inverse binds", skin.inverseBindMatrices);
                    Property("Joints", JoinInts(skin.joints));
                    DrawTinyExtensionsAndExtras(skin.extensions, skin.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Animations", model.animations.size()))
        {
            for (unsigned i = 0; i < model.animations.size(); ++i)
            {
                const auto& animation = model.animations[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "animation", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(animation.name, i).c_str()))
                {
                    if (TreeNode("Channels", animation.channels.size()))
                    {
                        for (unsigned channelIndex = 0; channelIndex < animation.channels.size(); ++channelIndex)
                        {
                            const auto& channel = animation.channels[channelIndex];
                            ui::PushID(channelIndex);
                            if (ui::TreeNodeEx(
                                    "channel", ImGuiTreeNodeFlags_SpanAvailWidth, "Channel %u", channelIndex))
                            {
                                Property("Sampler", channel.sampler);
                                Property("Target node", channel.target_node);
                                Property("Target path", channel.target_path.c_str());
                                DrawTinyExtensionsAndExtras(channel.extensions, channel.extras);
                                if (!channel.target_extensions.empty()
                                    && TreeNode("Target extensions", channel.target_extensions.size()))
                                {
                                    for (const auto& [name, value] : channel.target_extensions)
                                        DrawTinyValue(name.c_str(), value);
                                    ui::TreePop();
                                }
                                ui::TreePop();
                            }
                            ui::PopID();
                        }
                        ui::TreePop();
                    }
                    if (TreeNode("Samplers", animation.samplers.size()))
                    {
                        for (unsigned samplerIndex = 0; samplerIndex < animation.samplers.size(); ++samplerIndex)
                        {
                            const auto& sampler = animation.samplers[samplerIndex];
                            ui::PushID(samplerIndex);
                            if (ui::TreeNodeEx(
                                    "sampler", ImGuiTreeNodeFlags_SpanAvailWidth, "Sampler %u", samplerIndex))
                            {
                                Property("Input accessor", sampler.input);
                                Property("Output accessor", sampler.output);
                                Property("Interpolation", sampler.interpolation.c_str());
                                DrawTinyExtensionsAndExtras(sampler.extensions, sampler.extras);
                                ui::TreePop();
                            }
                            ui::PopID();
                        }
                        ui::TreePop();
                    }
                    DrawTinyExtensionsAndExtras(animation.extensions, animation.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
    }

    void DrawGLTFCamerasAndLights(const tinygltf::Model& model)
    {
        if (TreeNode("Cameras", model.cameras.size()))
        {
            for (unsigned i = 0; i < model.cameras.size(); ++i)
            {
                const auto& camera = model.cameras[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "camera", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(camera.name, i).c_str()))
                {
                    Property("Type", camera.type.c_str());
                    if (camera.type == "perspective")
                    {
                        Property("Aspect ratio", camera.perspective.aspectRatio);
                        Property("Y FOV", camera.perspective.yfov);
                        Property("Near", camera.perspective.znear);
                        Property("Far", camera.perspective.zfar);
                    }
                    else
                    {
                        Property("X magnification", camera.orthographic.xmag);
                        Property("Y magnification", camera.orthographic.ymag);
                        Property("Near", camera.orthographic.znear);
                        Property("Far", camera.orthographic.zfar);
                    }
                    DrawTinyExtensionsAndExtras(camera.extensions, camera.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Lights", model.lights.size()))
        {
            for (unsigned i = 0; i < model.lights.size(); ++i)
            {
                const auto& light = model.lights[i];
                ui::PushID(i);
                if (ui::TreeNodeEx(
                        "light", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(light.name, i).c_str()))
                {
                    Property("Type", light.type.c_str());
                    Property("Color", JoinDoubles(light.color));
                    Property("Intensity", light.intensity);
                    Property("Range", light.range);
                    Property("Inner cone", light.spot.innerConeAngle);
                    Property("Outer cone", light.spot.outerConeAngle);
                    DrawTinyExtensionsAndExtras(light.extensions, light.extras);
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
    }

    void DrawResources()
    {
        Property("Resource count", assetResources_.size());
        for (unsigned i = 0; i < assetResources_.size(); ++i)
        {
            Resource* resource = assetResources_[i];
            if (!resource)
                continue;
            ui::PushID(i);
            if (ui::TreeNodeEx("resource", ImGuiTreeNodeFlags_SpanAvailWidth, "%s: %s", resource->GetTypeName().c_str(),
                    resource->GetName().c_str()))
            {
                DrawResource(resource);
                ui::TreePop();
            }
            ui::PopID();
        }
    }

    void DrawResource(Resource* resource)
    {
        Property("Type", resource->GetTypeName());
        Property("Name", resource->GetName());
        Property("Name hash", resource->GetNameHash().Value());
        Property("Absolute file", resource->GetAbsoluteFileName());
        Property("Memory use", FormatBytes(resource->GetMemoryUse()));
        Property("References", resource->Refs());

        if (auto* metadataResource = dynamic_cast<ResourceWithMetadata*>(resource))
        {
            if (TreeNode("Metadata", metadataResource->GetMetadataKeys().size()))
            {
                for (const ea::string& key : metadataResource->GetMetadataKeys())
                    Property(key.c_str(),
                        Format("{}: {}", metadataResource->GetMetadata(key).GetTypeName(),
                            metadataResource->GetMetadata(key).ToString()));
                ui::TreePop();
            }
        }
        if (auto* model = dynamic_cast<Model*>(resource))
            DrawModel(model);
        else if (auto* material = dynamic_cast<Material*>(resource))
            DrawMaterial(material);
        else if (auto* animation = dynamic_cast<Animation*>(resource))
            DrawAnimation(animation);
        else if (auto* effect = dynamic_cast<ParticleEffect*>(resource))
            DrawParticleEffect(effect);
        else if (auto* effect = dynamic_cast<ParticleGraphEffect*>(resource))
            DrawParticleGraphEffect(effect);
        else if (auto* effect = dynamic_cast<ParticleEffect2D*>(resource))
            DrawParticleEffect2D(effect);
        else if (auto* sprite = dynamic_cast<Sprite2D*>(resource))
        {
            Property("Texture", sprite->GetTexture() ? sprite->GetTexture()->GetName() : "(null)");
            Property("Rectangle", sprite->GetRectangle().ToString());
            Property("Hot spot", sprite->GetHotSpot().ToString());
            Property("Offset", sprite->GetOffset().ToString());
            Property("Texture edge offset", sprite->GetTextureEdgeOffset());
            Property("Sprite sheet", sprite->GetSpriteSheet() ? sprite->GetSpriteSheet()->GetName() : "(null)");
        }
        else if (auto* texture = dynamic_cast<Texture*>(resource))
            DrawTexture(texture);
        else if (auto* image = dynamic_cast<Image*>(resource))
        {
            Property("Dimensions", Format("{} x {} x {}", image->GetWidth(), image->GetHeight(), image->GetDepth()));
            Property("Components", image->GetComponents());
            Property("GPU format", static_cast<unsigned>(image->GetGPUFormat()));
            Property("Compressed", image->IsCompressed() ? "true" : "false");
            Property("Compressed format", static_cast<unsigned>(image->GetCompressedFormat()));
            Property("Compressed levels", image->GetNumCompressedLevels());
            Property("Cubemap", image->IsCubemap() ? "true" : "false");
            Property("Array", image->IsArray() ? "true" : "false");
            Property("Source sRGB", image->IsSRGB() ? "true" : "false");
            Property("Alpha channel", image->HasAlphaChannel() ? "true" : "false");
            if (image->IsCompressed() && TreeNode("Compressed mip levels", image->GetNumCompressedLevels()))
            {
                for (unsigned i = 0; i < image->GetNumCompressedLevels(); ++i)
                {
                    const CompressedLevel level = image->GetCompressedLevel(i);
                    Property(Format("Level {}", i).c_str(),
                        Format("{} x {} x {} | format={} | {}", level.width_, level.height_, level.depth_,
                            static_cast<unsigned>(level.format_), FormatBytes(level.dataSize_)));
                }
                ui::TreePop();
            }
            unsigned siblingIndex = 0;
            for (const Image* sibling = image; sibling; sibling = sibling->GetNextSibling())
                ++siblingIndex;
            Property("Images / faces / layers", siblingIndex);
        }
    }

    void DrawParticleEffect(ParticleEffect* effect)
    {
        Property("Material", effect->GetMaterial() ? effect->GetMaterial()->GetName() : "(null)");
        Property("Maximum particles", effect->GetNumParticles());
        Property("Update invisible", effect->GetUpdateInvisible() ? "true" : "false");
        Property("Relative", effect->IsRelative() ? "true" : "false");
        Property("Scaled", effect->IsScaled() ? "true" : "false");
        Property("Sorted", effect->IsSorted() ? "true" : "false");
        Property("Fixed screen size", effect->IsFixedScreenSize() ? "true" : "false");
        Property("Animation LOD bias", effect->GetAnimationLodBias());
        Property("Emitter type", static_cast<unsigned>(effect->GetEmitterType()));
        Property("Emitter size", effect->GetEmitterSize().ToString());
        Property("Direction minimum", effect->GetMinDirection().ToString());
        Property("Direction maximum", effect->GetMaxDirection().ToString());
        Property("Constant force", effect->GetConstantForce().ToString());
        Property("Damping force", effect->GetDampingForce());
        Property("Active time", effect->GetActiveTime());
        Property("Inactive time", effect->GetInactiveTime());
        Property("Emission rate minimum", effect->GetMinEmissionRate());
        Property("Emission rate maximum", effect->GetMaxEmissionRate());
        Property("Particle size minimum", effect->GetMinParticleSize().ToString());
        Property("Particle size maximum", effect->GetMaxParticleSize().ToString());
        Property("TTL minimum", effect->GetMinTimeToLive());
        Property("TTL maximum", effect->GetMaxTimeToLive());
        Property("Velocity minimum", effect->GetMinVelocity());
        Property("Velocity maximum", effect->GetMaxVelocity());
        Property("Rotation minimum", effect->GetMinRotation());
        Property("Rotation maximum", effect->GetMaxRotation());
        Property("Rotation speed minimum", effect->GetMinRotationSpeed());
        Property("Rotation speed maximum", effect->GetMaxRotationSpeed());
        Property("Size additive", effect->GetSizeAdd());
        Property("Size multiplier", effect->GetSizeMul());
        Property("Face camera mode", static_cast<unsigned>(effect->GetFaceCameraMode()));
        if (TreeNode("Color frames", effect->GetColorFrames().size()))
        {
            for (unsigned i = 0; i < effect->GetColorFrames().size(); ++i)
            {
                const ColorFrame& frame = effect->GetColorFrames()[i];
                Property(Format("[{}]", i).c_str(), Format("time={} color={}", frame.time_, frame.color_.ToString()));
            }
            ui::TreePop();
        }
        if (TreeNode("Texture frames", effect->GetTextureFrames().size()))
        {
            for (unsigned i = 0; i < effect->GetTextureFrames().size(); ++i)
            {
                const TextureFrame& frame = effect->GetTextureFrames()[i];
                Property(Format("[{}]", i).c_str(), Format("time={} uv={}", frame.time_, frame.uv_.ToString()));
            }
            ui::TreePop();
        }
    }

    void DrawParticleEffect2D(ParticleEffect2D* effect)
    {
        Property("Sprite", effect->GetSprite() ? effect->GetSprite()->GetName() : "(null)");
        Property("Source position variance", effect->GetSourcePositionVariance().ToString());
        Property("Speed", effect->GetSpeed());
        Property("Speed variance", effect->GetSpeedVariance());
        Property("Particle lifespan", effect->GetParticleLifeSpan());
        Property("Lifespan variance", effect->GetParticleLifespanVariance());
        Property("Angle", effect->GetAngle());
        Property("Angle variance", effect->GetAngleVariance());
        Property("Gravity", effect->GetGravity().ToString());
        Property("Radial acceleration", effect->GetRadialAcceleration());
        Property("Radial accel variance", effect->GetRadialAccelVariance());
        Property("Tangential acceleration", effect->GetTangentialAcceleration());
        Property("Tangential accel variance", effect->GetTangentialAccelVariance());
        Property("Start color", effect->GetStartColor().ToString());
        Property("Start color variance", effect->GetStartColorVariance().ToString());
        Property("Finish color", effect->GetFinishColor().ToString());
        Property("Finish color variance", effect->GetFinishColorVariance().ToString());
        Property("Maximum particles", effect->GetMaxParticles());
        Property("Start size", effect->GetStartParticleSize());
        Property("Start size variance", effect->GetStartParticleSizeVariance());
        Property("Finish size", effect->GetFinishParticleSize());
        Property("Finish size variance", effect->GetFinishParticleSizeVariance());
        Property("Duration", effect->GetDuration());
        Property("Emitter type", static_cast<unsigned>(effect->GetEmitterType()));
        Property("Maximum radius", effect->GetMaxRadius());
        Property("Maximum radius variance", effect->GetMaxRadiusVariance());
        Property("Minimum radius", effect->GetMinRadius());
        Property("Minimum radius variance", effect->GetMinRadiusVariance());
        Property("Rotate per second", effect->GetRotatePerSecond());
        Property("Rotate per second variance", effect->GetRotatePerSecondVariance());
        Property("Blend mode", static_cast<unsigned>(effect->GetBlendMode()));
        Property("Rotation start", effect->GetRotationStart());
        Property("Rotation start variance", effect->GetRotationStartVariance());
        Property("Rotation end", effect->GetRotationEnd());
        Property("Rotation end variance", effect->GetRotationEndVariance());
    }

    void DrawParticleGraphSpan(const char* label, const ParticleGraphSpan& span)
    {
        Property(label, Format("offset={} size={}", span.offset_, FormatBytes(span.size_)));
    }

    void DrawParticleGraph(const char* label, ParticleGraph& graph)
    {
        if (!TreeNode(label, graph.GetNumNodes()))
            return;
        for (unsigned nodeIndex = 0; nodeIndex < graph.GetNumNodes(); ++nodeIndex)
        {
            ParticleGraphNode* node = graph.GetNode(nodeIndex);
            ui::PushID(nodeIndex);
            if (ui::TreeNodeEx(
                    "node", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", node->GetTypeName().c_str(), nodeIndex))
            {
                Property("Type hash", node->GetType().Value());
                Property("Instance size", FormatBytes(node->EvaluateInstanceSize()));
                DrawSerializable(node);
                if (TreeNode("Pins", node->GetNumPins()))
                {
                    for (unsigned pinIndex = 0; pinIndex < node->GetNumPins(); ++pinIndex)
                    {
                        const ParticleGraphPin& pin = node->GetPin(pinIndex);
                        ui::PushID(pinIndex);
                        if (ui::TreeNodeEx(
                                "pin", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", pin.GetName().c_str(), pinIndex))
                        {
                            Property("Direction", pin.IsInput() ? "Input" : "Output");
                            Property("Flags", pin.GetFlags().AsInteger());
                            Property("Name hash", pin.GetNameHash().Value());
                            Property("Requested type", Variant::GetTypeName(pin.GetRequestedType()));
                            Property("Runtime type", Variant::GetTypeName(pin.GetValueType()));
                            Property("Container type", static_cast<unsigned>(pin.GetContainerType()));
                            Property("Attribute index", pin.GetAttributeIndex());
                            const ParticleGraphPinRef memory = pin.GetMemoryReference();
                            Property("Memory reference",
                                Format("container={} index={}", static_cast<unsigned>(memory.type_), memory.index_));
                            Property("Connected", pin.GetConnected() ? "true" : "false");
                            if (pin.GetConnected())
                                Property("Source",
                                    Format("node={} pin={}", pin.GetConnectedNodeIndex(), pin.GetConnectedPinIndex()));
                            ui::TreePop();
                        }
                        ui::PopID();
                    }
                    ui::TreePop();
                }
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawParticleGraphEffect(ParticleGraphEffect* effect)
    {
        Property("Layers", effect->GetNumLayers());
        for (unsigned layerIndex = 0; layerIndex < effect->GetNumLayers(); ++layerIndex)
        {
            ParticleGraphLayer* layer = effect->GetLayer(layerIndex);
            ui::PushID(layerIndex);
            if (ui::TreeNodeEx("layer", ImGuiTreeNodeFlags_SpanAvailWidth, "Layer %u", layerIndex))
            {
                Property("Capacity", layer->GetCapacity());
                Property("Time scale", layer->GetTimeScale());
                Property("Loop", layer->IsLoop() ? "true" : "false");
                Property("Duration", layer->GetDuration());
                Property("Temporary buffer", FormatBytes(layer->GetTempBufferSize()));
                DrawSerializable(layer);

                const ParticleGraphAttributeLayout& attributes = layer->GetAttributeLayout();
                if (TreeNode("Particle attributes", attributes.GetNumAttributes()))
                {
                    Property("Required memory", FormatBytes(attributes.GetRequiredMemory()));
                    for (unsigned i = 0; i < attributes.GetNumAttributes(); ++i)
                    {
                        const ParticleGraphSpan span = attributes.GetSpan(i);
                        Property(Format("{} [{}]", attributes.GetName(i), i).c_str(),
                            Format("type={} offset={} size={}", Variant::GetTypeName(attributes.GetType(i)),
                                span.offset_, FormatBytes(span.size_)));
                    }
                    ui::TreePop();
                }

                const ParticleGraphLayer::AttributeBufferLayout& layout = layer->GetAttributeBufferLayout();
                if (ui::TreeNodeEx("Runtime memory layout", ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    Property("Attribute buffer", FormatBytes(layout.attributeBufferSize_));
                    DrawParticleGraphSpan("Emit pointers", layout.emitNodePointers_);
                    DrawParticleGraphSpan("Init pointers", layout.initNodePointers_);
                    DrawParticleGraphSpan("Update pointers", layout.updateNodePointers_);
                    DrawParticleGraphSpan("Node instances", layout.nodeInstances_);
                    DrawParticleGraphSpan("Indices", layout.indices_);
                    DrawParticleGraphSpan("Scalar indices", layout.scalarIndices_);
                    DrawParticleGraphSpan("Natural indices", layout.naturalIndices_);
                    DrawParticleGraphSpan("Destruction queue", layout.destructionQueue_);
                    DrawParticleGraphSpan("Values", layout.values_);
                    ui::TreePop();
                }
                DrawParticleGraph("Emit graph", layer->GetEmitGraph());
                DrawParticleGraph("Initialization graph", layer->GetInitGraph());
                DrawParticleGraph("Update graph", layer->GetUpdateGraph());
                ui::TreePop();
            }
            ui::PopID();
        }
    }

    void DrawModel(Model* model)
    {
        const BoundingBox& box = model->GetBoundingBox();
        Property("Bounds min", box.min_.ToString());
        Property("Bounds max", box.max_.ToString());
        Property("Bounds size", box.Size().ToString());
        Property("Geometry centers", model->GetGeometryCenters().size());
        if (TreeNode("Vertex buffers", model->GetVertexBuffers().size()))
        {
            for (unsigned i = 0; i < model->GetVertexBuffers().size(); ++i)
            {
                VertexBuffer* buffer = model->GetVertexBuffers()[i];
                ui::PushID(i);
                if (ui::TreeNodeEx("vb", ImGuiTreeNodeFlags_SpanAvailWidth, "Vertex buffer %u", i))
                {
                    Property("Vertices", buffer->GetVertexCount());
                    Property("Vertex size", FormatBytes(buffer->GetVertexSize()));
                    Property("Total size",
                        FormatBytes(
                            static_cast<unsigned long long>(buffer->GetVertexCount()) * buffer->GetVertexSize()));
                    Property("Dynamic", buffer->IsDynamic() ? "true" : "false");
                    Property("Morph range start", model->GetMorphRangeStart(i));
                    Property("Morph range count", model->GetMorphRangeCount(i));
                    if (TreeNode("Elements", buffer->GetElements().size()))
                    {
                        for (unsigned elementIndex = 0; elementIndex < buffer->GetElements().size(); ++elementIndex)
                        {
                            const VertexElement& element = buffer->GetElements()[elementIndex];
                            Property(Format("Element {}", elementIndex).c_str(),
                                Format("type={} semantic={} index={} offset={} step={} size={}",
                                    static_cast<unsigned>(element.type_), static_cast<unsigned>(element.semantic_),
                                    element.index_, element.offset_, element.stepRate_,
                                    ELEMENT_TYPESIZES[element.type_]));
                        }
                        ui::TreePop();
                    }
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Index buffers", model->GetIndexBuffers().size()))
        {
            for (unsigned i = 0; i < model->GetIndexBuffers().size(); ++i)
            {
                IndexBuffer* buffer = model->GetIndexBuffers()[i];
                Property(Format("Index buffer {}", i).c_str(),
                    Format("{} indices, {}-byte, {}, dynamic={}", buffer->GetIndexCount(), buffer->GetIndexSize(),
                        FormatBytes(static_cast<unsigned long long>(buffer->GetIndexCount()) * buffer->GetIndexSize()),
                        buffer->IsDynamic()));
            }
            ui::TreePop();
        }
        if (TreeNode("Geometries", model->GetGeometries().size()))
        {
            for (unsigned geometryIndex = 0; geometryIndex < model->GetGeometries().size(); ++geometryIndex)
            {
                ui::PushID(geometryIndex);
                if (ui::TreeNodeEx("geometry", ImGuiTreeNodeFlags_SpanAvailWidth, "Geometry %u", geometryIndex))
                {
                    Property("Center", model->GetGeometryCenter(geometryIndex).ToString());
                    if (geometryIndex < model->GetGeometryBoneMappings().size())
                        Property("Bone mapping", JoinUnsigned(model->GetGeometryBoneMappings()[geometryIndex]));
                    for (unsigned lod = 0; lod < model->GetGeometries()[geometryIndex].size(); ++lod)
                    {
                        Geometry* geometry = model->GetGeometries()[geometryIndex][lod];
                        ui::PushID(lod);
                        if (ui::TreeNodeEx("lod", ImGuiTreeNodeFlags_SpanAvailWidth, "LOD %u", lod))
                        {
                            Property("LOD distance", geometry->GetLodDistance());
                            Property("Primitive type", static_cast<unsigned>(geometry->GetPrimitiveType()));
                            Property("Primitive count", geometry->GetPrimitiveCount());
                            Property("Index start", geometry->GetIndexStart());
                            Property("Index count", geometry->GetIndexCount());
                            Property("Vertex start", geometry->GetVertexStart());
                            Property("Vertex count", geometry->GetVertexCount());
                            Property("Vertex buffers", geometry->GetNumVertexBuffers());
                            Property("Indexed", geometry->GetIndexBuffer() ? "true" : "false");
                            ui::TreePop();
                        }
                        ui::PopID();
                    }
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        DrawSkeleton(model->GetSkeleton());
        if (TreeNode("Morphs", model->GetMorphs().size()))
        {
            for (unsigned i = 0; i < model->GetMorphs().size(); ++i)
            {
                const ModelMorph& morph = model->GetMorphs()[i];
                ui::PushID(i);
                if (ui::TreeNodeEx("morph", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", morph.name_.c_str(), i))
                {
                    Property("Name hash", morph.nameHash_.Value());
                    Property("Default weight", morph.weight_);
                    if (TreeNode("Buffers", morph.buffers_.size()))
                    {
                        for (const auto& [bufferIndex, bufferMorph] : morph.buffers_)
                        {
                            Property(Format("Buffer {}", bufferIndex).c_str(),
                                Format("mask={} vertices={} data={}", bufferMorph.elementMask_.AsInteger(),
                                    bufferMorph.vertexCount_, FormatBytes(bufferMorph.dataSize_)));
                        }
                        ui::TreePop();
                    }
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
    }

    void DrawSkeleton(const Skeleton& skeleton)
    {
        if (!TreeNode("Skeleton", skeleton.GetBones().size()))
            return;
        Property("Bone order", JoinUnsigned(skeleton.GetBonesOrder()));
        for (unsigned i = 0; i < skeleton.GetBones().size(); ++i)
        {
            const Bone& bone = skeleton.GetBones()[i];
            ui::PushID(i);
            if (ui::TreeNodeEx("bone", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", bone.name_.c_str(), i))
            {
                Property("Name hash", bone.nameHash_.Value());
                Property("Parent", bone.parentIndex_);
                Property("Position", bone.initialPosition_.ToString());
                Property("Rotation", bone.initialRotation_.ToString());
                Property("Scale", bone.initialScale_.ToString());
                Property("Offset matrix", bone.offsetMatrix_.ToString());
                Property("Animated", bone.animated_ ? "true" : "false");
                Property("Collision mask", bone.collisionMask_.AsInteger());
                Property("Radius", bone.radius_);
                Property("Bounds min", bone.boundingBox_.min_.ToString());
                Property("Bounds max", bone.boundingBox_.max_.ToString());
                ui::TreePop();
            }
            ui::PopID();
        }
        ui::TreePop();
    }

    void DrawMaterial(Material* material)
    {
        Property("Techniques", material->GetNumTechniques());
        Property("Cull mode", cullModeNames[material->GetCullMode()]);
        Property("Shadow cull", cullModeNames[material->GetShadowCullMode()]);
        Property("Fill mode", fillModeNames[material->GetFillMode()]);
        Property("Render order", material->GetRenderOrder());
        if (TreeNode("Technique entries", material->GetTechniques().size()))
        {
            for (unsigned i = 0; i < material->GetTechniques().size(); ++i)
            {
                const TechniqueEntry& entry = material->GetTechniques()[i];
                Property(Format("Technique {}", i).c_str(),
                    Format("{} | quality={} | LOD distance={}",
                        entry.technique_ ? entry.technique_->GetName() : EMPTY_STRING,
                        static_cast<unsigned>(entry.qualityLevel_), entry.lodDistance_));
            }
            ui::TreePop();
        }
        if (TreeNode("Textures", material->GetTextures().size()))
        {
            for (const auto& [unit, texture] : material->GetTextures())
                Property(texture.name_.c_str(),
                    texture.value_ ? Format("{} ({})", texture.value_->GetName(), unit.Value()) : "(null)");
            ui::TreePop();
        }
        if (TreeNode("Shader parameters", material->GetShaderParameters().size()))
        {
            for (const auto& [hash, parameter] : material->GetShaderParameters())
                Property(parameter.name_.c_str(),
                    Format("{}: {} | hash={} | custom={}", parameter.value_.GetTypeName(), parameter.value_.ToString(),
                        hash.Value(), parameter.isCustom_));
            ui::TreePop();
        }
    }

    void DrawAnimation(Animation* animation)
    {
        Property("Animation name", animation->GetAnimationName());
        Property("Name hash", animation->GetAnimationNameHash().Value());
        Property("Length", animation->GetLength());
        if (TreeNode("Bone/node tracks", animation->GetTracks().size()))
        {
            unsigned trackIndex = 0;
            for (const auto& [_, track] : animation->GetTracks())
            {
                ui::PushID(trackIndex++);
                if (ui::TreeNodeEx("track", ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%zu keyframes)",
                        track.name_.c_str(), track.keyFrames_.size()))
                {
                    Property("Name hash", track.nameHash_.Value());
                    Property("Channels", track.channelMask_.AsInteger());
                    Property("Position weight", track.positionWeight_);
                    Property("Rotation weight", track.rotationWeight_);
                    Property("Scale weight", track.scaleWeight_);
                    if (TreeNode("Keyframes", track.keyFrames_.size()))
                    {
                        for (unsigned i = 0; i < track.keyFrames_.size(); ++i)
                        {
                            const AnimationKeyFrame& frame = track.keyFrames_[i];
                            Property(Format("[{}] t={}", i, frame.time_).c_str(),
                                Format("P={} R={} S={}", frame.position_.ToString(), frame.rotation_.ToString(),
                                    frame.scale_.ToString()));
                        }
                        ui::TreePop();
                    }
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Variant tracks", animation->GetVariantTracks().size()))
        {
            unsigned trackIndex = 0;
            for (const auto& [_, track] : animation->GetVariantTracks())
            {
                ui::PushID(trackIndex++);
                if (ui::TreeNodeEx("track", ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%zu keyframes)",
                        track.name_.c_str(), track.keyFrames_.size()))
                {
                    Property("Name hash", track.nameHash_.Value());
                    Property("Type", Variant::GetTypeName(track.type_));
                    Property("Interpolation", static_cast<unsigned>(track.interpolation_));
                    Property("Spline tension", track.splineTension_);
                    Property("Weight", track.weight_);
                    for (unsigned i = 0; i < track.keyFrames_.size(); ++i)
                        Property(Format("[{}] t={}", i, track.keyFrames_[i].time_).c_str(),
                            track.keyFrames_[i].value_.ToString());
                    ui::TreePop();
                }
                ui::PopID();
            }
            ui::TreePop();
        }
        if (TreeNode("Triggers", animation->GetTriggers().size()))
        {
            for (unsigned i = 0; i < animation->GetTriggers().size(); ++i)
            {
                const AnimationTriggerPoint& trigger = animation->GetTriggers()[i];
                Property(Format("Trigger {}", i).c_str(),
                    Format("t={} | {}: {}", trigger.time_, trigger.data_.GetTypeName(), trigger.data_.ToString()));
            }
            ui::TreePop();
        }
    }

    void DrawTexture(Texture* texture)
    {
        Property("Dimensions", Format("{} x {} x {}", texture->GetWidth(), texture->GetHeight(), texture->GetDepth()));
        Property("Levels", texture->GetLevels());
        Property("Format", static_cast<unsigned>(texture->GetFormat()));
        Property("Components", texture->GetComponents());
        Property("Compressed", texture->IsCompressed() ? "true" : "false");
        Property("Linear", texture->GetLinear() ? "true" : "false");
        Property("sRGB", texture->GetSRGB() ? "true" : "false");
        Property("Filter mode", static_cast<unsigned>(texture->GetFilterMode()));
        Property("Address U", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::U)));
        Property("Address V", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::V)));
        Property("Address W", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::W)));
        Property("Anisotropy", texture->GetAnisotropy());
        Property("Minimum LOD", texture->GetMinLod());
        Property("Maximum LOD", texture->GetMaxLod());
        Property("Shadow compare", texture->GetShadowCompare() ? "true" : "false");
        Property("Multisample", texture->GetMultiSample());
        Property("Autoresolve", texture->GetAutoResolve() ? "true" : "false");
        Property("Render target", texture->IsRenderTarget() ? "true" : "false");
        Property("Depth stencil", texture->IsDepthStencil() ? "true" : "false");
        Property("Unordered access", texture->IsUnorderedAccess() ? "true" : "false");
        Property("Backup texture", texture->GetBackupTexture() ? texture->GetBackupTexture()->GetName() : "(none)");
        if (TreeNode("Mip levels", texture->GetLevels()))
        {
            for (unsigned i = 0; i < texture->GetLevels(); ++i)
            {
                const int width = texture->GetLevelWidth(i);
                const int height = texture->GetLevelHeight(i);
                const int depth = texture->GetLevelDepth(i);
                Property(Format("Level {}", i).c_str(),
                    Format("{} x {} x {} | row={} | {}", width, height, depth,
                        FormatBytes(texture->GetRowDataSize(width)),
                        FormatBytes(texture->GetDataSize(width, height, depth))));
            }
            ui::TreePop();
        }
        if (RenderSurface* surface = texture->GetRenderSurface())
        {
            Property("Render surface size", surface->GetSize().ToString());
            Property("Render surface update mode", static_cast<unsigned>(surface->GetUpdateMode()));
        }
    }

    void DrawNode(Node* node)
    {
        if (!node || node == cameraNode_ || node == viewerZoneNode_ || node == viewerLightNode_)
            return;
        ui::PushID(node);
        const ea::string label =
            Format("{} [{}]", node->GetName().empty() ? "(unnamed)" : node->GetName(), node->GetID());
        if (ui::TreeNodeEx(
                "node", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen, "%s", label.c_str()))
        {
            Property("Enabled", node->IsEnabled() ? "true" : "false");
            Property("Temporary", node->IsTemporary() ? "true" : "false");
            Property("Tags", JoinStrings(node->GetTags()));
            Property("Local position", node->GetPosition().ToString());
            Property("Local rotation", node->GetRotation().ToString());
            Property("Local scale", node->GetScale().ToString());
            Property("World position", node->GetWorldPosition().ToString());
            Property("World rotation", node->GetWorldRotation().ToString());
            Property("World scale", node->GetWorldScale().ToString());
            if (TreeNode("Variables", node->GetVars().size()))
            {
                for (const auto& [key, value] : node->GetVars())
                    Property(key.c_str(), Format("{}: {}", value.GetTypeName(), value.ToString()));
                ui::TreePop();
            }
            if (TreeNode("Node attributes", node->GetNumAttributes()))
            {
                DrawSerializable(node);
                ui::TreePop();
            }
            if (TreeNode("Components", node->GetComponents().size()))
            {
                for (const SharedPtr<Component>& component : node->GetComponents())
                {
                    ui::PushID(component.Get());
                    if (ui::TreeNodeEx("component", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]",
                            component->GetTypeName().c_str(), component->GetID()))
                    {
                        Property("Enabled", component->IsEnabled() ? "true" : "false");
                        Property("Temporary", component->IsTemporary() ? "true" : "false");
                        Property("Type hash", component->GetType().Value());
                        DrawSerializable(component);
                        ui::TreePop();
                    }
                    ui::PopID();
                }
                ui::TreePop();
            }
            if (TreeNode("Children", node->GetChildren().size()))
            {
                for (const SharedPtr<Node>& child : node->GetChildren())
                    DrawNode(child);
                ui::TreePop();
            }
            ui::TreePop();
        }
        ui::PopID();
    }

    void DrawSerializable(Serializable* serializable)
    {
        const ea::vector<AttributeInfo>* attributes = serializable->GetAttributes();
        if (!attributes)
            return;
        for (unsigned i = 0; i < attributes->size(); ++i)
        {
            const AttributeInfo& attribute = (*attributes)[i];
            const Variant value = serializable->GetAttribute(i);
            ea::string renderedValue = value.ToString();
            if (!attribute.enumNames_.empty() && value.GetUInt() < attribute.enumNames_.size())
                renderedValue = Format("{} ({})", attribute.enumNames_[value.GetUInt()], value.GetUInt());
            Property(attribute.name_.c_str(),
                Format("{}: {} | default={} | mode={}", Variant::GetTypeName(attribute.type_), renderedValue,
                    attribute.defaultValue_.ToString(), attribute.mode_.AsInteger()));
        }
    }

private:
    SharedPtr<Scene> scene_;
    WeakPtr<Node> contentRoot_;
    SharedPtr<Node> cameraNode_;
    WeakPtr<FreeFlyController> freeFlyController_;
    WeakPtr<Node> viewerZoneNode_;
    WeakPtr<Node> viewerLightNode_;
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

    ea::string startupAsset_;
    ea::string sourceFile_;
    ea::string primaryFBXFile_;
    ea::string loadingFile_;
    ea::string convertedFile_;
    ea::string sourceJson_;
    ea::string sourceText_;
    ea::string converterOutput_;
    ea::string error_;
    ea::string blender_{"blender"};
    ea::string uiIniPath_;

    Vector3 cameraTarget_;
    float yaw_{180.0f};
    float pitch_{12.0f};
    float distance_{5.0f};
    float animationTime_{};
    float animationSpeed_{1.0f};
    float particleAutoFrameTimeRemaining_{};
    unsigned framesUntilFrame_{};
    unsigned fallbackMaterialSlots_{};
    int selectedAnimation_{-1};
    bool animationLooped_{true};
    bool animationsPlaying_{};
    bool particlesEmitting_{true};
    bool particleAutoFramePending_{};
    bool is2DPreview_{};
    bool freeLookEnabled_{};
    bool sceneHovered_{};
    bool loading_{};
    BoundingBox particleAutoFrameBounds_;
};

} // namespace Urho3D

URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::AssetViewer);
