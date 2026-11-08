// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "AssetViewer.h"

#include <SDL.h>

namespace Urho3D
{

using namespace AssetViewerDetail;

void AssetViewer::Setup()
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
    engineParameters_[EP_RESOURCE_PATHS] = "CoreData;Data;EditorData"; // EditorData has the Editor fonts.
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir() + ";..;../..";

    GetCommandLineParser()
        .add_option("asset", startupAssets_, "Asset to open on startup. More files are added as animations")
        ->type_name("path")
        ->expected(0, -1);
    GetCommandLineParser().add_option("--blender", blender_, "Path to Blender executable")->type_name("path");
}

void AssetViewer::Start()
{
    // The layout comes from code each frame. The viewer never reads or writes an ImGui ini file.
    ImGuiIO& io = ui::GetIO();
    io.IniFilename = nullptr;
    // Use the same font and style as the Editor.
    static const ImWchar notoSansRanges[] = {0x20, 0x52f, 0x1ab0, 0x2189, 0x2c60, 0x2e44, 0xa640, 0xab65, 0};
    auto* systemUI = GetSubsystem<SystemUI>();
    systemUI->ApplyStyleDefault(true, 1.0f);
    systemUI->ApplyStyleEditor();
    io.FontDefault = systemUI->AddFont("Fonts/NotoSans-Regular.ttf", notoSansRanges, 16.0f);
    GetSubsystem<Input>()->SetMouseMode(MM_ABSOLUTE);
    GetSubsystem<Input>()->SetMouseVisible(true);
    // SystemUI marks every mouse event over an ImGui window as consumed. The camera needs these events.
    // UpdateCameraInput decides which events belong to the camera.
    GetSubsystem<SystemUI>()->SetPassThroughEvents(true);

    // The video driver decides how a file drop reaches the application.
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    URHO3D_LOGINFO("AssetViewer video driver: {}", videoDriver ? videoDriver : "(none)");

    CreateScene();
    LoadSettings();
    SubscribeToEvent(E_DROPFILE, URHO3D_HANDLER(AssetViewer, HandleDropFile));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(AssetViewer, HandleUpdate));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(AssetViewer, HandleKeyDown));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(AssetViewer, HandlePostRenderUpdate));

    for (unsigned i = 0; i < startupAssets_.size(); ++i)
    {
        const ea::string file = NormalizeDroppedPath(ea::string(startupAssets_[i].c_str()));
        if (i == 0)
            LoadAsset(file);
        else
            pendingStartupFiles_.push_back(file);
    }
}

void AssetViewer::Stop()
{
    SaveSettings();
}

void AssetViewer::LoadSettings()
{
    settingsPath_ = GetSubsystem<Engine>()->GetAppPreferencesDir() + "AssetViewerSettings.json";
    if (!GetSubsystem<FileSystem>()->FileExists(settingsPath_))
    {
        URHO3D_LOGDEBUG("AssetViewer has no settings file '{}', using defaults", settingsPath_);
        return;
    }

    ea::string text;
    {
        File file(context_, settingsPath_, FILE_READ);
        text.resize(file.GetSize());
        if (!text.empty() && file.Read(text.data(), text.size()) != text.size())
            text.clear();
    }
    JSONValue root;
    if (!JSONFile::ParseJSON(text, root, false) || !root.IsObject())
    {
        URHO3D_LOGDEBUG("AssetViewer settings file '{}' is not valid, using defaults", settingsPath_);
        return;
    }

    // A missing or wrong field keeps the default value.
    const auto getBool = [&](const char* key, bool defaultValue)
    { return root.Get(key).GetBool(defaultValue); };
    const JSONValue& width = root.Get("sidePanelWidth");
    if (width.IsNumber() && !IsNaN(width.GetFloat()))
        sidePanelWidth_ = width.GetFloat();
    const JSONValue& lighting = root.Get("lightingMode");
    if (lighting.IsNumber())
        environment_.SetLightingMode(static_cast<LightingMode>(Clamp(lighting.GetInt(), 0, 2)));
    environment_.SetSkybox(getBool("skybox", environment_.GetSkybox()));
    environment_.SetGrid(getBool("grid", environment_.GetGrid()));
    environment_.SetGround(getBool("ground", environment_.GetGround()));
    environment_.SetBounds(getBool("bounds", environment_.GetBounds()));
    environment_.SetWireframe(getBool("wireframe", environment_.GetWireframe()));
    freeLookEnabled_ = getBool("freeLook", freeLookEnabled_);
    URHO3D_LOGDEBUG("AssetViewer loaded settings from '{}'", settingsPath_);
}

void AssetViewer::SaveSettings()
{
    if (settingsPath_.empty())
        return;
    JSONFile file(context_);
    JSONValue& root = file.GetRoot();
    root = JSONValue(JSON_OBJECT);
    root.Set("sidePanelWidth", sidePanelWidth_);
    root.Set("lightingMode", static_cast<int>(environment_.GetLightingMode()));
    root.Set("skybox", environment_.GetSkybox());
    root.Set("grid", environment_.GetGrid());
    root.Set("ground", environment_.GetGround());
    root.Set("bounds", environment_.GetBounds());
    root.Set("wireframe", environment_.GetWireframe());
    root.Set("freeLook", freeLookEnabled_);
    if (file.SaveFile(settingsPath_))
        URHO3D_LOGDEBUG("AssetViewer saved settings to '{}'", settingsPath_);
    else
        URHO3D_LOGWARNING("AssetViewer could not save settings to '{}'", settingsPath_);
}

void AssetViewer::ResetViewSettings()
{
    sidePanelWidth_ = 420.0f;
    environment_.SetLightingMode(LightingMode::Auto);
    environment_.SetSkybox(false);
    environment_.SetGrid(true);
    environment_.SetGround(false);
    environment_.SetBounds(false);
    environment_.SetWireframe(false);
    freeLookEnabled_ = false;
    SetNeutralMaterials(false);
    if (freeFlyController_)
        freeFlyController_->SetEnabled(false);
}

void AssetViewer::CreateScene()
{
    scene_ = MakeShared<Scene>(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    cameraNode_ = scene_->CreateChild("Viewer Camera");
    cameraNode_->SetTemporary(true);
    camera_ = cameraNode_->CreateComponent<Camera>();
    freeFlyController_ = cameraNode_->CreateComponent<FreeFlyController>();
    freeFlyController_->SetEnabled(false);
    camera_->SetNearClip(0.01f);
    camera_->SetFarClip(100000.0f);
    viewport_ = MakeShared<Viewport>(context_, scene_, camera_);
    GetSubsystem<Renderer>()->SetViewport(0, viewport_);
    environment_.Attach(scene_, camera_);
    UpdateCameraTransform();
}

void AssetViewer::ClearAsset()
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
    lastFailedFile_.clear();
    loading_ = false;
    fallbackMaterialSlots_ = 0;
    savedMaterials_.clear();
    lodNodes_.clear();
    lodModels_.clear();
    lodLevelCount_ = 0;
    selectedLod_ = 0;
    animationSources_.clear();
    missingTextures_.clear();
    statusMessage_.clear();
    CreateScene();
}

void AssetViewer::HandleDropFile(StringHash, VariantMap& eventData)
{
    AddDroppedFile(eventData[DropFile::P_FILENAME].GetString());
}

ea::string AssetViewer::DescribeEmptyDrop(const ea::string& rawFile) const
{
    return Format("{}, frame {}. Try SDL_VIDEODRIVER=x11 or SDL_VIDEODRIVER=wayland", DescribeRawDrop(rawFile),
        GetSubsystem<Time>()->GetFrameNumber());
}

void AssetViewer::AddDroppedFile(const ea::string& rawFile)
{
    const ea::string file = NormalizeDroppedPath(rawFile);
    if (file.empty())
    {
        if (dropBurstHasValid_ && GetSubsystem<Time>()->GetFrameNumber() == dropBurstFrame_)
            URHO3D_LOGDEBUG("AssetViewer ignored an empty drop after a valid drop, {}", DescribeEmptyDrop(rawFile));
        else
            URHO3D_LOGINFO("AssetViewer ignored an empty drop, {}", DescribeEmptyDrop(rawFile));
        return;
    }

    // SDL sends one event for each file of a drop, all in one frame. Then it can send an empty end marker.
    const unsigned frame = GetSubsystem<Time>()->GetFrameNumber();
    if (frame != dropBurstFrame_)
    {
        dropBurstFrame_ = frame;
        dropBurstHasValid_ = false;
        queuedFile_.clear();
    }

    if (file != rawFile)
        URHO3D_LOGINFO("AssetViewer normalized drop '{}' to '{}'", rawFile, file);

    FileSystem* fs = GetSubsystem<FileSystem>();
    const bool valid = fs->FileExists(file) && IsSupportedAssetExtension(GetExtension(file));
    // The first valid file of the drop wins. Without a valid file, the first file shows its error.
    // A later drop burst replaces the queue.
    if (!queuedFile_.empty() && (dropBurstHasValid_ || !valid))
    {
        URHO3D_LOGINFO("AssetViewer ignored dropped '{}' because '{}' was dropped first", file, queuedFile_);
        return;
    }
    queuedFile_ = file;
    dropBurstHasValid_ = valid;
    if (loading_)
        URHO3D_LOGINFO("AssetViewer queued '{}'", file);
}

void AssetViewer::StartQueuedFile()
{
    if (loading_)
        return;
    if (queuedFile_.empty())
    {
        // Startup files load in order, one after the previous file is done.
        if (pendingStartupFiles_.empty())
            return;
        queuedFile_ = pendingStartupFiles_.front();
        pendingStartupFiles_.erase(pendingStartupFiles_.begin());
    }
    const ea::string file = queuedFile_;
    queuedFile_.clear();
    LoadAsset(file);
}

void AssetViewer::DismissError()
{
    error_.clear();
    lastFailedFile_.clear();
}

void AssetViewer::HandleKeyDown(StringHash, VariantMap& eventData)
{
    // Ignore the key repeat of a held key. Ignore the keys of a text field.
    if (eventData[KeyDown::P_REPEAT].GetBool() || ui::GetIO().WantTextInput)
        return;
    const int key = eventData[KeyDown::P_KEY].GetInt();
    if (key == KEY_F)
    {
        particleAutoFramePending_ = false;
        FrameContent();
    }
    else if (key == KEY_ESCAPE)
    {
        if (!error_.empty())
            DismissError();
        else
            engine_->Exit();
    }
    else if (!ui::GetIO().WantTextInput)
    {
        if (key == KEY_L)
        {
            const auto mode = static_cast<LightingMode>((static_cast<int>(environment_.GetLightingMode()) + 1) % 3);
            environment_.SetLightingMode(mode);
            URHO3D_LOGINFO("AssetViewer lighting: {}", ViewerEnvironment::GetLightingModeName(mode));
        }
        else if (key == KEY_K)
            environment_.SetSkybox(!environment_.GetSkybox());
        else if (key == KEY_G)
            environment_.SetGrid(!environment_.GetGrid());
        else if (key == KEY_O)
            environment_.SetGround(!environment_.GetGround());
        else if (key == KEY_B)
            environment_.SetBounds(!environment_.GetBounds());
        else if (key == KEY_Z)
            environment_.SetWireframe(!environment_.GetWireframe());
        else if (key == KEY_M)
            SetNeutralMaterials(!neutralMaterials_);
        else if (key == KEY_SPACE && !animationControllers_.empty())
            SetAnimationsPlaying(!animationsPlaying_);
    }
}

void AssetViewer::HandleUpdate(StringHash, VariantMap& eventData)
{
    StartQueuedFile();
    if (framesUntilFrame_ > 0 && --framesUntilFrame_ == 0)
        FrameContent();
    UpdateParticleAutoFrame(eventData[Update::P_TIMESTEP].GetFloat());
    // The engine can close the window in the middle of a frame (Exit). Then SystemUI does not start an ImGui frame.
    if (ui::GetCurrentContext()->WithinFrameScope)
        RenderUI();
    UpdateCameraInput();
    UpdateLodOverride();
}

void AssetViewer::HandlePostRenderUpdate(StringHash, VariantMap&)
{
    environment_.UpdateRigDirection();

    DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();
    if (!debug)
        return;
    environment_.DrawDebug(debug);
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

bool AssetViewer::Fail(const ea::string& message)
{
    error_ = message;
    lastFailedFile_ = requestedFile_;
    URHO3D_LOGERROR("AssetViewer: {}", message);
    return false;
}

} // namespace Urho3D

URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::AssetViewer);
