//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Audio/Audio.h"
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Thread.h"
#include "../Core/WorkQueue.h"
#ifdef URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/Console.h"
#include "../SystemUI/DebugHud.h"
#include "../SystemUI/StandardSerializableHooks.h"
#endif
#include "../Engine/Engine.h"
#include "../Engine/EngineDefs.h"
#include "../Engine/StateManager.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../RenderAPI/PipelineState.h"
#include "../RenderAPI/RenderAPIUtils.h"
#include "../RenderAPI/RenderDevice.h"
#include "../Resource/JSONArchive.h"
#include "../Graphics/Renderer.h"
#include "../Input/Input.h"
#include "../Input/DirectionalPadAdapter.h"
#include "../IO/FileSystem.h"
#include "../IO/VirtualFileSystem.h"
#include "../IO/MountedDirectory.h"
#include "../IO/Log.h"
#include "../IO/PackageFile.h"
#ifdef URHO3D_GLOW
#include "../Glow/StaticModelForLightmap.h"
#endif
#ifdef URHO3D_IK
#include "../IK/IK.h"
#endif
#ifdef URHO3D_NAVIGATION
#include "../Navigation/NavigationMesh.h"
#endif
#ifdef URHO3D_NETWORK
#include "../Network/Network.h"
#endif
#ifdef URHO3D_PHYSICS
#include "../Physics/PhysicsWorld.h"
#include "../Physics/RaycastVehicle.h"
#endif
#ifdef URHO3D_PHYSICS2D
#include "../Physics2D/Physics2D.h"
#endif
#include "../Resource/ResourceCache.h"
#include "../Resource/Localization.h"
#include "../RenderPipeline/RenderPass.h"
#include "../RenderPipeline/RenderPath.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../RenderPipeline/Passes/AmbientOcclusionPass.h"
#include "../RenderPipeline/Passes/AutoExposurePass.h"
#include "../RenderPipeline/Passes/BloomPass.h"
#include "../RenderPipeline/Passes/FullScreenShaderPass.h"
#include "../RenderPipeline/Passes/OutlineRenderPass.h"
#include "../RenderPipeline/Passes/ToneMappingPass.h"
#include "../Resource/JSONArchive.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../UI/UI.h"
#ifdef URHO3D_RMLUI
#include "../RmlUI/RmlUI.h"
#endif
#ifdef URHO3D_URHO2D
#include "../Urho2D/Urho2D.h"
#endif
#include "../Engine/EngineEvents.h"
#ifdef URHO3D_PARTICLE_GRAPH
#include "../Particles/ParticleGraphSystem.h"
#endif
#include "../Plugins/PluginManager.h"
#include "../Utility/AnimationVelocityExtractor.h"
#include "../Utility/AssetPipeline.h"
#include "../Utility/AssetTransformer.h"
#include "../Utility/SceneViewerApplication.h"
#ifdef URHO3D_ACTIONS
#include "../Actions/ActionManager.h"
#endif
#ifdef URHO3D_XR
    #include "Urho3D/XR/VRRig.h"
    #include "Urho3D/XR/OpenXR.h"
#endif

#ifdef URHO3D_PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#endif

#include "StateManager.h"
#include "../Core/CommandLine.h"

#include "../DebugNew.h"

#if defined(_MSC_VER) && defined(_DEBUG)
// From dbgint.h
#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
    struct _CrtMemBlockHeader* pBlockHeaderNext;
    struct _CrtMemBlockHeader* pBlockHeaderPrev;
    char* szFileName;
    int nLine;
    size_t nDataSize;
    int nBlockUse;
    long lRequest;
    unsigned char gap[nNoMansLandSize];
} _CrtMemBlockHeader;
#endif

#ifdef URHO3D_PLATFORM_WEB
static void OnCanvasResize(int width, int height, bool isFullScreen, float dpiScale)
{
    if (auto context = Urho3D::Context::GetInstance())
    {
        if (auto engine = context->GetSubsystem<Urho3D::Engine>())
            engine->OnCanvasResize(width, height, isFullScreen, dpiScale);
    }
}

EMSCRIPTEN_BINDINGS(Module)
{
    emscripten::function("JSCanvasSize", &OnCanvasResize);
}
#endif

namespace Urho3D
{

namespace
{

struct ResourceRootEntry
{
    ea::string fullName_;
    ea::string shortName_;
};

SharedPtr<File> OpenResourceRootFile(FileSystem* fileSystem, const ea::string& fileName)
{
    if (fileName.empty())
        return nullptr;

    ea::string programDir = AddTrailingSlash(fileSystem->GetProgramDir());
    while (!programDir.empty())
    {
        const ea::string fullFileName = programDir + fileName;
        if (fileSystem->FileExists(fullFileName))
            return MakeShared<File>(fileSystem->GetContext(), fullFileName, FILE_READ);
        programDir = GetParentPath(programDir);
    }

    return nullptr;
}

ea::vector<ResourceRootEntry> ReadResourceRootFile(File* file)
{
    if (!file)
        return {};

    const ea::string path = GetPath(file->GetAbsoluteName());
    const ea::string text = file->ReadText();
    const StringVector lines = text.replaced("\r\n", "\n").split('\n');

    unsigned lineIndex = 0;
    ea::vector<ResourceRootEntry> result;
    for (const ea::string& sourceLine : lines)
    {
        ++lineIndex;
        const ea::string line = sourceLine.trimmed();
        if (line.empty() || line.starts_with("#") || line.starts_with(";"))
            continue;

        const auto parts = line.split('=');
        if (parts.size() != 2)
        {
            URHO3D_LOGERROR("Invalid line #{} in {}", lineIndex, file->GetAbsoluteName());
            continue;
        }

        const ea::string shortName = parts[0].trimmed();
        const ea::string fullDirectoryName = AddTrailingSlash(path + parts[1].trimmed());
        result.push_back(ResourceRootEntry{fullDirectoryName, shortName});
    }

    return result;
}

} // namespace

extern const char* logLevelNames[];

Engine::Engine(Context* context) :
    Object(context),
    timeStep_(0.0f),
    timeStepSmoothing_(2),
    minFps_(10),
#if defined(IOS) || defined(TVOS) || defined(__ANDROID__) || defined(__arm__) || defined(__aarch64__)
    maxFps_(60),
    maxInactiveFps_(10),
    pauseMinimized_(true),
#else
    maxFps_(200),
    maxInactiveFps_(60),
    pauseMinimized_(false),
#endif
    timeOut_(0),
    autoExit_(true),
    initialized_(false),
    exiting_(false),
    headless_(false),
    audioPaused_(false)
{
    PopulateDefaultParameters();

    // Register self as a subsystem
    context_->RegisterSubsystem(this);

    // Create subsystems which do not depend on engine initialization or startup parameters
    context_->RegisterSubsystem(new Time(context_));
    context_->RegisterSubsystem(new WorkQueue(context_));
    context_->RegisterSubsystem(new FileSystem(context_));
    context_->RegisterSubsystem(new VirtualFileSystem(context_));
#ifdef URHO3D_LOGGING
    context_->RegisterSubsystem(new Log(context_));
#endif
    context_->RegisterSubsystem(new ResourceCache(context_));
    context_->RegisterSubsystem(new Localization(context_));
#ifdef URHO3D_NETWORK
    context_->RegisterSubsystem(new Network(context_));
#endif
    // Required in headless mode as well.
    RegisterGraphicsLibrary(context_);
    // Register object factories for libraries which are not automatically registered along with subsystem creation
    RegisterSceneLibrary(context_);
    // Register UI library object factories before creation of subsystem. This is not done inside subsystem because
    // there may exist multiple instances of UI.
    RegisterUILibrary(context_);

#ifdef URHO3D_GLOW
    // Light baker needs only one class so far, so register it directly.
    // Extract this code into function if you are adding more.
    StaticModelForLightmap::RegisterObject(context_);
#endif

    // Register render pipeline.
    // Extract this code into function if you are adding more.
    RenderPipeline::RegisterObject(context_);
    RenderPath::RegisterObject(context_);
    RenderPass::RegisterObject(context_);
    AmbientOcclusionPass::RegisterObject(context_);
    AutoExposurePass::RegisterObject(context_);
    BloomPass::RegisterObject(context_);
    FullScreenShaderPass::RegisterObject(context_);
    OutlineRenderPass::RegisterObject(context_);
    ToneMappingPass::RegisterObject(context_);

#ifdef URHO3D_IK
    RegisterIKLibrary(context_);
#endif

#ifdef URHO3D_PHYSICS
    RegisterPhysicsLibrary(context_);
#endif

#ifdef URHO3D_PHYSICS2D
    RegisterPhysics2DLibrary(context_);
#endif

#ifdef URHO3D_NAVIGATION
    RegisterNavigationLibrary(context_);
#endif

#ifdef URHO3D_ACTIONS
    context_->RegisterSubsystem<ActionManager>();
#endif

#ifdef URHO3D_XR
    RegisterVRLibrary(context_);
#endif

    SceneViewerApplication::RegisterObject();
    context_->AddFactoryReflection<AssetPipeline>();
    context_->AddFactoryReflection<AssetTransformer>();
    AnimationVelocityExtractor::RegisterObject(context_);

    SubscribeToEvent(E_EXITREQUESTED, URHO3D_HANDLER(Engine, HandleExitRequested));
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Engine, HandleEndFrame));
}

Engine::~Engine() = default;

bool Engine::Initialize(const StringVariantMap& applicationParameters, const StringVariantMap& commandLineParameters)
{
    if (initialized_)
        return true;

    URHO3D_PROFILE("InitEngine");

    engineParameters_->DefineVariables(applicationParameters);
    engineParameters_->UpdatePriorityVariables(commandLineParameters);

    auto* fileSystem = GetSubsystem<FileSystem>();

    appPreferencesDir_ = GetParameter(EP_APPLICATION_PREFERENCES_DIR).GetString();
    if (appPreferencesDir_.empty())
    {
        const ea::string& organizationName = GetParameter(EP_ORGANIZATION_NAME).GetString();
        const ea::string& applicationName = GetParameter(EP_APPLICATION_NAME).GetString();
        appPreferencesDir_ = fileSystem->GetAppPreferencesDir(organizationName, applicationName);
    }

    // Start logging
    auto* log = GetSubsystem<Log>();
    if (log)
    {
        log->SetLevel(static_cast<LogLevel>(GetParameter(EP_LOG_LEVEL).GetInt()));
        log->SetQuiet(GetParameter(EP_LOG_QUIET).GetBool());
        const ea::string logFileName = GetLogFileName(GetParameter(EP_LOG_NAME).GetString());
        if (!logFileName.empty())
            log->Open(logFileName);
    }

    // Initialize app preferences directory
    if (!appPreferencesDir_.empty())
        fileSystem->CreateDir(appPreferencesDir_);

    // Initialize virtual file system
    const bool prefixPathsInCommandLine = commandLineParameters.contains(EP_RESOURCE_PREFIX_PATHS);
    const bool enableResourceRootFile = !prefixPathsInCommandLine;
    InitializeVirtualFileSystem(enableResourceRootFile);

    // Read and merge configs
    LoadConfigFiles();

    // Override config values with command line parameters
    engineParameters_->DefineVariables(commandLineParameters);

    // Set headless mode
    headless_ = GetParameter(EP_HEADLESS).GetBool();

    // Register the rest of the subsystems
    context_->RegisterSubsystem(new Input(context_));
    RegisterInputLibrary(context_);

    context_->RegisterSubsystem(new UI(context_));

#ifdef URHO3D_RMLUI
    RegisterRmlUILibrary(context_);
    context_->RegisterSubsystem(new RmlUI(context_));
#endif

    context_->RegisterSubsystem(new Audio(context_));
    if (!headless_)
    {
        context_->RegisterSubsystem(new Graphics(context_));
        context_->RegisterSubsystem(new Renderer(context_));
    }
    context_->RegisterSubsystem(new StateManager(context_));
#ifdef URHO3D_PARTICLE_GRAPH
    context_->RegisterSubsystem(new ParticleGraphSystem(context_));
#endif

#ifdef URHO3D_URHO2D
    // 2D graphics library is dependent on 3D graphics library
    RegisterUrho2DLibrary(context_);
#endif

    context_->RegisterSubsystem(new PluginManager(context_));

    // Set maximally accurate low res timer
    GetSubsystem<Time>()->SetTimerPeriod(1);

    // Configure max FPS
    if (GetParameter(EP_FRAME_LIMITER) == false)
        SetMaxFps(0);

    // Set amount of worker threads according to the available physical CPU cores. Using also hyperthreaded cores results in
    // unpredictable extra synchronization overhead. Also reserve one core for the main thread
#ifdef URHO3D_THREADING
    const unsigned numThreads = GetParameter(EP_WORKER_THREADS).GetBool() ? GetNumPhysicalCPUs() - 1 : 0;
#else
    const unsigned numThreads = 0;
#endif
    GetSubsystem<WorkQueue>()->Initialize(numThreads);

    auto* cache = GetSubsystem<ResourceCache>();

    // Initialize graphics & audio output
    if (!headless_)
    {
        auto* graphics = GetSubsystem<Graphics>();
        auto* renderer = GetSubsystem<Renderer>();

        const RenderBackend backend = SelectRenderBackend(GetParameter(EP_RENDER_BACKEND).GetOptional<RenderBackend>());
        const bool needXR = GetParameter(EP_XR).GetBool();

        if (needXR)
        {
#ifdef URHO3D_XR
            auto* xr = context_->RegisterSubsystem<OpenXR, VirtualReality>();
            if (!xr->InitializeSystem(backend))
            {
                URHO3D_LOGERROR("Failed to initialize OpenXR subsystem");
                return false;
            }
#else
            URHO3D_LOGERROR("OpenXR subsystem is not available in this build configuration");
            return false;
#endif
        }

        GraphicsSettings graphicsSettings;
        graphicsSettings.backend_ = backend;
        graphicsSettings.externalWindowHandle_ = GetParameter(EP_EXTERNAL_WINDOW).GetVoidPtr();
        graphicsSettings.gpuDebug_ = GetParameter(EP_GPU_DEBUG).GetBool();
        graphicsSettings.adapterId_ = GetParameter(EP_RENDER_ADAPTER_ID).GetOptional<unsigned>();
        graphicsSettings.shaderTranslationPolicy_ = SelectShaderTranslationPolicy(
            graphicsSettings.backend_, GetParameter(EP_SHADER_POLICY).GetOptional<ShaderTranslationPolicy>());

        const auto vulkanTweaks = FromJSONString<RenderDeviceSettingsVulkan>(GetParameter(EP_TWEAK_VULKAN).GetString());
        graphicsSettings.vulkan_ = vulkanTweaks.value_or(RenderDeviceSettingsVulkan{});
        const auto d3d12Tweaks = FromJSONString<RenderDeviceSettingsD3D12>(GetParameter(EP_TWEAK_D3D12).GetString());
        graphicsSettings.d3d12_ = d3d12Tweaks.value_or(RenderDeviceSettingsD3D12{});

        graphicsSettings.shaderCacheDir_ = FileIdentifier::FromUri(GetParameter(EP_SHADER_CACHE_DIR).GetString());
        graphicsSettings.logShaderSources_ = GetParameter(EP_SHADER_LOG_SOURCES).GetBool();
        graphicsSettings.validateShaders_ = GetParameter(EP_VALIDATE_SHADERS).GetBool();
        graphicsSettings.discardShaderCache_ = GetParameter(EP_DISCARD_SHADER_CACHE).GetBool();
        graphicsSettings.cacheShaders_ = GetParameter(EP_SAVE_SHADER_CACHE).GetBool();

        WindowSettings windowSettings;
        const int width = GetParameter(EP_WINDOW_WIDTH).GetInt();
        const int height = GetParameter(EP_WINDOW_HEIGHT).GetInt();
        if (width && height)
            windowSettings.size_ = {width, height};
        if (GetPlatform() == PlatformId::Web)
            windowSettings.mode_ = WindowMode::Windowed;
        else if (GetParameter(EP_FULL_SCREEN).GetBool())
            windowSettings.mode_ = WindowMode::Fullscreen;
        else if (GetParameter(EP_BORDERLESS).GetBool())
            windowSettings.mode_ = WindowMode::Borderless;
        windowSettings.resizable_ = GetParameter(EP_WINDOW_RESIZABLE).GetBool();
        windowSettings.vSync_ = GetParameter(EP_VSYNC).GetBool();
        windowSettings.multiSample_ = GetParameter(EP_MULTI_SAMPLE).GetInt();
        windowSettings.monitor_ = GetParameter(EP_MONITOR).GetInt();
        windowSettings.refreshRate_ = GetParameter(EP_REFRESH_RATE).GetInt();
        windowSettings.orientations_ = GetParameter(EP_ORIENTATIONS).GetString().split(' ');

#ifdef URHO3D_XR
        auto virtualReality = GetSubsystem<VirtualReality>();
        if (needXR && virtualReality && virtualReality->IsInstanceOf<OpenXR>())
        {
            auto* xr = static_cast<OpenXR*>(virtualReality);
            const OpenXRTweaks& tweaks = xr->GetTweaks();

            // Arbitrary value high enough that XR swap chain is never throttled
            maxInactiveFps_ = maxFps_ = 500;

            graphicsSettings.vulkan_.instanceExtensions_ = tweaks.vulkanInstanceExtensions_;
            graphicsSettings.vulkan_.deviceExtensions_ = tweaks.vulkanDeviceExtensions_;
            graphicsSettings.adapterId_ = tweaks.adapterId_;

            windowSettings.vSync_ = false;
            if (tweaks.orientation_)
                windowSettings.orientations_ = {*tweaks.orientation_};
        }
#endif

        graphics->Configure(graphicsSettings);

        graphics->SetWindowTitle(GetParameter(EP_WINDOW_TITLE).GetString());
        graphics->SetWindowIcon(cache->GetResource<Image>(GetParameter(EP_WINDOW_ICON).GetString()));

        SubscribeToEvent(E_SCREENMODE, [this](VariantMap& eventData)
        {
            using namespace ScreenMode;

            const bool isBorderless = eventData[P_BORDERLESS].GetBool();

            // TODO: Uncomment when we have consistent handling of pixels vs points
            // TODO: Also see PopulateDefaultParameters()
            //SetParameter(EP_WINDOW_WIDTH, isBorderless ? 0 : eventData[P_WIDTH].GetInt());
            //SetParameter(EP_WINDOW_HEIGHT, isBorderless ? 0 : eventData[P_HEIGHT].GetInt());
            SetParameter(EP_FULL_SCREEN, eventData[P_FULLSCREEN].GetBool());
            SetParameter(EP_BORDERLESS, isBorderless);
            SetParameter(EP_MONITOR, eventData[P_MONITOR].GetInt());
        });

        if (!graphics->SetDefaultWindowModes(windowSettings))
            return false;

        if (HasParameter(EP_WINDOW_POSITION_X) && HasParameter(EP_WINDOW_POSITION_Y))
            graphics->SetWindowPosition(GetParameter(EP_WINDOW_POSITION_X).GetInt(),
                GetParameter(EP_WINDOW_POSITION_Y).GetInt());

        if (GetParameter(EP_WINDOW_MAXIMIZE).GetBool())
            graphics->Maximize();

        graphics->InitializePipelineStateCache(FileIdentifier::FromUri(GetParameter(EP_PSO_CACHE).GetString()));

        renderer->SetTextureQuality((MaterialQuality)GetParameter(EP_TEXTURE_QUALITY).GetInt());
        renderer->SetTextureFilterMode((TextureFilterMode)GetParameter(EP_TEXTURE_FILTER_MODE).GetInt());
        renderer->SetTextureAnisotropy(GetParameter(EP_TEXTURE_ANISOTROPY).GetInt());

        if (GetParameter(EP_SOUND).GetBool())
        {
            GetSubsystem<Audio>()->SetMode(
                GetParameter(EP_SOUND_BUFFER).GetInt(),
                GetParameter(EP_SOUND_MIX_RATE).GetInt(),
                (SpeakerMode)GetParameter(EP_SOUND_MODE).GetInt(),
                GetParameter(EP_SOUND_INTERPOLATION).GetBool()
            );
        }

#ifdef URHO3D_RMLUI
        const auto rmlUi = GetSubsystem<RmlUI>();

        const bool loadFonts = GetParameter(EP_LOAD_FONTS).GetBool();
        if (loadFonts)
            rmlUi->ReloadFonts();

        const auto renderDevice = GetSubsystem<RenderDevice>();
        const float dpiScale = renderDevice->GetDpiScale();
        rmlUi->SetScale(dpiScale);
#endif
    }

    // Init FPU state of main thread
    InitFPU();

    // Initialize input
    if (HasParameter(EP_TOUCH_EMULATION))
        GetSubsystem<Input>()->SetTouchEmulation(GetParameter(EP_TOUCH_EMULATION).GetBool());

    // Initialize network
#ifdef URHO3D_NETWORK
    if (HasParameter(EP_PACKAGE_CACHE_DIR))
        GetSubsystem<Network>()->SetPackageCacheDir(GetParameter(EP_PACKAGE_CACHE_DIR).GetString());
#endif

    if (HasParameter(EP_TIME_OUT))
        timeOut_ = GetParameter(EP_TIME_OUT).GetInt() * 1000000LL;

    if (!headless_)
    {
#ifdef URHO3D_SYSTEMUI
        context_->RegisterSubsystem(new SystemUI(context_,
            GetParameter(EP_SYSTEMUI_FLAGS).GetUInt()));
        RegisterStandardSerializableHooks(context_);
#endif
    }

    frameTimer_.Reset();

    URHO3D_LOGINFO("Initialized engine");
    initialized_ = true;
    SendEvent(E_ENGINEINITIALIZED);
    return true;
}

void Engine::InitializeVirtualFileSystem(bool enableResourceRootFile)
{
    auto fileSystem = GetSubsystem<FileSystem>();
    auto vfs = GetSubsystem<VirtualFileSystem>();

    const ea::string& resourceRootFileName = GetParameter(EP_RESOURCE_ROOT_FILE).GetString();
    const StringVector prefixPaths = GetParameter(EP_RESOURCE_PREFIX_PATHS).GetString().split(';');
    const StringVector paths = GetParameter(EP_RESOURCE_PATHS).GetString().split(';');
    const StringVector packages = GetParameter(EP_RESOURCE_PACKAGES).GetString().split(';');
    const StringVector autoLoadPaths = GetParameter(EP_AUTOLOAD_PATHS).GetString().split(';');

    const auto resourceRootFile = OpenResourceRootFile(fileSystem, resourceRootFileName);
    const auto resourceRootEntries = ReadResourceRootFile(resourceRootFile);

    if (resourceRootFileName.empty())
        URHO3D_LOGINFO("Resource root file lookup is disabled by the application");
    else if (resourceRootEntries.empty())
        URHO3D_LOGINFO("Resource root file is not found or invalid");
    else if (!enableResourceRootFile)
        URHO3D_LOGINFO("Resource root file is found but ignored due to explicitly specified prefix paths");
    else
        URHO3D_LOGINFO("Resource root file is found and used: {}", resourceRootFile->GetAbsoluteName());

    // Mount common points
    vfs->UnmountAll();
    vfs->MountAliasRoot();
    vfs->MountRoot();

    if (!resourceRootEntries.empty() && enableResourceRootFile)
    {
        for (const ResourceRootEntry& entry : resourceRootEntries)
        {
            if (!fileSystem->DirExists(entry.fullName_))
            {
                URHO3D_LOGERROR("Resource directory is not found: {}", entry.fullName_);
                continue;
            }

            MountPoint* mountPoint = vfs->MountDir(entry.fullName_);
            if (mountPoint)
                vfs->MountAlias(Format("res:{}", entry.shortName_), mountPoint);
        }
    }
    else
    {
        const ea::string& programDir = fileSystem->GetProgramDir();
        StringVector absolutePrefixPaths = GetAbsolutePaths(prefixPaths, programDir, true);
        if (!absolutePrefixPaths.contains(programDir))
            absolutePrefixPaths.push_back(programDir);

        vfs->MountExistingDirectoriesOrPackages(absolutePrefixPaths, paths);
        vfs->MountExistingPackages(absolutePrefixPaths, packages);

        // Add auto load folders. Prioritize these (if exist) before the default folders
        for (const ea::string& autoLoadPath : autoLoadPaths)
        {
            if (IsAbsolutePath(autoLoadPath))
                vfs->AutomountDir(autoLoadPath);
            else
            {
                for (const ea::string& prefixPath : absolutePrefixPaths)
                    vfs->AutomountDir(AddTrailingSlash(prefixPath) + autoLoadPath);
            }
        }
    }

#ifndef __EMSCRIPTEN__
    vfs->MountDir("conf", GetAppPreferencesDir());
#else
    vfs->MountDir("conf", "/IndexedDB/");
#endif
}

void Engine::RunFrame()
{
    URHO3D_PROFILE("RunFrame");
    {
        assert(initialized_);

        // If not headless, and the graphics subsystem no longer has a window open, assume we should exit
        if (!headless_ && !GetSubsystem<Graphics>()->IsInitialized())
            exiting_ = true;

        if (exiting_)
            return;
    }

    // Note: there is a minimal performance cost to looking up subsystems (uses a hashmap); if they would be looked up several
    // times per frame it would be better to cache the pointers
    auto* time = GetSubsystem<Time>();
    auto* input = GetSubsystem<Input>();
    auto* audio = GetSubsystem<Audio>();

    {
        URHO3D_PROFILE("DoFrame");
        time->BeginFrame(timeStep_);

        // If pause when minimized -mode is in use, stop updates and audio as necessary
        if (pauseMinimized_ && input->IsMinimized())
        {
            if (audio->IsPlaying())
            {
                audio->Stop();
                audioPaused_ = true;
            }
        }
        else
        {
            // Only unpause when it was paused by the engine
            if (audioPaused_)
            {
                audio->Play();
                audioPaused_ = false;
            }

            Update();
        }

        Render();
    }
    ApplyFrameLimit();

    time->EndFrame();

    // Mark a frame for profiling
    URHO3D_PROFILE_FRAME();
}

void Engine::OnCanvasResize(int width, int height, bool isFullScreen, float dpiScale)
{
    URHO3D_LOGINFO(
        "Web canvas resized to {}x{}{} with DPI scale={}", width, height, isFullScreen ? " FullScreen" : " ", dpiScale);

    auto input = GetSubsystem<Input>();
    auto ui = GetSubsystem<UI>();
    auto graphics = GetSubsystem<Graphics>();
#ifdef URHO3D_RMLUI
    auto rmlUi = GetSubsystem<RmlUI>();
#endif

    bool uiCursorVisible = false;
    bool systemCursorVisible = false;
    MouseMode mouseMode{};

    // Detect current system pointer state
    if (input)
    {
        systemCursorVisible = input->IsMouseVisible();
        mouseMode = input->GetMouseMode();
    }

    if (ui)
    {
        ui->SetScale(dpiScale);

        // Detect current UI pointer state
        if (Cursor* cursor = ui->GetCursor())
            uiCursorVisible = cursor->IsVisible();
    }

#ifdef URHO3D_RMLUI
    if (rmlUi)
        rmlUi->SetScale(dpiScale);
#endif

    // Apply new resolution
    graphics->SetMode(width, height);

    // Reset the pointer state as it was before resolution change
    if (input)
    {
        if (uiCursorVisible)
            input->SetMouseVisible(false);
        else
            input->SetMouseVisible(systemCursorVisible);

        input->SetMouseMode(mouseMode);
    }

    if (ui)
    {
        if (Cursor* cursor = ui->GetCursor())
        {
            cursor->SetVisible(uiCursorVisible);

            const IntVector2 mousePos = input->GetMousePosition();
            cursor->SetPosition(ui->ConvertSystemToUI(mousePos));
        }
    }
}

Console* Engine::CreateConsole()
{
    if (headless_ || !initialized_)
        return nullptr;

#ifdef URHO3D_SYSTEMUI
    // Return existing console if possible
    auto* console = GetSubsystem<Console>();
    if (!console)
    {
        console = new Console(context_);
        context_->RegisterSubsystem(console);
    }

    return console;
#else
    return nullptr;
#endif
}

DebugHud* Engine::CreateDebugHud()
{
    if (headless_ || !initialized_)
        return nullptr;

#ifdef URHO3D_SYSTEMUI
    // Return existing debug HUD if possible
    auto* debugHud = GetSubsystem<DebugHud>();
    if (!debugHud)
    {
        debugHud = new DebugHud(context_);
        context_->RegisterSubsystem(debugHud);
    }

    return debugHud;
#else
    return nullptr;
#endif
}

void Engine::SetTimeStepSmoothing(int frames)
{
    timeStepSmoothing_ = (unsigned)Clamp(frames, 1, 20);
}

void Engine::SetMinFps(int fps)
{
    minFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetMaxFps(int fps)
{
    maxFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetMaxInactiveFps(int fps)
{
    maxInactiveFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetPauseMinimized(bool enable)
{
    pauseMinimized_ = enable;
}

void Engine::SetAutoExit(bool enable)
{
    // On mobile platforms exit is mandatory if requested by the platform itself and should not be attempted to be disabled
#if defined(__ANDROID__) || defined(IOS) || defined(TVOS)
    enable = true;
#endif
    autoExit_ = enable;
}

void Engine::SetNextTimeStep(float seconds)
{
    timeStep_ = Max(seconds, 0.0f);
}

void Engine::SetParameter(const ea::string& name, const Variant& value)
{
    engineParameters_->SetVariable(name, value);
}

bool Engine::HasParameter(const ea::string& name) const
{
    return engineParameters_->HasVariable(name);
}

void Engine::Exit()
{
#if defined(IOS) || defined(TVOS)
    // On iOS/tvOS it's not legal for the application to exit on its own, instead it will be minimized with the home key
#else
    DoExit();
#endif
}

void Engine::DumpProfiler()
{
#ifdef URHO3D_LOGGING
    if (!Thread::IsMainThread())
        return;

    // TODO: Put something here or remove API
#endif
}

void Engine::DumpResources(bool dumpFileName)
{
#ifdef URHO3D_LOGGING
    if (!Thread::IsMainThread())
        return;

    auto* cache = GetSubsystem<ResourceCache>();
    const ea::unordered_map<StringHash, ResourceGroup>& resourceGroups = cache->GetAllResources();
    if (dumpFileName)
    {
        URHO3D_LOGINFO("Used resources:");
        for (auto i = resourceGroups.begin(); i !=
            resourceGroups.end(); ++i)
        {
            const ea::unordered_map<StringHash, SharedPtr<Resource> >& resources = i->second.resources_;
            if (dumpFileName)
            {
                for (auto j = resources.begin(); j !=
                    resources.end(); ++j)
                    URHO3D_LOGINFO(j->second->GetName());
            }
        }
    }
    else
        URHO3D_LOGINFO(cache->PrintMemoryUsage());
#endif
}

void Engine::DumpMemory()
{
#ifdef URHO3D_LOGGING
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState state;
    _CrtMemCheckpoint(&state);
    _CrtMemBlockHeader* block = state.pBlockHeader;
    unsigned total = 0;
    unsigned blocks = 0;

    for (;;)
    {
        if (block && block->pBlockHeaderNext)
            block = block->pBlockHeaderNext;
        else
            break;
    }

    while (block)
    {
        if (block->nBlockUse > 0)
        {
            if (block->szFileName)
                URHO3D_LOGINFO("Block {}: {} bytes, file {} line {}", block->lRequest, block->nDataSize, block->szFileName, block->nLine);
            else
                URHO3D_LOGINFO("Block {}: {} bytes", block->lRequest, block->nDataSize);

            total += block->nDataSize;
            ++blocks;
        }
        block = block->pBlockHeaderPrev;
    }

    URHO3D_LOGINFO("Total allocated memory {} bytes in {} blocks", total, blocks);
#else
    URHO3D_LOGINFO("DumpMemory() supported on MSVC debug mode only");
#endif
#endif
}

void Engine::Update()
{
    URHO3D_PROFILE("Update");

    using namespace Update;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_TIMESTEP] = timeStep_;

    // Pre-update event that indicates
    SendEvent(E_INPUTREADY, eventData);

    // Logic update event
    SendEvent(E_UPDATE, eventData);

    // Logic post-update event
    SendEvent(E_POSTUPDATE, eventData);

    // Rendering update event
    SendEvent(E_RENDERUPDATE, eventData);

    // Post-render update event
    SendEvent(E_POSTRENDERUPDATE, eventData);
}

void Engine::Render()
{
    if (headless_)
        return;

    URHO3D_PROFILE("Render");

    // If device is lost, BeginFrame will fail and we skip rendering
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics->BeginFrame())
        return;

    GetSubsystem<Renderer>()->Render();

    // Render UI after scene is rendered, but only do so if user has not rendered it manually
    // anywhere (for example using renderpath or to a texture).
    graphics->ResetRenderTargets();
    if (UI* ui = GetSubsystem<UI>())
    {
        if (!ui->IsRendered() && ui->GetRenderTarget() == nullptr)
            ui->Render();
    }

    graphics->EndFrame();
}

void Engine::ApplyFrameLimit()
{
    if (!initialized_)
        return;

    unsigned maxFps = maxFps_;
    auto* input = GetSubsystem<Input>();
    if (input && !input->HasFocus())
        maxFps = Min(maxInactiveFps_, maxFps);

    long long elapsed = 0;

#ifndef __EMSCRIPTEN__
    // Perform waiting loop if maximum FPS set
#if !defined(IOS) && !defined(TVOS)
    if (maxFps)
#else
    // If on iOS/tvOS and target framerate is 60 or above, just let the animation callback handle frame timing
    // instead of waiting ourselves
    if (maxFps && maxFps < 60)
#endif
    {
        URHO3D_PROFILE("ApplyFrameLimit");

        long long targetMax = 1000000LL / maxFps;

        for (;;)
        {
            elapsed = frameTimer_.GetUSec(false);
            if (elapsed >= targetMax)
                break;

            // Sleep if 1 ms or more off the frame limiting goal
            if (targetMax - elapsed >= 1000LL)
            {
                auto sleepTime = (unsigned)((targetMax - elapsed) / 1000LL);
                Time::Sleep(sleepTime);
            }
        }
    }
#endif

    elapsed = frameTimer_.GetUSec(true);

    if (timeOut_ > 0)
    {
        timeOut_ -= elapsed;
        if (timeOut_ <= 0)
            Exit();
    }

    // If FPS lower than minimum, clamp elapsed time
    if (minFps_)
    {
        long long targetMin = 1000000LL / minFps_;
        if (elapsed > targetMin)
            elapsed = targetMin;
    }

    // Perform timestep smoothing
    timeStep_ = 0.0f;
    lastTimeSteps_.push_back(elapsed / 1000000.0f);
    if (lastTimeSteps_.size() > timeStepSmoothing_)
    {
        // If the smoothing configuration was changed, ensure correct amount of samples
        lastTimeSteps_.erase_at(0, timeStepSmoothing_);
        for (unsigned i = 0; i < lastTimeSteps_.size(); ++i)
            timeStep_ += lastTimeSteps_[i];
        timeStep_ /= lastTimeSteps_.size();
    }
    else
        timeStep_ = lastTimeSteps_.back();
}

#if DESKTOP
void Engine::DefineParameters(CLI::App& commandLine, StringVariantMap& engineParameters)
{
    auto addFlagInternal = [&](const char* name, const char* description, CLI::callback_t fun) {
        CLI::Option *opt = commandLine.add_option(name, fun, description, false);
        if(opt->get_positional())
            throw CLI::IncorrectConstruction::PositionalFlag(name);
        opt->type_size(0);
        return opt;
    };

    auto addFlag = [&](const char* name, const ea::string& param, const Variant& value, const char* description) {
        CLI::callback_t fun = [&engineParameters, param, value](CLI::results_t) {
            engineParameters[param] = value;
            return true;
        };
        return addFlagInternal(name, description, fun);
    };

    auto addOptionPrependString = [&](const char* name, const ea::string& param, const ea::string& value, const char* description) {
        CLI::callback_t fun = [&engineParameters, param, value](CLI::results_t) {
            engineParameters[param] = value + engineParameters[param].GetString();
            return true;
        };
        return addFlagInternal(name, description, fun);
    };

    auto addOptionString = [&](const char* name, const ea::string& param, const char* description) {
        CLI::callback_t fun = [&engineParameters, param](CLI::results_t res) {
            engineParameters[param] = res[0].c_str();
            return true;
        };
        auto* opt = addFlagInternal(name, description, fun);
        opt->type_name("string");
        opt->type_size(1);
        return opt;
    };

    auto addOptionInt = [&](const char* name, const ea::string& param, const char* description) {
        CLI::callback_t fun = [&engineParameters, param](CLI::results_t res) {
            int value = 0;
            if (CLI::detail::lexical_cast(res[0], value))
            {
                engineParameters[param] = value;
                return true;
            }
            return false;
        };
        auto* opt = addFlagInternal(name, description, fun);
        opt->type_name("int");
        opt->type_size(1);
        return opt;
    };

    auto createOptions = [](const char* format, const char* options[]) {
        StringVector items;
        for (unsigned i = 0; options[i]; i++)
            items.push_back(options[i]);
        return ToString(format, ea::string::joined(items, "|").to_lower().replaced('_', '-').c_str());
    };

    addFlag("--headless", EP_HEADLESS, true, "Do not initialize graphics subsystem");
    addFlag("--validate-shaders", EP_VALIDATE_SHADERS, true, "Validate shaders before submitting them to GAPI");
    addFlag("--nolimit", EP_FRAME_LIMITER, false, "Disable frame limiter");
    addOptionPrependString("--landscape", EP_ORIENTATIONS, "LandscapeLeft LandscapeRight ", "Force landscape orientation");
    addOptionPrependString("--portrait", EP_ORIENTATIONS, "Portrait PortraitUpsideDown ", "Force portrait orientation");
    addFlag("--nosound", EP_SOUND, false, "Disable sound");
    addFlag("--noip", EP_SOUND_INTERPOLATION, false, "Disable sound interpolation");
    addOptionInt("--speakermode", EP_SOUND_MODE, "Force sound speaker output mode (default is automatic)");
    addFlag("--nothreads", EP_WORKER_THREADS, false, "Disable multithreading");
    addFlag("-v,--vsync", EP_VSYNC, true, "Enable vsync");
    addFlag("-w,--windowed", EP_BORDERLESS, false, "Windowed mode");
    addFlag("-f,--full-screen", EP_FULL_SCREEN, true, "Full screen mode");
    addFlag("--borderless", EP_BORDERLESS, true, "Borderless window mode");
    addFlag("-s,--resizable", EP_WINDOW_RESIZABLE, true, "Enable window resizing");
    addFlag("-q,--quiet", EP_LOG_QUIET, true, "Disable logging");
    addFlagInternal("-l,--log", "Logging level", [&](CLI::results_t res) {
        unsigned logLevel = GetStringListIndex(ea::string(res[0].c_str()).to_upper().c_str(), logLevelNames, M_MAX_UNSIGNED);
        if (logLevel == M_MAX_UNSIGNED)
            return false;
        engineParameters[EP_LOG_LEVEL] = logLevel;
        return true;
    })->type_name(createOptions("string in {%s}", logLevelNames).c_str())->type_size(1);
    addOptionString("--log-file", EP_LOG_NAME, "Log output file");
    addOptionInt("-x,--width", EP_WINDOW_WIDTH, "Window width");
    addOptionInt("-y,--height", EP_WINDOW_HEIGHT, "Window height");
    addOptionInt("--monitor", EP_MONITOR, "Create window on the specified monitor");
    addOptionInt("--hz", EP_REFRESH_RATE, "Use custom refresh rate");
    addOptionInt("-m,--multisample", EP_MULTI_SAMPLE, "Multisampling samples");
    addOptionInt("-b,--sound-buffer", EP_SOUND_BUFFER, "Sound buffer size");
    addOptionInt("-r,--mix-rate", EP_SOUND_MIX_RATE, "Sound mixing rate");
    addOptionString("--pp,--prefix-paths", EP_RESOURCE_PREFIX_PATHS, "Resource prefix paths")->envname("URHO3D_PREFIX_PATH")->type_name("path1;path2;...");
    addOptionString("--pr,--resource-paths", EP_RESOURCE_PATHS, "Resource paths")->type_name("path1;path2;...");
    addOptionString("--pf,--resource-packages", EP_RESOURCE_PACKAGES, "Resource packages")->type_name("path1;path2;...");
    addOptionString("--ap,--autoload-paths", EP_AUTOLOAD_PATHS, "Resource autoload paths")->type_name("path1;path2;...");
    addOptionString("--cn,--config-name", EP_CONFIG_NAME, "Config name")->type_name("filename");
    addFlagInternal("--tq", "Texture quality", [&](CLI::results_t res) {
        unsigned value = 0;
        if (CLI::detail::lexical_cast(res[0], value) && value >= QUALITY_LOW && value <= QUALITY_MAX)
        {
            engineParameters[EP_TEXTURE_QUALITY] = value;
            return true;
        }
        return false;
    })->type_name(ToString("int {%d-%d}", QUALITY_LOW, QUALITY_MAX).c_str())->type_size(1);
    addFlagInternal("--tf", "Texture filter mode", [&](CLI::results_t res) {
        unsigned mode = GetStringListIndex(ea::string(res[0].c_str()).to_upper().replaced('-', '_').c_str(), textureFilterModeNames, M_MAX_UNSIGNED);
        if (mode == M_MAX_UNSIGNED)
            return false;
        engineParameters[EP_TEXTURE_FILTER_MODE] = mode;
        return true;
    })->type_name(createOptions("string in {%s}", textureFilterModeNames).c_str())->type_size(1);
    addFlagInternal("--af", "Use anisotropic filtering", [&](CLI::results_t res) {
        int value = 0;
        if (CLI::detail::lexical_cast(res[0], value) && value >= 1)
        {
            engineParameters[EP_TEXTURE_FILTER_MODE] = FILTER_ANISOTROPIC;
            engineParameters[EP_TEXTURE_ANISOTROPY] = value;
            return true;
        }
        return false;
    })->type_name("int")->type_size(1);
    addFlag("--touch", EP_TOUCH_EMULATION, true, "Enable touch emulation");
    addOptionInt("--timeout", EP_TIME_OUT, "Quit application after specified time");
    addOptionString("--plugins", EP_PLUGINS, "Plugins to be loaded")->type_name("plugin1;plugin2;...");
    addOptionString("--main", EP_MAIN_PLUGIN, "Plugin to be treated as main entry point")->type_name("plugin");
    addFlag("--log-shader-sources", EP_SHADER_LOG_SOURCES, true, "Log shader sources into shader cache directory");
    addFlag("--discard-shader-cache", EP_DISCARD_SHADER_CACHE, true, "Discard all cached shader bytecode and logged shader sources");
    addFlag("--no-save-shader-cache", EP_SAVE_SHADER_CACHE, false, "Disable saving shader bytecode to cache directory");
    addFlag("--xr", EP_XR, true, "Launch the engine in XR mode");

    addFlag("--d3d11", EP_RENDER_BACKEND, static_cast<int>(RenderBackend::D3D11), "Use Direct3D11 rendering backend");
    addFlag("--d3d12", EP_RENDER_BACKEND, static_cast<int>(RenderBackend::D3D12), "Use Direct3D12 rendering backend");
    addFlag("--opengl", EP_RENDER_BACKEND, static_cast<int>(RenderBackend::OpenGL), "Use OpenGL rendering backend");
    addFlag("--vulkan", EP_RENDER_BACKEND, static_cast<int>(RenderBackend::Vulkan), "Use Vulkan rendering backend");

    // Define --win32-console command line argument.
    // Actual argument handling is done at ParseArguments function from ProcessUtils.cpp.
#if defined(_WIN32) && !defined(UWP)
    CLI::callback_t showConsole = [](CLI::results_t res) { return true; };
    addFlagInternal("--win32-console", "Show console", showConsole);
#endif
}
#endif

const Variant& Engine::GetParameter(const ea::string& name) const
{
    return engineParameters_->GetVariable(name);
}

void Engine::LoadConfigFiles()
{
    const auto configName = GetParameter(EP_CONFIG_NAME).GetString();
    if (configName.empty())
        return;

    engineParameters_->LoadDefaults(configName, ApplicationFlavor::Platform);
    engineParameters_->LoadOverrides("conf://" + configName);
}

void Engine::SaveConfigFile()
{
    auto configName = GetParameter(EP_CONFIG_NAME).GetString();
    if (configName.empty())
        return;

    engineParameters_->SaveOverrides("conf://" + configName, ApplicationFlavor::Platform);
}

void Engine::PopulateDefaultParameters()
{
#if URHO3D_OCULUS_QUEST
    const bool defaultXR = true;
#else
    const bool defaultXR = false;
#endif

    RenderDeviceSettingsVulkan vulkanTweaks;
    vulkanTweaks.dynamicHeapSize_ = 32 * 1024 * 1024;

    RenderDeviceSettingsD3D12 d3d12Tweaks;
    d3d12Tweaks.gpuDescriptorHeapSize_[0] = 32 * 1024;
    d3d12Tweaks.gpuDescriptorHeapDynamicSize_[0] = 24 * 1024;

    engineParameters_ = MakeShared<ConfigFile>(context_);

    engineParameters_->DefineVariable(EP_APPLICATION_NAME, "Unspecified Application");
    engineParameters_->DefineVariable(EP_APPLICATION_PREFERENCES_DIR, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_AUTOLOAD_PATHS, "Autoload").CommandLinePriority();
    engineParameters_->DefineVariable(EP_CONFIG_NAME, "EngineParameters.json");
    engineParameters_->DefineVariable(EP_BORDERLESS, true).Overridable();
    engineParameters_->DefineVariable(EP_DISCARD_SHADER_CACHE, false);
    engineParameters_->DefineVariable(EP_ENGINE_AUTO_LOAD_SCRIPTS, false);
    engineParameters_->DefineVariable(EP_ENGINE_CLI_PARAMETERS, true);
    engineParameters_->DefineVariable(EP_EXTERNAL_WINDOW, static_cast<void*>(nullptr));
    engineParameters_->DefineVariable(EP_FRAME_LIMITER, true).Overridable();
    engineParameters_->DefineVariable(EP_FULL_SCREEN, false).Overridable();
    engineParameters_->DefineVariable(EP_GPU_DEBUG, false);
    engineParameters_->DefineVariable(EP_HEADLESS, false);
    engineParameters_->DefineVariable(EP_LOAD_FONTS, true);
    engineParameters_->DefineVariable(EP_LOG_LEVEL, LOG_TRACE).CommandLinePriority();
    engineParameters_->DefineVariable(EP_LOG_NAME, "conf://Urho3D.log").CommandLinePriority();
    engineParameters_->DefineVariable(EP_LOG_QUIET, false).CommandLinePriority();
    engineParameters_->DefineVariable(EP_MAIN_PLUGIN, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_MONITOR, 0).Overridable();
    engineParameters_->DefineVariable(EP_MULTI_SAMPLE, 1);
    engineParameters_->DefineVariable(EP_ORGANIZATION_NAME, "Urho3D Rebel Fork");
    engineParameters_->DefineVariable(EP_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    engineParameters_->DefineVariable(EP_PACKAGE_CACHE_DIR, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_PLUGINS, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_RENAME_PLUGINS, false);
    engineParameters_->DefineVariable(EP_REFRESH_RATE, 0).Overridable();
    engineParameters_->DefineVariable(EP_RESOURCE_PACKAGES, EMPTY_STRING).CommandLinePriority();
    engineParameters_->DefineVariable(EP_RESOURCE_PATHS, "CoreData;Cache;Data").CommandLinePriority();
    engineParameters_->DefineVariable(EP_RESOURCE_PREFIX_PATHS, EMPTY_STRING).CommandLinePriority();
    engineParameters_->DefineVariable(EP_RESOURCE_ROOT_FILE, "ResourceRoot.ini");
    engineParameters_->DefineVariable(EP_SAVE_SHADER_CACHE, true);
    engineParameters_->DefineVariable(EP_SHADER_CACHE_DIR, "conf://ShaderCache");
    engineParameters_->DefineVariable(EP_SHADER_POLICY).SetOptional<int>();
    engineParameters_->DefineVariable(EP_SHADER_LOG_SOURCES, false);
    engineParameters_->DefineVariable(EP_SOUND, true);
    engineParameters_->DefineVariable(EP_SOUND_BUFFER, 100);
    engineParameters_->DefineVariable(EP_SOUND_INTERPOLATION, true);
    engineParameters_->DefineVariable(EP_SOUND_MIX_RATE, 44100);
    engineParameters_->DefineVariable(EP_SOUND_MODE, SpeakerMode::SPK_AUTO);
    engineParameters_->DefineVariable(EP_SYSTEMUI_FLAGS, 0u);
    engineParameters_->DefineVariable(EP_TEXTURE_ANISOTROPY, 4).Overridable();
    engineParameters_->DefineVariable(EP_TEXTURE_FILTER_MODE, FILTER_TRILINEAR).Overridable();
    engineParameters_->DefineVariable(EP_TEXTURE_QUALITY, QUALITY_HIGH).Overridable();
    engineParameters_->DefineVariable(EP_TIME_OUT, 0);
    engineParameters_->DefineVariable(EP_TOUCH_EMULATION, false);
    engineParameters_->DefineVariable(EP_TWEAK_D3D12, ToJSONString(d3d12Tweaks).value_or(""));
    engineParameters_->DefineVariable(EP_TWEAK_VULKAN, ToJSONString(vulkanTweaks).value_or(""));
    engineParameters_->DefineVariable(EP_VALIDATE_SHADERS, false);
    engineParameters_->DefineVariable(EP_VSYNC, false).Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_HEIGHT, 0); //.Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_ICON, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_WINDOW_MAXIMIZE, true).Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_POSITION_X, 0);
    engineParameters_->DefineVariable(EP_WINDOW_POSITION_Y, 0);
    engineParameters_->DefineVariable(EP_WINDOW_RESIZABLE, false);
    engineParameters_->DefineVariable(EP_WINDOW_TITLE, "Urho3D");
    engineParameters_->DefineVariable(EP_WINDOW_WIDTH, 0); //.Overridable();
    engineParameters_->DefineVariable(EP_WORKER_THREADS, true);
    engineParameters_->DefineVariable(EP_PSO_CACHE, "conf://psocache.bin");
    engineParameters_->DefineVariable(EP_RENDER_BACKEND).SetOptional<int>();
    engineParameters_->DefineVariable(EP_XR, defaultXR);
}

void Engine::HandleExitRequested(StringHash eventType, VariantMap& eventData)
{
    if (autoExit_)
    {
        exitRequired_ = true;
    }
}

void Engine::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    if (exitRequired_)
    {
        // Do not call Exit() here, as it contains mobile platform -specific tests to not exit.
        // If we do receive an exit request from the system on those platforms, we must comply
        DoExit();
    }
}

void Engine::DoExit()
{
    auto* graphics = GetSubsystem<Graphics>();
    if (graphics)
    {
        graphics->SavePipelineStateCache(FileIdentifier::FromUri(GetParameter(EP_PSO_CACHE).GetString()));
        graphics->Close();
    }

    SaveConfigFile();

    exiting_ = true;
#if defined(__EMSCRIPTEN__)
    // TODO: Revisit this place
    // emscripten_force_exit(EXIT_SUCCESS);    // Some how this is required to signal emrun to stop
#endif
}

ea::string Engine::GetLogFileName(const ea::string& uri) const
{
    // We cannot really use VirtualFileSystem here, as it is not initialized yet.
    // Emulate file:// and conf:// schemes in the same way.
    // Empty scheme means relative to executable directory instead of resource directory.
    const auto fileIdentifier = FileIdentifier::FromUri(uri);
    if (fileIdentifier.scheme_ == "file")
    {
        return fileIdentifier.fileName_;
    }
    else if (fileIdentifier.scheme_ == "conf")
    {
#ifndef __EMSCRIPTEN__
        return appPreferencesDir_ + fileIdentifier.fileName_;
#endif
    }
    else if (fileIdentifier.scheme_ == "")
    {
        auto fileSystem = GetSubsystem<FileSystem>();
        return fileSystem->GetProgramDir() + fileIdentifier.fileName_;
    }

    // Nothing we can do about it
    return "";
}

} // namespace Urho3D
