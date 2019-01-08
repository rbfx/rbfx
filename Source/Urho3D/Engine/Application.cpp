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

#include "../Core/Profiler.h"
#include "../Engine/Application.h"
#include "../Engine/EngineDefs.h"
#include "../Engine/EngineEvents.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"

#if defined(IOS) || defined(TVOS)
#include "../Graphics/Graphics.h"
#include <SDL/SDL.h>
#endif
#include <Urho3D/Core/CommandLine.h>

#include "../DebugNew.h"

namespace Urho3D
{

#if defined(IOS) || defined(TVOS) || defined(__EMSCRIPTEN__)
// Code for supporting SDL_iPhoneSetAnimationCallback() and emscripten_set_main_loop_arg()
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
void RunFrame(void* data)
{
    static_cast<Engine*>(data)->RunFrame();
}
#endif

/// Command line parser.
static CLI::App commandLine_{};

Application::Application(Context* context) :
    Object(context),
    exitCode_(EXIT_SUCCESS)
{
    // Create the Engine, but do not initialize it yet. Subsystems except Graphics & Renderer are registered at this point
    engine_ = new Engine(context);

    // Subscribe to log messages so that can show errors if ErrorExit() is called with empty message
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(Application, HandleLogMessage));
}

int Application::Run()
{
    // Profiler requires main thread to be named "Main" as fps calculations depend on it.
    URHO3D_PROFILE_THREAD("Main");
#if !defined(__GNUC__) || __EXCEPTIONS
    try
    {
#endif
        // Register engine command line arguments
        Engine::DefineParameters(commandLine_, engineParameters_);

        // Register application command line arguments or set up engine parameters
        Setup();
        if (exitCode_)
            return exitCode_;

        // Parse command line parameters
        {
            const StringVector& rawArguments = GetArguments();
            std::vector<std::string> cliArgs;
            cliArgs.reserve(rawArguments.Size());
            // CLI11 detail - arguments must be in reversed order.
            for (auto i = static_cast<int>(rawArguments.Size() - 1); i >= 0; i--)
                cliArgs.emplace_back(rawArguments[static_cast<unsigned>(i)].CString());

            try {
                commandLine_.parse(cliArgs);
            } catch(const CLI::ParseError &e) {
                exitCode_ = commandLine_.exit(e);
                return exitCode_;
            }
        }

        if (!engine_->Initialize(engineParameters_))
        {
            ErrorExit();
            return exitCode_;
        }

        Start();
        if (exitCode_ || engine_->IsExiting())
        {
            Stop();
            return exitCode_;
        }

        SendEvent(E_APPLICATIONSTARTED);

        // Platforms other than iOS/tvOS and Emscripten run a blocking main loop
#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
        while (!engine_->IsExiting())
            engine_->RunFrame();

        Stop();
        // iOS/tvOS will setup a timer for running animation frames so eg. Game Center can run. In this case we do not
        // support calling the Stop() function, as the application will never stop manually
#else
#if defined(IOS) || defined(TVOS)
        SDL_iPhoneSetAnimationCallback(GetSubsystem<Graphics>()->GetWindow(), 1, &RunFrame, engine_);
#elif defined(__EMSCRIPTEN__)
        emscripten_set_main_loop_arg(RunFrame, engine_, 0, 1);
#endif
#endif

        return exitCode_;
#if !defined(__GNUC__) || __EXCEPTIONS
    }
    catch (std::bad_alloc&)
    {
        ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
        return EXIT_FAILURE;
    }
#endif
}

void Application::ErrorExit(const String& message)
{
    engine_->Exit(); // Close the rendering window
    exitCode_ = EXIT_FAILURE;

    std::function<void(const String&)> showError;
    if (engineParameters_[EP_HEADLESS].GetBool())
    {
        showError = [this](const String& message)
        {
            URHO3D_LOGERROR(message.CString());
        };
    }
    else
    {
        showError = [this](const String& message)
        {
            ErrorDialog(GetTypeName(), message);
        };
    }

    if (!message.Length())
    {
        showError(startupErrors_.Length() ?
            startupErrors_ : String("Application has been terminated due to unexpected error."));
    }
    else
        showError(message);
}

void Application::HandleLogMessage(StringHash eventType, VariantMap& eventData)
{
    using namespace LogMessage;

    if (eventData[P_LEVEL].GetInt() == LOG_ERROR)
    {
        // Strip the timestamp if necessary
        String error = eventData[P_MESSAGE].GetString();
        unsigned bracketPos = error.Find(']');
        if (bracketPos != String::NPOS)
            error = error.Substring(bracketPos + 2);

        startupErrors_ += error + "\n";
    }
}

CLI::App& Application::GetCommandLineParser()
{
    return commandLine_;
}


}
