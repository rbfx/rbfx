//
// Copyright (c) 2008-2018 the Urho3D project.
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
#include "../Core/WorkQueue.h"
#ifdef URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/Console.h"
#include "../SystemUI/DebugHud.h"
#endif
#include "../Engine/Engine.h"
#include "../Engine/EngineDefs.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../Input/Input.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/PackageFile.h"
#include "IO/FileWatcher.h"
#include "../Misc/FreeFunctions.h"
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
#include "../Resource/ResourceCache.h"
#include "../Resource/Localization.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../UI/UI.h"
#ifdef URHO3D_URHO2D
#include "../Urho2D/Urho2D.h"
#endif
#include "../Core/Tasks.h"
#include "../Engine/EngineEvents.h"

#if defined(__EMSCRIPTEN__) && defined(URHO3D_TESTING)
#include <emscripten/emscripten.h>
#endif

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

extern const char* logLevelPrefixes[];

Engine::Engine(Context* context) :
    Object(context),
#if defined(IOS) || defined(TVOS) || defined(__ANDROID__) || defined(__arm__) || defined(__aarch64__)
    pauseMinimized_(true),
#else
    pauseMinimized_(false),
#endif
#ifdef URHO3D_TESTING
    timeOut_(0),
#endif
    autoExit_(true),
    initialized_(false),
    exiting_(false),
    headless_(false),
    audioPaused_(false)
{



    // Register self as a subsystem
    context_->RegisterSubsystem(this);

    // Create subsystems which do not depend on engine initialization or startup parameters
    context_->RegisterSubsystem(new Time(context_));
    context_->RegisterSubsystem(new WorkQueue(context_));
#ifdef URHO3D_PROFILING
    context_->RegisterSubsystem(new Profiler(context_));
#endif
    context_->RegisterSubsystem(new FileSystem(context_));
	context_->RegisterFactory<FileWatcher>();
#ifdef URHO3D_LOGGING
    context_->RegisterSubsystem(new Log(context_));
#endif
    context_->RegisterSubsystem(new ResourceCache(context_));
    context_->RegisterSubsystem(new Localization(context_));
#ifdef URHO3D_NETWORK
    context_->RegisterSubsystem(new Network(context_));
#endif
    context_->RegisterSubsystem(new Input(context_));
    context_->RegisterSubsystem(new Audio(context_));
    context_->RegisterSubsystem(new UI(context_));
#if URHO3D_TASKS
    context_->RegisterSubsystem(new Tasks(context_));
#endif
	context_->RegisterSubsystem(new FreeFunctions(context_));
    // Register object factories for libraries which are not automatically registered along with subsystem creation
    RegisterSceneLibrary(context_);

#ifdef URHO3D_IK
    RegisterIKLibrary(context_);
#endif

#ifdef URHO3D_PHYSICS
    RegisterPhysicsLibrary(context_);
#endif

#ifdef URHO3D_NAVIGATION
    RegisterNavigationLibrary(context_);
#endif




    SubscribeToEvent(E_EXITREQUESTED, URHO3D_HANDLER(Engine, HandleExitRequested));
}

Engine::~Engine() = default;

bool Engine::Initialize(const VariantMap& parameters)
{
    if (initialized_)
        return true;

#ifdef URHO3D_PROFILING
    if (auto* profiler = GetSubsystem<Profiler>())
    {
        profiler->SetEnabled(true);
        profiler->SetEventProfilingEnabled(GetParameter(parameters, EP_EVENT_PROFILER, true).GetBool());
        if (GetParameter(parameters, EP_PROFILER_LISTEN, false).GetBool())
            profiler->StartListen((unsigned short)GetParameter(parameters, EP_PROFILER_PORT, PROFILER_DEFAULT_PORT).GetInt());
    }
#endif

    URHO3D_PROFILE(InitEngine);

    // Set headless mode
    headless_ = GetParameter(parameters, EP_HEADLESS, false).GetBool();

    // Register the rest of the subsystems
    if (!headless_)
    {
        context_->RegisterSubsystem(new Graphics(context_));
        context_->RegisterSubsystem(new Renderer(context_));
        context_->graphics_ = context_->GetSubsystem<Graphics>();
        context_->renderer_ = context_->GetSubsystem<Renderer>();
    }
    else
    {
        // Register graphics library objects explicitly in headless mode to allow them to work without using actual GPU resources
        RegisterGraphicsLibrary(context_);
    }

#ifdef URHO3D_URHO2D
    // 2D graphics library is dependent on 3D graphics library
    RegisterUrho2DLibrary(context_);
#endif

    // Start logging
    auto* log = GetSubsystem<Log>();
    if (log)
    {
        if (HasParameter(parameters, EP_LOG_LEVEL))
            log->SetLevel(GetParameter(parameters, EP_LOG_LEVEL).GetInt());
        log->SetQuiet(GetParameter(parameters, EP_LOG_QUIET, false).GetBool());
        log->Open(GetParameter(parameters, EP_LOG_NAME, "Urho3D.log").GetString());
    }

    // Set maximally accurate low res timer
    GetSubsystem<Time>()->SetTimerPeriod(1);

    // Configure max FPS
    //if (GetParameter(parameters, EP_FRAME_LIMITER, true) == false)
    //    SetMaxFps(0);

    // Set amount of worker threads according to the available physical CPU cores. Using also hyperthreaded cores results in
    // unpredictable extra synchronization overhead. Also reserve one core for the main thread
#ifdef URHO3D_THREADING
    unsigned numThreads = GetParameter(parameters, EP_WORKER_THREADS, true).GetBool() ? GetNumPhysicalCPUs() - 1 : 0;
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
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Initialize graphics & audio output
    if (!headless_)
    {
        auto* graphics = GetSubsystem<Graphics>();
        auto* renderer = GetSubsystem<Renderer>();

        if (HasParameter(parameters, EP_EXTERNAL_WINDOW))
            graphics->SetExternalWindow(GetParameter(parameters, EP_EXTERNAL_WINDOW).GetVoidPtr());
        graphics->SetWindowTitle(GetParameter(parameters, EP_WINDOW_TITLE, "Urho3D").GetString());
        graphics->SetWindowIcon(cache->GetResource<Image>(GetParameter(parameters, EP_WINDOW_ICON, String::EMPTY).GetString()));
        graphics->SetFlushGPU(GetParameter(parameters, EP_FLUSH_GPU, false).GetBool());
        graphics->SetOrientations(GetParameter(parameters, EP_ORIENTATIONS, "LandscapeLeft LandscapeRight").GetString());

        if (HasParameter(parameters, EP_WINDOW_POSITION_X) && HasParameter(parameters, EP_WINDOW_POSITION_Y))
            graphics->SetWindowPosition(GetParameter(parameters, EP_WINDOW_POSITION_X).GetInt(),
                GetParameter(parameters, EP_WINDOW_POSITION_Y).GetInt());

#ifdef URHO3D_OPENGL
        if (HasParameter(parameters, EP_FORCE_GL2))
            graphics->SetForceGL2(GetParameter(parameters, EP_FORCE_GL2).GetBool());
#endif

        if (!graphics->SetMode(
            GetParameter(parameters, EP_WINDOW_WIDTH, 0).GetInt(),
            GetParameter(parameters, EP_WINDOW_HEIGHT, 0).GetInt(),
            GetParameter(parameters, EP_FULL_SCREEN, true).GetBool(),
            GetParameter(parameters, EP_BORDERLESS, false).GetBool(),
            GetParameter(parameters, EP_WINDOW_RESIZABLE, false).GetBool(),
            GetParameter(parameters, EP_HIGH_DPI, true).GetBool(),
            GetParameter(parameters, EP_VSYNC, false).GetBool(),
            GetParameter(parameters, EP_TRIPLE_BUFFER, false).GetBool(),
            GetParameter(parameters, EP_MULTI_SAMPLE, 1).GetInt(),
            GetParameter(parameters, EP_MONITOR, 0).GetInt(),
            GetParameter(parameters, EP_REFRESH_RATE, 0).GetInt()
        ))
            return false;

        graphics->SetShaderCacheDir(GetParameter(parameters, EP_SHADER_CACHE_DIR, fileSystem->GetAppPreferencesDir("urho3d", "shadercache")).GetString());

        if (HasParameter(parameters, EP_DUMP_SHADERS))
            graphics->BeginDumpShaders(GetParameter(parameters, EP_DUMP_SHADERS, String::EMPTY).GetString());
        if (HasParameter(parameters, EP_RENDER_PATH))
            renderer->SetDefaultRenderPath(cache->GetResource<XMLFile>(GetParameter(parameters, EP_RENDER_PATH).GetString()));

        renderer->SetDrawShadows(GetParameter(parameters, EP_SHADOWS, true).GetBool());
        if (renderer->GetDrawShadows() && GetParameter(parameters, EP_LOW_QUALITY_SHADOWS, false).GetBool())
            renderer->SetShadowQuality(SHADOWQUALITY_SIMPLE_16BIT);
        renderer->SetMaterialQuality(GetParameter(parameters, EP_MATERIAL_QUALITY, QUALITY_HIGH).GetInt());
        renderer->SetTextureQuality(GetParameter(parameters, EP_TEXTURE_QUALITY, QUALITY_HIGH).GetInt());
        renderer->SetTextureFilterMode((TextureFilterMode)GetParameter(parameters, EP_TEXTURE_FILTER_MODE, FILTER_TRILINEAR).GetInt());
        renderer->SetTextureAnisotropy(GetParameter(parameters, EP_TEXTURE_ANISOTROPY, 4).GetInt());

        if (GetParameter(parameters, EP_SOUND, true).GetBool())
        {
            GetSubsystem<Audio>()->SetMode(
                GetParameter(parameters, EP_SOUND_BUFFER, 100).GetInt(),
                GetParameter(parameters, EP_SOUND_MIX_RATE, 44100).GetInt(),
                GetParameter(parameters, EP_SOUND_STEREO, true).GetBool(),
                GetParameter(parameters, EP_SOUND_INTERPOLATION, true).GetBool()
            );
        }
    }

    // Init FPU state of main thread
    InitFPU();

    // Initialize input
    if (HasParameter(parameters, EP_TOUCH_EMULATION))
        GetSubsystem<Input>()->SetTouchEmulation(GetParameter(parameters, EP_TOUCH_EMULATION).GetBool());

    // Initialize network
#ifdef URHO3D_NETWORK
    if (HasParameter(parameters, EP_PACKAGE_CACHE_DIR))
        GetSubsystem<Network>()->SetPackageCacheDir(GetParameter(parameters, EP_PACKAGE_CACHE_DIR).GetString());
#endif

#ifdef URHO3D_TESTING
    if (HasParameter(parameters, EP_TIME_OUT))
        timeOut_ = GetParameter(parameters, EP_TIME_OUT, 0).GetInt() * 1000000LL;
#endif
    if (!headless_)
    {
#ifdef URHO3D_SYSTEMUI
        context_->RegisterSubsystem(new SystemUI(context_));
#endif
    }




    updateTimerTracker_.Reset();
	renderTimerTracker_.Reset();

	updateUpdateTimeTimer();
	updateFpsGoalTimer();
	
	SetUpdateAveragingTimeUs(1 * 1000000);
	SetRenderAveragingTimeUs(1 * 1000000);

    URHO3D_LOGINFO("Initialized engine");
    initialized_ = true;
    SendEvent(E_ENGINEINITIALIZED);
    return true;
}

bool Engine::InitializeResourceCache(const VariantMap& parameters, bool removeOld /*= true*/)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Remove all resource paths and packages
    if (removeOld)
    {
        Vector<String> resourceDirs = cache->GetResourceDirs();
        Vector<SharedPtr<PackageFile> > packageFiles = cache->GetPackageFiles();
        for (unsigned i = 0; i < resourceDirs.Size(); ++i)
            cache->RemoveResourceDir(resourceDirs[i]);
        for (unsigned i = 0; i < packageFiles.Size(); ++i)
            cache->RemovePackageFile(packageFiles[i]);
    }

    // Add resource paths
    Vector<String> resourcePrefixPaths = GetParameter(parameters, EP_RESOURCE_PREFIX_PATHS, String::EMPTY).GetString().Split(';', true);
    for (unsigned i = 0; i < resourcePrefixPaths.Size(); ++i)
        resourcePrefixPaths[i] = AddTrailingSlash(
            IsAbsolutePath(resourcePrefixPaths[i]) ? resourcePrefixPaths[i] : fileSystem->GetProgramDir() + resourcePrefixPaths[i]);
    Vector<String> resourcePaths = GetParameter(parameters, EP_RESOURCE_PATHS, "Data;CoreData").GetString().Split(';');
    Vector<String> resourcePackages = GetParameter(parameters, EP_RESOURCE_PACKAGES).GetString().Split(';');
    Vector<String> autoLoadPaths = GetParameter(parameters, EP_AUTOLOAD_PATHS, "Autoload").GetString().Split(';');

    for (unsigned i = 0; i < resourcePaths.Size(); ++i)
    {
        // If path is not absolute, prefer to add it as a package if possible
        if (!IsAbsolutePath(resourcePaths[i]))
        {
            unsigned j = 0;
            for (; j < resourcePrefixPaths.Size(); ++j)
            {
                String packageName = resourcePrefixPaths[j] + resourcePaths[i] + ".pak";
                if (fileSystem->FileExists(packageName))
                {
                    if (cache->AddPackageFile(packageName))
                        break;
                    else
                        return false;   // The root cause of the error should have already been logged
                }
                String pathName = resourcePrefixPaths[j] + resourcePaths[i];
                if (fileSystem->DirExists(pathName))
                {
                    if (cache->AddResourceDir(pathName))
                        break;
                    else
                        return false;
                }
            }
            if (j == resourcePrefixPaths.Size() && !headless_)
            {
                URHO3D_LOGERRORF(
                    "Failed to add resource path '%s', check the documentation on how to set the 'resource prefix path'",
                    resourcePaths[i].CString());
                return false;
            }
        }
        else
        {
            String pathName = resourcePaths[i];
            if (fileSystem->DirExists(pathName))
                if (!cache->AddResourceDir(pathName))
                    return false;
        }
    }

    // Then add specified packages
    for (unsigned i = 0; i < resourcePackages.Size(); ++i)
    {
        unsigned j = 0;
        for (; j < resourcePrefixPaths.Size(); ++j)
        {
            String packageName = resourcePrefixPaths[j] + resourcePackages[i];
            if (fileSystem->FileExists(packageName))
            {
                if (cache->AddPackageFile(packageName))
                    break;
                else
                    return false;
            }
        }
        if (j == resourcePrefixPaths.Size() && !headless_)
        {
            URHO3D_LOGERRORF(
                "Failed to add resource package '%s', check the documentation on how to set the 'resource prefix path'",
                resourcePackages[i].CString());
            return false;
        }
    }

    // Add auto load folders. Prioritize these (if exist) before the default folders
    for (unsigned i = 0; i < autoLoadPaths.Size(); ++i)
    {
        bool autoLoadPathExist = false;

        for (unsigned j = 0; j < resourcePrefixPaths.Size(); ++j)
        {
            String autoLoadPath(autoLoadPaths[i]);
            if (!IsAbsolutePath(autoLoadPath))
                autoLoadPath = resourcePrefixPaths[j] + autoLoadPath;

            if (fileSystem->DirExists(autoLoadPath))
            {
                autoLoadPathExist = true;

                // Add all the subdirs (non-recursive) as resource directory
                Vector<String> subdirs;
                fileSystem->ScanDir(subdirs, autoLoadPath, "*", SCAN_DIRS, false);
                for (unsigned y = 0; y < subdirs.Size(); ++y)
                {
                    String dir = subdirs[y];
                    if (dir.StartsWith("."))
                        continue;

                    String autoResourceDir = AddTrailingSlash(autoLoadPath) + dir;
                    if (!cache->AddResourceDir(autoResourceDir, 0))
                        return false;
                }

                // Add all the found package files (non-recursive)
                Vector<String> paks;
                fileSystem->ScanDir(paks, autoLoadPath, "*.pak", SCAN_FILES, false);
                for (unsigned y = 0; y < paks.Size(); ++y)
                {
                    String pak = paks[y];
                    if (pak.StartsWith("."))
                        continue;

                    String autoPackageName = autoLoadPath + "/" + pak;
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
        if (!autoLoadPathExist && (autoLoadPaths.Size() > 1 || autoLoadPaths[0] != "Autoload"))
            URHO3D_LOGDEBUGF(
                "Skipped autoload path '%s' as it does not exist, check the documentation on how to set the 'resource prefix path'",
                autoLoadPaths[i].CString());
    }

    return true;
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


float Engine::GetAverageRenderTimeMs()
{
	return avgRenderTimeUs_/1000.0f;
}

float Engine::GetAverageUpdateTimeMs()
{
	return avgUpdateTimeUs_/1000.0f;
}

bool Engine::GetUpdateIsLimited()
{
	if (avgUpdateTimeUs_ > updateTimeGoalUs + updateTimeGoalUs/5) //5% buffer
		return true;
	
	return false;
}

bool Engine::GetRenderIsLimited()
{
	if (avgRenderTimeUs_ > renderTimeGoalUs + renderTimeGoalUs / 5)//5% buffer
		return true;

	return false;
}

void Engine::SetRenderFpsGoal(int fps)
{
	renderTimeGoalUs = (1.0f/float(fps))*1000000.0f;
	updateFpsGoalTimer();
}

void Engine::SetRenderTimeGoalUs(unsigned timeUs)
{
	renderTimeGoalUs = timeUs;
	updateFpsGoalTimer();
}

void Engine::SetUpdateFpsGoal(unsigned fps)
{
	updateTimeGoalUs = (1.0f / float(fps))*1000000.0f;
	updateUpdateTimeTimer();
}


void Engine::SetUpdateTimeGoalUs(unsigned timeUs)
{
	updateTimeGoalUs = timeUs;
	updateUpdateTimeTimer();
}

void Engine::SetUpdateAveragingTimeUs(unsigned averagingTimeUs)
{
	updateAveragingTimeWindowUs_ = averagingTimeUs;
	updateAveragingTimeWindows();
}

void Engine::SetRenderAveragingTimeUs(unsigned averagingTimeUs)
{
	renderAveragingTimeWindowUs_ = averagingTimeUs;
	updateAveragingTimeWindows();
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
    const HashMap<StringHash, ResourceGroup>& resourceGroups = cache->GetAllResources();
    if (dumpFileName)
    {
        URHO3D_LOGRAW("Used resources:\n");
        for (HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups.Begin(); i != resourceGroups.End(); ++i)
        {
            const HashMap<StringHash, SharedPtr<Resource> >& resources = i->second_.resources_;
            if (dumpFileName)
            {
                for (HashMap<StringHash, SharedPtr<Resource> >::ConstIterator j = resources.Begin(); j != resources.End(); ++j)
                    URHO3D_LOGRAW(j->second_->GetName() + "\n");
            }
        }
    }
    else
        URHO3D_LOGRAW(cache->PrintMemoryUsage() + "\n");
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
                URHO3D_LOGRAW("Block " + String((int)block->lRequest) + ": " + String(block->nDataSize) + " bytes, file " + String(block->szFileName) + " line " + String(block->nLine) + "\n");
            else
                URHO3D_LOGRAW("Block " + String((int)block->lRequest) + ": " + String(block->nDataSize) + " bytes\n");

            total += block->nDataSize;
            ++blocks;
        }
        block = block->pBlockHeaderPrev;
    }

    URHO3D_LOGRAW("Total allocated memory " + String(total) + " bytes in " + String(blocks) + " blocks\n\n");
#else
    URHO3D_LOGRAW("DumpMemory() supported on MSVC debug mode only\n\n");
#endif
#endif
}

unsigned Engine::FreeUpdate()
{
	
	// If not headless, and the graphics subsystem no longer has a window open, assume we should exit
	if (!headless_ && !GetSubsystem<Graphics>()->IsInitialized())
		exiting_ = true;

	if (exiting_)
		return 0;


	// Note: there is a minimal performance cost to looking up subsystems (uses a hashmap); if they would be looked up several
	// times per frame it would be better to cache the pointers
	auto* time = static_cast<Time*>(context_->time_);
	auto* input = static_cast<Input*>(context_->input_);

	updateAudioPausing();


	if (updateTimer_.IsTimedOut()) {
		updateTimer_.Reset();
		Update();
		return 0;
	}
	else if (renderGoalTimer_.IsTimedOut())
	{
		//Render
		renderGoalTimer_.Reset();
		Render();
		return 0;
	}
	
	//lets compute approximate time we have until next update or render
	{
		unsigned updateTimeLeft = updateTimer_.GetTimeoutDuration() - updateTimer_.GetUSec(false);
		unsigned renderTimeLeft = renderGoalTimer_.GetTimeoutDuration() - renderGoalTimer_.GetUSec(false);

		unsigned timeLeftUS = Urho3D::Min(updateTimeLeft, renderTimeLeft);
		if (timeLeftUS > 0)
			return timeLeftUS;
		else
			return 0;
	}


	return 0;
}



void Engine::Update()
{
    URHO3D_PROFILE(Update);

	//compute times
	updateTick_++;
	lastUpdateTimeUs_ = updateTimerTracker_.GetUSec(true);

	//update rolling average
	avgUpdateTimeUs_ -= updateTimeUsBuffer_[updateTimeAvgIdx_] / updateTimeUsBuffer_.Size();
	updateTimeUsBuffer_[updateTimeAvgIdx_] = lastUpdateTimeUs_;
	avgUpdateTimeUs_ += lastUpdateTimeUs_ / updateTimeUsBuffer_.Size();

	updateTimeAvgIdx_++;
	if (updateTimeAvgIdx_ >= updateTimeUsBuffer_.Size())
		updateTimeAvgIdx_ = 0;


    // Logic update event
    VariantMap& eventData = GetEventDataMap();
    eventData[Update::P_TIMESTEP] = float(lastUpdateTimeUs_) / 1000000.0f;
	eventData[Update::P_UPDATETICK] = updateTick_;
    SendEvent(E_UPDATE, eventData);

    // Logic post-update event
    SendEvent(E_POSTUPDATE, eventData);
	

}

void Engine::Render()
{
    if (headless_)
        return;

    URHO3D_PROFILE(Render);


    // If device is lost, BeginFrame will fail and we skip rendering
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics->BeginFrame())
        return;

	//compute times
	renderTick_++;
	lastRenderTimeUs_ = renderTimerTracker_.GetUSec(true);


	//update rolling average
	avgRenderTimeUs_ -= renderTimeUsBuffer_[renderTimeAvgIdx_] / renderTimeUsBuffer_.Size();
	renderTimeUsBuffer_[renderTimeAvgIdx_] = lastRenderTimeUs_;
	avgRenderTimeUs_ += lastRenderTimeUs_ / renderTimeUsBuffer_.Size();

	renderTimeAvgIdx_++;
	if (renderTimeAvgIdx_ >= renderTimeUsBuffer_.Size())
		renderTimeAvgIdx_ = 0;




	VariantMap& eventData = GetEventDataMap();
	eventData[RenderUpdate::P_TIMESTEP] = float(lastRenderTimeUs_)/1000.0f;
	eventData[RenderUpdate::P_RENDERTICK] = renderTick_;
	
	// Rendering update event
	SendEvent(E_RENDERUPDATE, eventData);


    GetSubsystem<Renderer>()->Render();
    GetSubsystem<UI>()->Render();
    graphics->EndFrame();



	// Post-render update event
	SendEvent(E_POSTRENDERUPDATE, eventData);
}

VariantMap Engine::ParseParameters(const Vector<String>& arguments)
{
    VariantMap ret;

    // Pre-initialize the parameters with environment variable values when they are set
    if (const char* paths = getenv("URHO3D_PREFIX_PATH"))
        ret[EP_RESOURCE_PREFIX_PATHS] = paths;

    for (unsigned i = 0; i < arguments.Size(); ++i)
    {
        if (arguments[i].Length() > 1 && arguments[i][0] == '-')
        {
            String argument = arguments[i].Substring(1).ToLower();
            String value = i + 1 < arguments.Size() ? arguments[i + 1] : String::EMPTY;

            if (argument == "headless")
                ret[EP_HEADLESS] = true;
            else if (argument == "nolimit")
                ret[EP_FRAME_LIMITER] = false;
            else if (argument == "flushgpu")
                ret[EP_FLUSH_GPU] = true;
            else if (argument == "gl2")
                ret[EP_FORCE_GL2] = true;
            else if (argument == "landscape")
                ret[EP_ORIENTATIONS] = "LandscapeLeft LandscapeRight " + ret[EP_ORIENTATIONS].GetString();
            else if (argument == "portrait")
                ret[EP_ORIENTATIONS] = "Portrait PortraitUpsideDown " + ret[EP_ORIENTATIONS].GetString();
            else if (argument == "nosound")
                ret[EP_SOUND] = false;
            else if (argument == "noip")
                ret[EP_SOUND_INTERPOLATION] = false;
            else if (argument == "mono")
                ret[EP_SOUND_STEREO] = false;
            else if (argument == "prepass")
                ret[EP_RENDER_PATH] = "RenderPaths/Prepass.xml";
            else if (argument == "deferred")
                ret[EP_RENDER_PATH] = "RenderPaths/Deferred.xml";
            else if (argument == "renderpath" && !value.Empty())
            {
                ret[EP_RENDER_PATH] = value;
                ++i;
            }
            else if (argument == "noshadows")
                ret[EP_SHADOWS] = false;
            else if (argument == "lqshadows")
                ret[EP_LOW_QUALITY_SHADOWS] = true;
            else if (argument == "nothreads")
                ret[EP_WORKER_THREADS] = false;
            else if (argument == "v")
                ret[EP_VSYNC] = true;
            else if (argument == "t")
                ret[EP_TRIPLE_BUFFER] = true;
            else if (argument == "w")
                ret[EP_FULL_SCREEN] = false;
            else if (argument == "borderless")
                ret[EP_BORDERLESS] = true;
            else if (argument == "lowdpi")
                ret[EP_HIGH_DPI] = false;
            else if (argument == "s")
                ret[EP_WINDOW_RESIZABLE] = true;
            else if (argument == "hd")
                ret[EP_HIGH_DPI] = true;
            else if (argument == "q")
                ret[EP_LOG_QUIET] = true;
            else if (argument == "log" && !value.Empty())
            {
                unsigned logLevel = GetStringListIndex(value.CString(), logLevelPrefixes, M_MAX_UNSIGNED);
                if (logLevel != M_MAX_UNSIGNED)
                {
                    ret[EP_LOG_LEVEL] = logLevel;
                    ++i;
                }
            }
            else if (argument == "x" && !value.Empty())
            {
                ret[EP_WINDOW_WIDTH] = ToInt(value);
                ++i;
            }
            else if (argument == "y" && !value.Empty())
            {
                ret[EP_WINDOW_HEIGHT] = ToInt(value);
                ++i;
            }
            else if (argument == "monitor" && !value.Empty()) {
                ret[EP_MONITOR] = ToInt(value);
                ++i;
            }
            else if (argument == "hz" && !value.Empty()) {
                ret[EP_REFRESH_RATE] = ToInt(value);
                ++i;
            }
            else if (argument == "m" && !value.Empty())
            {
                ret[EP_MULTI_SAMPLE] = ToInt(value);
                ++i;
            }
            else if (argument == "b" && !value.Empty())
            {
                ret[EP_SOUND_BUFFER] = ToInt(value);
                ++i;
            }
            else if (argument == "r" && !value.Empty())
            {
                ret[EP_SOUND_MIX_RATE] = ToInt(value);
                ++i;
            }
            else if (argument == "pp" && !value.Empty())
            {
                ret[EP_RESOURCE_PREFIX_PATHS] = value;
                ++i;
            }
            else if (argument == "p" && !value.Empty())
            {
                ret[EP_RESOURCE_PATHS] = value;
                ++i;
            }
            else if (argument == "pf" && !value.Empty())
            {
                ret[EP_RESOURCE_PACKAGES] = value;
                ++i;
            }
            else if (argument == "ap" && !value.Empty())
            {
                ret[EP_AUTOLOAD_PATHS] = value;
                ++i;
            }
            else if (argument == "ds" && !value.Empty())
            {
                ret[EP_DUMP_SHADERS] = value;
                ++i;
            }
            else if (argument == "mq" && !value.Empty())
            {
                ret[EP_MATERIAL_QUALITY] = ToInt(value);
                ++i;
            }
            else if (argument == "tq" && !value.Empty())
            {
                ret[EP_TEXTURE_QUALITY] = ToInt(value);
                ++i;
            }
            else if (argument == "tf" && !value.Empty())
            {
                ret[EP_TEXTURE_FILTER_MODE] = ToInt(value);
                ++i;
            }
            else if (argument == "af" && !value.Empty())
            {
                ret[EP_TEXTURE_FILTER_MODE] = FILTER_ANISOTROPIC;
                ret[EP_TEXTURE_ANISOTROPY] = ToInt(value);
                ++i;
            }
            else if (argument == "touch")
                ret[EP_TOUCH_EMULATION] = true;
#ifdef URHO3D_TESTING
            else if (argument == "timeout" && !value.Empty())
            {
                ret[EP_TIME_OUT] = ToInt(value);
                ++i;
            }
#endif
#ifdef URHO3D_PROFILE
            else if (argument == "pr")
                ret[EP_PROFILER_LISTEN] = true;
            else if (argument == "prp" && !value.Empty())
            {
                ret[EP_PROFILER_PORT] = ToInt(value);
                ++i;
            }
            else if (argument == "prnoev")
                ret[EP_EVENT_PROFILER] = false;
#endif
        }
    }

    return ret;
}

bool Engine::HasParameter(const VariantMap& parameters, const String& parameter)
{
    StringHash nameHash(parameter);
    return parameters.Find(nameHash) != parameters.End();
}

const Variant& Engine::GetParameter(const VariantMap& parameters, const String& parameter, const Variant& defaultValue)
{
    StringHash nameHash(parameter);
    VariantMap::ConstIterator i = parameters.Find(nameHash);
    return i != parameters.End() ? i->second_ : defaultValue;
}

void Engine::HandleExitRequested(StringHash eventType, VariantMap& eventData)
{
    if (autoExit_)
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

    exiting_ = true;
#if defined(__EMSCRIPTEN__) && defined(URHO3D_TESTING)
    emscripten_force_exit(EXIT_SUCCESS);    // Some how this is required to signal emrun to stop
#endif
}

void Engine::updateAudioPausing()
{
	auto* audio = static_cast<Audio*>(context_->audio_);
	auto* input = GetSubsystem<Input>();

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
	}
}

void Engine::updateFpsGoalTimer()
{
	renderGoalTimer_.SetTimeoutDuration(renderTimeGoalUs, false);
}

void Engine::updateUpdateTimeTimer()
{
	updateTimer_.SetTimeoutDuration(updateTimeGoalUs, false);
}

void Engine::updateAveragingTimeWindows()
{

	renderTimeUsBuffer_.Resize(renderAveragingTimeWindowUs_ / renderTimeGoalUs);
	//reset the buffer contents to the goal
	for (int i = 0; i < renderTimeUsBuffer_.Size(); i++) {
		renderTimeUsBuffer_[i] = renderTimeGoalUs;
	}
	//reset the current average
	avgRenderTimeUs_ = renderTimeGoalUs;
	renderTimeAvgIdx_ = 0;



	updateTimeUsBuffer_.Resize(updateAveragingTimeWindowUs_ / updateTimeGoalUs);

	//reset the buffer contents to the goal
	for (int i = 0; i < updateTimeUsBuffer_.Size(); i++) {
		updateTimeUsBuffer_[i] = updateTimeGoalUs;
	}

	//reset the current average
	avgUpdateTimeUs_ = updateTimeGoalUs;
	updateTimeAvgIdx_ = 0;


}

}
