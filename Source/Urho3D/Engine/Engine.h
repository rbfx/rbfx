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

#pragma once

#include "../Core/Object.h"
#include "../Core/Timer.h"

namespace CLI
{

class App;

}

namespace Urho3D
{

class Console;
class DebugHud;

/// Urho3D engine. Creates the other subsystems.
class URHO3D_API Engine : public Object
{
    URHO3D_OBJECT(Engine, Object);

public:
    /// Construct.
    explicit Engine(Context* context);
    /// Destruct. Free all subsystems.
    ~Engine() override;

    /// Initialize engine using parameters given and show the application window. Return true if successful.
    bool Initialize(const StringVariantMap& parameters);
    /// Reinitialize resource cache subsystem using parameters given. Implicitly called by Initialize. Return true if successful.
    bool InitializeResourceCache(const StringVariantMap& parameters, bool removeOld = true);
    /// Run one frame.
    void RunFrame();
    /// Create the console and return it. May return null if engine configuration does not allow creation (headless mode).
    Console* CreateConsole();
    /// Create the debug hud.
    DebugHud* CreateDebugHud();
    /// Set minimum frames per second. If FPS goes lower than this, time will appear to slow down.
    /// @property
    void SetMinFps(int fps);
    /// Set maximum frames per second. The engine will sleep if FPS is higher than this.
    /// @property
    void SetMaxFps(int fps);
    /// Set maximum frames per second when the application does not have input focus.
    /// @property
    void SetMaxInactiveFps(int fps);
    /// Set how many frames to average for timestep smoothing. Default is 2. 1 disables smoothing.
    /// @property
    void SetTimeStepSmoothing(int frames);
    /// Set whether to pause update events and audio when minimized.
    /// @property
    void SetPauseMinimized(bool enable);
    /// Set whether to exit automatically on exit request (window close button).
    /// @property
    void SetAutoExit(bool enable);
    /// Override timestep of the next frame. Should be called in between RunFrame() calls.
    void SetNextTimeStep(float seconds);
    /// Set engine parameter. Not all parameter changes will have effect.
    void SetParameter(const ea::string& name, const Variant& value);
    /// Return whether engine parameters contains a specific parameter.
    bool HasParameter(const ea::string& name) const;
    /// Return engine parameter or default value.
    const Variant& GetParameter(const ea::string& name, const Variant& defaultValue = Variant::EMPTY) const;
    static const Variant& GetParameter(const StringVariantMap& parameters, const ea::string& name, const Variant& defaultValue = Variant::EMPTY);
    /// Close the graphics window and set the exit flag. No-op on iOS/tvOS, as an iOS/tvOS application can not legally exit.
    void Exit();
    /// Dump profiling information to the log.
    void DumpProfiler();
    /// Dump information of all resources to the log.
    void DumpResources(bool dumpFileName = false);
    /// Dump information of all memory allocations to the log. Supported in MSVC debug mode only.
    void DumpMemory();

    /// Return preference directory name.
    const ea::string& GetAppPreferencesDir() const { return appPreferencesDir_; }

    /// Get timestep of the next frame. Updated by ApplyFrameLimit().
    float GetNextTimeStep() const { return timeStep_; }

    /// Return the minimum frames per second.
    /// @property
    int GetMinFps() const { return minFps_; }

    /// Return the maximum frames per second.
    /// @property
    int GetMaxFps() const { return maxFps_; }

    /// Return the maximum frames per second when the application does not have input focus.
    /// @property
    int GetMaxInactiveFps() const { return maxInactiveFps_; }

    /// Return how many frames to average for timestep smoothing.
    /// @property
    int GetTimeStepSmoothing() const { return timeStepSmoothing_; }

    /// Return whether to pause update events and audio when minimized.
    /// @property
    bool GetPauseMinimized() const { return pauseMinimized_; }

    /// Return whether to exit automatically on exit request.
    /// @property
    bool GetAutoExit() const { return autoExit_; }

    /// Return whether engine has been initialized.
    /// @property
    bool IsInitialized() const { return initialized_; }

    /// Return whether exit has been requested.
    /// @property
    bool IsExiting() const { return exiting_; }

    /// Return whether the engine has been created in headless mode.
    /// @property
    bool IsHeadless() const { return headless_; }

    /// Send frame update events.
    void Update();
    /// Render after frame update.
    void Render();
    /// Get the timestep for the next frame and sleep for frame limiting if necessary.
    void ApplyFrameLimit();

#if DESKTOP
    /// Parse the engine startup parameters map from command line arguments.
    static void DefineParameters(CLI::App& commandLine, StringVariantMap& engineParameters);
#endif

private:
    /// Set flag indicating that exit request has to be handled.
    void HandleExitRequested(StringHash eventType, VariantMap& eventData);
    /// Do housekeeping tasks at the end of frame. Actually handles exit requested event. Auto-exit if enabled.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);
    /// Actually perform the exit actions.
    void DoExit();

    /// App preference directory.
    ea::string appPreferencesDir_;
    /// Frame update timer.
    HiresTimer frameTimer_;
    /// Previous timesteps for smoothing.
    ea::vector<float> lastTimeSteps_;
    /// Next frame timestep in seconds.
    float timeStep_;
    /// How many frames to average for the smoothed timestep.
    unsigned timeStepSmoothing_;
    /// Minimum frames per second.
    unsigned minFps_;
    /// Maximum frames per second.
    unsigned maxFps_;
    /// Maximum frames per second when the application does not have input focus.
    unsigned maxInactiveFps_;
    /// Pause when minimized flag.
    bool pauseMinimized_;
#ifdef URHO3D_TESTING
    /// Time out counter for testing.
    long long timeOut_;
#endif
    /// Auto-exit flag.
    bool autoExit_;
    /// Initialized flag.
    bool initialized_;
    /// Engine parameters used for initialization.
    StringVariantMap parameters_;
    /// Whether the exit is required by operating system.
    bool exitRequired_{};
    /// Whether the exiting is in progress.
    bool exiting_;
    /// Headless mode flag.
    bool headless_;
    /// Audio paused flag.
    bool audioPaused_;
};

}
