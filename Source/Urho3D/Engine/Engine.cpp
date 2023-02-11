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
#include "../Graphics/Renderer.h"
#include "../Input/Input.h"
#include "../Input/FreeFlyController.h"
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
#include "../RenderPipeline/RenderPipeline.h"
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
#ifdef URHO3D_COMPUTE
#include "../Graphics/ComputeDevice.h"
#endif
#include "../Utility/AnimationVelocityExtractor.h"
#include "../Utility/AssetPipeline.h"
#include "../Utility/AssetTransformer.h"
#include "../Utility/SceneViewerApplication.h"
#ifdef URHO3D_ACTIONS
#include "../Actions/ActionManager.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
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

namespace Urho3D
{

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

    SceneViewerApplication::RegisterObject();
    context_->AddFactoryReflection<AssetPipeline>();
    context_->AddFactoryReflection<AssetTransformer>();
    AnimationVelocityExtractor::RegisterObject(context_);

    SubscribeToEvent(E_EXITREQUESTED, URHO3D_HANDLER(Engine, HandleExitRequested));
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Engine, HandleEndFrame));
}

Engine::~Engine() = default;

bool Engine::Initialize(const StringVariantMap& parameters)
{
    if (initialized_)
        return true;

    URHO3D_PROFILE("InitEngine");

    engineParameters_->DefineVariables(parameters);
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Start logging
    auto* log = GetSubsystem<Log>();
    if (log)
    {
        if (HasParameter(EP_LOG_LEVEL))
            log->SetLevel(static_cast<LogLevel>(GetParameter(EP_LOG_LEVEL).GetInt()));
        log->SetQuiet(GetParameter(EP_LOG_QUIET).GetBool());
        log->Open(GetParameter(EP_LOG_NAME).GetString());
    }

    // Initialize app preferences directory
    appPreferencesDir_ = GetParameter(EP_APPLICATION_PREFERENCES_DIR).GetString();
    if (appPreferencesDir_.empty())
    {
        const ea::string& organizationName = GetParameter(EP_ORGANIZATION_NAME).GetString();
        const ea::string& applicationName = GetParameter(EP_APPLICATION_NAME).GetString();
        appPreferencesDir_ = fileSystem->GetAppPreferencesDir(organizationName, applicationName);
    }
    if (!appPreferencesDir_.empty())
        fileSystem->CreateDir(appPreferencesDir_);

    InitializeVirtualFileSystem();

    // Read and merge configs
    LoadConfigFiles();

    // Set headless mode
    headless_ = GetParameter(EP_HEADLESS).GetBool();

    // Register the rest of the subsystems
    context_->RegisterSubsystem(new Input(context_));
    context_->AddFactoryReflection<FreeFlyController>();

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
#ifdef URHO3D_COMPUTE
        context_->RegisterSubsystem(new ComputeDevice(context_, context_->GetSubsystem<Graphics>()));
#endif
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
    unsigned numThreads = GetParameter(EP_WORKER_THREADS).GetBool() ? GetNumPhysicalCPUs() - 1 : 0;
    if (numThreads)
    {
        GetSubsystem<WorkQueue>()->CreateThreads(numThreads);

        URHO3D_LOGINFOF("Created %u worker thread%s", numThreads, numThreads > 1 ? "s" : "");
    }
#endif

    // Add resource paths
    if (!InitializeResourceCache(parameters, false))
        return false;

    auto* cache = GetSubsystem<ResourceCache>();

    // Initialize graphics & audio output
    if (!headless_)
    {
        auto* graphics = GetSubsystem<Graphics>();
        auto* renderer = GetSubsystem<Renderer>();

        if (HasParameter(EP_EXTERNAL_WINDOW))
            graphics->SetExternalWindow(GetParameter(EP_EXTERNAL_WINDOW).GetVoidPtr());
        graphics->SetWindowTitle(GetParameter(EP_WINDOW_TITLE).GetString());
        graphics->SetWindowIcon(cache->GetResource<Image>(GetParameter(EP_WINDOW_ICON).GetString()));
        graphics->SetFlushGPU(GetParameter(EP_FLUSH_GPU).GetBool());
        graphics->SetOrientations(GetParameter(EP_ORIENTATIONS).GetString());
        graphics->SetShaderValidationEnabled(GetParameter(EP_VALIDATE_SHADERS).GetBool());

        SubscribeToEvent(E_SCREENMODE, [this](StringHash, VariantMap& eventData)
        {
            using namespace ScreenMode;

            const bool isBorderless = eventData[P_BORDERLESS].GetBool();

            SetParameter(EP_WINDOW_WIDTH, isBorderless ? 0 : eventData[P_WIDTH].GetInt());
            SetParameter(EP_WINDOW_HEIGHT, isBorderless ? 0 : eventData[P_HEIGHT].GetInt());
            SetParameter(EP_FULL_SCREEN, eventData[P_FULLSCREEN].GetBool());
            SetParameter(EP_BORDERLESS, isBorderless);
            SetParameter(EP_MONITOR, eventData[P_MONITOR].GetInt());
        });

#ifdef URHO3D_OPENGL
        if (HasParameter(EP_FORCE_GL2))
            graphics->SetForceGL2(GetParameter(EP_FORCE_GL2).GetBool());
#endif

        if (!graphics->SetMode(
            GetParameter(EP_WINDOW_WIDTH).GetInt(),
            GetParameter(EP_WINDOW_HEIGHT).GetInt(),
            GetParameter(EP_FULL_SCREEN).GetBool(),
            GetParameter(EP_BORDERLESS).GetBool(),
            GetParameter(EP_WINDOW_RESIZABLE).GetBool(),
            GetParameter(EP_HIGH_DPI).GetBool(),
            GetParameter(EP_VSYNC).GetBool(),
            GetParameter(EP_TRIPLE_BUFFER).GetBool(),
            GetParameter(EP_MULTI_SAMPLE).GetInt(),
            GetParameter(EP_MONITOR).GetInt(),
            GetParameter(EP_REFRESH_RATE).GetInt(),
            GetParameter(EP_GPU_DEBUG).GetBool()
        ))
            return false;

        if (HasParameter(EP_WINDOW_POSITION_X) && HasParameter(EP_WINDOW_POSITION_Y))
            graphics->SetWindowPosition(GetParameter(EP_WINDOW_POSITION_X).GetInt(),
                GetParameter(EP_WINDOW_POSITION_Y).GetInt());

        if (HasParameter(EP_WINDOW_MAXIMIZE) && GetParameter(EP_WINDOW_MAXIMIZE).GetBool())
            graphics->Maximize();

        const ea::string shaderCacheDir = appPreferencesDir_ + GetParameter(EP_SHADER_CACHE_DIR).GetString();
        graphics->SetShaderCacheDir(shaderCacheDir);

        if (HasParameter(EP_DUMP_SHADERS))
            graphics->BeginDumpShaders(GetParameter(EP_DUMP_SHADERS).GetString());

        renderer->SetDrawShadows(GetParameter(EP_SHADOWS).GetBool());
        if (renderer->GetDrawShadows() && GetParameter(EP_LOW_QUALITY_SHADOWS).GetBool())
            renderer->SetShadowQuality(SHADOWQUALITY_SIMPLE_16BIT);
        renderer->SetMaterialQuality((MaterialQuality)GetParameter(EP_MATERIAL_QUALITY).GetInt());
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
        RegisterStandardSerializableHooks();
#endif
    }
    frameTimer_.Reset();

    URHO3D_LOGINFO("Initialized engine");
    initialized_ = true;
    SendEvent(E_ENGINEINITIALIZED);
    return true;
}

void Engine::InitializeVirtualFileSystem()
{
    auto fileSystem = GetSubsystem<FileSystem>();
    auto vfs = GetSubsystem<VirtualFileSystem>();

    const StringVector prefixPaths = GetParameter(EP_RESOURCE_PREFIX_PATHS).GetString().split(';');
    const StringVector paths = GetParameter(EP_RESOURCE_PATHS).GetString().split(';');
    const StringVector packages = GetParameter(EP_RESOURCE_PACKAGES).GetString().split(';');
    // TODO: Implement autoload
    //const StringVector autoloadPaths = engine->GetParameter(EP_AUTOLOAD_PATHS).GetString().split(';');

    const ea::string& programDir = fileSystem->GetProgramDir();
    StringVector absolutePrefixPaths = GetAbsolutePaths(prefixPaths, programDir, true);
    if (!absolutePrefixPaths.contains(programDir))
        absolutePrefixPaths.push_back(programDir);

    vfs->UnmountAll();
    vfs->MountExistingDirectoriesOrPackages(absolutePrefixPaths, paths);
    vfs->MountExistingPackages(absolutePrefixPaths, packages);

#ifndef __EMSCRIPTEN__
    vfs->MountDir("conf", GetAppPreferencesDir());
#else
    vfs->MountDir("conf", "/IndexedDB/");
#endif
}

bool Engine::InitializeResourceCache(const StringVariantMap& parameters, bool removeOld /*= true*/)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Remove all resource paths and packages
    if (removeOld)
    {
        cache->RemoveAllResourceDirs();
        ea::vector<SharedPtr<PackageFile> > packageFiles = cache->GetPackageFiles();
        for (unsigned i = 0; i < packageFiles.size(); ++i)
            cache->RemovePackageFile(packageFiles[i].Get());
    }

    // Add resource paths
    ea::vector<ea::string> resourcePrefixPaths = GetParameter(EP_RESOURCE_PREFIX_PATHS).GetString().split(';', true);
    for (unsigned i = 0; i < resourcePrefixPaths.size(); ++i)
        resourcePrefixPaths[i] = AddTrailingSlash(
            IsAbsolutePath(resourcePrefixPaths[i]) ? resourcePrefixPaths[i] : fileSystem->GetProgramDir() + resourcePrefixPaths[i]);
    ea::vector<ea::string> resourcePaths = GetParameter(EP_RESOURCE_PATHS).GetString().split(';');
    ea::vector<ea::string> resourcePackages = GetParameter(EP_RESOURCE_PACKAGES).GetString().split(';');
    ea::vector<ea::string> autoLoadPaths = GetParameter(EP_AUTOLOAD_PATHS).GetString().split(';');

    for (unsigned i = 0; i < resourcePaths.size(); ++i)
    {
        // If path is not absolute, prefer to add it as a package if possible
        if (!IsAbsolutePath(resourcePaths[i]))
        {
            unsigned j = 0;
            for (; j < resourcePrefixPaths.size(); ++j)
            {
                ea::string packageName = resourcePrefixPaths[j] + resourcePaths[i] + ".pak";
                if (fileSystem->FileExists(packageName))
                {
                    if (cache->AddPackageFile(packageName))
                        break;
                    else
                        return false;   // The root cause of the error should have already been logged
                }
                ea::string pathName = resourcePrefixPaths[j] + resourcePaths[i];
                if (fileSystem->DirExists(pathName))
                {
                    if (cache->AddResourceDir(pathName))
                        break;
                    else
                        return false;
                }
            }
            if (j == resourcePrefixPaths.size() && !headless_)
            {
                URHO3D_LOGERRORF(
                    "Failed to add resource path '%s', check the documentation on how to set the 'resource prefix path'",
                    resourcePaths[i].c_str());
                return false;
            }
        }
        else
        {
            ea::string pathName = resourcePaths[i];
            if (fileSystem->DirExists(pathName))
                if (!cache->AddResourceDir(pathName))
                    return false;
        }
    }

    // Then add specified packages
    for (unsigned i = 0; i < resourcePackages.size(); ++i)
    {
        unsigned j = 0;
        for (; j < resourcePrefixPaths.size(); ++j)
        {
            ea::string packageName = resourcePrefixPaths[j] + resourcePackages[i];
            if (fileSystem->FileExists(packageName))
            {
                if (cache->AddPackageFile(packageName))
                    break;
                else
                    return false;
            }
        }
        if (j == resourcePrefixPaths.size() && !headless_)
        {
            URHO3D_LOGERRORF(
                "Failed to add resource package '%s', check the documentation on how to set the 'resource prefix path'",
                resourcePackages[i].c_str());
            return false;
        }
    }

    // Add auto load folders. Prioritize these (if exist) before the default folders
    for (unsigned i = 0; i < autoLoadPaths.size(); ++i)
    {
        bool autoLoadPathExist = false;

        for (unsigned j = 0; j < resourcePrefixPaths.size(); ++j)
        {
            ea::string autoLoadPath(autoLoadPaths[i]);
            if (!IsAbsolutePath(autoLoadPath))
                autoLoadPath = resourcePrefixPaths[j] + autoLoadPath;

            if (fileSystem->DirExists(autoLoadPath))
            {
                autoLoadPathExist = true;

                // Add all the subdirs (non-recursive) as resource directory
                ea::vector<ea::string> subdirs;
                fileSystem->ScanDir(subdirs, autoLoadPath, "*", SCAN_DIRS, false);
                for (unsigned y = 0; y < subdirs.size(); ++y)
                {
                    ea::string dir = subdirs[y];
                    if (dir.starts_with("."))
                        continue;

                    ea::string autoResourceDir = AddTrailingSlash(autoLoadPath) + dir;
                    if (!cache->AddResourceDir(autoResourceDir, 0))
                        return false;
                }

                // Add all the found package files (non-recursive)
                ea::vector<ea::string> paks;
                fileSystem->ScanDir(paks, autoLoadPath, "*.pak", SCAN_FILES, false);
                for (unsigned y = 0; y < paks.size(); ++y)
                {
                    ea::string pak = paks[y];
                    if (pak.starts_with("."))
                        continue;

                    ea::string autoPackageName = autoLoadPath + "/" + pak;
                    if (!cache->AddPackageFile(autoPackageName, 0))
                        return false;
                }
            }
        }

        // The following debug message is confusing when user is not aware of the autoload feature
        // Especially because the autoload feature is enabled by default without user intervention
        // The following extra conditional check below is to suppress unnecessary debug log entry under such default situation
        // The cleaner approach is to not enable the autoload by default, i.e. do not use 'Autoload' as default value for 'AutoloadPaths' engine parameter
        // However, doing so will break the existing applications that rely on this
        if (!autoLoadPathExist && (autoLoadPaths.size() > 1 || autoLoadPaths[0] != "Autoload"))
            URHO3D_LOGDEBUGF(
                "Skipped autoload path '%s' as it does not exist, check the documentation on how to set the 'resource prefix path'",
                autoLoadPaths[i].c_str());
    }

    return true;
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
    if (maxFps < 60)
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

    auto addFlag = [&](const char* name, const ea::string& param, bool value, const char* description) {
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
    addFlag("--flushgpu", EP_FLUSH_GPU, true, "Enable GPU flushing");
    addFlag("--gl2", EP_FORCE_GL2, true, "Force OpenGL2");
    addOptionPrependString("--landscape", EP_ORIENTATIONS, "LandscapeLeft LandscapeRight ", "Force landscape orientation");
    addOptionPrependString("--portrait", EP_ORIENTATIONS, "Portrait PortraitUpsideDown ", "Force portrait orientation");
    addFlag("--nosound", EP_SOUND, false, "Disable sound");
    addFlag("--noip", EP_SOUND_INTERPOLATION, false, "Disable sound interpolation");
    addOptionInt("--speakermode", EP_SOUND_MODE, "Force sound speaker output mode (default is automatic)");
    auto* optNoShadows = addFlag("--noshadows", EP_SHADOWS, false, "Disable shadows");
    auto optLowQualityShadows = addFlag("--lqshadows", EP_LOW_QUALITY_SHADOWS, true, "Use low quality shadows")->excludes(optNoShadows);
    optNoShadows->excludes(optLowQualityShadows);
    addFlag("--nothreads", EP_WORKER_THREADS, false, "Disable multithreading");
    addFlag("-v,--vsync", EP_VSYNC, true, "Enable vsync");
    addFlag("-t,--tripple-buffer", EP_TRIPLE_BUFFER, true, "Enable tripple-buffering");
    addFlag("-w,--windowed", EP_FULL_SCREEN, false, "Windowed mode");
    addFlag("-f,--full-screen", EP_FULL_SCREEN, true, "Full screen mode");
    addFlag("--borderless", EP_BORDERLESS, true, "Borderless window mode");
    addFlag("--lowdpi", EP_HIGH_DPI, false, "Disable high-dpi handling");
    addFlag("--highdpi", EP_HIGH_DPI, true, "Enable high-dpi handling");
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
    addOptionString("--ds,--dump-shaders", EP_DUMP_SHADERS, "Dump shaders")->type_name("filename");
    addFlagInternal("--mq,--material-quality", "Material quality", [&](CLI::results_t res) {
        unsigned value = 0;
        if (CLI::detail::lexical_cast(res[0], value) && value >= QUALITY_LOW && value <= QUALITY_MAX)
        {
            engineParameters[EP_MATERIAL_QUALITY] = value;
            return true;
        }
        return false;
    })->type_name(ToString("int {%d-%d}", QUALITY_LOW, QUALITY_MAX).c_str())->type_size(1);
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
    engineParameters_ = MakeShared<ConfigFile>(context_);

    engineParameters_->DefineVariable(EP_APPLICATION_NAME, "Unspecified Application");
    engineParameters_->DefineVariable(EP_APPLICATION_PREFERENCES_DIR, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_AUTOLOAD_PATHS, "Autoload");
    engineParameters_->DefineVariable(EP_CONFIG_NAME, "EngineParameters.json");
    engineParameters_->DefineVariable(EP_BORDERLESS, true).Overridable();
    engineParameters_->DefineVariable(EP_DUMP_SHADERS, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_ENGINE_AUTO_LOAD_SCRIPTS, false);
    engineParameters_->DefineVariable(EP_ENGINE_CLI_PARAMETERS, true);
    engineParameters_->DefineVariable(EP_EXTERNAL_WINDOW, static_cast<void*>(nullptr));
    engineParameters_->DefineVariable(EP_FLUSH_GPU, false);
    engineParameters_->DefineVariable(EP_FORCE_GL2, false);
    engineParameters_->DefineVariable(EP_FRAME_LIMITER, true).Overridable();
    engineParameters_->DefineVariable(EP_FULL_SCREEN, false).Overridable();
    engineParameters_->DefineVariable(EP_GPU_DEBUG, false);
    engineParameters_->DefineVariable(EP_HEADLESS, false);
    engineParameters_->DefineVariable(EP_HIGH_DPI, true);
    engineParameters_->DefineVariable(EP_LOG_LEVEL, LOG_TRACE);
    engineParameters_->DefineVariable(EP_LOG_NAME, "Urho3D.log");
    engineParameters_->DefineVariable(EP_LOG_QUIET, false);
    engineParameters_->DefineVariable(EP_LOW_QUALITY_SHADOWS, false).Overridable();
    engineParameters_->DefineVariable(EP_MAIN_PLUGIN, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_MATERIAL_QUALITY, QUALITY_HIGH).Overridable();
    engineParameters_->DefineVariable(EP_MONITOR, 0).Overridable();
    engineParameters_->DefineVariable(EP_MULTI_SAMPLE, 1);
    engineParameters_->DefineVariable(EP_ORGANIZATION_NAME, "Urho3D Rebel Fork");
    engineParameters_->DefineVariable(EP_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    engineParameters_->DefineVariable(EP_PACKAGE_CACHE_DIR, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_PLUGINS, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_REFRESH_RATE, 0).Overridable();
    engineParameters_->DefineVariable(EP_RESOURCE_PACKAGES, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_RESOURCE_PATHS, "Data;CoreData");
    engineParameters_->DefineVariable(EP_RESOURCE_PREFIX_PATHS, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_SHADER_CACHE_DIR, "ShaderCache");
    engineParameters_->DefineVariable(EP_SHADOWS, true).Overridable();
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
    engineParameters_->DefineVariable(EP_TRIPLE_BUFFER, false);
    engineParameters_->DefineVariable(EP_VALIDATE_SHADERS, false);
    engineParameters_->DefineVariable(EP_VSYNC, false).Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_HEIGHT, 0).Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_ICON, EMPTY_STRING);
    engineParameters_->DefineVariable(EP_WINDOW_MAXIMIZE, true).Overridable();
    engineParameters_->DefineVariable(EP_WINDOW_POSITION_X, 0);
    engineParameters_->DefineVariable(EP_WINDOW_POSITION_Y, 0);
    engineParameters_->DefineVariable(EP_WINDOW_RESIZABLE, false);
    engineParameters_->DefineVariable(EP_WINDOW_TITLE, "Urho3D");
    engineParameters_->DefineVariable(EP_WINDOW_WIDTH, 0).Overridable();
    engineParameters_->DefineVariable(EP_WORKER_THREADS, true);
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
        graphics->Close();

    SaveConfigFile();

    exiting_ = true;
#if defined(__EMSCRIPTEN__)
    // TODO: Revisit this place
    // emscripten_force_exit(EXIT_SUCCESS);    // Some how this is required to signal emrun to stop
#endif
}

}
